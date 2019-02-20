#version 330 core

//uniform float zoom;
//uniform vec2 translation;
uniform mat4 mat;
in vec3 vtx_position;
in vec3 vtx_color;
out vec3 var_color;

void main()
{
  //gl_Position = vec4(zoom*vtx_position.xy+translation,-1*vtx_position.z ,1.);
  gl_Position = mat*vec4(vtx_position,1);
  var_color=vtx_color;
}
