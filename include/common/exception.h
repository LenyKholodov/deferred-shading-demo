#pragma once

#include <exception>
#include <memory>
#include <cstdarg>

namespace engine {
namespace common {

/// Basic exception with stack tracing
class Exception: public std::exception
{
  public:
    /// Constructor
    Exception(const char* message);

    /// Create exception with formatted message
    static Exception format(const char* format, ...);
    static Exception vformat(const char* format, va_list args);

    /// Exception reason override
    const char* what() const noexcept;

  private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};

/// Create null argument exception
Exception make_null_argument_exception(const char* param_name);

}}
