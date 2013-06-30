/**
 * @file   connection.cpp
 * @author Alexey Bochkovskiy <alexeyab84@gmail.com>
 *
 * @brief PortMapping Connection
 *
 *
 */
// ----------------------------------------------------------------------------
#include "connection.hpp"

// ----------------------------------------------------------------------------
#include <boost/move/move.hpp>
#include <boost/bind.hpp>
// ----------------------------------------------------------------------------

	/// 
	/// Constructor for class, initilize socket for this connection
	/// 
	/// @param io_service reference to io_service of executors in which this connection will work
	/// @param T_hide_me() temporary object that made a constructor private 
	/// 
	/// @return nothing
	///
	T_connection::T_connection(ba::io_service& io_service, T_hide_me) :
		io_service_(io_service), client_socket_(io_service), server_socket_(io_service), count_of_events_loops_(1)
	{
		std::cout << "T_connection() \n";
	}

	T_connection::~T_connection() { std::cout << "~T_connection() \n"; }
	// ----------------------------------------------------------------------------

	/// 
	/// Perform all input/output operations in async mode:
	/// for a start try to connect to endpoint by endpoint_iterator
	/// 
	/// @param shared_this shared pointer of this (current connection)
	/// @param endpoint_iterator forward iterable object, that points to the connection endpoints of remote server
	///
	void T_connection::run(T_shared_this shared_this, const ba::ip::tcp::resolver::iterator endpoint_iterator) {
		// try/catch and then output to std::cerr exception message .what()
		if (!try_catch_to_cerr(THROW_PLACE, [&]() {
			memorypool_shared_this_ = boost::move(shared_this);
			const bool first_time = true;
			handle_connect(boost::system::error_code(), endpoint_iterator, first_time);
		} )	)
			shutdown(boost::system::error_code(), "");
	}
	// ----------------------------------------------------------------------------

	/// 
	/// Try to connect to the server
	/// 
	/// @param err reference to error code returned by async_connect
	/// @param endpoint_iterator forward iterable object, that points to the connection endpoints of remote server
	/// @param first_time flag that show that function launched at first time or no
	///
	void T_connection::handle_connect(const boost::system::error_code& err,
									ba::ip::tcp::resolver::iterator endpoint_iterator, const bool first_time) {
	//	std::cout << "handle_connect. Error: " << err << "\n";
		if (!err && !first_time) {
			// no one instruction after that expression will not executed before the atomic variable will not incremented by 1
			count_of_events_loops_.fetch_add(1, std::memory_order_acquire);	
			handle_write_to_client(bs::error_code(), 0);
			handle_write_to_server(bs::error_code(), 0);
		} else if (endpoint_iterator != ba::ip::tcp::resolver::iterator()) {
			//ssocket_.close();
			ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
			//std::cout << std::string("Endpoint: " + endpoint.address().to_string() + ":" + boost::lexical_cast<std::string>(endpoint.port()) + ", " + 
				//boost::lexical_cast<std::string>(first_time) + "\n");
			server_socket_.async_connect(endpoint,
								   client_bind(boost::bind(&T_connection::handle_connect, this,
														   boost::asio::placeholders::error,
														   ++endpoint_iterator, false)) );
		} else {
			shutdown(err, THROW_PLACE);
		}
	}
	// ----------------------------------------------------------------------------

	/// 
	/// Writing data to the client
	/// after read them from a server to server_buffer_
	/// 
	/// @param err 
	/// @param len length of data in bytes, that have been read
	///
	void T_connection::handle_read_from_server(const bs::error_code& err, const size_t len) {
		//std::cout << "handle_read_from_server, len= " << len << ", eof is: " << (err == ba::error::eof) << std::endl;
		if(!err)
		{
			ba::async_write(client_socket_, ba::buffer(server_buffer_, len),
							server_bind(boost::bind(&T_connection::handle_write_to_client, this,
													ba::placeholders::error,
													ba::placeholders::bytes_transferred)) );
		} else {
			shutdown(err, THROW_PLACE);
		}
	}

	/// 
	/// Reading data from a server to server_buffer_
	/// after yet another write data to the client
	///
	/// @param err 
	/// @param len length of data in bytes, that have been writen
	///
	void T_connection::handle_write_to_client(const bs::error_code& err, const size_t len) {
		//std::cout << "handle_write_to_client, len= " << len << ", eof is: " << (err == ba::error::eof) << std::endl;
		if(!err) {
			server_socket_.async_read_some(ba::buffer(server_buffer_),
					   server_bind(boost::bind(&T_connection::handle_read_from_server, this,
											   ba::placeholders::error,
											   ba::placeholders::bytes_transferred)) );
		} else {
			shutdown(err, THROW_PLACE);
		}
	}
	// ----------------------------------------------------------------------------

	/// 
	/// Writing data to the server
	/// after read them from a client to client_buffer_
	/// 
	/// @param err 
	/// @param len length of data in bytes, that have been read
	///
	void T_connection::handle_read_from_client(const bs::error_code& err, const size_t len) {
		//std::cout << "handle_read_from_client, len= " << len << ", eof is: " << (err == ba::error::eof) << std::endl;
		if(!err)
		{
			ba::async_write(server_socket_, ba::buffer(client_buffer_, len),
							client_bind(boost::bind(&T_connection::handle_write_to_server, this,
													ba::placeholders::error,
													ba::placeholders::bytes_transferred)) );
		} else {
			shutdown(err, THROW_PLACE);
		}
	}

	/// 
	/// Reading data from a client to client_buffer_
	/// after yet another write data to the server
	///
	/// @param err 
	/// @param len length of data in bytes, that have been writen
	///
	void T_connection::handle_write_to_server(const bs::error_code& err, const size_t len) {
		//std::cout << "handle_write_to_server, len= " << len << ", eof is: " << (err == ba::error::eof) << std::endl;
		if(!err) {
			client_socket_.async_read_some(ba::buffer(client_buffer_),
					   client_bind(boost::bind(&T_connection::handle_read_from_client, this,
											   ba::placeholders::error,
											   ba::placeholders::bytes_transferred)) );
		} else {
			shutdown(err, THROW_PLACE);
		}
	}
	// ----------------------------------------------------------------------------

	/// 
	/// Close both sockets: for client and server
	/// 
	/// @param err 
	/// @param throw_place string with filename, its datetime, line number in file and function name which call this shutdown() function 
	///
	inline void T_connection::shutdown(const bs::error_code& err, const std::string& throw_place) {
		// try/catch and then output to std::cerr exception message .what()
		try_catch_to_cerr(THROW_PLACE, [&]() {
			//if(!!err && err != ba::error::eof) std::cerr << "Boost error_code: " << err.message() << "\n ->throw place: " << throw_place << std::endl;
			if(count_of_events_loops_.fetch_sub(1, std::memory_order_acq_rel)-1 == 0)	// if(--count_of_events_loops_ == 0)
			{
				client_socket_.close();
				server_socket_.close();
				memorypool_shared_this_.reset();
			}
		} );
	}
	// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
