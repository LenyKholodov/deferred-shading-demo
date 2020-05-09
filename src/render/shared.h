#pragma once

#include <render/device.h>
#include <application/window.h>
#include <common/exception.h>
#include <common/log.h>

#include <string>
#include <vector>

extern "C"
{
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
}

namespace engine {
namespace render {

/// Basic class for internal render objects
class BaseObject
{
  public:
    BaseObject() {}

    /// Non-copyable pattern
    BaseObject(const BaseObject&) = delete;
    BaseObject(BaseObject&&) = delete;
    BaseObject& operator = (const BaseObject&) = delete;
    BaseObject& operator = (BaseObject&&) = delete;
};

/// Device context implementation
class DeviceContextImpl: BaseObject
{
  public:
    /// Constructor
    DeviceContextImpl(const Window& window, const DeviceOptions& options);

    /// Destructor
    ~DeviceContextImpl();

    /// Window handle
    GLFWwindow* handle() const { return context; }

    /// Access to a render window
    const Window& window() const { return render_window; }

    /// Options
    const DeviceOptions& options() const { return device_options; }

    /// Make context current
    void make_current()
    {
      make_current(context);
    }

    /// Check errors
    void check_errors()
    {
      if (!device_options.debug)
        return;

      check_errors_impl();
    }

    /// Clear all errors
    static void clear_errors()
    {
      while (glGetError () != GL_NO_ERROR);
    }    

  private:
    static void make_current(GLFWwindow* context)
    {
      static GLFWwindow* current_context = 0;

      if (current_context == context)
        return;

      engine_log_debug("glfwMakeContextCurrent(%p)", context);

      glfwMakeContextCurrent(context);

      if (current_context)
        check_errors_impl();

      current_context = context;
    }    

    static void check_errors_impl()
    {
      using common::Exception;

      GLenum error = glGetError ();

      clear_errors();

      switch (error)
      {
        case GL_NO_ERROR:
          break;
        case GL_INVALID_ENUM:
          throw Exception::format("OpenGL error: invalid enum");
        case GL_INVALID_VALUE:
          throw Exception::format("OpenGL error: invalid value");
        case GL_INVALID_OPERATION:
          throw Exception::format("OpenGL error: invalid operation");
        case GL_STACK_OVERFLOW:
          throw Exception::format("OpenGL error: stack overflow");
        case GL_STACK_UNDERFLOW:
          throw Exception::format("OpenGL error: stack underflow");
        case GL_OUT_OF_MEMORY:
          throw Exception::format("OpenGL error: out of memory");
        case GL_INVALID_FRAMEBUFFER_OPERATION:
          throw Exception::format("OpenGL error: invalid framebuffer operation");
        default:
          throw Exception::format("OpenGL error: code=0x%04x", error);
      }
    }

  private:
    Window render_window; //target window
    GLFWwindow* context; //context
    DeviceOptions device_options; //device options
};

}}