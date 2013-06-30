/**
 * @file   server.cpp
 * @author Alexey Bochkovskiy <alexeyab84@gmail.com>
 *
 * @brief PortMapping Server
 *
 *
 */
// ----------------------------------------------------------------------------
#include "server.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
// ----------------------------------------------------------------------------

/// 
/// Initialize all needed data
/// 
/// @param io_service reference to io_service
/// @param thread_num_acceptors number of threads in thread pool for acceptors
/// @param thread_num_executors number of threads in thread pool for executors
/// @param remote_port port to port mapping on
/// @param remote_address address to port mapping on
/// @param local_port port to listen on, by default - 10001
/// @param local_interface_address local interface address to listen on
///
T_server::T_server(ba::io_service& io_service_acceptors, ba::io_service& io_service_executors, 
				   unsigned int thread_num_acceptors, unsigned int thread_num_executors, 
				   unsigned int remote_port, std::string remote_address,
				   unsigned int local_port, std::string local_interface_address)
	: io_service_acceptors_(io_service_acceptors),
	  io_service_executors_(io_service_executors),
	  work_acceptors_(io_service_acceptors_),
	  work_executors_(io_service_executors_),
	  local_endpoint_(local_interface_address.empty()?	
				(ba::ip::tcp::endpoint(ba::ip::tcp::v4(), local_port)): // INADDR_ANY for v4 (in6addr_any if the fix to v6)
				ba::ip::tcp::endpoint(ba::ip::address().from_string(local_interface_address), local_port) ),   // specified ip address
	  acceptor_(io_service_acceptors_, local_endpoint_)                 // By default set option to reuse the address (i.e. SO_REUSEADDR)
{
	// Resolve remote address:port of server
	boost::asio::ip::tcp::resolver resolver(io_service_executors_);
	ba::ip::tcp::resolver::query query(remote_address, boost::lexical_cast<std::string>(remote_port) );
	remote_endpoint_it_ = resolver.resolve(query);

	std::clog << "Remote resolved endpoints for: " << remote_address << std::endl;
	unsigned remote_number = 0;
	for(auto it = remote_endpoint_it_; it != ba::ip::tcp::resolver::iterator(); ++it, ++remote_number)
		std::clog << remote_number << ": " << it->endpoint() << std::endl;
	std::clog << std::endl;

	const ba::ip::tcp::endpoint remote_endpoint_ = *remote_endpoint_it_;
	std::clog << "Start with remote: " << remote_endpoint_ << std::endl;
	std::clog << "Start listener: " << local_endpoint_ << std::endl << std::endl;

	// create threads in pool for executors
	for(size_t i = 0; i < thread_num_executors; ++i)
		thr_grp_executors_.emplace_back(boost::bind(&boost::asio::io_service::run, &io_service_executors_));

	// create threads in pool and start acceptors
	for(size_t i = 0; i < thread_num_acceptors; ++i) {
		// create threads in pool
		if(i != 0)	// one main thread already in pool from: int main() { ... io_service_acceptors.run(); ... }
			thr_grp_acceptors_.emplace_back(boost::bind(&boost::asio::io_service::run, &io_service_acceptors_));

		// create memory pool for objects of connections	
		T_memory_pool_ptr memory_pool_ptr(new T_memory_pool, T_memory_pool_deleter() );
		
		// create next connection, that will accepted next
		T_connection * const memory_pool_raw_ptr = reinterpret_cast<T_connection *>( memory_pool_ptr.get() );
		T_connection * const new_connection_raw_ptr = T_connection::create(memory_pool_raw_ptr, 0, io_service_executors_);
		
		// start another acceptor in async mode
		acceptor_.async_accept(new_connection_raw_ptr->socket(),
								new_connection_raw_ptr->server_bind(
										   boost::bind(&T_server::handle_accept, this, 
													   boost::move(memory_pool_ptr),   // doesn't copy and doesn't use the atomic counter with memory barrier
													   0,
													   ba::placeholders::error)) );
	}
}
// ----------------------------------------------------------------------------

/// 
/// Stop io_services and
/// wait for all executing threads
/// 
///
T_server::~T_server() {
	io_service_acceptors_.stop();
	io_service_executors_.stop();
	for(auto &i : thr_grp_acceptors_) i.join();
	for(auto &i : thr_grp_executors_) i.join();
}
// ----------------------------------------------------------------------------

/// 
/// Run when new connection is accepted
/// 
/// @param memory_pool_ptr shared pointer to the allocated memory poll for connections
/// @param i_connect index of current connection in memory pool 
/// @param e reference to error object
///
void T_server::handle_accept( T_memory_pool_ptr memory_pool_ptr, size_t i_connect, const boost::system::error_code& e) {
	if (!e) {
		// get pointer of current connection
		T_connection * const current_memory_pool_raw_ptr = reinterpret_cast<T_connection *>( memory_pool_ptr.get() );
		T_connection * const current_connection_raw_ptr = &(current_memory_pool_raw_ptr[i_connect]);
		T_connection::T_shared_this current_connection_ptr(memory_pool_ptr, current_connection_raw_ptr );

		// schedule new task to thread pool
		current_connection_raw_ptr->run(boost::move(current_connection_ptr), remote_endpoint_it_);	// sync launch of short-task: run()
		
		// increment index of connections
		++i_connect;	
		
		// if the limit of connections in the memory pool have been reached, then create a new memory pool
		if(i_connect == connections_in_memory_pool) {
			i_connect = 0;
			memory_pool_ptr.reset(new T_memory_pool, T_memory_pool_deleter() );
		}

		// create next connection, that will accepted
		T_connection * const new_memory_pool_raw_ptr = reinterpret_cast<T_connection *>( memory_pool_ptr.get() );
		T_connection * const new_connection_raw_ptr = T_connection::create(new_memory_pool_raw_ptr, i_connect, io_service_executors_);

		// start new accept operation		
		acceptor_.async_accept(new_connection_raw_ptr->socket(),
								new_connection_raw_ptr->server_bind(
										   boost::bind(&T_server::handle_accept, this, 
													   boost::move(memory_pool_ptr),   // doesn't copy and doesn't use the atomic counter with memory barrier
													   i_connect,
													   ba::placeholders::error)) );
	}
}
// ----------------------------------------------------------------------------

