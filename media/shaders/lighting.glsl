//TODO optimize

#shader vertex
#version 410 core

in vec3 vPosition;
in vec2 vTexCoord;
out vec2 texCoord;

void main()
{
  gl_Position = vec4(vPosition, 1.0);
  texCoord = vTexCoord.xy;
}

#shader pixel
#version 410 core

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;

in vec2 texCoord;
out vec4 outColor;

const float MIN_DIFFUSE_AMOUNT = 0.1; // ambient light
const float DIFFUSE_AMOUNT = 1.0; // diffuse light multiplier
const float SPECULAR_AMOUNT = 1.0; // specular light multiplier
const vec3 VIEW_POS = vec3(0.0, 0.0, 0.0); // camera position

#define MAX_POINT_LIGHTS 32
#define MAX_SPOT_LIGHTS 2

uniform vec3 point_light_positions[MAX_POINT_LIGHTS];
uniform vec3 point_light_colors[MAX_POINT_LIGHTS];
uniform vec3 point_light_attenuations[MAX_POINT_LIGHTS];
uniform float point_light_ranges[MAX_POINT_LIGHTS];

uniform vec3 spot_light_positions[MAX_SPOT_LIGHTS];
uniform vec3 spot_light_directions[MAX_SPOT_LIGHTS];
uniform vec3 spot_light_colors[MAX_SPOT_LIGHTS];
uniform vec3 spot_light_attenuations[MAX_SPOT_LIGHTS];
uniform float spot_light_ranges[MAX_SPOT_LIGHTS];
uniform float spot_light_angles[MAX_SPOT_LIGHTS];
uniform float spot_light_exponents[MAX_SPOT_LIGHTS];

vec3 ComputeDiffuseColor(const in vec3 normal, const in vec3 lightDir, const in vec3 texDiffuseColor)
{
  return texDiffuseColor * max(dot(lightDir, normal), MIN_DIFFUSE_AMOUNT);
}

vec3 ComputeSpecularColor(const in vec3 normal, const in vec3 lightDir, const in vec3 eyeDir, const in vec3 texSpecularColor, const in float shininess)
{
  float specularFactor = pow(clamp(dot(reflect(-lightDir, normal), eyeDir), 0.00001, 1.0), shininess);

  return texSpecularColor * specularFactor;
}

void main()
{
  vec3 position = texture(positionTexture, texCoord).xyz;
  vec3 normal = texture(normalTexture, texCoord).xyz;
  vec3 albedo = texture(albedoTexture, texCoord).xyz;
  vec4 specular = texture(specularTexture, texCoord);
  
  vec3 color = vec3(0.0);

  for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
  {  
    vec3 light_position = point_light_positions[i];
    vec3 light_color = point_light_colors[i];
    vec3 light_attenuation = point_light_attenuations[i];
    float light_range = point_light_ranges[i];

    float distance = length(light_position - position);
    float attenuation = min(1.0, light_range / (light_attenuation.x + light_attenuation.y * distance + light_attenuation.z * (distance * distance))); 
    vec3 normalizedLightDirection = normalize(light_position - position);
    
    color += ComputeDiffuseColor(normal, normalizedLightDirection, albedo) * DIFFUSE_AMOUNT * light_color * attenuation;
    color += ComputeSpecularColor(normal, normalizedLightDirection, normalize(VIEW_POS - position),
       specular.xyz, specular.w) * SPECULAR_AMOUNT * light_color * attenuation;
  }

  for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
  {
    vec3 spot_light_position = spot_light_positions[i];
    vec3 spot_light_direction = spot_light_directions[i];
    float spot_light_angle = spot_light_angles[i];

    vec3 spotLightDirection = normalize(spot_light_position - position);
    float spotLightTheta = acos(dot(spotLightDirection, normalize(spot_light_direction)));

    if (spotLightTheta < spot_light_angle)
    {
      vec3 spot_light_color = spot_light_colors[i];
      vec3 spot_light_attenuation = spot_light_attenuations[i];
      float spot_light_range = spot_light_ranges[i];
      float spot_light_exponent = spot_light_exponents[i];

      float distance = length(spot_light_position - position);
      float attenuation = min(1.0, spot_light_range / (spot_light_attenuation.x +
        spot_light_attenuation.y * distance + spot_light_attenuation.z * (distance * distance))); 
    
      attenuation *= pow(1 - spotLightTheta / spot_light_angle, spot_light_exponent); 

      color += ComputeDiffuseColor(normal, spotLightDirection, albedo) * DIFFUSE_AMOUNT * spot_light_color * attenuation;
      color += ComputeSpecularColor(normal, spotLightDirection, normalize(VIEW_POS - position),
        specular.xyz, specular.w) * SPECULAR_AMOUNT * spot_light_color * attenuation;
    }
  }
  
  outColor = vec4(color, 1.0);
  
  // debug output
/*  if (texCoord.x < 0.5 && texCoord.y < 0.5)
  {
    vec4 albedo = texture(albedoTexture, texCoord * 2.0);

    outColor = vec4(albedo.xyz, 1.f);
  }
  else if (texCoord.y < 0.5)
  {
    vec4 specular = texture(specularTexture, vec2((texCoord.x - 0.5) * 2.0, texCoord.y * 2.0));

    outColor = vec4(specular.xyz, 1.f);
  }
  else if (texCoord.x < 0.5)
  {
    vec4 position = texture(positionTexture, vec2(texCoord.x * 2.0, (texCoord.y - 0.5) * 2.0));

    outColor = vec4(position.xyz, 1.f);
  }
  else
  {
    vec4 normal = texture(normalTexture, vec2((texCoord.x - 0.5) * 2.0, (texCoord.y - 0.5) * 2.0));

    outColor = vec4(normal.xyz, 1.f);
  }*/
}
