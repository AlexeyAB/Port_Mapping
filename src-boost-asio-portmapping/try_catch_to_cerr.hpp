/**
 * @file   try_catch_to_cerr.hpp
 * @author Alexey Bochkovskiy <alexeyab84@gmail.com>
 *
 * @brief try/catch templated function for catch exceptions and for output to console
 *
 *
 */
// ----------------------------------------------------------------------------
#ifndef TRY_CATCH_TO_CERR_HPP
#define TRY_CATCH_TO_CERR_HPP
// ----------------------------------------------------------------------------
#include "seh_exception.hpp"
#include <boost/system/system_error.hpp>
// ----------------------------------------------------------------------------
#ifndef __FUNCTION__
	#define __FUNCTION__
#endif

/// macro to determine the location of catching an exception
#define THROW_PLACE std::string("(") + __FILE__ + ":" + std::to_string(__LINE__) + ": " + __DATE__ + " " + __TIME__ + ", " + __FUNCTION__ +  ")\n\t"
// ----------------------------------------------------------------------------

/// try/catch and then output to std::cerr exception message .what()
/// 
/// @param throw_place string with filename, its datetime, line number in file and function name which call this try_catch_to_cerr() function
/// @param func function that will launch in try/catch
/// 
/// @return true if no exceptions, false if catched an exception
///
template<typename T_func>
inline bool try_catch_to_cerr(const std::string& throw_place, T_func func) {
	try {
		func();
	} catch(const seh::T_seh_exception& e) {
		std::cerr << "T_seh_exception: " << e.what() << "\n ->throw place: " << throw_place << std::endl;
		return false;
	} catch(const bs::system_error& e) {
		std::cerr << "Boost system_error exception: " << e.what() << "\n ->throw place: " << throw_place << std::endl;
		return false;
	} catch(const std::exception &e) {
		std::cerr << "Exception: " << e.what() << "\n ->throw place: " << throw_place << std::endl;
		return false;
	} catch(...) {
		std::cerr << "Unknown exception!" << "\n ->throw place: " << throw_place << std::endl;
		return false;
	}	
	return true;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#endif // TRY_CATCH_TO_CERR_HPP