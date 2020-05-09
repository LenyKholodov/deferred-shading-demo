#shader vertex
#version 410 core

uniform mat4 MVP;
in vec4 vColor;
in vec3 vPosition;
out vec4 color;

void main()
{
  gl_Position = MVP * vec4(vPosition, 1.0);
  color = vColor;
}

#shader pixel
#version 410 core

in vec4 color;
out vec4 outColor;

void main()
{
  outColor = vec4(gl_FragCoord.z);
}
