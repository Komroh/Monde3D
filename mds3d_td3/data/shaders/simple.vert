#version 330 core

//uniform float zoom;
//uniform vec2 translation;
uniform mat4 mat;

in vec3 vtx_position;
in vec3 vtx_color;
in vec3 vtx_normal;
out vec3 var_color;


vec3 blinn(vec3 n, vec3 v, vec3 l,vec3 dCol, vec3 sCol, float s)
{

   float dc=max(0,dot(n,l));
   float ds=max(0,dot(n,normalize(v+l)));
   return  dc*dCol+sCol*l*pow(ds,s);
}

void main()
{
  //gl_Position = vec4(zoom*vtx_position.xy+translation,-1*vtx_position.z ,1.);
  gl_Position = mat*vec4(vtx_position,1);
  var_color=blinn(vtx_normal,vec3(0),normalize(vec3(1,1,1)), vec3(0.5,0,0.2),vec3(0.7,0,0.5),10);

}


