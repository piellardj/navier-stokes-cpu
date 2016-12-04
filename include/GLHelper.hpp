#ifndef GLHELPER_HPP_INCLUDED
#define GLHELPER_HPP_INCLUDED

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/Shader.hpp>

#include "glm.hpp"


void gl_CheckError(const char* file, unsigned int line, const char* expression);

#ifdef DEBUG
    #define GLCHECK(expr) do { expr; gl_CheckError(__FILE__, __LINE__, #expr); } while (false)
#else
    #define GLCHECK(expr) (expr)
#endif // DEBUG

#endif // GLHELPER_HPP_INCLUDED
