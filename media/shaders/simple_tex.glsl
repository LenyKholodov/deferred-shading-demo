#shader vertex
#version 410 core

uniform mat4 MVP;
in vec4 vColor;
in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;
out vec3 position;
out vec3 normal;
out vec4 color;
out vec2 texcoord;

void main()
{
  gl_Position = MVP * vec4(vPosition, 1.0);
  position = gl_Position.xyz;
  texcoord = vTexCoord;
  normal = vNormal;
  color = vColor;
}

#shader pixel
#version 410 core

in vec3 position;
in vec3 normal;
in vec4 color;
in vec2 texcoord;

uniform sampler2D diffuse_texture;

out vec4 out_color;

void main()
{
  out_color = vec4(texture(diffuse_texture, texcoord.xy).rgb, 1.f) * color;
}
