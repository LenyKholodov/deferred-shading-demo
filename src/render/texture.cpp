#include "shared.h"

using namespace engine::render;
using namespace engine::common;

namespace
{

size_t get_max_size(size_t a, size_t b)
{
  return a > b ? a : b;
}

size_t get_mips_count(size_t size)
{
  return (size_t)(::log((double)size) / ::log (2.0)) + 1;
}

size_t get_mips_count(size_t width, size_t height)
{
  return get_mips_count(get_max_size(width, height));
}

}

/// Implementation details of texture
struct Texture::Impl
{
  DeviceContextPtr context; //device context
  size_t width; //texture width
  size_t height; //texture height
  size_t layers; //number of layers
  size_t mips_count; //number of mipmaps
  PixelFormat format; //pixel format
  TextureFilter min_filter; //minimal filter
  TextureFilter mag_filter; //magnifying filter
  bool need_reapply_sampler; //sample applying
  GLenum gl_internal_format; //GL pixel format
  GLuint texture_id; //GL texture
  GLenum target; //GL target for this texture

  Impl(const DeviceContextPtr& context,
       size_t width,
       size_t height,
       size_t layers,
       PixelFormat format,
       size_t mips_count)
    : context(context)
    , width(width)
    , height(height)
    , layers(layers)
    , mips_count(mips_count)
    , format(format)
    , min_filter(TextureFilter_Linear)
    , mag_filter(TextureFilter_Linear)
    , need_reapply_sampler(true)
    , texture_id()
    , target()
  {
    context->make_current();

    glGenTextures(1, &texture_id);

    if (!texture_id)
      throw Exception::format("Can't create GL texture");

    GLenum gl_uncompressed_format = GL_NONE;
    GLenum gl_uncompressed_type = GL_NONE;

    switch (format)
    {
      case PixelFormat_RGBA8:
        gl_internal_format = GL_RGBA8;
        gl_uncompressed_format = GL_RGBA;
        gl_uncompressed_type = GL_UNSIGNED_BYTE;
        break;
      case PixelFormat_RGB16F:
        gl_internal_format = GL_RGB16F;
        gl_uncompressed_format = GL_RGB;
        gl_uncompressed_type = GL_FLOAT;
        break;
      case PixelFormat_D24:
        gl_internal_format = GL_DEPTH_COMPONENT;
        gl_uncompressed_format = GL_DEPTH_COMPONENT;
        gl_uncompressed_type = GL_UNSIGNED_INT;      
        break;
      default:
        throw Exception::format("Invalid texture pixel format %d", format);
    }

    switch (layers)
    {
      case 1:
      {
        target = GL_TEXTURE_2D;

        bind();

        mips_count = std::min(mips_count, get_mips_count(width, height));

        GLint level_width = static_cast<GLint>(width);
        GLint level_height = static_cast<GLint>(height);

        for (GLint level=0; level<mips_count; level++)
        {
          glTexImage2D(target, level, gl_internal_format, level_width, level_height, 0,
            gl_uncompressed_format, gl_uncompressed_type, nullptr);

          level_width = level_width > 1 ? level_width / 2 : 1;
          level_height = level_height > 1 ? level_height / 2 : 1;
        }

        break;
      }
      default:
        engine_check(layers == 1); //no support of other textures for now
        break;
    }

    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mips_count - 1));

    context->check_errors();
  }

  ~Impl()
  {
    try
    {
      glDeleteTextures(1, &texture_id);
    }
    catch (...)
    {
      //ignore exceptions in destructors
    }
  }

  void bind()
  {
    context->make_current();

    glBindTexture(target, texture_id);

    if (need_reapply_sampler)
    {
      apply_sampler();
    }

    context->check_errors();
  }

  void apply_sampler()
  {
    auto convert_filter = [](TextureFilter filter) {
      switch (filter)
      {
        case TextureFilter_Point: return GL_NEAREST;
        case TextureFilter_Linear: return GL_LINEAR;
        case TextureFilter_LinearMipLinear: return GL_LINEAR_MIPMAP_LINEAR;
        default: throw Exception::format("Invalid texture filter %d", filter);
      }
    };

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, convert_filter(min_filter));
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, convert_filter(mag_filter));
    
    need_reapply_sampler = false;
  }
};

Texture::Texture(const DeviceContextPtr& context, size_t width, size_t height, size_t layers, PixelFormat format, size_t mips_count)
  : impl(std::make_shared<Impl>(context, width, height, layers, format, mips_count))
{
}

size_t Texture::width() const
{
  return impl->width;
}

size_t Texture::height() const
{
  return impl->height;
}

size_t Texture::layers() const
{
  return impl->layers;
}

size_t Texture::mips_count() const
{
  return impl->mips_count;
}

PixelFormat Texture::format() const
{
  return impl->format;
}

TextureFilter Texture::min_filter() const
{
  return impl->min_filter;
}

void Texture::set_min_filter(TextureFilter filter)
{
  impl->min_filter = filter;
}

TextureFilter Texture::mag_filter() const
{
  return impl->mag_filter;
}

void Texture::set_mag_filter(TextureFilter filter)
{
  impl->mag_filter = filter;
}

void Texture::set_data(size_t layer, size_t x, size_t y, size_t width, size_t height, const void* data)
{
  unimplemented();
}

void Texture::get_data(size_t layer, size_t x, size_t y, size_t width, size_t height, void* data)
{
  unimplemented();
}

void Texture::bind()
{
  impl->bind();
}

void Texture::generate_mips()
{
  impl->bind();

  glGenerateMipmap(impl->target);
}

void Texture::get_level_info(size_t layer, size_t level, TextureLevelInfo& out_info) const
{
  engine_check_range(layer, impl->layers);
  engine_check_range(level, impl->mips_count);

  GLint level_width = static_cast<GLint>(impl->width);
  GLint level_height = static_cast<GLint>(impl->height);

  for (GLint level=0; level<impl->mips_count; level++)
  {
    level_width = level_width > 1 ? level_width / 2 : 1;
    level_height = level_height > 1 ? level_height / 2 : 1;
  }

  switch (impl->layers)
  {
    case 1:
    {
      out_info.target = impl->target;
      out_info.texture_id = impl->texture_id;
      out_info.width = level_width;
      out_info.height = level_height;
      break;
    }
    default:
      unimplemented(); //cubemaps are not supported for now
  }
}
