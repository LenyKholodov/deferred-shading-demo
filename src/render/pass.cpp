#include "shared.h"

#include <vector>

using namespace engine::render;
using namespace engine::common;

#if defined (_MSC_VER) || defined (__APPLE_CC__)
  #define engine_offsetof(X,Y) offsetof(X,Y)
#else
  #define engine_offsetof(X,Y) (reinterpret_cast<size_t> (&(static_cast<X*> (0)->*(&X::Y))))
#endif

/// Constants
static constexpr size_t PRIMITIVES_RESERVE_SIZE = 128; //number of reserved primitives per frame

///
/// Internal structures
///

namespace
{

/// Arrays layout
struct InputLayout
{
  GLint position_attribute_location; //location of vertex position
  GLint normal_attribute_location; //location of vertex normal
  GLint color_attribute_location; //location of vertex color
  GLint texcoord_attribute_location; //location of vertex texcoord

  InputLayout(Program& program)
  {
      //big simplification here due to a fixed vertex layout

    static const char* POSITION_ATTRIBUTE_NAME = "vPosition";
    static const char* NORMAL_ATTRIBUTE_NAME = "vNormal";
    static const char* COLOR_ATTRIBUTE_NAME = "vColor";
    static const char* TEXCOORD_ATTRIBUTE_NAME = "vTexCoord";

      //search attributes in a program

    position_attribute_location = program.find_attribute_location(POSITION_ATTRIBUTE_NAME);
    normal_attribute_location = program.find_attribute_location(NORMAL_ATTRIBUTE_NAME);
    color_attribute_location = program.find_attribute_location(COLOR_ATTRIBUTE_NAME);
    texcoord_attribute_location = program.find_attribute_location(TEXCOORD_ATTRIBUTE_NAME);

      //bind

    enable_attribute(position_attribute_location);
    enable_attribute(normal_attribute_location);
    enable_attribute(color_attribute_location);
    enable_attribute(texcoord_attribute_location);
  }

  ~InputLayout()
  {
      //unbind

    disable_attribute(position_attribute_location);
    disable_attribute(normal_attribute_location);
    disable_attribute(color_attribute_location);
    disable_attribute(texcoord_attribute_location);
  }

  static void enable_attribute(GLint attribute)
  {
    if (attribute < 0)
      return;

    glEnableVertexAttribArray(attribute);
  }

  static void disable_attribute(GLint attribute)
  {
    if (attribute < 0)
      return;

    glDisableVertexAttribArray(attribute); 
  }

  static void bind_vertex_float_attrib(GLint attribute, size_t offset, size_t size)
  {
    if (attribute < 0)
      return;

    glVertexAttribPointer(attribute, static_cast<GLint>(size / sizeof(float)), GL_FLOAT, GL_FALSE, sizeof(Vertex),
      reinterpret_cast<void*>(offset));
  }
};

typedef std::vector<Primitive> PrimitiveArray;

}

/// Implementation details of pass
struct Pass::Impl
{
  DeviceContextPtr context; //device context
  PrimitiveArray primitives; //primitives
  Program program; //program for this pass
  FrameBuffer frame_buffer; //frame buffer for this pass
  math::vec4f clear_color; //clear color  
  ClearFlags clear_flags; //clear flags
  DepthStencilState depth_stencil_state; //depth stencil state
  BlendState blend_state; //blend state

  Impl(const DeviceContextPtr& context, const FrameBuffer& frame_buffer, const Program& program)
    : context(context)
    , program(program)
    , frame_buffer(frame_buffer)
    , clear_flags(Clear_All)
    , depth_stencil_state(false, false, CompareMode_AlwaysPass)
    , blend_state(false, BlendArgument_One, BlendArgument_Zero)
  {
    engine_check_null(context);

    primitives.reserve(PRIMITIVES_RESERVE_SIZE);
  }

  void render()
  {
    engine_check(sizeof(IndexBuffer::index_type) == 2); //change glDrawElements call for different sizes

      //setup frame buffer

    frame_buffer.bind();

    clear();

    bind_depth_stencil_state();

    bind_blend_state();

      //bind program

    program.bind();

      //prepare input layout

    InputLayout input_layout(program);

      //draw primitives

    context->check_errors();    

    for (auto& primitive : primitives)
    {
      render_primitive(primitive, input_layout);
    }

      //clear pass

    primitives.clear();
  }

  void render_primitive(Primitive& primitive, InputLayout& input_layout)
  {
      //setup buffers

    primitive.vertex_buffer.bind();
    primitive.index_buffer.bind();

      //setup input layout
      //TODO: VAO

    size_t vb_offset = primitive.base_vertex * sizeof(Vertex);

    InputLayout::bind_vertex_float_attrib(
      input_layout.position_attribute_location,
      vb_offset + engine_offsetof(Vertex, position),
      sizeof(Vertex::position));
    InputLayout::bind_vertex_float_attrib(
      input_layout.normal_attribute_location,
      vb_offset + engine_offsetof(Vertex, normal),
      sizeof(Vertex::normal));
    InputLayout::bind_vertex_float_attrib(
      input_layout.color_attribute_location,
      vb_offset + engine_offsetof(Vertex, color),
      sizeof(Vertex::color));
    InputLayout::bind_vertex_float_attrib(
      input_layout.texcoord_attribute_location,
      vb_offset + engine_offsetof(Vertex, tex_coord),
      sizeof(Vertex::tex_coord));    

      //convert to GL primitive type and offsets

    GLenum gl_primitive_type = GL_NONE;
    GLsizei gl_first = 0;
    GLsizei gl_count = 0;

    switch (primitive.type)
    {
      case media::geometry::PrimitiveType_TriangleList:
        gl_primitive_type = GL_TRIANGLES;
        gl_first = static_cast<GLsizei>(primitive.first * 3);
        gl_count = static_cast<GLsizei>(primitive.count * 3);
        break;
      default:
        throw Exception::format("Unexpected primitive type %d", primitive.type);
    }

      //draw primitive

    size_t offset = gl_first * sizeof(IndexBuffer::index_type);

    glDrawElements(gl_primitive_type, gl_count, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(offset));

    context->check_errors();
  }

  void clear()
  {
    GLuint gl_flags = 0;

    if (clear_flags & Clear_Color)   gl_flags |= GL_COLOR_BUFFER_BIT;
    if (clear_flags & Clear_Depth)   gl_flags |= GL_DEPTH_BUFFER_BIT;
    if (clear_flags & Clear_Stencil) gl_flags |= GL_STENCIL_BUFFER_BIT;

    if (clear_flags)
    {
      glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    }

    if (gl_flags)
      glClear(gl_flags);

    context->check_errors();
  }

  void bind_depth_stencil_state()
  {
    if (depth_stencil_state.depth_test_enable)
    {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(get_gl_compare_mode(depth_stencil_state.depth_compare_mode));
    }
    else
      glDisable(GL_DEPTH_TEST);

    glDepthMask(depth_stencil_state.depth_write_enable);

    context->check_errors();
  }

  GLenum get_gl_compare_mode(CompareMode mode)
  {
    switch (mode)
    {
      case CompareMode_AlwaysFail: return GL_NEVER;
      case CompareMode_AlwaysPass: return GL_ALWAYS;
      case CompareMode_Equal: return GL_EQUAL;
      case CompareMode_NotEqual: return GL_NOTEQUAL;
      case CompareMode_Less: return GL_LESS;
      case CompareMode_LessEqual: return GL_LEQUAL;
      case CompareMode_Greater: return GL_GREATER;
      case CompareMode_GreaterEqual: return GL_GEQUAL;
      default:
        throw Exception::format("Unsupported CompareMode %d", mode);
    }
  }

  GLenum get_gl_blend_argument(BlendArgument arg)
  {
    switch (arg)
    {
      case BlendArgument_Zero: return GL_ZERO;
      case BlendArgument_One: return GL_ONE;
      case BlendArgument_SourceColor: return GL_SRC_COLOR;
      case BlendArgument_SourceAlpha: return GL_SRC_ALPHA;
      case BlendArgument_InverseSourceColor: return GL_ONE_MINUS_SRC_COLOR;
      case BlendArgument_InverseSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
      case BlendArgument_DestinationColor: return GL_DST_COLOR;
      case BlendArgument_DestinationAlpha: return GL_DST_ALPHA;
      case BlendArgument_InverseDestinationColor: return GL_ONE_MINUS_DST_COLOR;
      case BlendArgument_InverseDestinationAlpha: return GL_ONE_MINUS_DST_ALPHA;
      default:
        throw Exception::format("Unsupported BlendArgument %d", arg);
    }
  }

  void bind_blend_state()
  {
    if (blend_state.blend_enable)
    {
      GLenum src_arg = get_gl_blend_argument(blend_state.blend_source_argument),
             dst_arg = get_gl_blend_argument(blend_state.blend_destination_argument);

      glEnable(GL_BLEND);
      glBlendFunc(src_arg, dst_arg);
    }
    else
      glDisable(GL_BLEND);

    context->check_errors();
  }
};

Pass::Pass(const DeviceContextPtr& context, const FrameBuffer& frame_buffer, const Program& program)
  : impl(std::make_shared<Impl>(context, frame_buffer, program))
{
}

void Pass::set_frame_buffer(const FrameBuffer& frame_buffer)
{
  impl->frame_buffer = frame_buffer;
}

FrameBuffer& Pass::frame_buffer() const
{
  return impl->frame_buffer;
}

void Pass::set_program(const Program& program)
{
  impl->program = program;
}

Program& Pass::program() const
{
  return impl->program;
}

void Pass::set_clear_color(const math::vec4f& color)
{
  impl->clear_color = color;
}

const math::vec4f& Pass::clear_color() const
{
  return impl->clear_color;
}

void Pass::set_clear_flags(ClearFlags clear_flags)
{
  impl->clear_flags = clear_flags;
}

ClearFlags Pass::clear_flags() const
{
  return impl->clear_flags;
}

void Pass::set_depth_stencil_state(const DepthStencilState& state)
{
  impl->depth_stencil_state = state;
}

const DepthStencilState& Pass::depth_stencil_state() const
{
  return impl->depth_stencil_state;
}

void Pass::set_blend_state(const BlendState& state)
{
  impl->blend_state = state;
}

const BlendState& Pass::blend_state() const
{
  return impl->blend_state;
}

size_t Pass::primitives_count() const
{
  return impl->primitives.size();
}

void Pass::add_primitive(const Primitive& primitive)
{
  impl->primitives.push_back(primitive);
}

void Pass::add_primitive(Primitive&& primitive)
{
  impl->primitives.emplace_back(primitive);
}

/// Add mesh to a pass
void Pass::add_mesh(const Mesh& mesh)
{
  for (size_t i = 0, count = mesh.primitives_count(); i < count; i++)
    add_primitive(mesh.primitive(i));
}

void Pass::remove_all_primitives()
{
  impl->primitives.clear();
}

void Pass::reserve_primitives(size_t count)
{
  impl->primitives.reserve(count);
}

size_t Pass::primitives_capacity() const
{
  return impl->primitives.capacity();
}

void Pass::render()
{
  impl->render();
}
