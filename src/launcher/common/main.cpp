#include <application/application.h>
#include <application/window.h>
#include <common/exception.h>
#include <common/log.h>

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

static const struct
{
    float x, y;
    float r, g, b;
} vertices[3] =
{
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    {  0.6f, -0.4f, 0.f, 1.f, 0.f },
    {   0.f,  0.6f, 0.f, 0.f, 1.f }
};

static const char* vertex_shader_text =
"#version 110\n"
"uniform mat4 MVP;\n"
"attribute vec3 vCol;\n"
"attribute vec2 vPos;\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    color = vCol;\n"
"}\n";

static const char* fragment_shader_text =
"#version 110\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

int main(void)
{
  try
  {
    engine_log_info("Application has been started");

    Application app;
    Window window("Render test", 800, 600);

    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    glfwMakeContextCurrent(window.handle());
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    mvp_location = glGetUniformLocation(program, "MVP");
    vpos_location = glGetAttribLocation(program, "vPos");
    vcol_location = glGetAttribLocation(program, "vCol");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) 0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) (sizeof(float) * 2));

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

    app.main_loop([&](){
      if (window.should_close())
        app.exit();

      float ratio;
      int width, height;
      mat4x4 m, p, mvp;

      glfwGetFramebufferSize(window.handle(), &width, &height);
      ratio = width / (float) height;

      glViewport(0, 0, width, height);
      glClear(GL_COLOR_BUFFER_BIT);

      mat4x4_identity(m);
      mat4x4_rotate_Z(m, m, (float) glfwGetTime());
      mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
      mat4x4_mul(mvp, p, m);

      glUseProgram(program);
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
