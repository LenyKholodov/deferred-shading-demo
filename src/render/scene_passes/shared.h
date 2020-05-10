#include <render/scene_render.h>

#include <scene/camera.h>
#include <scene/mesh.h>
#include <scene/light.h>

#include <application/window.h>

#include <common/exception.h>
#include <common/log.h>
#include <common/string.h>
#include <common/component.h>

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
