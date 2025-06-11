#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <fstream>
#include "piece.hpp"

using namespace std;

enum class Scene { TITLE, MODE_SELECT, GAME, PAUSE, GAME_OVER };
Scene scene = Scene::TITLE;

enum class GameMode { NONE, NORMAL, ADVANCED, EASY, HARD };
GameMode gameMode = GameMode::NONE;

vector<TetrominoType> nextPieces;
vector<TetrominoType> bag;
int bagPos = 0;
mt19937 rng(std::random_device{}());

const int boardWidth = 10;
const int boardHeight = 20;
const int cellSize = 30; // px

float musicVolume = 100.0f;   
float effectsVolume = 100.0f;  
int score = 0;
int linesClearedTotal = 0;
int level = 1;
int drought_I = 0;
const int drought_limit = 10; // np. 10 ruchów bez I tryb hard
int lastLevel = 1;
const int drought_limit_advanced = 12;

int droughtAdvanced[7] = {0,0,0,0,0,0,0};
int highscore[5] = {0, 0, 0, 0, 0};
int highscoreLevel[5] = {0, 0, 0, 0, 0};
int highscoreLines[5] = {0, 0, 0, 0, 0};

int board[boardHeight][boardWidth] = {0};

Piece* holdPiece = nullptr;
bool holdUsed = false;

TetrominoType drawFromBag();

Piece currentPiece(drawFromBag());

sf::SoundBuffer bufferClear, bufferDrop, bufferGameOver, bufferLevelUp;
sf::Sound soundClear, soundDrop, soundGameOver, soundLevelUp;
sf::Music soundtrack;

sf::Clock lockClock;
bool isTouchingGround = false;
float lockDelayTime = 0.5f; // ile sekund na manewrowanie po dotknięciu ziemi

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

int countHolesAfter(const Piece& piece)  //Do trybu Hard 
{
    // Skopiuj planszę
    int tempBoard[boardHeight][boardWidth];
    memcpy(tempBoard, board, sizeof(board));
    // Wstaw klocek na planszę
    for (int py = 0; py < 4; ++py)
        for (int px = 0; px < 4; ++px)
            if (piece.shape[py][px]) {
                int bx = piece.x + px;
                int by = piece.y + py;
                if (bx >= 0 && bx < boardWidth && by >= 0 && by < boardHeight)
                    tempBoard[by][bx] = (int)piece.type + 1;
            }
    // Licz dziury (puste pole z czymś nad sobą)
    int holes = 0;
    for (int x = 0; x < boardWidth; ++x) {
        bool blockFound = false;
        for (int y = 0; y < boardHeight; ++y) {
            if (tempBoard[y][x]) blockFound = true;
            else if (blockFound) holes++;
        }
    }
    return holes;
}

bool canPlace(const Piece& piece) 
{
    for (int py = 0; py < 4; ++py)
    for (int px = 0; px < 4; ++px)
        if (piece.shape[py][px]) {
            int nx = piece.x + px;
            int ny = piece.y + py;
            if (nx < 0 || nx >= boardWidth || ny < 0 || ny >= boardHeight || board[ny][nx])
                return false;
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

bool hasGoodSpot(TetrominoType type) 
{
    Piece test(type);
    for (int rot = 0; rot < 4; ++rot) 
    {
        for (int x = -2; x < boardWidth; ++x) 
        {
            test.x = x;
            test.y = 0;
            // Spuść klocek na sam dół
            while (canMoveDown(test)) test.y++;
            // Sprawdź czy pasuje bez kolizji
            bool ok = true;
            for (int py = 0; py < 4; ++py)
                for (int px = 0; px < 4; ++px)
                    if (test.shape[py][px]) 
                    {
                        int nx = test.x + px;
                        int ny = test.y + py;
                        if (nx < 0 || nx >= boardWidth || ny < 0 || ny >= boardHeight || board[ny][nx])
                            ok = false;
                    }
            if (ok) return true;
        }
        test.rotateRight();
    }
    return false;
}

TetrominoType drawEasy() 
{
    vector<TetrominoType> allTypes = {TetrominoType::I, TetrominoType::O, TetrominoType::T, TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L};
    vector<TetrominoType> weighted;
    for (auto type : allTypes) 
    {
        int waga = hasGoodSpot(type) ? 10 : 1;
        for (int i = 0; i < waga; ++i) weighted.push_back(type);
    }
    uniform_int_distribution<int> dist(0, weighted.size()-1);
    return weighted[dist(rng)];
}

TetrominoType drawVeryHard() 
{
    vector<TetrominoType> allTypes = {TetrominoType::I, TetrominoType::O, TetrominoType::T, TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L};
    int maxHoles = -1;
    vector<TetrominoType> worst;
    for (auto type : allTypes) 
    {
        if (type == TetrominoType::I && drought_I < drought_limit) continue; // przetrzymujemy I
        int localMax = -1;
        for (int rot = 0; rot < 4; ++rot) {
            for (int x = -2; x < boardWidth; ++x) {
                Piece test(type);
                test.x = x;
                test.y = 0;
                for (int r = 0; r < rot; ++r) test.rotateRight();
                while (canMoveDown(test)) test.y++;
                if (canPlace(test)) {
                    int holes = countHolesAfter(test);
                    if (holes > localMax) localMax = holes;
                }
            }
        }
        if (localMax > maxHoles) {
            maxHoles = localMax;
            worst.clear();
            worst.push_back(type);
        } else if (localMax == maxHoles) {
            worst.push_back(type);
        }
    }
    // Wybieramy losowo z tych najgorszych
    uniform_int_distribution<int> dist(0, worst.size() - 1);
    TetrominoType chosen = worst[dist(rng)];
    if (chosen == TetrominoType::I) drought_I = 0; else drought_I++;
    return chosen;
}

TetrominoType drawAdvanced() 
{
    vector<TetrominoType> allTypes = 
    {
        TetrominoType::I, TetrominoType::O, TetrominoType::T,
        TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L
    };

    // Sprawdź, czy któryś klocek osiągnął limit drought
    for (int i = 0; i < 7; ++i) 
    {
        if (droughtAdvanced[i] >= drought_limit_advanced) 
        {
            // Resetuj droughty
            for (int j = 0; j < 7; ++j) 
            {
                if (j == i) droughtAdvanced[j] = 0;
                else droughtAdvanced[j]++;
            }
            return (TetrominoType)i;
        }
    }

    // Budujemy wagę: im większy drought, tym większa szansa
    vector<int> weights;
    int totalWeight = 0;
    for (int i = 0; i < 7; ++i) 
    {
        int w = 1 + droughtAdvanced[i];
        weights.push_back(w);
        totalWeight += w;
    }

    // Losujemy klocek proporcjonalnie do wagi
    uniform_int_distribution<int> dist(1, totalWeight);
    int rnd = dist(rng);
    int sum = 0;
    int chosenIdx = -1;
    for (int i = 0; i < 7; ++i) 
    {
        sum += weights[i];
        if (rnd <= sum) 
        {
            chosenIdx = i;
            break;
        }
    }
    if (chosenIdx == -1) chosenIdx = 0; // fallback, nie powinno się zdarzyć

    // Aktualizacja droughtów
    for (int i = 0; i < 7; ++i) 
    {
        if (i == chosenIdx) droughtAdvanced[i] = 0;
        else droughtAdvanced[i]++;
    }
    return (TetrominoType)chosenIdx;
}

void loadHighscores() 
{
    ifstream f("highscore.dat");
    for (int i = 0; i < 5; ++i) 
    {
        if (!(f >> highscore[i] >> highscoreLevel[i] >> highscoreLines[i]))
        highscore[i] = highscoreLevel[i] = highscoreLines[i] = 0;
    }
}

void saveHighscores() 
{
    ofstream f("highscore.dat");
    for (int i = 0; i < 5; ++i)
        f << highscore[i] << " " << highscoreLevel[i] << " " << highscoreLines[i] << "\n";
}

void refillBag() 
{
    bag.clear();
    for (int i = 0; i < 7; ++i)
        bag.push_back((TetrominoType)i);
    shuffle(bag.begin(), bag.end(), rng);
    bagPos = 0;
}

void updateNextPieces() 
{
    while (nextPieces.size() < 3) 
    {
        nextPieces.push_back(drawFromBag());
    }
}

TetrominoType drawFromBag() 
{
    if (gameMode == GameMode::EASY)
        return drawEasy();
    if (gameMode == GameMode::HARD)
        return drawVeryHard();
    if (gameMode == GameMode::ADVANCED)
        return drawAdvanced();

    if (bagPos >= (int)bag.size())
        refillBag();
    return bag[bagPos++];
}

sf::Color getPieceColor(TetrominoType type);
void drawNextPieces(sf::RenderWindow& window, vector<TetrominoType>& nextPieces, int offsetX, int offsetY, int cellSize) 
{
    for (int idx = 0; idx < 3; ++idx) 
    {
        Piece preview(nextPieces[idx]);
        int previewX = offsetX + boardWidth * cellSize + 40;
        int previewY = offsetY + 40 + idx * 120;

        for (int py = 0; py < 4; ++py) 
        {
            for (int px = 0; px < 4; ++px) 
            {
                if (preview.shape[py][px]) 
                {
                    sf::RectangleShape cell(sf::Vector2f(cellSize - 8, cellSize - 8));
                    cell.setPosition(previewX + px * (cellSize - 6), previewY + py * (cellSize - 6));
                    cell.setFillColor(getPieceColor(preview.type));
                    window.draw(cell);
                }
            }
        }
    }
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

sf::Color getPieceColor(TetrominoType type)
{
    switch (type)
    {
        case TetrominoType::I: return sf::Color(100, 220, 220);   // pastelowy niebieski
        case TetrominoType::O: return sf::Color(240, 230, 140);   // jasny żółty
        case TetrominoType::T: return sf::Color(170, 120, 200);   // fioletowy pastel
        case TetrominoType::S: return sf::Color(140, 220, 140);   // zielony pastel
        case TetrominoType::Z: return sf::Color(240, 140, 140);   // różowy pastel
        case TetrominoType::J: return sf::Color(120, 150, 220);   // niebieski pastel
        case TetrominoType::L: return sf::Color(240, 180, 90);    // pomarańcz pastel
        default: return sf::Color(200, 200, 200);
    }
}

void resetGame(sf::RenderWindow& window) 
{
    nextPieces.clear();
    updateNextPieces();
    currentPiece = Piece(nextPieces.front());
    nextPieces.erase(nextPieces.begin());
    updateNextPieces();
    setPieceXToMouse(window, currentPiece);
    holdUsed = false;
    score = 0;
    linesClearedTotal = 0;
    level = 1;
    drought_I = 0;
    for (int y = 0; y < boardHeight; y++)
        for (int x = 0; x < boardWidth; x++)
            board[y][x] = 0;
    for (int i = 0; i < 7; ++i) droughtAdvanced[i] = 0;
}

int main()
{
    loadHighscores();
    refillBag();
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "Tetris!", sf::Style::Fullscreen);

    sf::Font font;
    font.loadFromFile("arial.ttf");

    bufferClear.loadFromFile("sounds/clear.wav");
    soundClear.setBuffer(bufferClear);

    bufferDrop.loadFromFile("sounds/drop.wav");
    soundDrop.setBuffer(bufferDrop);

    bufferGameOver.loadFromFile("sounds/gameover.wav");
    soundGameOver.setBuffer(bufferGameOver);

    bufferLevelUp.loadFromFile("sounds/levelup.wav");
    soundLevelUp.setBuffer(bufferLevelUp);

    // Muzyka w tle
    soundtrack.openFromFile("sounds/soundtrack.wav"); 
    soundtrack.setLoop(true);
    soundtrack.play();

    soundtrack.setVolume(musicVolume);
    soundClear.setVolume(effectsVolume);
    soundDrop.setVolume(effectsVolume);
    soundGameOver.setVolume(effectsVolume);
    soundLevelUp.setVolume(effectsVolume);

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

    sf::Text menuText("WYBIERZ TRYB GRY:\n1 - Normalny\n2 - Zaawansowany\n3 - Latwy\n4 - Trudny", font, 32);
    menuText.setPosition(100, 150);

    sf::Text notAvailableText("Ten tryb nie jest jeszcze dostępny\nWcisnij ENTER żeby wrocic do menu", font, 32);
    notAvailableText.setPosition(100, 400);

    sf::Clock fallClock;
    float fallDelay = 0.5f;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // ----------------
            // OBSŁUGA SCEN
            // ----------------
            if (scene == Scene::TITLE)
            {
                if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space)
                        scene = Scene::MODE_SELECT;
                    else if (event.key.code == sf::Keyboard::Q)
                        window.close();
                }
            }
            else if (scene == Scene::MODE_SELECT)
            {
                if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::Num1) { gameMode = GameMode::NORMAL; resetGame(window); scene = Scene::GAME; }
                    else if (event.key.code == sf::Keyboard::Num2) { gameMode = GameMode::ADVANCED; resetGame(window); scene = Scene::GAME; }
                    else if (event.key.code == sf::Keyboard::Num3) { gameMode = GameMode::EASY; resetGame(window); scene = Scene::GAME; }
                    else if (event.key.code == sf::Keyboard::Num4) { gameMode = GameMode::HARD; resetGame(window); scene = Scene::GAME; }
                    else if (event.key.code == sf::Keyboard::Escape) { scene = Scene::TITLE; }
                }
            }
            else if (scene == Scene::GAME)
            {
                if (event.type == sf::Event::KeyPressed)
                {
                    // Muzyka
                    if (event.key.code == sf::Keyboard::Add) 
                    {
                        musicVolume = std::min(musicVolume + 10.0f, 100.0f);
                        soundtrack.setVolume(musicVolume);
                    }
                    if (event.key.code == sf::Keyboard::Subtract) 
                    {
                        musicVolume = std::max(musicVolume - 10.0f, 0.0f);
                        soundtrack.setVolume(musicVolume);
                    }
                    // Efekty
                    if (event.key.code == sf::Keyboard::PageUp) 
                    {
                        effectsVolume = std::min(effectsVolume + 10.0f, 100.0f);
                        soundClear.setVolume(effectsVolume);
                        soundDrop.setVolume(effectsVolume);
                        soundGameOver.setVolume(effectsVolume);
                        soundLevelUp.setVolume(effectsVolume);
                    }
                    if (event.key.code == sf::Keyboard::PageDown) 
                    {
                        effectsVolume = std::max(effectsVolume - 10.0f, 0.0f);
                        soundClear.setVolume(effectsVolume);
                        soundDrop.setVolume(effectsVolume);
                        soundGameOver.setVolume(effectsVolume);
                        soundLevelUp.setVolume(effectsVolume);
                    }
                    if (event.key.code == sf::Keyboard::Escape)
                    {
                        scene = Scene::PAUSE;
                    }
                    else if (event.key.code == sf::Keyboard::Up)
                    {
                        if (currentPiece.type != TetrominoType::O)
                        {
                            bool rotated = false;
                            Piece tmp = currentPiece;
                            tmp.rotateRight();
                            if (canPlace(tmp)) { currentPiece = tmp; rotated = true; }
                            if (!rotated)
                            {
                                tmp.x = currentPiece.x - 1;
                                if (canPlace(tmp)) { currentPiece = tmp; rotated = true; }
                            }
                            if (!rotated)
                            {
                                tmp.x = currentPiece.x + 1;
                                if (canPlace(tmp)) { currentPiece = tmp; rotated = true; }
                            }
                            if (isTouchingGround) lockClock.restart();
                        }
                    }
                    else if (event.key.code == sf::Keyboard::Z)
                    {
                        if (currentPiece.type != TetrominoType::O)
                        {
                            bool rotated = false;
                            Piece tmp = currentPiece;
                            tmp.rotateLeft();
                            if (canPlace(tmp)) { currentPiece = tmp; rotated = true; }
                            if (!rotated)
                            {
                                tmp.x = currentPiece.x - 1;
                                if (canPlace(tmp)) { currentPiece = tmp; rotated = true; }
                            }
                            if (!rotated)
                            {
                                tmp.x = currentPiece.x + 1;
                                if (canPlace(tmp)) { currentPiece = tmp; rotated = true; }
                            }
                            if (isTouchingGround) lockClock.restart();
                        }
                    }
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
                        if (isTouchingGround) lockClock.restart();
                    }
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
                        if (isTouchingGround) lockClock.restart();
                    }
                    else if (event.key.code == sf::Keyboard::Down)
                    {
                        if (canMoveDown(currentPiece))
                        {
                            currentPiece.y++;
                            score += 1;
                            if (isTouchingGround) lockClock.restart();
                        }
                    }
                    else if (event.key.code == sf::Keyboard::Space)
                    {
                        int startY = currentPiece.y;
                        while (canMoveDown(currentPiece))
                            currentPiece.y++;
                        score += 2 * (currentPiece.y - startY);

                        for (int py = 0; py < 4; ++py)
                            for (int px = 0; px < 4; ++px)
                                if (currentPiece.shape[py][px])
                                    board[currentPiece.y + py][currentPiece.x + px] = (int)currentPiece.type + 1;

                        soundDrop.play();
                        int lines = clearLines();
                        if (lines > 0) soundClear.play();
                        if (lines == 1) score += 100;
                        else if (lines == 2) score += 300;
                        else if (lines == 3) score += 500;
                        else if (lines == 4) score += 800;
                        linesClearedTotal += lines;
                        level = 1 + (linesClearedTotal / 10);
                        fallDelay = std::max(0.1f, 0.5f - 0.05f * (level - 1));
                        if (level > lastLevel) { soundLevelUp.play(); lastLevel = level; }

                        currentPiece = Piece(nextPieces.front());
                        nextPieces.erase(nextPieces.begin());
                        updateNextPieces();
                        setPieceXToMouse(window, currentPiece);
                        holdUsed = false;

                        for (int px = 0; px < 4; ++px)
                            if (board[0][currentPiece.x + px])
                            {
                                soundGameOver.play();
                                soundtrack.stop();
                                scene = Scene::GAME_OVER;
                            }
                    }
                    else if (event.key.code == sf::Keyboard::C)
                    {
                        if (!holdUsed)
                        {
                            if (holdPiece == nullptr)
                            {
                                holdPiece = new Piece(currentPiece.type);
                                currentPiece = Piece(nextPieces.front());
                                nextPieces.erase(nextPieces.begin());
                                updateNextPieces();
                                setPieceXToMouse(window, currentPiece);
                                holdUsed = true;
                            }
                            else
                            {
                                std::swap(currentPiece.type, holdPiece->type);
                                currentPiece = Piece(currentPiece.type);
                                setPieceXToMouse(window, currentPiece);
                            }
                            holdUsed = true;
                        }
                    }
                }
                if (event.type == sf::Event::MouseMoved)
                {
                    setPieceXToMouse(window, currentPiece);
                }
                if (event.type == sf::Event::MouseButtonPressed)
                {
                    if (event.mouseButton.button == sf::Mouse::Left)
                    {
                        int startY = currentPiece.y;
                        while (canMoveDown(currentPiece))
                            currentPiece.y++;
                        score += 2 * (currentPiece.y - startY);

                        for (int py = 0; py < 4; ++py)
                            for (int px = 0; px < 4; ++px)
                                if (currentPiece.shape[py][px])
                                    board[currentPiece.y + py][currentPiece.x + px] = (int)currentPiece.type + 1;

                        soundDrop.play();
                        int lines = clearLines();
                        if (lines > 0) soundClear.play();
                        if (lines == 1) score += 100;
                        else if (lines == 2) score += 300;
                        else if (lines == 3) score += 500;
                        else if (lines == 4) score += 800;
                        linesClearedTotal += lines;
                        level = 1 + (linesClearedTotal / 10);
                        fallDelay = std::max(0.1f, 0.5f - 0.05f * (level - 1));
                        if (level > lastLevel) { soundLevelUp.play(); lastLevel = level; }

                        currentPiece = Piece(nextPieces.front());
                        nextPieces.erase(nextPieces.begin());
                        updateNextPieces();
                        setPieceXToMouse(window, currentPiece);
                        holdUsed = false;

                        for (int px = 0; px < 4; ++px)
                            if (board[0][currentPiece.x + px])
                            {
                                soundGameOver.play();
                                soundtrack.stop();
                                scene = Scene::GAME_OVER;
                            }
                    }
                }
            }
            else if (scene == Scene::PAUSE)
            {
                if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::Escape)
                    {
                        scene = Scene::GAME; // wraca do gry z pauzy
                    }
                    else if (event.key.code == sf::Keyboard::Q)
                    {
                        soundGameOver.play();
                        soundtrack.stop();
                        scene = Scene::GAME_OVER; // kończy grę
                    }
                }
            }
            else if (scene == Scene::GAME_OVER)
            {
                if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::Enter)
                    {
                        scene = Scene::TITLE;
                        soundtrack.play();
                        gameMode = GameMode::NONE;
                    }
                    else if (event.key.code == sf::Keyboard::R)
                    {
                        resetGame(window);
                        scene = Scene::GAME;
                        soundtrack.play();
                    }
                }
            }
        }

        // --------- LOGIKA automatycznego opadania ---------
        if (scene == Scene::GAME && fallClock.getElapsedTime().asSeconds() > fallDelay)
        {
            if (canMoveDown(currentPiece))
            {
                currentPiece.y++;
                isTouchingGround = false;
                lockClock.restart();
            }
            else
            {
                if (!isTouchingGround)
                {
                    isTouchingGround = true;
                    lockClock.restart();
                }
                if (lockClock.getElapsedTime().asSeconds() >= lockDelayTime)
                {
                    soundDrop.play();
                    for (int py = 0; py < 4; ++py)
                        for (int px = 0; px < 4; ++px)
                            if (currentPiece.shape[py][px])
                                board[currentPiece.y + py][currentPiece.x + px] = (int)currentPiece.type + 1;

                    int lines = clearLines();
                    if (lines > 0) soundClear.play();
                    if (lines == 1) score += 100;
                    else if (lines == 2) score += 300;
                    else if (lines == 3) score += 500;
                    else if (lines == 4) score += 800;
                    linesClearedTotal += lines;
                    level = 1 + (linesClearedTotal / 10);
                    fallDelay = std::max(0.1f, 0.5f - 0.05f * (level - 1));
                    if (level > lastLevel) { soundLevelUp.play(); lastLevel = level; }

                    currentPiece = Piece(nextPieces.front());
                    nextPieces.erase(nextPieces.begin());
                    updateNextPieces();
                    setPieceXToMouse(window, currentPiece);
                    holdUsed = false;

                    for (int px = 0; px < 4; ++px)
                        if (board[0][currentPiece.x + px])
                        {
                            soundGameOver.play();
                            soundtrack.stop();
                            scene = Scene::GAME_OVER;
                        }
                    isTouchingGround = false;
                }
            }
            fallClock.restart();
        }

        // ---------- RYSOWANIE -----------
        scoreText.setString("SCORE: " + std::to_string(score));
        linesText.setString("LINES: " + std::to_string(linesClearedTotal));
        levelText.setString("LEVEL: " + std::to_string(level));
        window.clear();

        if (scene == Scene::TITLE)
        {
            sf::Text title("TETRIS\n[Enter] lub [Space] - Start\n[Q] - Wyjscie", font, 40);
            title.setPosition(120, 140);
            window.draw(title);
        }
        else if (scene == Scene::MODE_SELECT)
        {
            sf::Text modeText("WYBIERZ TRYB:\n1 - Normalny\n2 - Zaawansowany\n3 - Latwy\n4 - Trudny\n\n[Esc] Powrot", font, 32);
            modeText.setPosition(100, 180);
            window.draw(modeText);
        }
        else if (scene == Scene::GAME)
        {
            int gridWidth = boardWidth * cellSize;
            int gridHeight = boardHeight * cellSize;
            int offsetX = (window.getSize().x - gridWidth) / 2;
            int offsetY = (window.getSize().y - gridHeight) / 2;

            for (int y = 0; y < boardHeight; ++y)
            {
                for (int x = 0; x < boardWidth; ++x) {
                    sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                    cell.setPosition(offsetX + x * cellSize, offsetY + y * cellSize);
                    if (board[y][x]) {
                        sf::Color color = getPieceColor((TetrominoType)(board[y][x] - 1));
                        cell.setFillColor(color);
                    } else {
                        cell.setFillColor(sf::Color(30,30,30));
                    }
                    window.draw(cell);
                }
            }
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    if (currentPiece.shape[py][px])
                    {
                        int by = currentPiece.y + py;
                        int bx = currentPiece.x + px;
                        if (by >= 0 && by < boardHeight && bx >= 0 && bx < boardWidth && board[by][bx] == 0)
                        {
                            sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                            cell.setPosition(offsetX + bx * cellSize, offsetY + by * cellSize);
                            sf::Color color = getPieceColor(currentPiece.type);
                            cell.setFillColor(color);
                            window.draw(cell);
                        }
                    }
                }
            }
            int ghostY = currentPiece.y;
            Piece ghostPiece = currentPiece;
            while (canMoveDown(ghostPiece))
                ghostPiece.y++;
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    if (ghostPiece.shape[py][px])
                    {
                        sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                        cell.setPosition(offsetX + (ghostPiece.x + px) * cellSize, offsetY + (ghostPiece.y + py) * cellSize);
                        cell.setFillColor(sf::Color(255, 255, 255, 90));
                        window.draw(cell);
                    }
                }
            }
            drawNextPieces(window, nextPieces, offsetX, offsetY, cellSize);
            window.draw(scoreText);
            window.draw(linesText);
            window.draw(levelText);

            if (holdPiece)
            {
                Piece preview(holdPiece->type);
                int previewX = 50;
                int previewY = 180;
                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px)
                    {
                        if (preview.shape[py][px])
                        {
                            sf::RectangleShape cell(sf::Vector2f(cellSize - 8, cellSize - 8));
                            cell.setPosition(previewX + px * (cellSize - 6), previewY + py * (cellSize - 6));
                            cell.setFillColor(getPieceColor(preview.type));
                            window.draw(cell);
                        }
                    }
                }
                sf::Text holdText("HOLD", font, 24);
                holdText.setPosition(previewX, previewY - 32);
                holdText.setFillColor(sf::Color::White);
                window.draw(holdText);
            }
        }
        else if (scene == Scene::PAUSE)
        {
            sf::Text pausedText("PAUSED\n[Esc] Resume\n[Q] Quit", font, 48);
            pausedText.setPosition(200, 250);
            pausedText.setFillColor(sf::Color::White);
            window.draw(pausedText);
        }
        else if (scene == Scene::GAME_OVER)
        {
            if (score > highscore[(int)gameMode])
            {
                highscore[(int)gameMode] = score;
                highscoreLevel[(int)gameMode] = level;
                highscoreLines[(int)gameMode] = linesClearedTotal;
                saveHighscores();
            }
            summaryText.setString(
                "SCORE: " + std::to_string(score) +
                "\nLINES: " + std::to_string(linesClearedTotal) +
                "\nLEVEL: " + std::to_string(level) +
                "\nHIGHSCORE: " + std::to_string(highscore[(int)gameMode]) +
                " (LVL " + std::to_string(highscoreLevel[(int)gameMode]) +
                ", LINES " + std::to_string(highscoreLines[(int)gameMode]) + ")"

            );
            sf::Text over("GAME OVER\n\n[Enter] - Menu\n[R] - Restart", font, 40);
            over.setPosition(110, 180);
            window.draw(over);
            window.draw(summaryText);
        }
        window.display();
    }
    if (holdPiece) delete holdPiece;
    return 0;
}