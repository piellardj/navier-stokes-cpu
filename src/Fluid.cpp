#include "Fluid.hpp"

#include <SFML/OpenGL.hpp>

using namespace std;
using namespace sf;

Fluid::Fluid ( int N, float visc, bool force):
            _N(N),
            _visc (visc),
            _sources ((_N+2)*(_N+2), 0.f),
            _forces ((_N+2)*(_N+2), Vector2f(0.f,0.f))
{
    _dens = new vector<float> ((_N+2)*(_N+2), 0.f);
    for ( int x = N/2-20 ; x <= N/2+20 ; x++ )
        for ( int y = N/2-20 ; y <= N/2+20 ; y++ )
            (*_dens)[Index(x,y)] = 1.f;
    _dens_prev = new vector<float> (*_dens);

    _velx = new vector<float> ((_N+2)*(_N+2), 0.f);
    _velx_prev=  new vector<float> (*_velx);

    _vely = new vector<float> ((_N+2)*(_N+2), 0.f);
    _vely_prev = new vector<float> (*_vely);

    if ( force )
    {
        _forces[Index(5,N/2-1)] = Vector2f ( 0.01f, 0.f );
        _forces[Index(5,N/2+0)] = Vector2f ( 0.02f, 0.f );
        _forces[Index(5,N/2+1)] = Vector2f ( 0.01f, 0.f );
    }
}

Fluid::~Fluid ()
{
    delete _dens;
    delete _dens_prev;
    delete _velx;
    delete _velx_prev;
    delete _vely;
    delete _vely_prev;
}

void Fluid::DrawDensity ( int left, int bottom, int cell_size, bool smooth) const
{
    if ( !smooth )
    {
        glPointSize ( cell_size);
        glBegin ( GL_POINTS );
        for ( int iY = 1 ; iY <= _N ; iY++ )
            for ( int iX = 1 ; iX <= _N ; iX++ )
            {
                glColor3f ( 2.f*(*_dens)[Index(iX,iY)], 2.f*(*_dens)[Index(iX,iY)]-1.f, 0 );
                glVertex2d ( left + cell_size*iX, bottom + cell_size*iY );
            }
        glEnd ();
    }
    else
    {
        glBegin ( GL_QUADS );
        for ( int iY = 1 ; iY <= _N ; iY++ )
            for ( int iX = 1 ; iX <= _N ; iX++ )
            {
                glColor3f ( 2.f*(*_dens)[Index(iX,iY)], 2.f*(*_dens)[Index(iX,iY)]-1.f, 0 );
                glVertex2d ( left + cell_size*iX, bottom + cell_size*iY );

                glColor3f ( 2.f*(*_dens)[Index(iX-1,iY)], 2.f*(*_dens)[Index(iX-1,iY)]-1.f, 0 );
                glVertex2d ( left + cell_size*(iX-1), bottom + cell_size*iY );

                glColor3f ( 2.f*(*_dens)[Index(iX-1,iY-1)], 2.f*(*_dens)[Index(iX-1,iY-1)]-1.f, 0 );
                glVertex2d ( left + cell_size*(iX-1), bottom + cell_size*(iY-1) );

                glColor3f ( 2.f*(*_dens)[Index(iX,iY-1)], 2.f*(*_dens)[Index(iX,iY-1)]-1.f, 0 );
                glVertex2d ( left + cell_size*iX, bottom + cell_size*(iY-1) );
            }
        glEnd ();
    }
}
void Fluid::DrawVelocity ( int left, int bottom, float cell_size ) const
{
    glLineWidth ( 1.f );
    glBegin ( GL_LINES );
    glColor3f ( 1.f, 1.f, 1.f );
    for ( int iY = 1 ; iY <= _N ; iY++ )
        for ( int iX = 1 ; iX <= _N ; iX++ )
        {
            Vector2f A(iX,iY);
            glVertex2d ( left + A.x*cell_size, bottom + A.y*cell_size );
            A.x += (*_velx)[Index(iX,iY)]*100.f;
            A.y += (*_vely)[Index(iX,iY)]*100.f;
            glVertex2d ( left + A.x*cell_size, bottom + A.y*cell_size );
        }
    glEnd ();
}

void Fluid::UpdateDensity ()
{
    AddSources ();
    swap (_dens, _dens_prev);
    Diffuse ( *_dens, *_dens_prev, 0 );
    swap (_dens, _dens_prev);
    Advect ( *_dens, *_dens_prev, *_velx, *_vely, 0 );
}
void Fluid::UpdateVelocity ()
{
    AddForces ();
    swap (_velx, _velx_prev);
    Diffuse  ( *_velx, *_velx_prev, 1 );
    swap (_vely, _vely_prev);
    Diffuse  ( *_vely, *_vely_prev, 2 );

    Project ();

    swap (_velx, _velx_prev);
    swap (_vely, _vely_prev);
    Advect (*_velx, *_velx_prev, *_velx_prev, *_vely_prev, 1);
    Advect (*_vely, *_vely_prev, *_velx_prev, *_vely_prev, 2);

    Project ();
}

void Fluid::AddDensity ( int x, int y )
{
    if ( x >= 1 && x <= _N && 1 >= 0 && y <= _N)
    {
        (*_dens)[Index(x,y)] = 1.f;
        (*_dens)[Index(x-1,y)] = 1.f;
        (*_dens)[Index(x-1,y-1)] = 1.f;
        (*_dens)[Index(x,y-1)] = 1.f;
    }
}
void Fluid::AddForce ( int x, int y, float velx, float vely )
{
    float k=0.5;

    if ( x >= 0 && x < _N && y >= 0 && y < _N)
    {
        (*_velx)[Index(x,y)] += velx; (*_vely)[Index(x,y)] += vely;

        (*_velx)[Index(x-1,y-1)] += velx*k; (*_vely)[Index(x-1,y-1)] += vely*k;
        (*_velx)[Index(x+0,y-1)] += velx*k; (*_vely)[Index(x+0,y-1)] += vely*k;
        (*_velx)[Index(x+1,y-1)] += velx*k; (*_vely)[Index(x+1,y-1)] += vely*k;

        (*_velx)[Index(x-1,y+1)] += velx*k; (*_vely)[Index(x-1,y+1)] += vely*k;
        (*_velx)[Index(x+0,y+1)] += velx*k; (*_vely)[Index(x+0,y+1)] += vely*k;
        (*_velx)[Index(x+1,y+1)] += velx*k; (*_vely)[Index(x+1,y+1)] += vely*k;

        (*_velx)[Index(x-1,y)] += velx*k; (*_vely)[Index(x-1,y)] += vely*k;
        (*_velx)[Index(x+1,y)] += velx*k; (*_vely)[Index(x+1,y)] += vely*k;
    }
}

inline int Fluid::Index ( int x, int y ) const
{
    return y*(_N+2) + x;
}

void Fluid::AddSources ()
{
    for ( int iC = _sources.size()-1 ; iC >= 0 ; iC-- )
    {
        (*_dens)[iC] += _sources[iC];

        if ( (*_dens)[iC] > 1.f )
            (*_dens)[iC] = 1.f;
        else if ( (*_dens)[iC] < 0.f )
            (*_dens)[iC] = 0.f;
    }
}

void Fluid::AddForces ()
{
    for ( int iC = _sources.size()-1 ; iC >= 0 ; iC-- )
    {
        (*_velx)[iC] += _forces[iC].x;
        (*_vely)[iC] += _forces[iC].y;
    }
}

void Fluid::Diffuse ( std::vector<float> &next, std::vector<float> &prev, int b )
{
    for ( int k=0 ; k<20 ; k++ )
    {
        for ( int iY = 1 ; iY <= _N ; iY++ )
            for ( int iX = 1 ; iX <= _N ; iX++ )
                next[Index(iX,iY)] = (prev[Index(iX,iY)] + _visc*(prev[Index(iX+1,iY)]+prev[Index(iX-1,iY)]+prev[Index(iX,iY+1)]+prev[Index(iX,iY-1)]))/(1.f+4.f*_visc);

        SetBnd ( next, b );
    }
}

void Fluid::Advect ( std::vector<float> &next, std::vector<float> &prev, std::vector<float> &velx, std::vector<float> &vely, int b )
{
    float a = 1.f * static_cast<float>(_N);

    for ( int iY = 1 ; iY <= _N ; iY++ )
        for ( int iX = 1 ; iX <= _N ; iX++ )
        {
            float x = static_cast<float>(iX) - a*velx[Index(iX,iY)],
                  y = static_cast<float>(iY) - a*vely[Index(iX,iY)];

            if ( x < 0.5 )
                x = 0.5;
            if ( x > static_cast<float>(_N)+0.5 )
                x = static_cast<float>(_N)+0.5;

            if ( y < 0.5 )
                y = 0.5;
            if ( y > static_cast<float>(_N)+0.5 )
                y = static_cast<float>(_N)+0.5;

            int x0 = x, y0 = y, x1 = x0+1, y1 = y0+1;
            float s1 = x - static_cast<float>(x0), s0 = 1.f-s1,
                  t1 = y - static_cast<float>(y0), t0 = 1.f-t1;

            next[Index(iX,iY)] = s0*( t0*prev[Index(x0,y0)] + t1*prev[Index(x0,y1)] )
                               + s1*( t0*prev[Index(x1,y0)] + t1*prev[Index(x1,y1)] );
        }

    SetBnd ( next, b);
}

void Fluid::Project ()
{
    for ( int iY = 1 ; iY <= _N ; iY++ )
        for ( int iX = 1 ; iX <= _N ; iX++ )
        {
            (*_vely_prev)[Index(iX,iY)] = -0.5*( (*_velx)[Index(iX+1,iY)] - (*_velx)[Index(iX-1,iY)] + (*_vely)[Index(iX,iY+1)] - (*_vely)[Index(iX,iY-1)]);
            (*_velx_prev)[Index(iX,iY)] = 0.f;
        }

    SetBnd  ( *_vely_prev, 0);
    SetBnd  ( *_velx_prev, 0);

    for ( int k=0 ; k<20 ; k++ )
    {
        for ( int iY = 1 ; iY <= _N ; iY++ )
            for ( int iX = 1 ; iX < _N ; iX++ )
                (*_velx_prev)[Index(iX,iY)] = ( (*_vely_prev)[Index(iX,iY)] + (*_velx_prev)[Index(iX-1,iY)]+(*_velx_prev)[Index(iX+1,iY)]+(*_velx_prev)[Index(iX,iY-1)]+(*_velx_prev)[Index(iX,iY+1)] )/4.f;
        SetBnd  ( *_velx_prev, 0 );
    }

    for ( int iY = 1 ; iY < _N ; iY++ )
        for ( int iX = 1 ; iX < _N ; iX++ )
        {
            (*_velx)[Index(iX,iY)] -= 0.5 * ( (*_velx_prev)[Index(iX+1,iY)]-(*_velx_prev)[Index(iX-1,iY)] );
            (*_vely)[Index(iX,iY)] -= 0.5 * ( (*_velx_prev)[Index(iX,iY+1)]-(*_velx_prev)[Index(iX,iY-1)] );
        }

    SetBnd  ( *_velx, 1 );
    SetBnd  ( *_vely, 2 );
}

void Fluid::SetBnd ( std::vector<float> &field, int b )
{
    for ( int i=1 ; i <= _N ; i++ )
    {
        field[Index(0   ,i)] = (b==1) ? -field[Index(1,i)] : field[Index(1,i)];
        field[Index(_N+1,i)] = (b==1) ? -field[Index(_N,i)] : field[Index(_N,i)];
        field[Index(i,0   )] = (b==2) ? -field[Index(i,1)] : field[Index(i,1)];
        field[Index(i,_N+1)]  = (b==2) ? -field[Index(i,_N)] : field[Index(i,_N)];
    }

    field[Index(0,0)] = (field[Index(1,0)]+field[Index(0,1)])/2.f;
    field[Index(0,_N+1)] = (field[Index(1,_N+1)]+field[Index(0,_N)])/2.f;
    field[Index(_N+1,0)] = (field[Index(_N,0)]+field[Index(_N+1,1)])/2.f;
    field[Index(_N+1,_N+1)] = (field[Index(_N,_N+1)]+field[Index(_N+1,_N)])/2.f;
}
