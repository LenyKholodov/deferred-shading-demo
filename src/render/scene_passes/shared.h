#include <render/scene_render.h>

#include <scene/camera.h>
#include <scene/mesh.h>
#include <scene/light.h>

#include <application/window.h>

#include <common/exception.h>
#include <common/log.h>
#include <common/string.h>
#include <common/component.h>

#include <math/utility.h>

namespace engine {
namespace render {
namespace scene {
namespace passes {

typedef std::vector<engine::scene::Mesh::Pointer> MeshArray;
typedef std::vector<engine::scene::PointLight::Pointer> PointLightArray;
typedef std::vector<engine::scene::SpotLight::Pointer> SpotLightArray;

/// Rendering mesh data
struct RenderableMesh
{
  low_level::Mesh mesh;

  RenderableMesh(engine::scene::Mesh& mesh, ScenePassContext& context)
    : mesh(context.device().create_mesh(mesh.mesh(), context.materials()))
  {
  }
};

/// Shadow
struct Shadow
{
  low_level::Texture shadow_texture;
  low_level::Pass shadow_pass;
  low_level::FrameBuffer shadow_frame_buffer;
  FrameNode shadow_frame;
  math::mat4f shadow_tm;

  Shadow(engine::render::low_level::Device& device, const low_level::Program& program, size_t shadow_map_size)
    : shadow_texture(device.create_texture2d(shadow_map_size, shadow_map_size, low_level::PixelFormat_D24))
    , shadow_pass(device.create_pass(program))
    , shadow_frame_buffer(device.create_frame_buffer())
    , shadow_tm(1.0f)
  {
    shadow_texture.set_min_filter(low_level::TextureFilter_Point);

    shadow_frame_buffer.attach_depth_buffer(shadow_texture);
    shadow_frame_buffer.set_viewport(low_level::Viewport(0, 0, (int)shadow_map_size, (int)shadow_map_size));

    shadow_pass.set_frame_buffer(shadow_frame_buffer);
    shadow_pass.set_depth_stencil_state(low_level::DepthStencilState(true, true, low_level::CompareMode_Less));
  }
};

/// Scene visitor
class SceneVisitor : private engine::scene::ISceneVisitor
{
  public:
    /// Constructor
    SceneVisitor();

    /// Meshes
    const MeshArray& meshes() const;

    /// Point lights
    const PointLightArray& point_lights() const;

    /// Spot lights
    const SpotLightArray& spot_lights() const;

    /// Reset results
    void reset();

    /// Traverse scene
    void traverse(engine::scene::Node&);

  private:
    void visit(engine::scene::Mesh&) override;
    void visit(engine::scene::SpotLight&) override;
    void visit(engine::scene::PointLight&) override;

  private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};

}}}}
