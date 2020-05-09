#include "shared.h"

#include <vector>

using namespace engine::render;

typedef std::vector<Primitive> PrimitiveArray;

/// Implementation details of mesh
struct Mesh::Impl
{
  DeviceContextPtr context; //device context
  VertexBuffer vertex_buffer; //vertex buffer
  IndexBuffer index_buffer; //index buffer
  PrimitiveArray primitives; //primitives

  Impl(const DeviceContextPtr& context, const media::geometry::Mesh& mesh)
    : context(context)
    , vertex_buffer(context, mesh.vertices_count())
    , index_buffer(context, mesh.indices_count())
  {
    primitives.reserve(mesh.primitives_count());

    vertex_buffer.set_data(0, mesh.vertices_count(), mesh.vertices_data());
    index_buffer.set_data(0, mesh.indices_count(), mesh.indices_data());

    for (uint32_t i = 0, count = mesh.primitives_count(); i < count; i++)
    {
      const media::geometry::Primitive& src_primitive = mesh.primitive(i);

      primitives.emplace_back(Primitive(src_primitive.type, vertex_buffer, index_buffer, src_primitive.first, src_primitive.count, src_primitive.base_vertex));
    }
  }
};

Mesh::Mesh(const DeviceContextPtr& context, const media::geometry::Mesh& mesh)
  : impl(std::make_shared<Impl>(context, mesh))
{
}

size_t Mesh::primitives_count() const
{
  return impl->primitives.size();
}

const Primitive& Mesh::primitive(size_t index) const
{
  if (index >= impl->primitives.size())
    throw common::Exception(common::format("engine::render::Mesh::primitive index %u out of bounds [0;%z)", index, impl->primitives.size()).c_str());

  return impl->primitives[index];
}
