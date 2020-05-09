#include "shared.h"

using namespace engine::render;

/// Implementation details of device
struct Device::Impl
{
  DeviceContextPtr context; //rendering context
  FrameBuffer window_frame_buffer; //window frame buffer

  Impl(const Window& window, const DeviceOptions& options)
    : context(std::make_shared<DeviceContextImpl>(window, options))
    , window_frame_buffer(context, window)
  {
    
  }
};

Device::Device(const Window& window, const DeviceOptions& options)
  : impl(new Impl(window, options))
{
}

const FrameBuffer& Device::window_frame_buffer() const
{
  return impl->window_frame_buffer;
}

VertexBuffer Device::create_vertex_buffer(size_t count)
{
  return VertexBuffer(impl->context, count);
}

IndexBuffer Device::create_index_buffer(size_t count)
{
  return IndexBuffer(impl->context, count);
}

Shader Device::create_vertex_shader(const char* name, const char* source_code)
{
  return Shader(impl->context, ShaderType_Vertex, name, source_code);
}

Shader Device::create_pixel_shader(const char* name, const char* source_code)
{
  return Shader(impl->context, ShaderType_Pixel, name, source_code);
}

Program Device::create_program(const char* name, const Shader& vertex_shader, const Shader& pixel_shader)
{
  return Program(impl->context, name, vertex_shader, pixel_shader);
}
