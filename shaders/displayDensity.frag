#version 130

uniform sampler2D palette;

in float density;

out vec4 fragColor;


void main()
{
    fragColor = texture(palette, vec2(density, .5));
}
