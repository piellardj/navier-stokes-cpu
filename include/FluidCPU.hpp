#ifndef FLUIDCPU_HPP_INCLUDED
#define FLUIDCPU_HPP_INCLUDED

#include "Fluid.hpp"

#include <array>
#include <vector>
#include <string>
#include <functional>


typedef std::vector<float> BufferFloat;
typedef std::vector<bool> BufferBool;

class FluidCPU: public Fluid
{
    public:
        FluidCPU (unsigned int nbCols, unsigned int nbLines, float viscosity);

        virtual void reset();

        /* position normalisée */
        virtual void addDensity (sf::Vector2f pos, float radius, float strength=0.1f);
        virtual void addVelocity (sf::Vector2f pos, sf::Vector2f dir);

        virtual void update(float dt);

    protected:
        virtual void fetchDensityBuffer();
        virtual void fetchVelocityBuffer();

    private:
        inline unsigned int index(unsigned int line, unsigned int col)
        {
            return line*_nbCols + col;
        }

        void solveDensity(float dt);
        void solveVelocity(float dt);

        void diffuse(BufferFloat const& src, BufferFloat& dst,
                     std::function<void(FluidCPU&, BufferFloat&)> boundaryConditions, float dt);

        void advect(BufferFloat const& src, BufferFloat& dst, BufferFloat const& velX, BufferFloat const& velY, float dt);

        /* Makes the vector field (velX, velY) an incompressible field */
        void project(BufferFloat& velX, BufferFloat& velY, BufferFloat& p, BufferFloat& div);

        /* Makes sure boundary conditions are respected. */
        void boundaryConditions (BufferFloat& buffer, float hFactor, float vFactor);
        void densityBoundaryConditions (BufferFloat& densities);
        void velXBoundaryConditions (BufferFloat& velX);
        void velYBoundaryConditions (BufferFloat& velY);


    private:
        unsigned int _currDensity; //0 or 1
        std::array<BufferFloat, 2> _densities;

        unsigned int _currVel; //0 or 1
        std::array<BufferFloat, 2> _velX;
        std::array<BufferFloat, 2> _velY;
};

#endif // FLUIDCPU_HPP_INCLUDED
