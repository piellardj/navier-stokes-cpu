#ifndef FLUID_HPP_INCLUDED
#define FLUID_HPP_INCLUDED

#include <vector>
#include <SFML/System/Vector2.hpp>

class Fluid
{
    private:
        int _N;

        float _visc;//vitesse de diffusion, entre 0 et 1/4

        std::vector<float>        *_dens, *_dens_prev,
                                  *_velx, *_velx_prev,
                                  *_vely, *_vely_prev;

        std::vector<float>        _sources;
        std::vector<sf::Vector2f> _forces;

    public:
        Fluid ( int N, float visc, bool );
        ~Fluid ();

        void DrawDensity ( int left, int bottom, int cell_size, bool smooth) const;
        void DrawVelocity ( int left, int bottom,float cell_size) const;

        void UpdateDensity ();
        void UpdateVelocity ();

        void AddDensity ( int x, int y );
        void AddForce( int x, int y, float velx, float vely );

    private:
        int Index ( int x, int y ) const;

        void AddSources ();
        void AddForces ();
        void Diffuse ( std::vector<float> &next, std::vector<float> &prev, int b );
        void Advect ( std::vector<float> &next, std::vector<float> &prev, std::vector<float> &velx, std::vector<float> &vely, int b);

        void Project ();

        void SetBnd ( std::vector<float> &field, int b );
};

#endif // FLUID_HPP_INCLUDED
