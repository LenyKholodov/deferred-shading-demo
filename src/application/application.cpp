#include <application/application.h>
#include <common/exception.h>
#include <common/log.h>
#include <mutex>

using namespace engine::common;
using namespace engine::application;

extern "C"
{
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
}

#include <cstdio>

/// Implementation details
struct Application::Impl
{
  std::mutex lock; //lock for application data
  bool exited; //has application exited
  int exit_code; //exit code of the application

  Impl()
    : exited(false)
    , exit_code(0)
  {
  }

  ~Impl()
  {
    glfwTerminate();
  }
};

namespace
{

static void error_callback(int error, const char* description)
{
  engine_log_error("GLFW error: %s", description);
}

}

Application::Application()
{
  engine_log_debug("Creating application...");
  engine_log_debug("GLFW version is %s", glfwGetVersionString());

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
  {
    glfwSetErrorCallback(nullptr);

    const char* error = "";
    glfwGetError(&error);

    throw Exception::format("GLFW initialization error: %s", error);
  }

  impl = std::make_shared<Impl>();
}

int Application::get_exit_code() const
{
  std::unique_lock<std::mutex> lock(impl->lock);
  return impl->exit_code;
}

bool Application::has_exited() const
{
  std::unique_lock<std::mutex> lock(impl->lock);
  return impl->exited;   
}

void Application::exit(int exit_code)
{
  std::unique_lock<std::mutex> lock(impl->lock);

  impl->exited = true;
  impl->exit_code = exit_code;
}

void Application::main_loop(const IdleHandler& idle_fn)
{
  const bool has_idle_fn = static_cast<bool>(idle_fn);

  engine_log_info("Starting application main loop...");

  while (!has_exited())
  {
    size_t max_timeout = 1000;

      //call idle function

    if (has_idle_fn)
    {
      try
      {
        size_t timeout = idle_fn();

        if (timeout < max_timeout)
          max_timeout = timeout;
      }
      catch (std::exception& e)
      {
        engine_log_error("%s", e.what());
      }
    }

      //process events

    glfwWaitEventsTimeout(max_timeout / 1000.0);
  }

  engine_log_info("Exited from application main loop");
}
