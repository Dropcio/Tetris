#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include "piece.hpp"

enum class GameState { MENU, GAME, GAME_OVER };

enum class GameMode { NONE, NORMAL, ADVANCED, EASY, HARD };
GameMode gameMode = GameMode::NONE;

const int boardWidth = 10;
const int boardHeight = 20;
const int cellSize = 30; // px

int score = 0;
int linesClearedTotal = 0;
int level = 1;

int board[boardHeight][boardWidth] = {0};

Piece currentPiece(randomTetromino());

// Funkcja kolizji w dół
bool canMoveDown(const Piece& piece) 
{
    for (int py = 0; py < 4; ++py) 
    {
        for (int px = 0; px < 4; ++px) 
        {
            if (piece.shape[py][px]) 
            {
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

    // Szukamy skrajnych px zajętych przez klocka (minPx, maxPx)
    int minPx = 4, maxPx = -1;
    for (int px = 0; px < 4; ++px)
        for (int py = 0; py < 4; ++py)
            if (piece.shape[py][px]) 
            {
                if (px < minPx) minPx = px;
                if (px > maxPx) maxPx = px;
            }

    // Ograniczenia brzegowe (lewa i prawa krawędź planszy)
    if (newX < -minPx) newX = -minPx;
    if (newX > boardWidth - 1 - maxPx) newX = boardWidth - 1 - maxPx;

    // Kolizja na X
    bool canMove = true;
    for (int py = 0; py < 4; ++py)
        for (int px = 0; px < 4; ++px)
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
        piece.x = 4; // fallback na środek jeśli się nie da
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Tetris!");

    GameState gameState = GameState::MENU;

    sf::Font font;
    font.loadFromFile("arial.ttf"); 

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

    sf::Text menuText("WYBIERZ TRYB GRY:\n1 - Normalny\n2 - Zaawansowany\n3 - Łatwy\n4 - Trudny", font, 32);
    menuText.setPosition(100, 150);

    sf::Text notAvailableText("Ten tryb nie jest jeszcze dostępny\nWciśnij ENTER żeby wrócić do menu", font, 32);
    notAvailableText.setPosition(100, 400);

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
                    if (gameState == GameState::MENU)
                    {
                        if (gameMode == GameMode::NONE) // Tylko jeśli jeszcze nie wybrano trybu
                        {
                            if (event.key.code == sf::Keyboard::Num1) 
                            {
                                gameMode = GameMode::NORMAL;
                                gameState = GameState::GAME;
                            }
                            else if (event.key.code == sf::Keyboard::Num2) 
                            {
                                gameMode = GameMode::ADVANCED;
                            }
                            else if (event.key.code == sf::Keyboard::Num3) 
                            {
                                gameMode = GameMode::EASY;
                            }
                            else if (event.key.code == sf::Keyboard::Num4) 
                            {
                                gameMode = GameMode::HARD;
                            }
                        }
                        else if (gameMode != GameMode::NORMAL)
                        {
                            // Każdy inny tryb poza NORMALNYM — nie obsługiwany
                            if (event.key.code == sf::Keyboard::Enter) 
                            {
                                gameMode = GameMode::NONE; // wróć do menu wyboru trybu
                            }
                        }
                    }
                }
                else if (gameState == GameState::GAME)
                {
                    // DODAJ TEN BLOK (obsługa klawiszy!)
                    if (event.key.code == sf::Keyboard::Escape) 
                    {
                        gameState = GameState::GAME_OVER;
                    }
                    else if (event.key.code == sf::Keyboard::Up) // rotate right
                    {
                        if (currentPiece.type != TetrominoType::O) 
                        {
                            Piece tmp = currentPiece;
                            tmp.rotateRight();
                            // Sprawdź kolizję po rotacji:
                            bool ok = true;
                            for (int py = 0; py < 4; ++py)
                            for (int px = 0; px < 4; ++px)
                                if (tmp.shape[py][px]) 
                                {
                                    int nx = tmp.x + px;
                                    int ny = tmp.y + py;
                                    if (nx < 0 || nx >= boardWidth || ny < 0 || ny >= boardHeight || board[ny][nx]) ok = false;
                                }
                            if (ok) currentPiece = tmp;
                        }
                    }
                    else if (event.key.code == sf::Keyboard::Z) // rotate left
                    {
                        if (currentPiece.type != TetrominoType::O) 
                        {
                            Piece tmp = currentPiece;
                            tmp.rotateLeft();
                            bool ok = true;
                            for (int py = 0; py < 4; ++py)
                            for (int px = 0; px < 4; ++px)
                                if (tmp.shape[py][px]) 
                                {
                                    int nx = tmp.x + px;
                                    int ny = tmp.y + py;
                                    if (nx < 0 || nx >= boardWidth || ny < 0 || ny >= boardHeight || board[ny][nx]) ok = false;
                                }
                            if (ok) currentPiece = tmp;
                        }
                    }
                    // --- lewo ---
                    else if (event.key.code == sf::Keyboard::Left) 
                    {
                        bool canMove = true;
                        for (int py = 0; py < 4; ++py) 
                            for (int px = 0; px < 4; ++px) 
                                if (currentPiece.shape[py][px]) 
                                {
                                    int nx = currentPiece.x + px - 1;
                                    int ny = currentPiece.y + py;
                                    if (nx < 0 || board[ny][nx]) 
                                        canMove = false;
                                }
                        if (canMove) currentPiece.x--;
                    }
                    // --- prawo ---
                    else if (event.key.code == sf::Keyboard::Right) 
                    {
                        bool canMove = true;
                        for (int py = 0; py < 4; ++py) 
                            for (int px = 0; px < 4; ++px) 
                                if (currentPiece.shape[py][px]) 
                                {
                                    int nx = currentPiece.x + px + 1;
                                    int ny = currentPiece.y + py;
                                    if (nx >= boardWidth || board[ny][nx]) 
                                        canMove = false;
                                }
                        if (canMove) currentPiece.x++;
                    }
                    // --- w dół ---
                    else if (event.key.code == sf::Keyboard::Down) 
                    {
                        if (canMoveDown(currentPiece)) 
                        {
                            currentPiece.y++;
                            score += 1;
                        }
                    }
                    // --- hard drop ---
                    else if (event.key.code == sf::Keyboard::Space) 
                    {
                        int startY = currentPiece.y;
                        while (canMoveDown(currentPiece)) 
                        {
                            currentPiece.y++;
                        }
                        score += 2 * (currentPiece.y - startY);

                        // "Wklej" klocek na planszę
                        for (int py = 0; py < 4; ++py) 
                            for (int px = 0; px < 4; ++px) 
                                if (currentPiece.shape[py][px]) 
                                    board[currentPiece.y + py][currentPiece.x + px] = 1;

                        int lines = clearLines();
                        if (lines == 1) score += 100;
                        else if (lines == 2) score += 300;
                        else if (lines == 3) score += 500;
                        else if (lines == 4) score += 800;
                        linesClearedTotal += lines;
                        level = 1 + (linesClearedTotal / 10);
                        fallDelay = std::max(0.1f, 0.5f - 0.05f * (level - 1)); // im wyższy level, tym szybciej, min. 0.1s

                        // Nowy klocek na górze
                        currentPiece = Piece(randomTetromino());
                        setPieceXToMouse(window, currentPiece);

                        // Wykrywanie końca gry
                        for (int px = 0; px < 4; ++px) 
                            if (board[0][currentPiece.x + px]) 
                                gameState = GameState::GAME_OVER;
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
                            for (int x = 0; x < boardWidth; x++) 
                                board[y][x] = 0;
                        currentPiece = Piece(randomTetromino());
                        setPieceXToMouse(window, currentPiece);

                        // *** TU DODAJ TO: ***
                        score = 0;
                        linesClearedTotal = 0;
                        level = 1;
                    }
                }
            }
            if (event.type == sf::Event::MouseMoved && gameState == GameState::GAME)
            {
                setPieceXToMouse(window, currentPiece);
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
                    for (int py = 0; py < 4; ++py) 
                    {
                        for (int px = 0; px < 4; ++px) 
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
                    currentPiece = Piece(randomTetromino());
                    setPieceXToMouse(window, currentPiece);

                    for (int px = 0; px < 4; ++px) 
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
            if (canMoveDown(currentPiece)) 
            {
                currentPiece.y++;
            } else {
                // "Wklej" klocek na planszę
                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px) 
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
                currentPiece = Piece(randomTetromino());
                setPieceXToMouse(window, currentPiece);

                // Wykrywanie końca gry
                for (int px = 0; px < 4; ++px) 
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
                if (gameMode != GameMode::NONE && gameMode != GameMode::NORMAL)
                {
                    window.draw(notAvailableText);
                }
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
                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px)
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
                while (canMoveDown(ghostPiece)) 
                {
                    ghostPiece.y++;
                }
                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px)
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