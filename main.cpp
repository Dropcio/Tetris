#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>

enum class GameState { MENU, GAME, GAME_OVER };

const int boardWidth = 10;
const int boardHeight = 20;
const int cellSize = 30; // px

int score = 0;
int linesClearedTotal = 0;
int level = 1;

int board[boardHeight][boardWidth] = {0};

struct Piece {
    int x, y;
    int shape[2][2] = { {1,1}, {1,1} }; // Klocek O
};

Piece currentPiece = {4, 0}; // Start na środku

// Funkcja kolizji w dół
bool canMoveDown(const Piece& piece) {
    for (int py = 0; py < 2; ++py) {
        for (int px = 0; px < 2; ++px) {
            if (piece.shape[py][px]) {
                int nx = piece.x + px;
                int ny = piece.y + py + 1;
                // Sprawdzamy czy poza planszę
                if (ny >= boardHeight) return false;
                // Sprawdzamy kolizję z innym klockiem
                if (board[ny][nx]) return false;
            }
        }
    }
    return true;
}

int clearLines() //Czyszczenie linii
{
    int linesCleared = 0;
    for (int y = boardHeight - 1; y >= 0; --y) 
    {
        bool full = true;
        for (int x = 0; x < boardWidth; ++x) 
        {
            if (board[y][x] == 0) 
            {
                full = false;
                break;
            }
        }
        if (full) 
        {
            // Przesuwamy wszystkie wiersze powyżej w dół
            for (int ty = y; ty > 0; --ty) 
            {
                for (int x = 0; x < boardWidth; ++x) 
                {
                    board[ty][x] = board[ty - 1][x];
                }
            }
            // Zerujemy górny wiersz
            for (int x = 0; x < boardWidth; ++x) 
            {
                board[0][x] = 0;
            }
            // Sprawdzamy ten sam wiersz jeszcze raz, bo spadła nowa linia
            y++;
            linesCleared++;
        }
    }
    return linesCleared;
}

void setPieceXToMouse(sf::RenderWindow& window, Piece& piece)
{
    int mx = sf::Mouse::getPosition(window).x;
    int gridWidth = boardWidth * cellSize;
    int offsetX = (window.getSize().x - gridWidth) / 2;

    int newX = (mx - offsetX) / cellSize;
    if (newX < 0) newX = 0;
    if (newX > boardWidth - 2) newX = boardWidth - 2;

    // Kolizja na X
    bool canMove = true;
    for (int py = 0; py < 2; ++py)
        for (int px = 0; px < 2; ++px)
            if (piece.shape[py][px])
            {
                int nx = newX + px;
                int ny = piece.y + py;
                if (nx < 0 || nx >= boardWidth || board[ny][nx])
                    canMove = false;
            }
    if (canMove)
        piece.x = newX;
    else
        piece.x = 4; // fallback na środek (jeśli się nie da)
}

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

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(32);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(50, 20);

    sf::Text linesText, levelText;
    linesText.setFont(font);
    linesText.setCharacterSize(32);
    linesText.setFillColor(sf::Color::White);
    linesText.setPosition(50, 60);
    
    levelText.setFont(font);
    levelText.setCharacterSize(32);
    levelText.setFillColor(sf::Color::White);
    levelText.setPosition(50, 100);

    sf::Text summaryText;  
    summaryText.setFont(font);
    summaryText.setCharacterSize(32);
    summaryText.setFillColor(sf::Color::White);
    summaryText.setPosition(100, 350);

    sf::Clock fallClock;
    float fallDelay = 0.5f;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) 
            {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed)
            {
                if (gameState == GameState::MENU)
                {
                    if (event.key.code == sf::Keyboard::Enter) 
                    {
                        gameState = GameState::GAME;
                    }
                }
                else if (gameState == GameState::GAME)
                {
                    if (event.key.code == sf::Keyboard::Escape) 
                    {
                        gameState = GameState::GAME_OVER;
                    }
                    // Sterowanie klockiem
                    if (event.key.code == sf::Keyboard::Left) 
                    {
                        // Ruch w lewo z kolizją
                        bool canMove = true;
                        for (int py = 0; py < 2; ++py) 
                        {
                            for (int px = 0; px < 2; ++px) 
                            {
                                if (currentPiece.shape[py][px]) 
                                {
                                    int nx = currentPiece.x + px - 1;
                                    int ny = currentPiece.y + py;
                                    if (nx < 0 || board[ny][nx]) 
                                    {
                                        canMove = false;
                                    }
                                }
                            }
                        }
                        if (canMove) 
                        {
                            currentPiece.x--;
                        }
                    }
                    else if (event.key.code == sf::Keyboard::Right) 
                    {
                        // Ruch w prawo z kolizją
                        bool canMove = true;
                        for (int py = 0; py < 2; ++py) 
                        {
                            for (int px = 0; px < 2; ++px) 
                            {
                                if (currentPiece.shape[py][px]) 
                                {
                                    int nx = currentPiece.x + px + 1;
                                    int ny = currentPiece.y + py;
                                    if (nx >= boardWidth || board[ny][nx]) 
                                    {
                                        canMove = false;
                                    }
                                }
                            }
                        }
                        if (canMove) {
                            currentPiece.x++;
                        }
                    }
                    else if (event.key.code == sf::Keyboard::Down) 
                    {
                        // Opuszczanie klocka w dół z kolizją - soft drop
                        if (canMoveDown(currentPiece)) 
                        {
                            currentPiece.y++;
                            score += 1;
                        }
                    }
                    else if (event.key.code == sf::Keyboard::Space) 
                    {
                        // Hard drop
                        int startY = currentPiece.y;
                        while (canMoveDown(currentPiece)) 
                        {
                            currentPiece.y++;
                        }
                        score += 2 * (currentPiece.y - startY);

                        // "Wklej" klocek na planszę
                        for (int py = 0; py < 2; ++py) 
                        {
                            for (int px = 0; px < 2; ++px) 
                            {
                                if (currentPiece.shape[py][px]) 
                                {
                                    board[currentPiece.y + py][currentPiece.x + px] = 1;
                                }
                            }
                        }

                        int lines = clearLines();
                        if (lines == 1) score += 100;
                        else if (lines == 2) score += 300;
                        else if (lines == 3) score += 500;
                        else if (lines == 4) score += 800;
                        linesClearedTotal += lines;
                        level = 1 + (linesClearedTotal / 10);
                        fallDelay = std::max(0.1f, 0.5f - 0.05f * (level - 1)); // im wyższy level, tym szybciej, min. 0.1s

                        // Nowy klocek na górze
                        currentPiece = {4, 0};
                        setPieceXToMouse(window, currentPiece);

                        // Wykrywanie końca gry
                        for (int px = 0; px < 2; ++px) 
                        {
                            if (board[0][currentPiece.x + px]) 
                            {
                                gameState = GameState::GAME_OVER;
                            }
                        }
                    }
                }
                else if (gameState == GameState::GAME_OVER)
                {
                    if (event.key.code == sf::Keyboard::Enter) 
                    {
                        gameState = GameState::MENU;
                    }
                    else if (event.key.code == sf::Keyboard::R) 
                    {
                        gameState = GameState::GAME;
                        // Resetuj planszę i klocek
                        for (int y = 0; y < boardHeight; y++) 
                        {
                            for (int x = 0; x < boardWidth; x++) 
                            {
                                board[y][x] = 0;
                            }
                        }
                        currentPiece = {4, 0};
                        setPieceXToMouse(window, currentPiece);
                    }
                }
            }
            if (event.type == sf::Event::MouseMoved && gameState == GameState::GAME)
            {
                int mx = event.mouseMove.x;
                int gridWidth = boardWidth * cellSize;
                int offsetX = (window.getSize().x - gridWidth) / 2;

                // Tylko jeśli mysz jest nad planszą (opcjonalnie, można usunąć ifa)
                if (mx >= offsetX && mx < offsetX + gridWidth)
                {
                    int newX = (mx - offsetX) / cellSize;
                    // Ogranicz pozycję dla klocka O (2x2)
                    if (newX < 0) newX = 0;
                    if (newX > boardWidth - 2) newX = boardWidth - 2;

                    // Sprawdź kolizje na X przed przesunięciem (dla O wystarczy sprawdzić całą szerokość)
                    bool canMove = true;
                    for (int py = 0; py < 2; ++py)
                        for (int px = 0; px < 2; ++px)
                            if (currentPiece.shape[py][px])
                            {
                                int nx = newX + px;
                                int ny = currentPiece.y + py;
                                if (nx < 0 || nx >= boardWidth || board[ny][nx])
                                    canMove = false;
                            }
                    if (canMove)
                        currentPiece.x = newX;
                }
            }
            if (event.type == sf::Event::MouseButtonPressed && gameState == GameState::GAME)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    // HARD DROP (identycznie jak spacja)
                    int startY = currentPiece.y;
                    while (canMoveDown(currentPiece)) 
                    {
                        currentPiece.y++;
                    }
                    score += 2 * (currentPiece.y - startY);

                    // "Wklej" klocek na planszę
                    for (int py = 0; py < 2; ++py) 
                    {
                        for (int px = 0; px < 2; ++px) 
                        {
                            if (currentPiece.shape[py][px]) 
                            {
                                board[currentPiece.y + py][currentPiece.x + px] = 1;
                            }
                        }
                    }

                    int lines = clearLines();
                    if (lines == 1) score += 100;
                    else if (lines == 2) score += 300;
                    else if (lines == 3) score += 500;
                    else if (lines == 4) score += 800;
                    linesClearedTotal += lines;
                    level = 1 + (linesClearedTotal / 10);
                    fallDelay = std::max(0.1f, 0.5f - 0.05f * (level - 1));
                    
                    // Nowy klocek na górze pod myszką
                    currentPiece = {4, 0};
                    setPieceXToMouse(window, currentPiece);

                    for (int px = 0; px < 2; ++px) 
                    {
                        if (board[0][currentPiece.x + px]) 
                        {
                            gameState = GameState::GAME_OVER;
                        }
                    }
                }
            }

        }

        // Automatyczne opadanie klocka
        if (gameState == GameState::GAME && fallClock.getElapsedTime().asSeconds() > fallDelay)
        {
            if (canMoveDown(currentPiece)) {
                currentPiece.y++;
            } else {
                // "Wklej" klocek na planszę
                for (int py = 0; py < 2; ++py)
                {
                    for (int px = 0; px < 2; ++px) 
                    {
                        if (currentPiece.shape[py][px]) 
                        {
                            board[currentPiece.y + py][currentPiece.x + px] = 1;
                        }
                    }
                }

                int lines = clearLines();
                if (lines == 1) score += 100;
                else if (lines == 2) score += 300;
                else if (lines == 3) score += 500;
                else if (lines == 4) score += 800;
                linesClearedTotal += lines;
                level = 1 + (linesClearedTotal / 10);
                fallDelay = std::max(0.1f, 0.5f - 0.05f * (level - 1)); // im wyższy level, tym szybciej, min. 0.1s

                // Nowy klocek na górze
                currentPiece = {4, 0};
                setPieceXToMouse(window, currentPiece);

                // Wykrywanie końca gry
                for (int px = 0; px < 2; ++px) 
                {
                    if (board[0][currentPiece.x + px]) 
                    {
                        gameState = GameState::GAME_OVER;
                    }
                }
            }
            fallClock.restart();
        }
        scoreText.setString("SCORE: " + std::to_string(score));
        linesText.setString("LINES: " + std::to_string(linesClearedTotal));
        levelText.setString("LEVEL: " + std::to_string(level));
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
                Piece ghostPiece = currentPiece;
                while (canMoveDown(ghostPiece)) {
                    ghostPiece.y++;
                }
                for (int py = 0; py < 2; ++py)
                {
                    for (int px = 0; px < 2; ++px)
                    {
                        if (ghostPiece.shape[py][px])
                        {
                            sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                            cell.setPosition(offsetX + (ghostPiece.x + px) * cellSize, offsetY + (ghostPiece.y + py) * cellSize);
                            cell.setFillColor(sf::Color(255, 255, 255, 90)); // przezroczysty biały
                            window.draw(cell);
                        }
                    }
                }
                window.draw(scoreText);
                window.draw(linesText);
                window.draw(levelText);
                break;
            }
            case GameState::GAME_OVER:
            {
                summaryText.setString(
                    "SCORE: " + std::to_string(score) +
                    "\nLINES: " + std::to_string(linesClearedTotal) +
                    "\nLEVEL: " + std::to_string(level)
                );
                window.draw(gameOverText);
                window.draw(summaryText);
                break;
            }
        }
        window.display();
    }
    return 0;
}
