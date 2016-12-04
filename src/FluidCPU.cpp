#include "FluidCPU.hpp"

#include <sstream>
#include <iostream>

#include "GLHelper.hpp"


inline int nextBuffer(int currBuffer)
{
    return (currBuffer + 1) % 2;
}

FluidCPU::FluidCPU (unsigned int nbCols, unsigned int nbLines, float visc):
            Fluid::Fluid(nbCols, nbLines, visc),
            _currDensity(0),
            _currVel(0)
{
    _densities[0].resize(nbCols*nbLines, 0.f);
    _densities[1].resize(nbCols*nbLines, 0.f);
    
    _velX[0].resize(nbCols*nbLines, 0.f);
    _velX[1].resize(nbCols*nbLines, 0.f);
    
    _velY[0].resize(nbCols*nbLines, 0.f);
    _velY[1].resize(nbCols*nbLines, 0.f);
}

void FluidCPU::reset()
{
    for (int i = 0 ; i <= 1 ; ++i) {
        std::fill(_densities[i].begin(), _densities[i].end(), 0.f);
        std::fill(_velX[i].begin(), _velX[i].end(), 0.f);
        std::fill(_velY[i].begin(), _velY[i].end(), 0.f);
    }
}

void FluidCPU::addDensity (sf::Vector2f pos, float radius, float strength)
{
    BufferFloat& oldDensities = _densities[_currDensity];
    BufferFloat& newDensities = _densities[nextBuffer(_currDensity)];

    float cCol = pos.y;
    float cLine = pos.x;

    for (unsigned int line = 1 ; line < _nbLines-1 ; ++line) {
        for (unsigned int col = 1 ; col < _nbCols-1 ; ++col) {
            int currIndex = index(line,col);

            float fLine = (float)(line) / (float)_nbLines;
            float fCol = (float)(col) / (float)_nbCols;
            float distSq = (fLine-cLine)*(fLine-cLine) + (fCol-cCol)*(fCol-cCol);

            float perturbation = (distSq < radius) ? strength : 0.f;
            
            newDensities[currIndex] = oldDensities[currIndex] + perturbation;

            newDensities[currIndex] = std::min(1.f, newDensities[currIndex]);
        }
    }

    _currDensity = nextBuffer(_currDensity);
}

void FluidCPU::addVelocity (sf::Vector2f pos, sf::Vector2f dir)
{
    BufferFloat& oldVelX = _velX[_currVel];
    BufferFloat& oldVelY = _velY[_currVel];
    BufferFloat& newVelX = _velX[nextBuffer(_currVel)];
    BufferFloat& newVelY = _velY[nextBuffer(_currVel)];

    float radius = 0.002f;
    float cCol = pos.y;
    float cLine = pos.x;

    for (unsigned int line = 1 ; line < _nbLines-1 ; ++line) {
        for (unsigned int col = 1 ; col < _nbCols-1 ; ++col) {
            int currIndex = index(line,col);

            float fLine = (float)(line) / (float)_nbLines;
            float fCol = (float)(col) / (float)_nbCols;
            float distSq = (fLine-cLine)*(fLine-cLine) + (fCol-cCol)*(fCol-cCol);

            glm::vec2 perturbation = (distSq < radius) ? 1000.f*glm::vec2(dir.y, dir.x) : glm::vec2(0.f, 0.f);
            
            newVelX[currIndex] = oldVelX[currIndex] + perturbation.x;
            newVelY[currIndex] = oldVelY[currIndex] + perturbation.y;
        }
    }

    _currVel = nextBuffer(_currVel);
}

void FluidCPU::update (float dt)
{
    solveDensity(dt);
    solveVelocity(dt);
}

void FluidCPU::solveDensity (float dt)
{
    BufferFloat& oldDensities = _densities[_currDensity];
    BufferFloat& newDensities = _densities[nextBuffer(_currDensity)];
    
    diffuse(oldDensities, newDensities, &FluidCPU::densityBoundaryConditions, dt);
    
    advect(newDensities, oldDensities, _velX[_currVel], _velY[_currVel], dt);
}

void FluidCPU::solveVelocity (float dt)
{
    diffuse(_velX[_currVel], _velX[nextBuffer(_currVel)], &FluidCPU::velXBoundaryConditions, dt);
    diffuse(_velY[_currVel], _velY[nextBuffer(_currVel)], &FluidCPU::velYBoundaryConditions, dt);
    
    project(_velX[nextBuffer(_currVel)], _velY[nextBuffer(_currVel)], _velX[_currVel], _velY[_currVel]);
    
    _currVel = nextBuffer(_currVel);
    
    advect(_velX[_currVel], _velX[nextBuffer(_currVel)], _velX[_currVel], _velY[_currVel], dt);
    advect(_velY[_currVel], _velY[nextBuffer(_currVel)], _velX[_currVel], _velY[_currVel], dt);
    
    project(_velX[nextBuffer(_currVel)], _velY[nextBuffer(_currVel)], _velX[_currVel], _velY[_currVel]);
    
    _currVel = nextBuffer(_currVel);
}

void FluidCPU::diffuse(BufferFloat const& src, BufferFloat& dst, std::function<void(FluidCPU&, BufferFloat&)> boundaryConditions, float dt)
{
    float a = _viscosity * _nbCols * _nbLines * dt;
    
    /* Gauss-Seidel relaxation for iteratively solving the linear system */
    for (int k = 0 ; k < 20 ; ++k) {
        for (unsigned int line = 1 ; line < _nbLines-1 ; ++line) {
            for (unsigned int col = 1 ; col < _nbCols-1 ; ++col) {
                float l_c = src[index(line,col)];
                float lm_c = dst[index(line-1,col)];
                float lp_c = dst[index(line+1,col)];
                float l_cm = dst[index(line,col-1)];
                float l_cp = dst[index(line,col+1)];
                
                dst[index(line,col)] = (l_c + a*(lm_c + lp_c + l_cm + l_cp)) / (1.f + 4.f * a);
            }
        }
        
        boundaryConditions(*this, dst);
    }
}

void FluidCPU::advect(BufferFloat const& src, BufferFloat& dst, BufferFloat const& velX, BufferFloat const& velY, float dt)
{
    for (unsigned int line=1 ; line < _nbLines-1 ; ++line) {
        for (unsigned int col=1 ; col < _nbCols-1 ; ++col) {
            float prevLine = static_cast<float>(line) - dt*velY[index(line,col)];
            float prevCol = static_cast<float>(col) - dt*velX[index(line,col)];
            
            prevLine = std::max(0.5f, prevLine);
            prevLine = std::min(static_cast<float>(_nbLines) - 0.5f, prevLine);
            
            prevCol = std::max(0.5f, prevCol);
            prevCol = std::min(static_cast<float>(_nbCols) - 0.5f, prevCol);
            
            /* Bilinear interpolation */
            int col0 = prevCol, col1 = col0+1;
            int line0 = prevLine, line1 = line0+1;
            
            float h = prevCol - static_cast<float>(col0);
            float v = prevLine - static_cast<float>(line0);

            dst[index(line,col)] = h   *   (v*src[index(line1,col1)] + (1.f-v)*src[index(line0,col1)]) +
                                   (1.f-h)*(v*src[index(line1,col0)] + (1.f-v)*src[index(line0,col0)]);
        }
    }
}

void FluidCPU::project(BufferFloat& velX, BufferFloat& velY, BufferFloat& p, BufferFloat& div)
{
    float h = 1.f / std::sqrt(_nbLines*_nbCols);
    
    for (unsigned int line = 1 ; line < _nbLines-1 ; ++line) {
        for (unsigned int col = 1 ; col < _nbCols-1 ; ++col) {
            div[index(line,col)] = -0.5f * h * (velX[index(line,col+1)] - velX[index(line,col-1)] +
                                                velY[index(line+1,col)] - velY[index(line-1,col)]);
        }
    }
    boundaryConditions(div, 1.f,  1.f);
    std::fill(p.begin(), p.end(), 0.f);
    
    /* Gauss Seidel relaxation */
    for (unsigned int k = 0 ; k < 20 ; ++k) {
        for (unsigned int line = 1 ; line < _nbLines-1 ; ++line) {
            for (unsigned int col = 1 ; col < _nbCols-1 ; ++col) {
                p[index(line,col)] = 0.25f * (div[index(line,col)] + p[index(line+1,col)] + p[index(line-1,col)] +
                                                                     p[index(line,col+1)] + p[index(line,col-1)]);
            }
        }
        boundaryConditions(p, 1.f, 1.f);
    }
    
    for (unsigned int line = 1 ; line < _nbLines-1 ; ++line) {
        for (unsigned int col = 1 ; col < _nbCols-1 ; ++col) {
            velX[index(line,col)] -= 0.5f * (p[index(line,col+1)] - p[index(line,col-1)]) / h;
            velY[index(line,col)] -= 0.5f * (p[index(line+1,col)] - p[index(line-1,col)]) / h;
        }
    }
    velXBoundaryConditions(velX);
    velYBoundaryConditions(velY);
}

void FluidCPU::boundaryConditions (BufferFloat& buffer, float hFactor, float vFactor)
{
    /* boundaries conditions */
    for (unsigned int line=1 ; line < _nbLines-1 ; ++line) {
        buffer[index(line,0)] = hFactor * buffer[index(line,1)];
        buffer[index(line,_nbCols-1)] = hFactor * buffer[index(line,_nbCols-2)];
    }
    for (unsigned int col=1 ; col < _nbCols-1 ; ++col) {
        buffer[index(0,col)] = vFactor * buffer[index(1,col)];
        buffer[index(_nbLines-1,col)] = vFactor* buffer[index(_nbLines-2,col)];
    }
    
    buffer[index(0,0)] = 0.5f * (buffer[index(1,0)] + buffer[index(0,1)]);
    buffer[index(0,_nbCols-1)] = 0.5f * (buffer[index(1,_nbCols-1)] + buffer[index(0,_nbCols-2)]);
    buffer[index(_nbLines-1,0)] = 0.5f * (buffer[index(_nbLines-2,0)] + buffer[index(_nbLines-1,1)]);
    buffer[index(_nbLines-1,_nbCols-1)] = 0.5f * (buffer[index(_nbLines-2,_nbCols-1)] + buffer[index(_nbLines-1,_nbCols-2)]);
}

void FluidCPU::densityBoundaryConditions(BufferFloat& densities)
{
    boundaryConditions(densities, 1.f, 1.f);
}

void FluidCPU::velXBoundaryConditions(BufferFloat& velX)
{
    boundaryConditions(velX, -1.f, 1.f);
}

void FluidCPU::velYBoundaryConditions(BufferFloat& velY)
{
    boundaryConditions(velY, 1.f, -1.f);
}

void FluidCPU::fetchDensityBuffer()
{
    BufferFloat const& density = _densities[_currDensity];
    
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _densBufferID));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, _nbCols*_nbLines*sizeof(float), density.data(), GL_DYNAMIC_DRAW));
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void FluidCPU::fetchVelocityBuffer()
{
    BufferFloat const& velX = _velX[_currVel];
    BufferFloat const& velY = _velY[_currVel];
    
    std::vector<glm::vec2> vel(2*_nbLines*_nbCols);
    std::size_t i=0;
    for (unsigned int col = 0 ; col < _nbCols ; ++col) {
        for (unsigned int line = 0 ; line < _nbLines ; ++line) {
            glm::vec2 p(static_cast<float>(line) / static_cast<float>(_nbLines),
                        static_cast<float>(col) / static_cast<float>(_nbCols));
            p = p*2.f - glm::vec2(1.f);
            vel[2*i] = p;
            vel[2*i+1] = p + 0.005f*glm::vec2(velY[index(line,col)], velX[index(line,col)]);
            ++i;
        }
    }
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _velBufferID));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, 2*_nbCols*_nbLines*sizeof(glm::vec2), vel.data(), GL_DYNAMIC_DRAW));
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
