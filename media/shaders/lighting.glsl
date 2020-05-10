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
const vec3 VIEW_POS = vec3(0.0, 0.0, -10.0); // camera position
const int LIGHTS_COUNT = 4;

struct Light {
  vec3 position;
  vec3 color;
  vec3 attenuation;
  float range;
};

struct SpotLight {
  vec3 position;
  vec3 direction;
  vec3 color;
  vec3 attenuation;
  float range;
  float radius;
  float exponent;
};

const SpotLight SPOT_LIGHT = SpotLight(vec3(0.0, 0.0, -10.0), vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 1.0), vec3(1.0, 0.35, 0.44), 100, 0.05, 0.8); //spot light

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
  Light LIGHTS[LIGHTS_COUNT];

  LIGHTS[0] = Light(vec3(10.0, 10.0, -10.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.35, 0.44), 100);
  LIGHTS[1] = Light(vec3(-10.0, 10.0, -10.0), vec3(0.0, 1.0, 0.0), vec3(1.0, 0.35, 0.44), 100);
  LIGHTS[2] = Light(vec3(-10.0, -10.0, -10.0), vec3(0.0, 0.0, 1.0), vec3(1.0, 0.35, 0.44), 100);
  LIGHTS[3] = Light(vec3(10.0, -10.0, -10.0), vec3(1.0, 1.0, 1.0), vec3(1.0, 0.35, 0.44), 100);

  vec3 position = texture(positionTexture, texCoord).xyz;
  vec3 normal = texture(normalTexture, texCoord).xyz;
  vec3 albedo = texture(albedoTexture, texCoord).xyz;
  vec4 specular = texture(specularTexture, texCoord);
  
  vec3 color = vec3(0.0);

  for (int i = 0; i < LIGHTS_COUNT; ++i)
  {  
    Light light = LIGHTS[i];
    float distance = length(light.position - position);
    float attenuation = min(1.0, light.range / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * (distance * distance))); 
    vec3 normalizedLightDirection = normalize(light.position - position);
    
    color += ComputeDiffuseColor(normal, normalizedLightDirection, albedo) * DIFFUSE_AMOUNT * LIGHTS[i].color * attenuation;
    color += ComputeSpecularColor(normal, normalizedLightDirection, normalize(VIEW_POS - position), specular.xyz, specular.w) * SPECULAR_AMOUNT * LIGHTS[i].color * attenuation;
  }

  vec3 spotLightDirection = normalize(SPOT_LIGHT.position - position);
  float spotLightTheta = acos(dot(spotLightDirection, normalize(-SPOT_LIGHT.direction)));

  if (spotLightTheta < SPOT_LIGHT.radius)
  {
    float distance = length(SPOT_LIGHT.position - position);
    float attenuation = min(1.0, SPOT_LIGHT.range / (SPOT_LIGHT.attenuation.x + SPOT_LIGHT.attenuation.y * distance + SPOT_LIGHT.attenuation.z * (distance * distance))); 
    
    attenuation *= pow(1 - spotLightTheta / SPOT_LIGHT.radius, SPOT_LIGHT.exponent); 

    color += ComputeDiffuseColor(normal, spotLightDirection, albedo) * DIFFUSE_AMOUNT * SPOT_LIGHT.color * attenuation;
    color += ComputeSpecularColor(normal, spotLightDirection, normalize(VIEW_POS - position), specular.xyz, specular.w) * SPECULAR_AMOUNT * SPOT_LIGHT.color * attenuation;
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
