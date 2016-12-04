#include "Fluid.hpp"

#include <iostream>
#include <sstream>

#include "GLHelper.hpp"


Fluid::Fluid(unsigned int nbCols, unsigned int nbLines, float viscosity):
            _nbLines(nbLines),
            _nbCols(nbCols),
            _posBufferID(-1),
            _indexBufferID(-1),
            _densBufferID(-1),
            _viscosity(viscosity)
{
    /* Shader loading */
    if (!_densityShader.loadFromFile("shaders/displayDensity.vert", "shaders/displayDensity.frag")) {
        std::cerr << "Error: couldn't load displayDensity shader." << std::endl;
    }
    if (!_velocityShader.loadFromFile("shaders/displayVelocity.vert", "shaders/displayVelocity.frag")) {
        std::cerr << "Error: couldn't load displayVelocity shader." << std::endl;
    }

    /* Textures loading */
    if (!_palette.loadFromFile("textures/palette.png")) {
        std::cerr << "Error: couldn't load palette.png." << std::endl;
    }
    _palette.setSmooth(true);
    
    /* Buffers for displaying */
    int nbPtX = _nbCols, nbPtY = _nbLines;
    float stepX = 1.f / static_cast<float>(nbPtX-1), stepY = 1.f / static_cast<float>(nbPtY-1);
    std::vector<glm::vec2> positions(nbPtX * nbPtY);
    for (int iX = 0 ; iX < nbPtX ; ++iX) {
        for (int iY = 0 ; iY < nbPtY ; ++iY) {
            int index = iX*nbPtY + iY;
            positions[index].x = 2.f * (static_cast<float>(iX) * stepX - 0.5f);
            positions[index].y = 2.f * (static_cast<float>(iY) * stepY - 0.5f);
        }
    }

    std::vector<glm::ivec3> indexes(2 * (nbPtX-1) * (nbPtY-1));
    for (int iX = 0 ; iX < nbPtX-1 ; ++iX) {
        for (int iY = 0 ; iY < nbPtY-1 ; ++iY) {
            int index = iX*(nbPtY-1) + iY;
            indexes[2*index+0] = glm::ivec3(iX,iX,iX+1)*nbPtY + glm::ivec3(iY+1, iY, iY);
            indexes[2*index+1] = glm::ivec3(iX,iX+1,iX+1)*nbPtY + glm::ivec3(iY+1, iY, iY+1);
        }
    }

    GLCHECK(glGenBuffers(1, &_posBufferID));
    GLCHECK(glGenBuffers(1, &_indexBufferID));
    GLCHECK(glGenBuffers(1, &_densBufferID)); //mapped to colors in the shader
    GLCHECK(glGenBuffers(1, &_velBufferID));
    
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _posBufferID));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, positions.size()*sizeof(glm::vec2), positions.data(), GL_STATIC_DRAW));
    GLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBufferID));
    GLCHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size()*sizeof(glm::ivec3), indexes.data(), GL_STATIC_DRAW));
    GLCHECK(glGenBuffers(1, &_densBufferID)); //corresponding texture pixel coordinates
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _densBufferID));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, nbCols*nbLines*sizeof(float), NULL, GL_DYNAMIC_DRAW));
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _velBufferID));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, 2*positions.size()*sizeof(glm::vec2), NULL, GL_DYNAMIC_DRAW));
    
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

Fluid::~Fluid ()
{
    if (_posBufferID != 0) {
        GLCHECK(glDeleteBuffers(1, &_posBufferID));
    }
    if (_indexBufferID != 0) {
        GLCHECK(glDeleteBuffers(1, &_indexBufferID));
    }
    if (_densBufferID != 0) {
        GLCHECK(glDeleteBuffers(1, &_densBufferID));
    }
    if (_velBufferID != 0) {
        GLCHECK(glDeleteBuffers(1, &_velBufferID));
    }
}

sf::Vector2i Fluid::getSize() const
{
    return sf::Vector2i(_nbCols, _nbLines);
}

void Fluid::draw(bool drawDensity, bool drawIntensity)
{

    GLCHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GLCHECK(glClearColor(0, 0, 0, 1.0));
    
    if (drawDensity)
        Fluid::drawDensity();
    if (drawIntensity)
        Fluid::drawIntensity();
}

void Fluid::drawDensity()
{
    fetchDensityBuffer();

    GLuint displayShaderHandle = _densityShader.getNativeHandle();
    if (displayShaderHandle == 0 || displayShaderHandle == (GLuint)(-1))
        return;

    sf::Shader::bind(&_densityShader);
    _densityShader.setParameter("palette", _palette);
    
    GLuint posALoc = glGetAttribLocation(displayShaderHandle, "vPos");
    GLuint densALoc = glGetAttribLocation(displayShaderHandle, "vDensity");

    if(posALoc != (GLuint)(-1)) {
        GLCHECK(glEnableVertexAttribArray(posALoc));
        GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _posBufferID));
        GLCHECK(glVertexAttribPointer(posALoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0));
    }
    if(densALoc != (GLuint)(-1)) {
        GLCHECK(glEnableVertexAttribArray(densALoc));
        GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _densBufferID));
        GLCHECK(glVertexAttribPointer(densALoc, 1, GL_FLOAT, GL_FALSE, 0, (void*)0));
    }
    GLCHECK(glDisable(GL_DEPTH_TEST));
    GLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBufferID));
    GLCHECK(glDrawElements(GL_TRIANGLES, 6*(_nbCols-1)*(_nbLines-1), GL_UNSIGNED_INT, (void*)0));

    if (densALoc != (GLuint)(-1)) {
        GLCHECK(glDisableVertexAttribArray(densALoc));
    }
    if (posALoc != (GLuint)(-1)) {
        GLCHECK(glDisableVertexAttribArray(posALoc));
    }
    GLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    sf::Shader::bind(0);
}

void Fluid::drawIntensity()
{
    fetchVelocityBuffer();

    GLuint displayShaderHandle = _velocityShader.getNativeHandle();
    if (displayShaderHandle == 0 || displayShaderHandle == (GLuint)(-1))
        return;

    sf::Shader::bind(&_velocityShader);
    
    GLuint posALoc = glGetAttribLocation(displayShaderHandle, "vPos");

    if(posALoc != (GLuint)(-1)) {
        GLCHECK(glEnableVertexAttribArray(posALoc));
        GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _velBufferID));
        GLCHECK(glVertexAttribPointer(posALoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0));
    }

    GLCHECK(glDisable(GL_DEPTH_TEST));
    GLCHECK(glDrawArrays(GL_LINES, 0, 2*_nbLines*_nbCols));

    if (posALoc != (GLuint)(-1)) {
        GLCHECK(glDisableVertexAttribArray(posALoc));
    }
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    sf::Shader::bind(0);
}

