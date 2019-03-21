#version 330 core

uniform mat4 obj_mat;
uniform mat4 proj_mat;
uniform mat4 view_mat;
uniform mat3 normal_mat;
float M_PI=3.14159265359;
in vec3 vtx_position;
in vec3 vtx_normal;
in vec2 vtx_texcoord;

out vec3 v_normal;
out vec3 v_view;
out vec2 v_uv;

vec3 cylinder(vec2 uv, vec3 A, vec3 B, float r)
{
    vec3 Zl=normalize(B-A);
    vec3 Xl=cross(Zl,vec3(1,0,0));
    vec3 Yl=cross(Zl,Xl);
    vec3 pv=(1-uv.y)*A+uv.y*B;
    vec3 c=vec3(r*cos(uv.x*(2*M_PI)),r*sin(uv.x*(2*M_PI)),0);

    vec3 s=mat3(Xl,Yl,Zl)*c+pv;
    return s;
}

void main()
{
  v_uv  = vtx_texcoord +0.0000000000000001*vtx_position.xy;
  v_normal = normalize(normal_mat * vtx_normal);
  vec3 A=vec3(0,0,0);
  vec3 B=vec3(0,3,5);
  vec3 v_pos=cylinder(v_uv,A,B,1);
  vec4 p = view_mat * (obj_mat * vec4(v_pos, 1.));
  v_view = normalize(-p.xyz);
  gl_Position = proj_mat * p;
}
