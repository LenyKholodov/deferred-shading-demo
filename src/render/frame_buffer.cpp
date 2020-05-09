#include "shared.h"

using namespace engine::render;
using namespace engine::common;

namespace
{

/// Render target type
enum RenderTargetType
{
  RenderTargetType_Window,
  RenderTargetType_Texture2D,
  RenderTargetType_RenderBuffer,  
};

/// Render target of frame buffer
struct RenderTarget
{
  RenderTargetType type; //type of the target

  RenderTarget(RenderTargetType type, const Viewport& viewport)
    : type(type)
  {
  }
};

typedef std::vector<RenderTarget> RenderTargetArray;

}

/// Implementation details of device
struct FrameBuffer::Impl
{
  DeviceContextPtr context; //device context
  GLuint frame_buffer_id; //identifier of frame buffer
  math::vec4f clear_color; //clear color
  RenderTargetArray render_targets; //rendering targets list
  Viewport viewport; //viewport for the buffer (TODO: multi-target viewport support)  

  Impl(const DeviceContextPtr& context, bool is_default)
    : context(context)
    , frame_buffer_id()
  {
    engine_check(context);

      //bind the context

    context->make_current();

      //generate new buffer ID

    if (is_default)
    {
      render_targets.emplace_back(RenderTarget(RenderTargetType_Window, get_default_viewport()));
    }
    else
    {
      glGenFramebuffers(1, &frame_buffer_id);
    
      if (!frame_buffer_id)
        context->check_errors();
    }
  }

  ~Impl()
  {
    if (frame_buffer_id)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDeleteFramebuffers(1, &frame_buffer_id);
    }
  }

  Viewport get_default_viewport() const
  {
    const Window& window = context->window();
    int width = window.frame_buffer_width();
    int height = window.frame_buffer_height();

    return Viewport(0, 0, width, height);
  }

  void bind()
  {
    engine_check(context);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id);

    context->check_errors();
  }

  void reconfigure()
  {
    unimplemented();
  }
};

FrameBuffer::FrameBuffer(const DeviceContextPtr& context)
  : impl(new Impl(context, false))
{
}

FrameBuffer::FrameBuffer(const DeviceContextPtr& context, const Window& window)
{
  engine_check(context);
  engine_check(context->handle() == window.handle());

  impl.reset(new Impl(context, true));
}

size_t FrameBuffer::render_targets_count() const
{
  return impl->render_targets.size();
}

void FrameBuffer::set_viewport(const Viewport& viewport)
{
  impl->viewport = viewport;
}

void FrameBuffer::reset_viewport()
{
  if (!impl->frame_buffer_id)
  {
      //for window based render targets just use window bounds

    impl->viewport = impl->get_default_viewport();
  }
  else
  {
    unimplemented();
  }
}

const Viewport& FrameBuffer::viewport() const
{
  return impl->viewport;
}

void FrameBuffer::set_clear_color(const math::vec4f& color)
{
  impl->clear_color = color;
}

const math::vec4f& FrameBuffer::clear_color() const
{
  return impl->clear_color;
}

void FrameBuffer::clear(unsigned int flags)
{
  impl->bind();

  GLuint gl_flags = 0;

  if (flags & ClearFlag_Color)   gl_flags |= GL_COLOR_BUFFER_BIT;
  if (flags & ClearFlag_Depth)   gl_flags |= GL_DEPTH_BUFFER_BIT;
  if (flags & ClearFlag_Stencil) gl_flags |= GL_STENCIL_BUFFER_BIT;

  const math::vec4f& clear_color = impl->clear_color;

  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(gl_flags);

  impl->context->check_errors();
}

void FrameBuffer::bind()
{
  impl->bind();

    //configure viewport

  if (!impl->frame_buffer_id)
  {
      //reset viewport each time for window based render target, because window size can change
      //TODO: cacheing

    reset_viewport();
  }

  const Viewport& v = impl->viewport;

  glViewport(v.x, v.y, v.width, v.height);

  impl->context->check_errors();
}
