#include <render/device.h>
#include <application/application.h>
#include <application/window.h>
#include <common/exception.h>
#include <common/log.h>

#include <string>

extern "C"
{
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "linmath.h"
}

#include <stdlib.h>
#include <stdio.h>

using namespace engine::common;
using namespace engine::application;
using namespace engine;

#if defined (_MSC_VER) || defined (__APPLE_CC__)
  #define engine_offsetof(X,Y) offsetof(X,Y)
#else
  #define engine_offsetof(X,Y) (reinterpret_cast<size_t> (&(static_cast<X*> (0)->*(&X::Y))))
#endif

static render::Vertex vertices[] = {
  {math::vec3f(-0.6f, -0.4f, 0), math::vec3f(), math::vec4f(1.f, 0.f, 0.f, 1.0f), math::vec2f(0, 0)},
  {math::vec3f( 0.6f, -0.4f, 0), math::vec3f(), math::vec4f(0.f, 1.f, 0.f, 1.0f), math::vec2f(0, 0)},
  {math::vec3f(  0.f,  0.6f, 0), math::vec3f(), math::vec4f(0.f, 0.f, 1.f, 1.0f), math::vec2f(0, 0)},
};

static const char* vertex_shader_text =
"#version 410 core\n"
"uniform mat4 MVP;\n"
"in vec3 vCol;\n"
"in vec2 vPos;\n"
"out vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    color = vCol;\n"
"}\n";

static const char* fragment_shader_text =
"#version 410 core\n"
"in vec3 color;\n"
"out vec4 outColor;\n"
"void main()\n"
"{\n"
"    outColor = vec4(color, 1.0);\n"
"}\n";

void print_compilation_log(GLint shader)
{
  GLint log_length = 0;

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  
  if (!log_length)
    return;

  std::string log_buffer;

  log_buffer.resize(log_length - 1);

  GLsizei real_log_size = 0;

  glGetShaderInfoLog(shader, log_length, &real_log_size, &log_buffer[0]);

  if (real_log_size)
    log_buffer.resize(real_log_size - 1);

  engine_log_info("%s", log_buffer.c_str());
}

void print_linking_log(GLint program)
{
  GLint log_length = 0;

  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
  
  if (!log_length)
    return;

  std::string log_buffer;

  log_buffer.resize(log_length - 1);

  GLsizei real_log_size = 0;

  glGetProgramInfoLog(program, log_length, &real_log_size, &log_buffer[0]);

  if (real_log_size)
    log_buffer.resize(real_log_size - 1);

  engine_log_info("%s", log_buffer.c_str());
}

int main(void)
{
  try
  {
    engine_log_info("Application has been started");

      //application setup

    Application app;
    Window window("Render test", 800, 600);

    window.set_keyboard_handler([&](Key key, bool pressed) {
      if (key == Key_Escape && pressed)
      {
        engine_log_info("Escape pressed. Exiting...");
        window.close();
      }
    });

    window.set_mouse_move_handler([&](double x, double y) {
      double relative_x = x / window.width();
      double relative_y = y / window.height();

      engine_log_info("mouse move pos=(%.1f, %.1f) <-> (%.2f, %.2f)", x, y, relative_x, relative_y);
    });

    window.set_mouse_button_handler([&](MouseButton button, bool pressed) {
      engine_log_info("mouse button=%d pressed=%d", button, pressed);
    });

      //render setup

    render::DeviceOptions render_options;

    //render_options.debug = false;

    render::Device render_device(window, render_options);
    render::FrameBuffer frame_buffer = render_device.window_frame_buffer();

    GLuint vertex_array;
    GLint mvp_location, vpos_location, vcol_location;

    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    render::VertexBuffer vb = render_device.create_vertex_buffer(sizeof(vertices));

    vb.set_data(0, sizeof(vertices)/sizeof(vertices[0]), vertices);

    render::Shader vertex_shader = render_device.create_vertex_shader("vs.default", vertex_shader_text);
    render::Shader pixel_shader = render_device.create_pixel_shader("ps.default", fragment_shader_text);
    render::Program program = render_device.create_program("default", vertex_shader, pixel_shader);

    vpos_location = program.get_attribute_location("vPos");
    vcol_location = program.get_attribute_location("vCol");
    mvp_location = program.get_uniform_location("MVP");

    engine_log_debug("vpos_location=%d, vcol_location=%d, mvp_location=%d",
        vpos_location, vcol_location, mvp_location);

    vb.bind();

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                         sizeof(vertices[0]), (void*)engine_offsetof(render::Vertex, position));
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*)engine_offsetof(render::Vertex, color));

      //main loop

    app.main_loop([&](){
      if (window.should_close())
        app.exit();

      int width = window.frame_buffer_width(), height = window.frame_buffer_height();
      float ratio = width / (float) height;

      mat4x4 m, p, mvp;

      frame_buffer.bind();
      frame_buffer.clear();

      mat4x4_identity(m);
      mat4x4_rotate_Z(m, m, (float) glfwGetTime());
      mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
      mat4x4_mul(mvp, p, m);

      program.bind();

      glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
      glDrawArrays(GL_TRIANGLES, 0, 3);

      window.swap_buffers();

      static const size_t TIMEOUT_MS = 20;

      return TIMEOUT_MS;
    });

    engine_log_info("Exiting from application...");

    return 0;
  }
  catch (std::exception& e)
  {
    engine_log_fatal("%s\n", e.what());
    return 1;
  }
}
