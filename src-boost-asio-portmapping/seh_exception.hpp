/**
 * @file   seh_exception.hpp
 * @author Alexey Bochkovskiy <alexeyab84@gmail.com>
 *
 * @brief Windows SEH-exception
 *
 *
 */
// ----------------------------------------------------------------------------
#ifndef SEH_EXCEPTION_HPP
#define SEH_EXCEPTION_HPP
// ----------------------------------------------------------------------------
#include <stdexcept>

namespace seh {
	/// Class wrapper, for SEH-exception
	struct T_seh_exception : std::runtime_error {
		T_seh_exception(const std::string& str) : runtime_error(str) {}
	};
	// ----------------------------------------------------------------------------
	/// SEH-exception init
	void seh_exception_init();
};

// ----------------------------------------------------------------------------
#endif // SEH_EXCEPTION_HPP