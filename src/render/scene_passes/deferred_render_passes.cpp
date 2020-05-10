#include "shared.h"

using namespace engine::render::scene;
using namespace engine::render::low_level;
using namespace engine::scene;
using namespace engine::common;

namespace engine {
namespace render {
namespace scene {
namespace passes {

///
/// Constants
///

static const char* GBUFFER_PROGRAM_FILE = "media/shaders/phong_gbuffer.glsl";
static const char* DEFERRED_LIGHTING_PROGRAM_FILE = "media/shaders/lighting.glsl";

///
/// G-Buffer
///

struct GBufferPass : IScenePass
{
  public:
    GBufferPass(SceneRenderer& renderer, Device& device)
      : g_buffer_width(device.window().frame_buffer_width())
      , g_buffer_height(device.window().frame_buffer_height())
      , g_buffer_program(device.create_program_from_file(GBUFFER_PROGRAM_FILE))
      , g_buffer_pass(device.create_pass(g_buffer_program))
      , positions_texture(device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat_RGB16F, 1))
      , normals_texture(device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat_RGB16F, 1))
      , albedo_texture(device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat_RGBA8, 1))
      , specular_texture(device.create_texture2d(g_buffer_width, g_buffer_height, PixelFormat_RGBA8, 1))
      , g_buffer_depth(device.create_render_buffer(g_buffer_width, g_buffer_height, PixelFormat_D24))
      , g_buffer_frame_buffer(device.create_frame_buffer())
    {
      TextureList shared_textures = renderer.textures();

      shared_textures.insert("positionTexture", positions_texture);
      shared_textures.insert("normalTexture", normals_texture);
      shared_textures.insert("albedoTexture", albedo_texture);
      shared_textures.insert("specularTexture", specular_texture);

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

      g_buffer_pass.set_frame_buffer(g_buffer_frame_buffer);

      g_buffer_pass.set_clear_color(0.0f);

      g_buffer_pass.set_depth_stencil_state(DepthStencilState(true, true, CompareMode_Less));

      engine_log_debug("G-Buffer has been created: %ux%u", g_buffer_width, g_buffer_height);
    }

    static IScenePass* create(SceneRenderer& renderer, Device& device)
    {
      return new GBufferPass(renderer, device);
    }

    void get_dependencies(std::vector<std::string>&)
    {
    }

    void render(ScenePassContext& context)
    {
      Node::Pointer root_node = context.root_node();

      if (!root_node)
        return;

        //traverse scene

      visitor.traverse(*root_node);

        //draw geometry

      for (auto& mesh : visitor.meshes())
      {
        render_mesh(*mesh, context);
      }

        //clear data

      visitor.reset();

        //update frame

      frame.add_pass(g_buffer_pass);
      context.root_frame_node().add_dependency(frame);
    }

  private:
    void render_mesh(engine::scene::Mesh& mesh, ScenePassContext& context)
    {
        //create mesh data

      RenderableMesh* renderable_mesh = mesh.find_user_data<RenderableMesh>();

      if (!renderable_mesh)
      {
        renderable_mesh = &mesh.set_user_data(RenderableMesh(mesh, context));
      }

        //add mess to pass

      g_buffer_pass.add_mesh(renderable_mesh->mesh, mesh.world_tm());
    }

  private:
    size_t g_buffer_width;
    size_t g_buffer_height;
    Program g_buffer_program;
    Pass g_buffer_pass;
    Texture positions_texture;
    Texture normals_texture;
    Texture albedo_texture;
    Texture specular_texture;
    RenderBuffer g_buffer_depth;
    FrameBuffer g_buffer_frame_buffer;
    SceneVisitor visitor;
    FrameNode frame;
};

///
/// Deferred lighting pass
///

struct DeferredLightingPass : IScenePass
{
  public:
    DeferredLightingPass(SceneRenderer& renderer, Device& device)
      : deferred_lighting_program(device.create_program_from_file(DEFERRED_LIGHTING_PROGRAM_FILE))
      , deferred_lighting_pass(device.create_pass(deferred_lighting_program))
      , plane(device.create_plane(Material()))
    {
      deferred_lighting_pass.set_depth_stencil_state(DepthStencilState(false, false, CompareMode_AlwaysPass));

      engine_log_debug("Deferred Lighting pass has been created");
    }

    static IScenePass* create(SceneRenderer& renderer, Device& device)
    {
      return new DeferredLightingPass(renderer, device);
    }

    void get_dependencies(std::vector<std::string>& deps)
    {
      deps.push_back("G-Buffer");
    }

    void render(ScenePassContext& context)
    {
      deferred_lighting_pass.add_primitive(plane);

      //TODO: add dependency for G-Buffer frame instead of implicit scene pass dependency for now

      frame.add_pass(deferred_lighting_pass);

      context.root_frame_node().add_dependency(frame);
    }

  private:
    Program deferred_lighting_program;
    Pass deferred_lighting_pass;
    Primitive plane;
    FrameNode frame;
};

///
/// Component registration
///

struct DeferredRenderingComponent : Component
{
  void load()
  {
    ScenePassFactory::register_scene_pass("G-Buffer", &GBufferPass::create);
    ScenePassFactory::register_scene_pass("Deferred Lighting", &DeferredLightingPass::create);
  }

  void unload()
  {
    ScenePassFactory::unregister_scene_pass("G-Buffer");
    ScenePassFactory::unregister_scene_pass("Deferred Lighting");
  }
};

static DeferredRenderingComponent component;

}}}}
