#pragma once

#include <math/matrix.h>
#include <math/quat.h>

#include <memory>

namespace engine {
namespace scene {

/// Scene node
class Node : public std::enable_shared_from_this<Node>
{
  public: 
    typedef std::shared_ptr<Node> Pointer;

    /// Create node
    static Pointer create();

    /// No copy
    Node(const Node&) = delete;
    Node& operator =(const Node&) = delete;

    /// Node parent
    Pointer parent() const;

    /// First node child
    Pointer first_child() const;

    /// Last node child
    Pointer last_child() const;

    /// Prev node (within parent's children chain)
    Pointer prev_child() const;

    /// Next node (within parent's children chain)
    Pointer next_child() const;

    /// Bind to parent
    void bind_to_parent(Node& parent);

    /// Unbind from parent
    void unbind();

    /// Unbind all children
    void unbind_all_children();

    /// Node local position
    const math::vec3f& position() const;

    /// Set node local position    
    void set_position(const math::vec3f&);
    
    /// Node local orientation
    const math::quatf& orientation() const;

    /// Set node local orientation
    void set_orientation(const math::quatf&);    

    /// Node scale
    const math::vec3f& scale() const;

    /// Set node scale
    void set_scale(const math::vec3f&);

    /// Local space node transformations
    const math::mat4f& local_tm() const;

    /// World space node transformations
    const math::mat4f& world_tm() const;
    
  protected:
    /// Constructor
    Node();

    /// Destructor
    virtual ~Node();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}}
