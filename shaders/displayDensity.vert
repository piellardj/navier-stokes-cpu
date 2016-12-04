#version 130


in vec2 vPos;
in float vDensity;

out float density;


void main()
{
    gl_Position = vec4(vPos, 0, 1);
    density = vDensity;
}
