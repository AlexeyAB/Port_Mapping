/**
 * @file   connection.hpp
 * @author Alexey Bochkovskiy <alexeyab84@gmail.com>
 *
 * @brief PortMapping Connection
 *
 *
 */
// ----------------------------------------------------------------------------
#ifndef CONNECTION_HPP
#define CONNECTION_HPP
// ----------------------------------------------------------------------------
#include "handler_allocator.hpp"
#include "try_catch_to_cerr.hpp"
// ----------------------------------------------------------------------------
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>

namespace ba = boost::asio;
namespace bs = boost::system;
// ----------------------------------------------------------------------------
#include <atomic>
// ----------------------------------------------------------------------------


///
/// Class for handling connection in sync mode
/// 
///
class T_connection {
	struct T_hide_me {};	/// Instead of having to make friend boost::make_shared<connection>()
public:
	typedef boost::shared_ptr<T_connection> T_shared_this;
		
	/// bind for clients handler with optimized memory allocation
	template<typename T_handler>
	inline T_custom_alloc_handler<T_handler> client_bind(T_handler binded_handler) {
		return make_custom_alloc_handler(client_allocator_, binded_handler);
	}

	/// bind for servers handler with optimized memory allocation
	template<typename T_handler>
	inline T_custom_alloc_handler<T_handler> server_bind(T_handler binded_handler) {
		return make_custom_alloc_handler(server_allocator_, binded_handler);
	}
	
	/// 
	/// Constructor for class, initilize socket for this connection
	/// 
	/// @param io_service reference to io_service of executors in which this connection will work
	/// @param T_hide_me() temporary object that made a constructor private 
	/// 
	/// @return nothing
	///
	T_connection(ba::io_service& io_service, T_hide_me);

	~T_connection();

	/// 
	/// Create new connection, throught placement new 
	/// 
	/// @param memory_pool_ptr shared pointer to the allocated memory poll for connections
	/// @param i_connect index of current connection in memory pool 
	/// @param io_service io_service in which this connection will work
	/// 
	/// @return pointer to newly allocated object
	///
	static inline T_connection *const T_connection::create(T_connection *const memory_pool_raw_ptr, const size_t i_connect, ba::io_service& io_service) {
		return new (&memory_pool_raw_ptr[i_connect]) T_connection(io_service, T_hide_me());
	}

	/// 
	/// Return socket, associated with this connection. This socket used in accept operation.
	/// 
	/// 
	/// @return reference to socket
	///
	inline ba::ip::tcp::socket& T_connection::socket() {
		return client_socket_;
	}

	/// 
	/// Perform all input/output operations in async mode:
	/// for a start try to connect to endpoint by endpoint_iterator
	/// 
	/// @param shared_this shared pointer of this (current connection)
	/// @param endpoint_iterator forward iterable object, that points to the connection endpoints of remote server
	///
	void T_connection::run(T_shared_this shared_this, const ba::ip::tcp::resolver::iterator endpoint_iterator);

private:
	/// 
	/// Try to connect to the server
	/// 
	/// @param err reference to error code returned by async_connect
	/// @param endpoint_iterator forward iterable object, that points to the connection endpoints of remote server
	/// @param first_time flag that show that function launched at first time or no
	///
	void T_connection::handle_connect(const boost::system::error_code& err,
									ba::ip::tcp::resolver::iterator endpoint_iterator, const bool first_time);

	/// 
	/// Writing data to the client
	/// after read them from a server to server_buffer_
	/// 
	/// @param err 
	/// @param len length of data in bytes, that have been read
	///
	void T_connection::handle_read_from_server(const bs::error_code& err, const size_t len);

	/// 
	/// Reading data from a server to server_buffer_
	/// after yet another write data to the client
	///
	/// @param err 
	/// @param len length of data in bytes, that have been writen
	///
	void T_connection::handle_write_to_client(const bs::error_code& err, const size_t len);

	/// 
	/// Writing data to the server
	/// after read them from a client to client_buffer_
	/// 
	/// @param err 
	/// @param len length of data in bytes, that have been read
	///
	void T_connection::handle_read_from_client(const bs::error_code& err, const size_t len);

	/// 
	/// Reading data from a client to client_buffer_
	/// after yet another write data to the server
	///
	/// @param err 
	/// @param len length of data in bytes, that have been writen
	///
	void T_connection::handle_write_to_server(const bs::error_code& err, const size_t len);

	/// 
	/// Close both sockets: for client and server
	/// 
	/// @param err 
	/// @param throw_place string with filename, its datetime, line number in file and function name which call this shutdown() function 
	///
	inline void T_connection::shutdown(const bs::error_code& err, const std::string& throw_place);

	enum { buffer_size = 16384 };           ///< size of buffer for storage input/output data
	enum { allocator_size = 1024 };         ///< size of buffer for handler allocator for storage boost::bind()

	std::atomic<int> count_of_events_loops_;///< atomic counter of event loops (client/server)
	T_shared_this memorypool_shared_this_;  ///< shared pointer of this with memory-pool counter
	ba::io_service& io_service_;            ///< reference to io_service, in which work this connection
	ba::ip::tcp::socket client_socket_;     ///< socket, associated with client
	ba::ip::tcp::socket server_socket_;     ///< socket, associated with server
	boost::array<char, buffer_size> client_buffer_;        ///< buffer, associated with client
	boost::array<char, buffer_size> server_buffer_;        ///< buffer, associated with server
	T_handler_allocator<allocator_size> client_allocator_; ///< allocator, to use for handler-based custom memory allocation for clients handlers
	T_handler_allocator<allocator_size> server_allocator_; ///< allocator, to use for handler-based custom memory allocation for servers handlers
};
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
#endif // CONNECTION_HPP
