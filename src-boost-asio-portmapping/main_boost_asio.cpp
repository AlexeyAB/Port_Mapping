/**
 * @file   main_boost_asio.cpp
 * @author Alexey Bochkovskiy <alexeyab84@gmail.com>
 * 
 * @brief Implementation of port mapping with usage of threads pool for acceptors and executors for 
 * implementation of strategy of many connections per each thread from thread pool for number of threads limitation.
 * IO client->server and server->client operations are uses in async mode.
 *
 * 
 */
// ----------------------------------------------------------------------------
#include "server.hpp"
#include "seh_exception.hpp"
// ----------------------------------------------------------------------------
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
// ----------------------------------------------------------------------------
#include <fstream>
#include <iostream>
#include <string>
#include <locale>


/// 
/// Main routine
/// 
/// @param argc number of arguments
/// @param argv pointers to arguments
/// 
/// @return error code
///
int main(int argc, char** argv) {
	std::ofstream file_log, file_error;
	try {
		std::locale::global(std::locale("rus"));
		std::cout << "Usage: main_boost_asio.exe [remote_port remote_address local_port local_address number_acceptors numer_executors language_locale]" << std::endl << std::endl;

#ifdef _MSC_VER
		std::cout << "_MSC_VER  = " << _MSC_VER  << std::endl; 
#endif
#ifdef __GNUC__
		std::cout << "__GNUC__  = " << __GNUC__  << ", __GNUC_MINOR__ = " << __GNUC_MINOR__ << std::endl;
#endif

		// Remote address::port
		unsigned short remote_port = 80;
		std::string remote_address = "google.com";

		// Local interface address::port
		unsigned short local_port = 10001;
		std::string local_interface_address = "0.0.0.0";

		// Number of thread for acceptors and executors
		unsigned int thread_num_acceptors = 2;
		unsigned int thread_num_executors = boost::thread::hardware_concurrency();

		std::cout << "(Default: main_boost_asio.exe " << remote_port << " " << remote_address << " " << 
			local_port << " " << local_interface_address << " " << 
			thread_num_acceptors << " " << thread_num_executors << " " << std::locale::global(std::locale()).name() << ")" << std::endl;

		// read remote port number from command line, if provided
		if(argc > 1)
			remote_port = boost::lexical_cast<unsigned short>(argv[1]);
		// read remote address from command line, if provided
		if(argc > 2)
			remote_address = argv[2];

		// read local port number from command line, if provided
		if(argc > 3)
			local_port = boost::lexical_cast<unsigned short>(argv[3]);
		// read local interface address from command line, if provided
		if(argc > 4)
			local_interface_address = argv[4];

		// read number of threads in thread pool from command line, if provided
		if(argc > 5)
			thread_num_acceptors = boost::lexical_cast<unsigned int>(argv[5]);
		if(argc > 6)
			thread_num_executors = boost::lexical_cast<unsigned int>(argv[6]);

		// set language locale
		if(argc > 7)
			setlocale(LC_ALL, argv[7]);
		// ----------------------------------------------------------------------------

		// Enable Windows SEH exceptions. Compile with key: /EHa 
		seh::seh_exception_init();

		if(true) std::cout.rdbuf(NULL);	// switch off std::cout
				
		/// Output errors(exceptions) to file instead of the console (std::cerr)
		if(false) {			
			file_error.open("log_errors.txt", std::ios_base::out | std::ios_base::trunc);	// std::ios_base::ate
			std::cerr.rdbuf(file_error.rdbuf());
		}

		/// Output to file instead of the console (std::clog)
		if(false) {			
			file_log.open("log.txt", std::ios_base::out | std::ios_base::app);
			std::clog.rdbuf(file_log.rdbuf());
		}
		std::clog << "----------------------------------------------------------------------------"  << std::endl;
		// ----------------------------------------------------------------------------


		boost::asio::io_service io_service_acceptors, io_service_executors;
		// construct new server object
		T_server s(io_service_acceptors, io_service_executors, thread_num_acceptors, thread_num_executors,
			remote_port, remote_address, local_port, local_interface_address);
		// run io_service object, that perform all dispatch operations
		io_service_acceptors.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Unknown exception!" << std::endl;
	}
	file_log.close(), file_error.close();

	return 0;
}
// ----------------------------------------------------------------------------

     

