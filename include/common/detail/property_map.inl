/// Property type mapping
template <class T> struct PropertyTypeMap;

template <> struct PropertyTypeMap<int>         { static constexpr PropertyType type = PropertyType_Int; };
template <> struct PropertyTypeMap<float>       { static constexpr PropertyType type = PropertyType_Float; };
template <> struct PropertyTypeMap<math::vec4f> { static constexpr PropertyType type = PropertyType_Vec4f; };
template <> struct PropertyTypeMap<math::mat4f> { static constexpr PropertyType type = PropertyType_Mat4f; };

/// Property value
struct Property::Value
{
  const PropertyType type;
  std::string name;

  Value(std::string&& name, PropertyType type) : type(type), name(name) {}
};

/// Property value implementation
template <class T> class Property::ValueImpl: Value
{
  T data;

  ValueImpl(std::string&& name, const T& data)
    : Value(name, PropertyTypeMap<T>::type)
    , data(data)
  {
  }
};

template <class T>
Property::Property(const char* name, const T& data)
{
  engine_check_null(name);

  std::string name_string = name;

  value = std::make_shared<ValueImpl<T>>(name_string, data);
}

template <class T>
T& Property::get()
{
  constexpr PropertyType expected_type = PropertyTypeMap<T>::type;

  if (expected_type == value->type)
    return static_cast<ValueImpl<T>*>(value)->data;
  
  auto type_to_string = [](PropertyType type)
  {
    switch (type)
    {
      case PropertyType_Int:   return "int";
      case PropertyType_Float: return "float";
      case PropertyType_Vec4f: return "vec4f";
      case PropertyType_Mat4f: return "mat4f";
      default:                 return "<unknown>";
    }
  };

  throw Exception::format("PropertyType mismatch: requested %s, actual %s", type_to_string(expected_type), type_to_string(value->type));
}

template <class T>
const T& Property::get() const
{
  return const_cast<Property&>(*this).get<T>();
}

template <class T>
void Property::set(const T& data) const
{
  constexpr PropertyType expected_type = PropertyTypeMap<T>::type;

  if (expected_type == value->type)
  {
    static_cast<ValueImpl<T>*>(value)->data = data;
    return;
  }

  value = std::make_shared<ValueImpl<T>>(value->name, data);
}

inline const Property& PropertyMap::operator[](const char* name) const
{
  return get(name);
}

inline Property& PropertyMap::operator[](const char* name)
{
  return get(name);
}

template <class T>
Property& PropertyMap::set(const char* name, const T& value)
{
  if (Property* property = find(name))
  {
    property->set(value);
    return *property;
  }

  Property property(name, value);

  size_t index = insert(name, property);

  return items()[index];
}
