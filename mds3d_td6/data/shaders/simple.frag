#version 330 core

in vec3 v_color;
in vec3 v_normal;
in vec3 v_view;
in vec2 v_texcoord;

uniform vec3 lightDir;
uniform sampler2D tex2D ;
uniform sampler2D tex2D_2;
uniform sampler2D tex2D_3;
uniform mat3 normal_mat;
out vec4 out_color;




vec3 blinn(vec3 n, vec3 v, vec3 l, vec3 dCol, vec3 sCol, float s)
{
  vec3 res = vec3(0,0,0);
  float dc = max(0,dot(n,l));
  if(dc>0) {
    res = dCol * dc;
    float sc = max(0,dot(n,normalize(v+l)));
    if(sc>0)
      res += sCol * pow(sc,s) * dc;
  }
  return res;
}

vec3 map(vec3 value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}
void main(void) {
  float ambient = 0.0;
  float shininess = 50;
  vec3 spec_color = vec3(1,1,1);
  float c=max(dot(normalize(v_normal),lightDir),0);
  vec3 norm=map(texture(tex2D_2,v_texcoord).xyz,0.0,1.0,-1.0,1.0);
  //norm= normalize(normal_mat*norm);
  //vec4 earth=texture(tex2D,v_texcoord);
  vec4 hey=texture(tex2D_2,v_texcoord);
  //vec4 night=texture(tex2D_3,v_texcoord);
  out_color=vec4(ambient * v_color/*texture(tex2D_3,v_texcoord).xyz*/ + blinn(normalize(norm),normalize(v_view), lightDir, v_color/*texture(tex2D_3,v_texcoord).xyz*/, spec_color, shininess),1.0)
;
}
