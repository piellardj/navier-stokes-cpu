#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/OpenGL.hpp>

#include <fstream>

#include "Fluid.hpp"

void LoadSettings ( int &N, int &cell_size, bool &smooth, bool &draw_speed, float &visc, bool &force );

int main()
{
    int N, cell_size;
    bool draw_speed, smooth, force;
    float visc;

    LoadSettings ( N, cell_size, smooth, draw_speed, visc, force );

    sf::Window app;

    app.create ( sf::VideoMode(N*cell_size*(draw_speed+1), N*cell_size),
                 "Stable Fluid",
                 sf::Style::Close );

    glDisable (GL_POINT_SMOOTH);
    gluOrtho2D (0, N*cell_size*(draw_speed+1),
                0, N*cell_size);

    Fluid *fluid = new Fluid( N, visc, force);

    sf::Event event;
    sf::Clock clock;
    sf::Vector2i mouse = sf::Mouse::getPosition (app)/7;

    do
    {
        while (app.pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                    app.close();
                    break;
                case sf::Event::MouseMoved:
                    if ( sf::Mouse::isButtonPressed(sf::Mouse::Right) )
                    {
                        sf::Vector2f vel (sf::Mouse::getPosition(app).x-mouse.x,
                                          - (sf::Mouse::getPosition(app).y-mouse.y));

                        fluid->AddForce (mouse.x/cell_size, N - mouse.y/cell_size,
                                        vel.x/150.f, vel.y/150.f);
                    }
                    break;
                case sf::Event::KeyPressed:
                    if ( event.key.code == sf::Keyboard::R )
                    {
                        LoadSettings ( N, cell_size, smooth, draw_speed, visc, force );
                        app.create ( sf::VideoMode(N*cell_size*(draw_speed+1), N*cell_size),
                                     "Stable Fluid",
                                     sf::Style::Close );

                        glDisable (GL_POINT_SMOOTH);
                        gluOrtho2D (0, N*cell_size*(draw_speed+1),
                                    0, N*cell_size);

                        delete fluid;
                        fluid = new Fluid( N, visc, force);
                    }
                default:
                    break;
            }
            mouse = sf::Mouse::getPosition (app);
    }

        if ( sf::Mouse::isButtonPressed(sf::Mouse::Left) )
            fluid->AddDensity ( mouse.x/cell_size, N - mouse.y/cell_size );

        fluid->UpdateVelocity ();
        fluid->UpdateDensity ();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            fluid->DrawDensity (0, 0, cell_size, smooth);
            if ( draw_speed )
                fluid->DrawVelocity (cell_size*N, 0, cell_size);
        app.display();

        //std::cout << 1.f/clock.restart().asSeconds() << std::endl;
    } while ( app.isOpen() );

    delete fluid;

    return 0;
}

void LoadSettings ( int &N, int &cell_size, bool &smooth, bool &draw_speed, float &visc, bool &force)
{
    std::ifstream file ( "lois.txt", std::ios::in );

    N = 100;
    cell_size = 5;
    smooth = true;
    draw_speed = false;
    visc = 0.000001;
    force = false;

    if ( file )
    {
        file.ignore(100000, ':');
        file >> N;
        file.ignore(100000, ':');
        file >> cell_size;
        file.ignore(100000, ':');
        file >> smooth;
        file.ignore(100000, ':');
        file >> draw_speed;
        file.ignore(100000, ':');
        file >> visc;
        file.ignore(100000, ':');
        file >> force;
    }
}
