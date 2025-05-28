#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>

enum class GameState { MENU, GAME, GAME_OVER };

const int boardWidth = 10;
const int boardHeight = 20;
const int cellSize = 30; // px

int board[boardHeight][boardWidth] = {0};

struct Piece {
    int x, y;
    int shape[2][2] = { {1,1}, {1,1} }; // Klocek O
};
Piece currentPiece = {4, 0}; // start na środku

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Tetris!");

    GameState gameState = GameState::MENU;

    sf::Font font;
    font.loadFromFile("arial.ttf"); 

    sf::Text menuText("TETRIS\n[Enter] START", font, 48);
    menuText.setPosition(200, 200);

    sf::Text gameText("Tetris Gra - [Esc] = Game Over", font, 32);
    gameText.setPosition(130, 250);

    sf::Text gameOverText("GAME OVER\n[Enter] Menu\n[R] Restart", font, 48);
    gameOverText.setPosition(100, 200);

    sf::Clock fallClock;
    float fallDelay = 0.5f;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed)
            {
                if (gameState == GameState::MENU)
                {
                    if (event.key.code == sf::Keyboard::Enter) {
                        gameState = GameState::GAME;
                    }
                }
                else if (gameState == GameState::GAME)
                {
                    if (event.key.code == sf::Keyboard::Escape) {
                        gameState = GameState::GAME_OVER;
                    }
                    // Sterowanie klockiem
                    if (event.key.code == sf::Keyboard::Left && currentPiece.x > 0) {
                        currentPiece.x--;
                    }
                    else if (event.key.code == sf::Keyboard::Right && currentPiece.x < boardWidth - 2) {
                        currentPiece.x++;
                    }
                    else if (event.key.code == sf::Keyboard::Down && currentPiece.y < boardHeight - 2) {
                        currentPiece.y++;
                    }
                    else if (event.key.code == sf::Keyboard::Space) {
                        while (currentPiece.y < boardHeight - 2) {
                            currentPiece.y++;
                        }
                        for (int py = 0; py < 2; ++py) {
                            for (int px = 0; px < 2; ++px) {
                                if (currentPiece.shape[py][px]) {
                                    board[currentPiece.y + py][currentPiece.x + px] = 1;
                                }
                            }
                        }
                        // Tworzymy nowy klocek na górze
                        currentPiece = {4, 0};
                    }
                }
                else if (gameState == GameState::GAME_OVER)
                {
                    if (event.key.code == sf::Keyboard::Enter) {
                        gameState = GameState::MENU;
                    }
                    else if (event.key.code == sf::Keyboard::R) {
                        gameState = GameState::GAME;
                        // Resetuj planszę i klocek
                        for (int y = 0; y < boardHeight; y++) {
                            for (int x = 0; x < boardWidth; x++) {
                                board[y][x] = 0;
                            }
                        }
                        currentPiece = {4, 0};
                    }
                }
            }
        }

        // Automatyczne opadanie klocka
        if (gameState == GameState::GAME && fallClock.getElapsedTime().asSeconds() > fallDelay)
        {
            if (currentPiece.y < boardHeight - 2) {
                currentPiece.y++;
            }
            fallClock.restart();
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

                // Rysowanie planszy
                for (int y = 0; y < boardHeight; ++y)
                {
                    for (int x = 0; x < boardWidth; ++x)
                    {
                        sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                        cell.setPosition(offsetX + x * cellSize, offsetY + y * cellSize);
                        cell.setFillColor(board[y][x] ? sf::Color::Red : sf::Color(30,30,30));
                        window.draw(cell);
                    }
                }
                // Rysowanie klocka
                for (int py = 0; py < 2; ++py)
                {
                    for (int px = 0; px < 2; ++px)
                    {
                        if (currentPiece.shape[py][px])
                        {
                            sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                            cell.setPosition(offsetX + (currentPiece.x + px) * cellSize, offsetY + (currentPiece.y + py) * cellSize);
                            cell.setFillColor(sf::Color::Yellow);
                            window.draw(cell);
                        }
                    }
                }

                // Rysowanie cienia (ghost piece)
                int ghostY = currentPiece.y;
                while (ghostY < boardHeight - 2) 
                {
                    ghostY++;
                // Tu w przyszłości możesz dodać break przy kolizji
                }
                for (int py = 0; py < 2; ++py) 
                {
                    for (int px = 0; px < 2; ++px) 
                    {
                        if (currentPiece.shape[py][px]) 
                        {
                            sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                            cell.setPosition(offsetX + (currentPiece.x + px) * cellSize, offsetY + (ghostY + py) * cellSize);
                            cell.setFillColor(sf::Color(255, 255, 255, 90)); // przezroczysty biały
                            window.draw(cell);
                        }
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
