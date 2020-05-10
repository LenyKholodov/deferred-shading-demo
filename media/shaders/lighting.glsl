#shader vertex
#version 410 core

uniform mat4 MVP;
in vec4 vColor;
in vec3 vPosition;
in vec2 vTexCoord;
out vec4 color;
out vec2 texCoord;

void main()
{
  gl_Position = MVP * vec4(vPosition, 1.0);
  color = vColor;
  texCoord = vTexCoord.xy;
}

#shader pixel
#version 410 core

uniform sampler2D albedoTexture;

in vec4 color;
in vec2 texCoord;
out vec4 outColor;

void main()
{
  vec4 albedo = texture(albedoTexture, texCoord);

  outColor = vec4(albedo.xyz, 1.f);
}
