#include <render/device.h>
#include <media/image.h>
#include <application/application.h>
#include <application/window.h>
#include <common/exception.h>
#include <common/log.h>
#include <common/component.h>

#include <string>
#include <ctime>

extern "C"
{
#include "linmath.h"
}

#include <stdlib.h>
#include <stdio.h>

using namespace engine::common;
using namespace engine::render::low_level;
using namespace engine::application;
using namespace engine;

int main(void)
{
  try
  {
    engine_log_info("Application has been started");

      //components loading

    ComponentScope components("render::scene::passes::*");

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

    DeviceOptions render_options;

    //render_options.debug = false;

    Device render_device(window, render_options);
    FrameBuffer g_buffer_frame_buffer = render_device.create_frame_buffer();
    FrameBuffer frame_buffer = render_device.window_frame_buffer();

    int g_buffer_width = window.frame_buffer_width();
    int g_buffer_height = window.frame_buffer_height();

    Texture positions_texture = render_device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat::PixelFormat_RGB16F, 1);
    Texture normals_texture = render_device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat::PixelFormat_RGB16F, 1);
    Texture albedo_texture = render_device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat::PixelFormat_RGBA8, 1);
    Texture specular_texture = render_device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat::PixelFormat_RGBA8, 1);

    g_buffer_frame_buffer.attach_color_target(positions_texture);
    g_buffer_frame_buffer.attach_color_target(normals_texture);
    g_buffer_frame_buffer.attach_color_target(albedo_texture);
    g_buffer_frame_buffer.attach_color_target(specular_texture);

    g_buffer_frame_buffer.reset_viewport();

    media::image::Image diffuse_map ("media/textures/brickwall_diffuse.jpg");
    media::image::Image normal_map ("media/textures/brickwall_normal.jpg");
    media::image::Image specular_map ("media/textures/brickwall_specular.jpg");

    Texture model_diffuse_texture = render_device.create_texture2d(diffuse_map.width(), diffuse_map.height(), PixelFormat::PixelFormat_RGBA8);
    Texture model_normal_texture = render_device.create_texture2d(normal_map.width(), normal_map.height(), PixelFormat::PixelFormat_RGBA8);
    Texture model_specular_texture = render_device.create_texture2d(specular_map.width(), specular_map.height(), PixelFormat::PixelFormat_RGBA8);

    model_diffuse_texture.set_min_filter(TextureFilter_LinearMipLinear);
    model_normal_texture.set_min_filter(TextureFilter_LinearMipLinear);
    model_specular_texture.set_min_filter(TextureFilter_LinearMipLinear);

    model_diffuse_texture.set_data(0, 0, 0, diffuse_map.width(), diffuse_map.height(), diffuse_map.bitmap());
    model_diffuse_texture.generate_mips();
    model_normal_texture.set_data(0, 0, 0, normal_map.width(), normal_map.height(), normal_map.bitmap());
    model_normal_texture.generate_mips();
    model_specular_texture.set_data(0, 0, 0, specular_map.width(), specular_map.height(), specular_map.bitmap());
    model_specular_texture.generate_mips();

    media::geometry::Mesh media_mesh = media::geometry::MeshFactory::create_sphere("mtl1", 0.5f);

    media_mesh = media_mesh.merge(media::geometry::MeshFactory::create_box("mtl1", 1.5f, 0.1f, 0.1f));
    media_mesh = media_mesh.merge(media::geometry::MeshFactory::create_box("mtl1", 0.1f, 1.5f, 0.1f));
    media_mesh = media_mesh.merge(media::geometry::MeshFactory::create_box("mtl1", 0.1f, 0.1f, 1.5f));

    media::geometry::Mesh media_mesh2 = media::geometry::MeshFactory::create_sphere("mtl1", 0.25f);

    media_mesh2 = media_mesh2.merge(media::geometry::MeshFactory::create_box("mtl1", 0.75f, 0.05f, 0.05f));
    media_mesh2 = media_mesh2.merge(media::geometry::MeshFactory::create_box("mtl1", 0.05f, 0.75f, 0.05f));
    media_mesh2 = media_mesh2.merge(media::geometry::MeshFactory::create_box("mtl1", 0.05f, 0.05f, 0.75f));

    Material mtl1;
    TextureList mtl1_textures = mtl1.textures();

    mtl1_textures.insert("diffuse_texture", model_diffuse_texture);

    MaterialList materials;

    materials.insert("mtl1", mtl1);

    Mesh mesh = render_device.create_mesh(media_mesh, materials);
    Mesh mesh2 = render_device.create_mesh(media_mesh2, materials);

    //Program program = render_device.create_program_from_file("media/shaders/simple.glsl");
    Program program = render_device.create_program_from_file("media/shaders/simple_tex.glsl");

    Pass pass = render_device.create_pass(program);

 //   pass.set_frame_buffer(g_buffer_frame_buffer);

    pass.set_depth_stencil_state(DepthStencilState(true, true, CompareMode_Less));

    Pass pass2 = render_device.create_pass(program);

   // pass2.set_frame_buffer(g_buffer_frame_buffer);

    pass2.set_depth_stencil_state(DepthStencilState(true, true, CompareMode_Less));

    pass2.set_clear_flags(ClearFlags::Clear_None);

    Program lighting_program = render_device.create_program_from_file("media/shaders/lighting.glsl");

    Pass light_pass = render_device.create_pass(lighting_program);

    Primitive plane = render_device.create_plane(mtl1);

    PropertyMap view_properties;

      //main loop

    app.main_loop([&](){
      if (window.should_close())
        app.exit();

        //viewport adjustments according to a window size

      render_device.window_frame_buffer().reset_viewport();

        //passes

      int width = window.frame_buffer_width(), height = window.frame_buffer_height();
      float ratio = width / (float) height;
      
      mat4x4 m, p, mvp;

      mat4x4_identity(m);
      mat4x4_translate(m, -0.5f, 0.f, 0.f);
      mat4x4_rotate_Y(m, m, (float) Application::time() / 2);
      mat4x4_rotate_Z(m, m, (float) Application::time());
      mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
      mat4x4_mul(mvp, p, m);

      math::mat4f mvp_transposed(&mvp[0][0]);

      view_properties.set("MVP", transpose(mvp_transposed));

      BindingContext frame_bindings(view_properties);

      pass.add_mesh(mesh);

      pass.render(&frame_bindings);

      mat4x4_identity(m);
      mat4x4_translate(m, 0.5f, 0.f, 0.f);
      mat4x4_rotate_Y(m, m, (float) Application::time() / 2);
      mat4x4_rotate_Z(m, m, (float) Application::time());
      mat4x4_rotate_Z(m, m, (float) Application::time());
      mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
      mat4x4_mul(mvp, p, m);

      mvp_transposed = math::mat4f(&mvp[0][0]);
      view_properties.set("MVP", transpose(mvp_transposed));

      pass2.add_mesh(mesh2);

      pass2.render(&frame_bindings);

      mat4x4_identity(m);
      mat4x4_ortho(p, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f);
      mat4x4_mul(mvp, p, m);

      lighting_program.bind();

      light_pass.add_primitive(plane);

//      light_pass.render();

        //image presenting

      window.swap_buffers();

      static const size_t TIMEOUT_MS = 10;

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
