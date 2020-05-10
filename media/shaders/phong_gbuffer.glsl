#shader vertex
#version 410 core

uniform mat4 MVP;
in vec4 vColor;
in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;
out vec3 position;
out vec3 eyeDirection;
out vec3 normal;
out vec4 color;
out vec2 texCoord;

const vec3 VIEW_POS = vec3(0.0, 0.0, -10.0); // camera_position

void main()
{
  gl_Position = MVP * vec4(vPosition, 1.0);
  position = gl_Position.xyz;
  eyeDirection = VIEW_POS - gl_Position.xyz;
  normal = normalize (MVP * vec4 (vNormal, 0.0)).xyz;
  color = vColor;
  texCoord = vTexCoord;
}

#shader pixel
#version 410 core

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outSpecular;

in vec3 position;
in vec3 eyeDirection;
in vec3 normal;
in vec4 color;
in vec2 texCoord;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D specularTexture;
uniform float shininess;

mat3 CotangentFrame(const in vec3 N, const in vec3 p, const in vec2 uv)
{
  // get edge vectors of the pixel triangle
  vec3 dp1 = dFdx(p);
  vec3 dp2 = dFdy(p);
  vec2 duv1 = dFdx(uv);
  vec2 duv2 = dFdy(uv);

  // solve the linear system
  vec3 dp2perp = cross(dp2, N);
  vec3 dp1perp = cross(N, dp1);
  vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

  // construct a scale-invariant frame
  float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    
  return mat3(T * invmax, B * invmax, N);
}

void main()
{
  vec3 normalizedEyeDirection = normalize(eyeDirection);

  mat3 tbn = CotangentFrame(normalize(normal), -normalizedEyeDirection, texCoord);

  vec3 mappedNormal = texture(normalTexture, texCoord).xyz * 2.0 - 1.0;

  mappedNormal = normalize(tbn * mappedNormal);

  outPosition = position;
  outNormal = mappedNormal;
  outAlbedo = texture(diffuseTexture, texCoord) * color;
  outSpecular = vec4(texture(specularTexture, texCoord).xyz * color.xyz, shininess);
}
