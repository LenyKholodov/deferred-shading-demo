#pragma once

#include <media/geometry.h>

#include <math/vector.h>

#include <memory>
#include <cstdint>

namespace engine {

namespace application {

//forward declarations
class Window;

}

namespace render {

using application::Window;
using media::geometry::Vertex;

/// Implementation forwards
class DeviceContextImpl;
struct BufferImpl;
struct ShaderImpl;

typedef std::shared_ptr<DeviceContextImpl> DeviceContextPtr;

/// Clear flags
enum ClearFlag
{
  ClearFlag_Color   = 1, //clear color buffer
  ClearFlag_Depth   = 2, //clear depth buffer
  ClearFlag_Stencil = 4, //clear stencil buffer

  ClearFlag_DepthStencil = ClearFlag_Depth | ClearFlag_Stencil,
  ClearFlag_All          = ClearFlag_DepthStencil | ClearFlag_Color,
};

/// Shader type
enum ShaderType
{
  ShaderType_Vertex, //vertex shader
  ShaderType_Pixel, //pixel shader
};

/// Viewport
struct Viewport
{
  int x, y, width, height;

  Viewport(int x=0, int y=0, int width=0, int height=0)
    : x(x), y(y), width(width), height(height) {}
};

/// Framebuffer
class FrameBuffer
{
  public:
    /// Empty buffer creation
    FrameBuffer(const DeviceContextPtr& context);

    /// Constructor
    FrameBuffer(const DeviceContextPtr& context, const Window& window);

    /// Number of rendering targets
    size_t render_targets_count() const;

    /// Attach render window
    void add_render_target(const Window& window);

    /// Set viewport
    void set_viewport(const Viewport& viewport);

    /// Set viewport based of underlying render buffer / texture size
    void reset_viewport();

    /// Get viewport
    const Viewport& viewport() const;

    /// Set clear color
    void set_clear_color(const math::vec4f& color);

    /// Get clear color
    const math::vec4f& clear_color() const;

    /// Clear buffers
    void clear(unsigned int flags = ClearFlag_All);

    /// Bind framebuffer for rendering to a context
    void bind();

    //TODO: attach textures as render targets
    //TODO: attach render buffer as depth-buffer

  private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};

/// Vertex buffer
/// (simplification: no streams and layouts)
class VertexBuffer
{
  public:
    /// Constructor
    VertexBuffer(const DeviceContextPtr& context, size_t vertices_count);

    /// Vertices count
    size_t vertices_count() const;

    /// Load data
    void set_data(size_t offset, size_t count, const Vertex* vertices);

    /// Bind buffer
    void bind();

  private:
    std::shared_ptr<BufferImpl> impl;
};

/// Index buffer
class IndexBuffer
{
  public:
    typedef uint16_t value_type;

    /// Constructor
    IndexBuffer(const DeviceContextPtr& context, size_t indices_count);

    /// Indices count
    size_t indices_count() const;

    /// Load data
    void set_data(size_t offset, size_t count, const value_type* indices);

    /// Bind buffer
    void bind();

  private:
    std::shared_ptr<BufferImpl> impl;
};

/// Shader
class Shader
{
  public:
    /// Constructor
    Shader(const DeviceContextPtr& context, ShaderType type, const char* name, const char* source_code);

    /// Name
    const char* name() const;

    /// Shader type
    ShaderType type() const;

    /// Implementation details
    ShaderImpl& get_impl() const;

  private:
    std::shared_ptr<ShaderImpl> impl;
};

/// Program
class Program
{
  public:
    /// Constructor
    Program(const DeviceContextPtr& context, const char* name, const Shader& vertex_shader, const Shader& pixel_shader);

    /// Uniform location
    int find_uniform_location(const char* name) const;

    /// Attribute location
    int find_attribute_location(const char* name) const;

    /// Uniform location (exception if not found)
    int get_uniform_location(const char* name) const;

    /// Attribute location (exception if not found)
    int get_attribute_location(const char* name) const;

    /// Bind
    void bind();

  private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};

/// Device options
struct DeviceOptions
{
  bool vsync; //vertical synchronization enabled
  bool debug; //should we check OpenGL errors and output debug messages

  DeviceOptions()
    : vsync(true)
    , debug(true)
  {
  }
};

/// Rendering device
class Device
{
  public:
    /// Constructor
    Device(const Window& window, const DeviceOptions& options);

    /// Window frame buffer
    const FrameBuffer& window_frame_buffer() const;

    /// Create vertex buffer
    VertexBuffer create_vertex_buffer(size_t count);

    /// Create index buffer
    IndexBuffer create_index_buffer(size_t count);

    /// Create vertex shader
    Shader create_vertex_shader(const char* name, const char* source_code);

    /// Create pixel shader
    Shader create_pixel_shader(const char* name, const char* source_code);

    /// Create program
    Program create_program(const char* name, const Shader& vertex_shader, const Shader& pixel_shader);

  private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};

}}
