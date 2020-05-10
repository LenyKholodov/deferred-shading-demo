#include "shared.h"

using namespace engine::scene;
using namespace engine::render::scene;
using namespace engine::render::low_level;
using namespace engine::common;

///
/// Constants
///

const size_t RESERVED_PASSES_COUNT = 16; //number of reserved passes per scene renderer

///
/// SceneViewport
///

/// Implementation details of scene viewport
struct SceneViewport::Impl
{
  Camera::Pointer camera; //camera of the scene viewport
  Viewport viewport; //viewport of the scene viewport
  PropertyMap properties; //viewport properties;
  TextureList textures; //viewport textures;
};

SceneViewport::SceneViewport()
  : impl(std::make_shared<Impl>())
{
}

const Viewport& SceneViewport::viewport() const
{
  return impl->viewport;
}

Viewport& SceneViewport::viewport()
{
  return impl->viewport;
}

void SceneViewport::set_viewport(const low_level::Viewport& viewport)
{
  impl->viewport = viewport;
}

Camera::Pointer& SceneViewport::camera() const
{
  return impl->camera;
}

void SceneViewport::set_camera(const scene::Camera::Pointer& camera)
{
  impl->camera = camera;
}

PropertyMap& SceneViewport::properties() const
{
  return impl->properties;
}

void SceneViewport::set_properties(const common::PropertyMap& properties)
{
  impl->properties = properties;
}

TextureList& SceneViewport::textures() const
{
  return impl->textures;
}

void SceneViewport::set_textures(const low_level::TextureList& textures)
{
  impl->textures = textures;
}

///
/// SceneRenderer
///

namespace
{

struct PassEntry
{
  ScenePassPtr pass; //scene pass
  std::string root_name; //pass name
  int priority; //priority of pass rendering

  PassEntry(const char* name, const ScenePassPtr& pass, int priority)
    : pass(pass)
    , root_name(name)
    , priority(priority)
  {
  }

  bool operator < (const PassEntry& other) const { return priority < other.priority; }
};

//TODO: use wrapper to avoid copying of strings
typedef std::vector<PassEntry> PassArray;

struct ScenePassContextImpl: ScenePassContext
{
  ScenePassContextImpl(ISceneRenderer& owner)
    : ScenePassContext(owner)
  {
  }

  using ScenePassContext::bind;
  using ScenePassContext::unbind;
};

}

/// Implementation details of scene renderer
struct SceneRenderer::Impl : ISceneRenderer
{
  Device render_device; //rendering device
  TextureList shared_textures; //shared textures
  MaterialList shared_materials; //shared materials
  FrameNodeList shared_frame_nodes; //shared frame_nodes
  common::PropertyMap shared_properties; //shared propertiess
  ScenePassContextImpl passes_context; //scene rendering context
  PassArray passes; //scene rendering passes
  bool need_sort_passes; //passes should be sorted on next rendering iteration

  Impl(const Device& device)
    : render_device(device)
    , passes_context(*this)
    , need_sort_passes()
  {
    passes.reserve(RESERVED_PASSES_COUNT);
  }

  PropertyMap& properties() override { return shared_properties; }
  TextureList& textures() override { return shared_textures; }
  MaterialList& materials() override { return shared_materials; }
  FrameNodeList& frame_nodes() override { return shared_frame_nodes; } 
  Device& device() override { return render_device; }
};

SceneRenderer::SceneRenderer(const Window& window, const DeviceOptions& options)
{
  Device device(window, options);

  impl = std::make_shared<Impl>(device);
}

Device& SceneRenderer::device() const
{
  return impl->render_device;
}

size_t SceneRenderer::passes_count() const
{
  return impl->passes.size();
}

namespace
{

struct PassResolver
{
  SceneRenderer& renderer;
  Device& device;
  int priority;
  std::string root_pass;
  PassArray passes;
  std::unordered_set<std::string> registered_passes;

  PassResolver(SceneRenderer& renderer, Device& device, const char* root_pass, int priority)
    : renderer(renderer)
    , device(device)
    , priority(priority)
    , root_pass(root_pass)
  {
    passes.reserve(RESERVED_PASSES_COUNT);
  }

  struct StackFrame
  {
    const char* name;    
    StackFrame* prev;

    StackFrame(const char* name, StackFrame* prev)
      : name(name)
      , prev(prev)
    {
    }
  };

  void add_pass(const char* name, StackFrame* parent)
  {
    StackFrame frame(name, parent);

      //check loops

    if (registered_passes.count(name))
    {
      std::string stack;

      for (StackFrame* it=&frame; it; it=it->prev)
      {
        if (stack.empty())
        {
          stack = it->name; 
        }
        else
        {
          stack = engine::common::format("%s -> %s", it->name, stack.c_str());
        }
      }

      throw Exception::format("Cant' create pass '%s' due to pass depenency loop: %s", root_pass.c_str(), stack.c_str());
    }

    registered_passes.insert(name);    

      //create pass

    ScenePassPtr pass = ScenePassFactory::create_pass(name, renderer, device);

      //create dependencies

    std::vector<std::string> deps;

    pass->get_dependencies(deps);

    for (auto& dep : deps)
    {
      add_pass(dep.c_str(), &frame);
    }

      //add pass after dependencies

    passes.push_back(PassEntry(root_pass.c_str(), pass, priority));
  }
};

}

void SceneRenderer::add_pass(const char* name, int priority)
{
  engine_check_null(name);

    //create pass

  PassResolver resolver(*this, impl->render_device, name, priority);

  resolver.add_pass(name, nullptr);

    //register pass

  impl->need_sort_passes = true;

  impl->passes.insert(impl->passes.end(), resolver.passes.begin(), resolver.passes.end());
}

void SceneRenderer::remove_pass(const char* name)
{
  if (!name)
    return;

  impl->passes.erase(std::remove_if(impl->passes.begin(), impl->passes.end(), [&](auto& pass_entry) {
    return pass_entry.root_name == name;
  }), impl->passes.end());
}

void SceneRenderer::render(const SceneViewport& viewport)
{
  render(1, &viewport);
}

void SceneRenderer::render(size_t viewports_count, const SceneViewport* viewports)
{
  if (viewports_count)
    engine_check_null(viewports);

    //update frame info

  impl->passes_context.set_current_frame_id(impl->passes_context.current_frame_id() + 1);

    //sort passes

  if (impl->need_sort_passes)
  {
    std::sort(impl->passes.begin(), impl->passes.end());

    impl->need_sort_passes = false;
  }

    //render passes

  struct ViewportContextBindings
  {
    BindingContext bindings;
    ScenePassContextImpl& context;

    ViewportContextBindings(ScenePassContextImpl& context)
      : context(context)
    {
      context.bind(&bindings);
    }

    ~ViewportContextBindings()
    {
      context.unbind(&bindings);
    }
  };

  BindingContext renderer_bindings(impl->shared_textures, impl->shared_properties);
  ScenePassContextImpl& context = impl->passes_context;
  Device& device = impl->render_device;
  FrameBuffer& window_frame_buffer = device.window_frame_buffer();

  for (size_t i=0; i<viewports_count; i++)
  {
      //setup viewport context

    const SceneViewport& scene_viewport = viewports[i];

    ViewportContextBindings viewport_bindings(context);

    viewport_bindings.bindings.bind(&renderer_bindings);
    viewport_bindings.bindings.bind(scene_viewport.properties());
    viewport_bindings.bindings.bind(scene_viewport.textures());

      //set camera

    context.set_view_node(scene_viewport.camera());

      //set framebuffer

    const Viewport& viewport = scene_viewport.viewport();

    if (!viewport.width && !viewport.height)
    {
      window_frame_buffer.reset_viewport();
    }
    else
    {
      window_frame_buffer.set_viewport(viewport);
    }

      //render passes

    for (auto& pass_entry : impl->passes)
    {
      pass_entry.pass->render(impl->passes_context);
    }

      //render frame nodes

    context.root_frame_node().render(context);
  }
}

PropertyMap& SceneRenderer::properties() const
{
  return impl->shared_properties;
}

TextureList& SceneRenderer::textures() const
{
  return impl->shared_textures;
}

MaterialList& SceneRenderer::materials() const
{
  return impl->shared_materials;
}

FrameNodeList& SceneRenderer::frame_nodes() const
{
  return impl->shared_frame_nodes;
}
