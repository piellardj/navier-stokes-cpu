#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <GL/glew.h>

#include "FluidCPU.hpp"


/* Returns relative mouse position in the window (in [0,1]x[0,1]) */
sf::Vector2f getRelativeMousePos (sf::Window const& window);
bool isMouseInWindow (sf::Window const& window);

int main(int argc, char *argv[])
{
    /* Creation of the windows and contexts */
    sf::ContextSettings openGL2DContext(0, 0, 0, //no depth, no stencil, no antialiasing
                                        3, 0, //openGL 3.0 requested
                                        sf::ContextSettings::Default);
    sf::RenderWindow window(sf::VideoMode(512,512), "Navier-Stokes",
                            sf::Style::Titlebar | sf::Style::Close,
                            openGL2DContext);
    window.setVerticalSyncEnabled(true);
    //window.setFramerateLimit(10);
    glewInit();

    sf::Font font;
    if (!font.loadFromFile("fonts/font.ttf")) {
        std::cerr << "Warning: unable to load fonts/font.ttf." << std::endl;
    }
    sf::Text text("", font, 18);

    FluidCPU fluidCPU(100, 100, 0.0001f);
    Fluid& fluid = fluidCPU;

    /* Main loop */
    unsigned int loops = 0;
    const sf::Clock clock; //for average fps computation
    sf::Clock fpsClock;
    sf::Vector2f mousePos = getRelativeMousePos(window);
    bool drawDensity = true, drawVelocity = false;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                break;
                case sf::Event::KeyReleased:
                    if (event.key.code == sf::Keyboard::Escape) {
                        window.close();
                    }
                    else if (event.key.code == sf::Keyboard::R) {
                        fluid.reset();
                    }
                    else if (event.key.code == sf::Keyboard::I) {
                        drawDensity = !drawDensity;
                    }
                    else if (event.key.code == sf::Keyboard::O) {
                        drawVelocity = !drawVelocity;
                    }
                break;
                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        fluid.addDensity(mousePos, 0.001f, 1.f);
                    }
                break;
                case sf::Event::MouseMoved:
                {
                    sf::Vector2f newMousePos = getRelativeMousePos(window);

                    if (isMouseInWindow(window)) {
                        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                            fluid.addDensity(newMousePos, 0.001f, 1.f);
                        } else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
                            fluid.addVelocity(mousePos, newMousePos - mousePos);
                        }
                    }
                    
                    mousePos = newMousePos;
                }
                break;
                default:
                    break;
            }
        }

        float dt = fpsClock.getElapsedTime().asSeconds();
        fpsClock.restart();

        fluid.update(dt);

        {
            float fps = 1.f / fpsClock.getElapsedTime().asSeconds();
            std::stringstream s;
            s << "fps: " << static_cast<int>(fps) << std::endl <<std::endl;
            s << "size: " << fluid.getSize().x << "x" << fluid.getSize().y << std::endl;
            s << "draw density (I): " << drawDensity << std::endl;
            s << "draw velocity (O): " << drawVelocity;
            
            text.setString(s.str());
        }
        
        window.clear();
        fluid.draw(drawDensity, drawVelocity);

        window.pushGLStates();
        window.draw(text);
        window.popGLStates();

        window.display();

        ++loops;

    }

    std::cout << "average fps: " << static_cast<float>(loops) / clock.getElapsedTime().asSeconds() << std::endl;

    return EXIT_SUCCESS;
}

sf::Vector2f getRelativeMousePos (sf::Window const& window)
{
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f pos(mousePos.x, window.getSize().y - mousePos.y);
    pos.x /= window.getSize().x;
    pos.y /= window.getSize().y;
    return pos;
}

bool isMouseInWindow (sf::Window const& window)
{
    sf::Vector2i pos = sf::Mouse::getPosition(window);
    return pos.x >= 0 && pos.y >= 0 && pos.x < (int)window.getSize().x && pos.y < (int)window.getSize().y;
}
