#shader vertex
#version 410 core

uniform mat4 MVP;
in vec4 vColor;
in vec3 vPosition;
in vec3 vNormal;
out vec3 position;
out vec3 normal;
out vec4 color;

void main()
{
  gl_Position = MVP * vec4(vPosition, 1.0);
  position = gl_Position.xyz;
  normal = vNormal;
  color = vColor;
}

#shader pixel
#version 410 core

in vec3 position;
in vec3 normal;
in vec4 color;

uniform sampler2D diffuse_texture;

out vec4 out_color;

void main()
{
  out_color = vec4(texture(diffuse_texture, position.xy).rgb, 1.f) * color;
}
