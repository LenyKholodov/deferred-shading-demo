#include <common/property_map.h>

#include <unordered_map>
#include <vector>

using namespace engine::common;

typedef std::vector<Property> PropertyArray;
typedef std::unordered_map<std::string, size_t> PropertyDict;

/// Implementation details of property map
struct PropertyMap::Impl
{
  PropertyArray properties;
  PropertyDict name_dict;
};

PropertyMap::PropertyMap()
  : impl(std::make_shared<Impl>())
{
}

size_t PropertyMap::count() const
{
  return impl->properties.size();
}

const Property* PropertyMap::items() const
{
  return const_cast<PropertyMap&>(*this).items();
}

Property* PropertyMap::items()
{
  if (!impl->properties.size())
    return nullptr;

  return &impl->properties[0];
}

const Property* PropertyMap::find(const char* name) const
{
  return const_cast<PropertyMap&>(*this).find(name);
}

Property* PropertyMap::find(const char* name)
{
  if (!name)
    return nullptr;
  
  PropertyDict::const_iterator iter = impl->name_dict.find(name);

  if (iter == impl->name_dict.end())
    return nullptr;

  return &impl->properties[iter->second];
}

const Property& PropertyMap::get(const char* name) const
{
  return const_cast<PropertyMap&>(*this).get(name);   
}

Property& PropertyMap::get(const char* name)
{
  engine_check_null(name);

  if (Property* property = find(name))
    return *property;

  throw Exception::format("Property '%s' has not been found", name);
}

size_t PropertyMap::insert(const char* name, const Property& property)
{
  engine_check_null(name);

  if (Property* property = find(name))
    throw Exception::format("Property '%s' has been already inserted", name);

  impl->properties.push_back(property);

  size_t index = impl->properties.size() - 1;

  try
  {
    impl->name_dict[name] = index;

    return index;
  }
  catch (...)
  {
    impl->properties.pop_back();
    throw;
  }
}
