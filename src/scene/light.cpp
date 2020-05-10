#include <scene/light.h>

using namespace engine::scene;

///
/// Light
///

/// Light implementation details
struct Light::Impl
{
  math::vec3f color; //light color
  math::vec3f attenuation; //light attenuation
  float intensity; //light intensity
  float range; //light range

  Impl()
    : color(1.0f)
    , attenuation()
    , intensity(1.f)
    , range(DEFAULT_LIGHT_RANGE)
  {
  }
};

Light::Light()
  : impl(std::make_unique<Impl>())
{
}

void Light::set_light_color(const math::vec3f& color)
{
  impl->color = color;
}

const math::vec3f& Light::light_color() const
{
  return impl->color;
}

void Light::set_intensity(float value)
{
  impl->intensity = value;
}

float Light::intensity() const
{
  return impl->intensity;
}

void Light::set_attenuation(const math::vec3f& multiplier)
{
  impl->attenuation = multiplier;
}

const math::vec3f& Light::attenuation() const
{
  return impl->attenuation;
}

void Light::set_range(float range)
{
  impl->range = range;
}

float Light::range() const
{
  return impl->range;
}

void Light::visit(ISceneVisitor& visitor)
{
  Node::visit(visitor);

  visitor.visit(*this);
}

///
/// PointLight
///

PointLight::Pointer PointLight::create()
{
  return PointLight::Pointer(new PointLight);
}

void PointLight::visit(ISceneVisitor& visitor)
{
  Light::visit(visitor);

  visitor.visit(*this);
}

///
/// SpotLight
///

/// Spot light implementation details
struct SpotLight::Impl
{
  math::anglef angle; //cone angle
  float exponent; //attenuation exponent
};

SpotLight::SpotLight()
  : impl(std::make_unique<Impl>())
{
}

SpotLight::Pointer SpotLight::create()
{
  return SpotLight::Pointer(new SpotLight);
}

void SpotLight::set_angle(const math::anglef& angle)
{
  impl->angle = angle;
}

void SpotLight::set_exponent(float exponent)
{
  impl->exponent = exponent;
}

const math::anglef& SpotLight::angle() const
{
  return impl->angle;
}

float SpotLight::exponent() const
{
  return impl->exponent;
}

void SpotLight::visit(ISceneVisitor& visitor)
{
  Light::visit(visitor);

  visitor.visit(*this);
}
