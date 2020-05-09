#pragma once

#include <common/exception.h>

#include <math/vector.h>
#include <math/matrix.h>

#include <memory>
#include <string>

namespace engine {
namespace common {

/// Property type
enum PropertyType
{
  PropertyType_Int,
  PropertyType_Float,
  PropertyType_Vec4f,
  PropertyType_Mat4f,
};

/// Property base class
class Property
{
  public:
    /// Constructors
    template <class T> Property(const char* name, const T& value);

    /// Type of property
    PropertyType type() const;

    /// Get value
    template <class T> T& get();
    template <class T> const T& get() const;

    /// Set value
    template <class T> void set(const T& data) const;

  private:
    struct Value;
    template <class T> struct ValueImpl;
    std::shared_ptr<Value> value;
};

/// Property map
class PropertyMap
{
  public:
    /// Constructor
    PropertyMap();

    /// Number of properties
    size_t count() const;

    /// Property list
    const Property* items() const;
    Property* items();

    /// Find property by name
    const Property* find(const char* name) const;
    Property* find(const char* name);

    /// Get property by name or throw an exception
    const Property& get(const char* name) const;
    Property& get(const char* name);

    const Property& operator[](const char* name) const;
    Property& operator[](const char* name);

    /// Add property
    size_t insert(const char* name, const Property& property);

    /// Set property
    template <class T> Property& set(const char* name, const T& value);

  private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};

#include <common/detail/property_map.inl>

}}
