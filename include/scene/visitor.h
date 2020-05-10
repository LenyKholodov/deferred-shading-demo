#pragma once

namespace engine {
namespace scene {

//forwards
class Node;
class Camera;
class PerspectiveCamera;
class Mesh;
class Light;
class SpotLight;
class PointLight;

/// Very simple scene visitor with hard dependencies from scene classes
class ISceneVisitor
{
  public:
    virtual void visit(Node&) {}
    virtual void visit(Camera&) {}
    virtual void visit(PerspectiveCamera&) {}
    virtual void visit(Mesh&) {}
    virtual void visit(Light&) {}
    virtual void visit(SpotLight&) {}
    virtual void visit(PointLight&) {}

  protected:
    virtual ~ISceneVisitor() = default;
};;

}}
