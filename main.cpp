#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>

enum class GameState { MENU, GAME, GAME_OVER };

const int boardWidth = 10;
const int boardHeight = 20;
const int cellSize = 30; // px

int board[boardHeight][boardWidth] = {0};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Tetris!");

    GameState gameState = GameState::MENU;

    // Proste fonty/napisy (placeholder)
    sf::Font font;
    font.loadFromFile("arial.ttf"); 

    sf::Text menuText("TETRIS\n[Enter] START", font, 48);
    menuText.setPosition(200, 200);

    sf::Text gameText("Tetris Gra - [Esc] = Game Over", font, 32);
    gameText.setPosition(130, 250);

    sf::Text gameOverText("GAME OVER\n[Enter] Menu\n[R] Restart", font, 48);
    gameOverText.setPosition(100, 200);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // Obsługa przełączania stanów (tylko do testów)
            if (event.type == sf::Event::KeyPressed)
            {
                if (gameState == GameState::MENU)
                {
                    if (event.key.code == sf::Keyboard::Enter)
                        gameState = GameState::GAME;
                }
                else if (gameState == GameState::GAME)
                {
                    if (event.key.code == sf::Keyboard::Escape)
                        gameState = GameState::GAME_OVER;
                }
                else if (gameState == GameState::GAME_OVER)
                {
                    if (event.key.code == sf::Keyboard::Enter)
                        gameState = GameState::MENU;
                    else if (event.key.code == sf::Keyboard::R)
                        gameState = GameState::GAME;
                }
            }
        }

        window.clear();

        switch (gameState)
        {
            case GameState::MENU:
            {
                window.draw(menuText);
                break;
            }
            case GameState::GAME:
            {
                int gridWidth = boardWidth * cellSize;
                int gridHeight = boardHeight * cellSize;
                int offsetX = (window.getSize().x - gridWidth) / 2;
                int offsetY = (window.getSize().y - gridHeight) / 2;
                
                for (int y = 0; y < boardHeight; ++y) // Rysowanie planszy
                {
                    for (int x = 0; x < boardWidth; ++x) 
                    {
                        sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                        cell.setPosition(offsetX + x * cellSize, offsetY + y * cellSize);
                        cell.setFillColor(board[y][x] ? sf::Color::Red : sf::Color(30,30,30));
                        window.draw(cell);
                    }   
                }
                break;      
            } 
            case GameState::GAME_OVER:
            {
                window.draw(gameOverText);
                break;
            }
        }
        window.display();
    }
    return 0;
}