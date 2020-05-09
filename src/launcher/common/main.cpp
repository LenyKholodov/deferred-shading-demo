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

    media::geometry::Mesh media_mesh = media::geometry::MeshFactory::create_sphere(0.5f);

    media_mesh = media_mesh.merge(media::geometry::MeshFactory::create_box(1.5f, 0.1f, 0.1f));
    media_mesh = media_mesh.merge(media::geometry::MeshFactory::create_box(0.1f, 1.5f, 0.1f));
    media_mesh = media_mesh.merge(media::geometry::MeshFactory::create_box(0.1f, 0.1f, 1.5f));

    media::geometry::Mesh media_mesh2 = media::geometry::MeshFactory::create_sphere(0.25f);

    media_mesh2 = media_mesh2.merge(media::geometry::MeshFactory::create_box(0.75f, 0.05f, 0.05f));
    media_mesh2 = media_mesh2.merge(media::geometry::MeshFactory::create_box(0.05f, 0.75f, 0.05f));
    media_mesh2 = media_mesh2.merge(media::geometry::MeshFactory::create_box(0.05f, 0.05f, 0.75f));

    render::Mesh mesh = render_device.create_mesh(media_mesh);
    render::Mesh mesh2 = render_device.create_mesh(media_mesh2);

    render::Program program = render_device.create_program_from_file("media/shaders/simple.glsl");

    GLint mvp_location;
    mvp_location = program.get_uniform_location("MVP");

    render::Pass pass = render_device.create_pass(program);

    pass.set_clear_color(math::vec4f(0.2f));

    pass.set_depth_stencil_state(render::DepthStencilState(true, true, render::CompareMode_Less));

    render::Pass pass2 = render_device.create_pass(program);

    pass2.set_blend_state(render::BlendState(true, render::BlendArgument::BlendArgument_SourceColor, render::BlendArgument::BlendArgument_SourceColor));
    pass2.set_depth_stencil_state(render::DepthStencilState(true, true, render::CompareMode_Less));
    pass2.set_clear_flags(render::Clear_DepthStencil);

      //main loop

    app.main_loop([&](){
      if (window.should_close())
        app.exit();

      int width = window.frame_buffer_width(), height = window.frame_buffer_height();
      float ratio = width / (float) height;

      mat4x4 m, p, mvp;

      mat4x4_identity(m);
      mat4x4_rotate_Y(m, m, (float) glfwGetTime() / 2);
      mat4x4_rotate_Z(m, m, (float) glfwGetTime());
      mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
      mat4x4_mul(mvp, p, m);

      program.bind();

      glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);

      pass.add_mesh(mesh);

      pass.render();

      pass2.add_mesh(mesh2);

      pass2.render();

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
