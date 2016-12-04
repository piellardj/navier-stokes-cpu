#ifndef FLUID_HPP_INCLUDED
#define FLUID_HPP_INCLUDED

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Shader.hpp>

#include <GL/glew.h>
#include "glm.hpp"
#include <SFML/OpenGL.hpp>


/* Handles the drawing part of the simulation */
class Fluid
{
    public:
        Fluid(unsigned int nbCols, unsigned int nbLines, float visc);
        virtual ~Fluid();

        sf::Vector2i getSize () const;

        virtual void reset() = 0;

        /* position normalisée */
        virtual void addDensity (sf::Vector2f pos, float radius, float strength=0.1f) = 0;
        virtual void addVelocity (sf::Vector2f pos, sf::Vector2f dir) = 0;
        virtual void update(float dt) = 0;

        virtual void draw(bool drawDensity=true, bool drawIntensity=false);

    protected:
        virtual void drawDensity();
        virtual void drawIntensity();

        /* Updates OpenGL buffers for drawing */
        virtual void fetchDensityBuffer() = 0;
        virtual void fetchVelocityBuffer() = 0;


    protected:
        const unsigned int _nbLines;
        const unsigned int _nbCols;

        GLuint _posBufferID;
        GLuint _indexBufferID;
        GLuint _densBufferID;

        GLuint _velBufferID;

        sf::Texture _palette;
        sf::Shader _densityShader;
        sf::Shader _velocityShader;

    public:
        float _viscosity;
};

#endif // FLUID_HPP_INCLUDED
