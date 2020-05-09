#include <common/string.h>
#include <common/uninitialized_storage.h>

#include <media/geometry.h>

#include <vector>

using namespace engine::media::geometry;
using namespace engine::common;

typedef std::vector<Primitive> PrimitiveArray;

/// Mesh implementation
struct Mesh::Impl
{
  UninitializedStorage<Vertex> vertices_data;
  UninitializedStorage<IndexType> indices_data;
  PrimitiveArray primitives;

  uint32_t add_primitive(PrimitiveType type, uint32_t first, uint32_t count, uint32_t base_vertex)
  {
    if (type < 0 || type >= PrimitiveType_Num)
      throw Exception(format("Can't add unknown primitive type %d", type).c_str());

    Primitive primitive;

    primitive.type = type;
    primitive.first = first;
    primitive.count = count;
    primitive.base_vertex = base_vertex;

    primitives.push_back(primitive);

    return (uint32_t)primitives.size() - 1;
  }

  uint32_t add_primitive(PrimitiveType type, const Vertex* vertices, IndexType vertices_count, const IndexType* indices, uint32_t indices_count)
  {
    //TODO check vertices/indices count limit

    uint32_t current_vertices_count = static_cast<uint32_t>(vertices_data.size());
    uint32_t current_indices_count = static_cast<uint32_t>(indices_data.size());

    //resize buffers to store additional data
    vertices_data.resize(current_vertices_count + vertices_count);

    try
    {
      indices_data.resize(current_indices_count + indices_count);
    }
    catch (...)
    {
      //restore previous vertices size if indices resize failed
      vertices_data.resize(current_vertices_count);
      throw;
    }

    //copy data
    memcpy(vertices_data.data() + current_vertices_count, vertices, vertices_count * sizeof(Vertex));
    memcpy(indices_data.data() + current_indices_count, indices, indices_count * sizeof(IndexType));

    //add primitive
    return add_primitive(type, 0, indices_count, current_indices_count);
  }

  Mesh merge(const Mesh& mesh)
  {
    //TODO check vertices/indices count limit
    //TODO code can be optimized - copy larger mesh first and smaller mesh second

    Mesh return_value;

    uint32_t vertices_count             = static_cast<uint32_t>(vertices_data.size()),
             indices_count              = static_cast<uint32_t>(indices_data.size()),
             second_mesh_vertices_count = mesh.vertices_count(),
             second_mesh_indices_count  = mesh.indices_count();

    //allocate mempry
    return_value.vertices_resize(vertices_count + second_mesh_vertices_count);
    return_value.indices_resize(indices_count + second_mesh_indices_count);

    //copy buffers
    memcpy(return_value.vertices_data(), vertices_data.data(), vertices_count * sizeof(Vertex));
    memcpy(return_value.indices_data(), indices_data.data(), indices_count * sizeof(IndexType));
    memcpy(return_value.vertices_data() + vertices_count, mesh.vertices_data(), second_mesh_vertices_count * sizeof(Vertex));
    memcpy(return_value.indices_data() + indices_count, mesh.indices_data(), second_mesh_indices_count * sizeof(IndexType));

    //fix second mesh indices
    IndexType* current_index = return_value.indices_data() + indices_count;

    for (uint32_t i = 0; i < second_mesh_indices_count; i++, current_index++)
      *current_index += indices_count;

    //copy primitives
    for (size_t i = 0, count = primitives.size(); i < count; i++)
    {
      const Primitive& primitive = primitives[i];

      return_value.add_primitive(primitive.type, primitive.first, primitive.count, primitive.base_vertex);
    }

    for (uint32_t i = 0, count = mesh.primitives_count(); i < count; i++)
    {
      const Primitive& primitive = mesh.primitive(i);

      return_value.add_primitive(primitive.type, primitive.first + indices_count, primitive.count, primitive.base_vertex + vertices_count);
    }

    return return_value;
  }
};

/// Mesh

/// Constructor
Mesh::Mesh()
  : impl(new Impl)
  {}

/// Vertices data
uint32_t Mesh::vertices_count() const
{
  return static_cast<uint32_t>(impl->vertices_data.size());
}

void Mesh::vertices_resize(uint32_t vertices_count)
{
  impl->vertices_data.resize(vertices_count);
}

const Vertex* Mesh::vertices_data() const
{
  return impl->vertices_data.data();
}

Vertex* Mesh::vertices_data()
{
  return impl->vertices_data.data();
}

void Mesh::vertices_clear()
{
  impl->vertices_data.resize(0);
}

uint32_t Mesh::vertices_capacity() const
{
  return static_cast<uint32_t>(impl->vertices_data.capacity());
}

void Mesh::vertices_reserve(uint32_t vertices_count)
{
  impl->vertices_data.reserve(vertices_count);
}

/// Indices data
uint32_t Mesh::indices_count() const
{
  return static_cast<uint32_t>(impl->indices_data.size());
}

void Mesh::indices_resize(uint32_t indices_count)
{
  impl->indices_data.resize(indices_count);
}

const Mesh::IndexType* Mesh::indices_data() const
{
  return impl->indices_data.data();
}

Mesh::IndexType* Mesh::indices_data()
{
  return impl->indices_data.data();
}

void Mesh::indices_clear()
{
  impl->indices_data.resize(0);
}

uint32_t Mesh::indices_capacity() const
{
  return static_cast<uint32_t>(impl->indices_data.capacity());
}

void Mesh::indices_reserve(uint32_t indices_count)
{
  impl->indices_data.reserve(indices_count);
}

/// Primitives data
uint32_t Mesh::primitives_count() const
{
  return static_cast<uint32_t>(impl->primitives.size());
}

const Primitive& Mesh::primitive(uint32_t index) const
{
  return impl->primitives[index];
}

/// Add / remove primitives
uint32_t Mesh::add_primitive(PrimitiveType type, uint32_t first, uint32_t count, uint32_t base_vertex)
{
  return impl->add_primitive(type, first, count, base_vertex);
}

uint32_t Mesh::add_primitive(PrimitiveType type, const Vertex* vertices, IndexType vertices_count, const IndexType* indices, uint32_t indices_count)
{
  return impl->add_primitive(type, vertices, vertices_count, indices, indices_count);
}

void Mesh::remove_primitive(uint32_t primitive_index)
{
  if (primitive_index >= impl->primitives.size())
    return;

  impl->primitives.erase(impl->primitives.begin() + primitive_index);
}

void Mesh::remove_all_primitives()
{
  impl->primitives.clear();
}

/// Clear all data
Mesh Mesh::merge(const Mesh& mesh) const
{
  return impl->merge(mesh);
}

/// Clear all data
void Mesh::clear()
{
  remove_all_primitives();
  indices_clear();
  vertices_clear();
}
