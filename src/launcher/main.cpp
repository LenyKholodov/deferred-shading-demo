#include <scene/camera.h>
#include <scene/mesh.h>
#include <scene/light.h>
#include <render/scene_render.h>
#include <media/image.h>
#include <scene/camera.h>
#include <scene/node.h>
#include <scene/light.h>
#include <scene/mesh.h>
#include <application/application.h>
#include <application/window.h>
#include <common/exception.h>
#include <common/log.h>
#include <common/component.h>
#include <math/utility.h>

#include <string>
#include <ctime>
#include <cstdlib>
#include <cstdio>

using namespace engine::common;
using namespace engine::render::scene;
using namespace engine::render::low_level;
using namespace engine::scene;
using namespace engine::application;
using namespace engine::scene;
using namespace engine;

namespace
{

const float CAMERA_MOVE_SPEED = 10.f;
const float CAMERA_ROTATE_SPEED = 0.5f;
const float FOV_X = 90.f;
const math::vec3f LIGHTS_ATTENUATION(1.0f, 0.35f, 0.44f);
const size_t LIGHTS_COUNT = 32;
const float LIGHTS_POSITION_RADIUS = 30.f;
const float LIGHTS_MIN_RANGE = 30.f;
const float LIGHTS_MAX_RANGE = 100.f;
const size_t SHADOW_MAP_SIZE = 1024;
const size_t MESHES_COUNT = 100;
const float MESHES_POSITION_RADIUS = 3.f;

}

int main(void)
{
  try
  {
    engine_log_info("Application has been started");

      //components loading

    ComponentScope components("engine::render::scene::passes::*");

      //application setup

    PerspectiveCamera::Pointer camera = PerspectiveCamera::create();
    math::vec3f camera_position(0.f, 10.f, -10.f);
    math::anglef camera_pitch(math::degree(30.f));
    math::anglef camera_yaw(math::degree(0.f));
    math::anglef camera_roll(math::degree(0.f));
    math::vec3f camera_move_direction(0.f);

    Application app;
    Window window("Render test");

    window.set_keyboard_handler([&](Key key, bool pressed) {
      math::vec3f direction_change;

      bool camera_position_changed = false;

      switch (key)
      {
        case Key_Up:
        case Key_W:
          direction_change = math::vec3f(0.f,0.f,pressed ? 1.f : -1.f);
          camera_position_changed = true;
          break;
        case Key_Down:
        case Key_S:
          direction_change = math::vec3f(0.f,0.f,pressed ? -1.f : 1.f);
          camera_position_changed = true;
          break;
        case Key_Right:
        case Key_D:
          direction_change = math::vec3f(pressed ? -1.f : 1.f,0.f,0.f);
          camera_position_changed = true;
          break;
        case Key_Left:
        case Key_A:
          direction_change = math::vec3f(pressed ? 1.f : -1.f,0.f,0.f);
          camera_position_changed = true;
          break;
        case Key_Escape:
          engine_log_info("Escape pressed. Exiting...");
          window.close();
          break;
        default:
          break;
      }

      camera_move_direction += direction_change;
    });

    bool left_mouse_button_pressed = false;
    double last_mouse_x;
    double last_mouse_y;

    window.set_mouse_move_handler([&](double x, double y) {
      //double relative_x = x / window.width();
      //double relative_y = y / window.height();

      //engine_log_info("mouse move pos=(%.1f, %.1f) <-> (%.2f, %.2f)", x, y, relative_x, relative_y);

      if (left_mouse_button_pressed)
      {
        double dx = x - last_mouse_x;
        double dy = y - last_mouse_y;

        camera_pitch += math::degree(dy * CAMERA_ROTATE_SPEED);
        camera_yaw -= math::degree(dx * CAMERA_ROTATE_SPEED);

        camera->set_orientation(math::to_quat(camera_pitch, camera_yaw, camera_roll));
      }

      last_mouse_x = x;
      last_mouse_y = y;
    });

    window.set_mouse_button_handler([&](MouseButton button, bool pressed) {
      //engine_log_info("mouse button=%d pressed=%d", button, pressed);

      if (button == MouseButton_Left)
        left_mouse_button_pressed = pressed;
    });

    float window_ratio = window.width() / (float) window.height();

      //scene setup

    Node::Pointer scene_root = Node::create();

    camera->set_z_near(1.f);
    camera->set_z_far(1000.f);
    camera->set_fov_x(math::degree(FOV_X));
    camera->set_fov_y(math::degree(FOV_X / window_ratio));
    camera->set_position(camera_position);
    camera->set_orientation(math::to_quat(camera_pitch, camera_yaw, camera_roll));

    camera->bind_to_parent(*scene_root);

      //scene geometry

    scene::Mesh::Pointer floor = scene::Mesh::create();
    media::geometry::Mesh floor_mesh = media::geometry::MeshFactory::create_box("mtl1", 50.f, 0.01f, 50.f);

    floor->set_mesh(floor_mesh);
    floor->bind_to_parent(*scene_root);

    scene::Mesh::Pointer mesh = scene::Mesh::create();

    media::geometry::Mesh media_mesh;

    int row_size = ::floor(sqrt(MESHES_COUNT));

    for (int i = 0; i < row_size; i++)
    {
      for (int j = 0; j < row_size; j++)
      {
        math::vec3f offset((i - row_size / 2) * MESHES_POSITION_RADIUS, 0.5f, (j - row_size / 2) * MESHES_POSITION_RADIUS);

        if ((i + j) % 2)
          media_mesh = media_mesh.merge(media::geometry::MeshFactory::create_box("mtl1", 1.f, 1.f, 1.f, offset));
        else
          media_mesh = media_mesh.merge(media::geometry::MeshFactory::create_sphere("mtl1", 1.f, offset));
      }
    }

    mesh->set_mesh(media_mesh.merge_primitives());
    mesh->bind_to_parent(*scene_root);


      //scene lights
    Node::Pointer lights_parent = Node::create();

    lights_parent->bind_to_parent(*scene_root);

    std::vector<scene::PointLight::Pointer> point_lights;

    point_lights.reserve(LIGHTS_COUNT);

    for (size_t i = 0, count = point_lights.size(); i < count; i++)
    {
      scene::PointLight::Pointer light = scene::PointLight::create();

      light->set_light_color(math::vec3f(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX));
      light->set_attenuation(LIGHTS_ATTENUATION);
      light->set_intensity(4.0f);
      light->set_range(LIGHTS_MIN_RANGE + rand() / (float)RAND_MAX * (LIGHTS_MAX_RANGE - LIGHTS_MIN_RANGE));
      light->set_position(math::vec3f(LIGHTS_POSITION_RADIUS * cos(math::constf::pi * 2.f * i / count), 5.f, LIGHTS_POSITION_RADIUS * sin(math::constf::pi * 2.f * i / count)));

      light->bind_to_parent(*lights_parent);

      point_lights.emplace_back(light);
    }

    scene::SpotLight::Pointer spot_light = scene::SpotLight::create();

    spot_light->set_attenuation(LIGHTS_ATTENUATION);
    spot_light->set_intensity(4.0f);
    spot_light->set_range(100.f);
    spot_light->set_angle(math::degree(30.f));
    spot_light->set_exponent(0.8f);
    spot_light->set_position(math::vec3f(-10.f, 10.f, 0.f));
    spot_light->set_orientation(math::to_quat(math::degree(-30.f), math::degree(45.f), camera_roll));
    spot_light->bind_to_parent(*lights_parent);

      //render setup

    DeviceOptions render_options;

    SceneRenderer scene_renderer(window, render_options);
    Device render_device = scene_renderer.device();

    scene_renderer.add_pass("Deferred Lighting");

      //resources creation

    media::image::Image diffuse_map ("media/textures/brickwall_diffuse.jpg");
    media::image::Image normal_map ("media/textures/brickwall_normal.jpg");
    media::image::Image specular_map ("media/textures/brickwall_specular.jpg");

    Texture model_diffuse_texture = render_device.create_texture2d(diffuse_map.width(), diffuse_map.height(), PixelFormat_RGBA8);
    Texture model_normal_texture = render_device.create_texture2d(normal_map.width(), normal_map.height(), PixelFormat_RGBA8);
    Texture model_specular_texture = render_device.create_texture2d(specular_map.width(), specular_map.height(), PixelFormat_RGBA8);

    model_diffuse_texture.set_min_filter(TextureFilter_LinearMipLinear);
    model_normal_texture.set_min_filter(TextureFilter_LinearMipLinear);
    model_specular_texture.set_min_filter(TextureFilter_LinearMipLinear);

    model_diffuse_texture.set_data(0, 0, 0, diffuse_map.width(), diffuse_map.height(), diffuse_map.bitmap());
    model_diffuse_texture.generate_mips();
    model_normal_texture.set_data(0, 0, 0, normal_map.width(), normal_map.height(), normal_map.bitmap());
    model_normal_texture.generate_mips();
    model_specular_texture.set_data(0, 0, 0, specular_map.width(), specular_map.height(), specular_map.bitmap());
    model_specular_texture.generate_mips();

    model_diffuse_texture.set_min_filter(TextureFilter_LinearMipLinear);
    model_normal_texture.set_min_filter(TextureFilter_LinearMipLinear);
    model_specular_texture.set_min_filter(TextureFilter_LinearMipLinear);

    Material mtl1;
    PropertyMap mtl1_properties = mtl1.properties();

    mtl1_properties.set("shininess", 50.f);

    TextureList mtl1_textures = mtl1.textures();

    mtl1_textures.insert("diffuseTexture", model_diffuse_texture);
    mtl1_textures.insert("normalTexture", model_normal_texture);
    mtl1_textures.insert("specularTexture", model_specular_texture);

    MaterialList materials = scene_renderer.materials();

    materials.insert("mtl1", mtl1);

      //scene viewport setup

    SceneViewport scene_viewport;

    scene_viewport.set_camera(camera);

      //main loop

    double last_time = app.time();

    app.main_loop([&]()
    {
      if (window.should_close())
        app.exit();

      double new_time = app.time();
      double dt = new_time - last_time;

      last_time = new_time;

      if (!math::equal(camera_move_direction, math::vec3f(0.f), 0.1f))
      {
        camera_position += math::to_quat(camera_pitch, camera_yaw, camera_roll) * camera_move_direction * CAMERA_MOVE_SPEED * dt;

        camera->set_position(camera_position);
      }

        //render scene

      scene_renderer.render(scene_viewport);

        //image presenting

      window.swap_buffers();

        //animate objects

      static const size_t TIMEOUT_MS = 10;

      return TIMEOUT_MS;
    });

#if 0
      //render setup

    DeviceOptions render_options;

    //render_options.debug = false;

    Device render_device(window, render_options);
    FrameBuffer shadow_frame_buffer = render_device.create_frame_buffer();
    FrameBuffer g_buffer_frame_buffer = render_device.create_frame_buffer();
    FrameBuffer frame_buffer = render_device.window_frame_buffer();

    Texture shadow_texture = render_device.create_texture2d(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, PixelFormat_D24, 1);

    shadow_texture.set_min_filter(TextureFilter_Point);

    shadow_frame_buffer.attach_depth_buffer(shadow_texture);

    shadow_frame_buffer.reset_viewport();

    int g_buffer_width = window.frame_buffer_width();
    int g_buffer_height = window.frame_buffer_height();

    Texture positions_texture = render_device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat_RGB16F, 1);
    Texture normals_texture = render_device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat_RGB16F, 1);
    Texture albedo_texture = render_device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat_RGBA8, 1);
    Texture specular_texture = render_device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat_RGBA8, 1);
    RenderBuffer g_buffer_depth = render_device.create_render_buffer(g_buffer_width, g_buffer_height, PixelFormat_D24);

    positions_texture.set_min_filter(TextureFilter_Point);
    normals_texture.set_min_filter(TextureFilter_Point);
    albedo_texture.set_min_filter(TextureFilter_Point);
    specular_texture.set_min_filter(TextureFilter_Point);

    g_buffer_frame_buffer.attach_color_target(positions_texture);
    g_buffer_frame_buffer.attach_color_target(normals_texture);
    g_buffer_frame_buffer.attach_color_target(albedo_texture);
    g_buffer_frame_buffer.attach_color_target(specular_texture);
    g_buffer_frame_buffer.attach_depth_buffer(g_buffer_depth);

    g_buffer_frame_buffer.reset_viewport();

    Program shadow_program = render_device.create_program_from_file("media/shaders/shadow.glsl");

    Pass shadow_pass = render_device.create_pass(shadow_program);

    shadow_pass.set_frame_buffer(shadow_frame_buffer);

    shadow_pass.set_depth_stencil_state(DepthStencilState(true, true, CompareMode_Less));

    Program program = render_device.create_program_from_file("media/shaders/phong_gbuffer.glsl");

    Pass pass = render_device.create_pass(program);

    pass.set_frame_buffer(g_buffer_frame_buffer);

    pass.set_depth_stencil_state(DepthStencilState(true, true, CompareMode_Less));

    Pass pass2 = render_device.create_pass(program);

    pass2.set_frame_buffer(g_buffer_frame_buffer);

    pass2.set_depth_stencil_state(DepthStencilState(true, true, CompareMode_Less));

    pass2.set_clear_flags(Clear_None);

    Program lighting_program = render_device.create_program_from_file("media/shaders/lighting.glsl");

    Pass light_pass = render_device.create_pass(lighting_program);

    light_pass.set_depth_stencil_state(DepthStencilState(false, false, CompareMode_AlwaysPass));

    light_pass.set_clear_flags(Clear_None);

    Primitive plane = render_device.create_plane(mtl2);

    PropertyMap view_properties;

    view_properties.set("shininess", 50.f);

      //main loop

    app.main_loop([&](){
      if (window.should_close())
        app.exit();

        //update scene
      lights_parent->set_orientation(math::to_quat(math::degree(0.f), math::degree((float)Application::time()), math::degree(0.f)));

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

      shadow_pass.add_mesh(mesh);

      shadow_pass.render(&frame_bindings);

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

      lighting_program.bind();

      light_pass.add_primitive(plane);

      light_pass.render(&frame_bindings);

        //image presenting

      window.swap_buffers();

      static const size_t TIMEOUT_MS = 10;

      return TIMEOUT_MS;
    });
#endif

    engine_log_info("Exiting from application...");

    return 0;
  }
  catch (std::exception& e)
  {
    engine_log_fatal("%s\n", e.what());
    return 1;
  }
}
