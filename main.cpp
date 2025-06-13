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
int cellSize;

float musicVolume = 2.0f;   
float effectsVolume = 20.0f;   //100.0f;

const int nextPanelWidth = 200;
const int nextPanelHeight = 380;

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
void drawRoundedCell(sf::RenderWindow& window, sf::Vector2f pos, float size, sf::Color color, float radius = 7.f);
void drawNextPanel(
    sf::RenderWindow& window, const sf::Font& font, 
    const std::vector<TetrominoType>& nextPieces, 
    int cellSize, int panelX, int panelY,
    int panelWidth, int panelHeight 
)
{
    const int numPreviews = 3;
    const int border = 8;

    // 1. Biała ramka
    sf::RectangleShape outer(sf::Vector2f(panelWidth, panelHeight));
    outer.setPosition(panelX, panelY);
    outer.setFillColor(sf::Color::White);
    window.draw(outer);

    // 2. DUŻY SZARY PROSTOKĄT (całe wnętrze, nie tylko pasek!)
    sf::RectangleShape darkbg(sf::Vector2f(panelWidth - 2 * border, panelHeight - 2 * border));
    darkbg.setPosition(panelX + border, panelY + border);
    darkbg.setFillColor(sf::Color(35, 35, 35));
    window.draw(darkbg);

    // 3. Pasek NEXT u góry (ciemniejszy szary, krótszy na wysokość)
    int nextBarH = 38;
    sf::RectangleShape bar(sf::Vector2f(panelWidth - 2 * border, nextBarH));
    bar.setPosition(panelX + border, panelY + border);
    bar.setFillColor(sf::Color(85, 85, 85));
    window.draw(bar);

    // 4. Napis NEXT (wyśrodkowany)
    sf::Text nextText("NEXT", font, 26);
    nextText.setStyle(sf::Text::Bold);
    nextText.setFillColor(sf::Color::White);
    float textX = panelX + border + ((panelWidth - 2 * border) - nextText.getLocalBounds().width) / 2;
    float textY = panelY + border + 2;
    nextText.setPosition(textX, textY);
    window.draw(nextText);

    // 5. Przestrzeń na klocki
    int piecesAreaY = panelY + border + nextBarH + 8;
    int piecesAreaHeight = panelHeight - 2 * border - nextBarH - 14;

    // Największy możliwy rozmiar komórki
    int previewCell = piecesAreaHeight / (numPreviews * 4 + 1); // +1 daje trochę marginesu

    // 6. Rysuj 3 kolejne klocki, równo rozmieszczone
    for (int idx = 0; idx < numPreviews && idx < (int)nextPieces.size(); ++idx) {
        Piece preview(nextPieces[idx]);

        // Znajdź rozmiar bounding boxa klocka
        int minPx = 4, maxPx = -1, minPy = 4, maxPy = -1;
        for (int py = 0; py < 4; ++py)
            for (int px = 0; px < 4; ++px)
                if (preview.shape[py][px]) {
                    if (px < minPx) minPx = px;
                    if (px > maxPx) maxPx = px;
                    if (py < minPy) minPy = py;
                    if (py > maxPy) maxPy = py;
                }
        int blocksW = maxPx - minPx + 1;
        int blocksH = maxPy - minPy + 1;

        // Pozycja pionowa dla tego klocka
        int slotHeight = piecesAreaHeight / numPreviews;
        int slotY = piecesAreaY + idx * slotHeight;

        // Wyśrodkowanie w slocie pionowo i poziomo
        int baseX = panelX + border + ((panelWidth - 2 * border) - blocksW * previewCell) / 2 - minPx * previewCell;
        int baseY = slotY + (slotHeight - blocksH * previewCell) / 2 - minPy * previewCell;

        for (int py = 0; py < 4; ++py)
            for (int px = 0; px < 4; ++px)
                if (preview.shape[py][px]) {
                    sf::RectangleShape rect(sf::Vector2f(previewCell-2, previewCell-2));
                    rect.setFillColor(getPieceColor(preview.type));
                    rect.setPosition(baseX + px * previewCell, baseY + py * previewCell);
                    window.draw(rect);
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

void drawRoundedCell(sf::RenderWindow& window, sf::Vector2f pos, float size, sf::Color color, float radius)
{
    // Podstawa - kwadrat
    sf::RectangleShape cell(sf::Vector2f(size, size));
    cell.setPosition(pos);
    cell.setFillColor(color);
    cell.setOutlineThickness(2); // grubość obramowania
    cell.setOutlineColor(sf::Color(50, 50, 50));
    window.draw(cell);

    // 4 rogi jako kółka
    sf::CircleShape corner(radius);
    corner.setFillColor(color);

    // Lewy górny
    corner.setPosition(pos.x, pos.y);
    window.draw(corner);
    // Prawy górny
    corner.setPosition(pos.x + size - 2*radius, pos.y);
    window.draw(corner);
    // Lewy dolny
    corner.setPosition(pos.x, pos.y + size - 2*radius);
    window.draw(corner);
    // Prawy dolny
    corner.setPosition(pos.x + size - 2*radius, pos.y + size - 2*radius);
    window.draw(corner);
}

void drawPanel(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size, const std::string& label, sf::Font& font)
{
    // Tło panelu z lekkim gradientem
    sf::RectangleShape panel(size);
    panel.setPosition(pos);
    // Gradient: górny szary, dolny jaśniejszy szary (imitacja gradientu ręcznie)
    sf::Color colorTop(180, 180, 180, 255);
    sf::Color colorBottom(240, 240, 240, 255);
    // Niestety SFML nie ma gradientu - więc prosty kompromis: dwa prostokąty
    panel.setFillColor(colorBottom);
    window.draw(panel);
    sf::RectangleShape topRect(sf::Vector2f(size.x, size.y/2));
    topRect.setPosition(pos);
    topRect.setFillColor(colorTop);
    window.draw(topRect);

    // Ramka z zaokrąglonymi rogami (imitacja)
    panel.setOutlineThickness(5);
    panel.setOutlineColor(sf::Color(220, 220, 220, 255));
    panel.setFillColor(sf::Color::Transparent);
    window.draw(panel);

    // Cień (pseudo-cień pod spodem)
    sf::RectangleShape shadow(size);
    shadow.setPosition(pos.x+4, pos.y+4);
    shadow.setFillColor(sf::Color(0,0,0,40));
    window.draw(shadow);

    // Label
    sf::Text text(label, font, 20);
    text.setStyle(sf::Text::Bold);
    text.setFillColor(sf::Color::White);
    text.setPosition(
        pos.x + (size.x - text.getLocalBounds().width) / 2,
        pos.y + 5
    );
    window.draw(text);
}

void drawHoldPanel(
    sf::RenderWindow& window, const sf::Font& font,
    TetrominoType holdType,
    int cellSize, int panelX, int panelY,
    int panelWidth, int panelHeight
) {
    const int border = 8;
    // 1. Biała ramka
    sf::RectangleShape outer(sf::Vector2f(panelWidth, panelHeight));
    outer.setPosition(panelX, panelY);
    outer.setFillColor(sf::Color::White);
    window.draw(outer);

    // 2. Szary prostokąt na całą powierzchnię środka
    sf::RectangleShape darkbg(sf::Vector2f(panelWidth - 2 * border, panelHeight - 2 * border));
    darkbg.setPosition(panelX + border, panelY + border);
    darkbg.setFillColor(sf::Color(35, 35, 35));
    window.draw(darkbg);

    // 3. Pasek na górze pod napisem HOLD
    int holdBarH = 38;
    sf::RectangleShape bar(sf::Vector2f(panelWidth - 2 * border, holdBarH));
    bar.setPosition(panelX + border, panelY + border);
    bar.setFillColor(sf::Color(80, 80, 80));
    window.draw(bar);

    // 4. Napis HOLD (wyśrodkowany)
    sf::Text holdText("HOLD", font, 26);
    holdText.setStyle(sf::Text::Bold);
    holdText.setFillColor(sf::Color::White);
    float textX = panelX + border + ((panelWidth - 2 * border) - holdText.getLocalBounds().width) / 2;
    float textY = panelY + border + 2;
    holdText.setPosition(textX, textY);
    window.draw(holdText);

    // 5. Przestrzeń na klocka
    int pieceAreaY = panelY + border + holdBarH + 8;
    int pieceAreaHeight = panelHeight - 2 * border - holdBarH - 14;
    int previewCell = pieceAreaHeight / 6; // rozmiar komórki – większy margines

    // Rysuj klocek jeśli jest
    if (holdType != TetrominoType(-1)) {
        Piece preview(holdType);

        // Bounding box klocka
        int minPx = 4, maxPx = -1, minPy = 4, maxPy = -1;
        for (int py = 0; py < 4; ++py)
            for (int px = 0; px < 4; ++px)
                if (preview.shape[py][px]) {
                    if (px < minPx) minPx = px;
                    if (px > maxPx) maxPx = px;
                    if (py < minPy) minPy = py;
                    if (py > maxPy) maxPy = py;
                }
        int blocksW = maxPx - minPx + 1;
        int blocksH = maxPy - minPy + 1;

        // Wyśrodkowanie klocka
        int baseX = panelX + border + ((panelWidth - 2 * border) - blocksW * previewCell) / 2 - minPx * previewCell;
        int baseY = pieceAreaY + (pieceAreaHeight - blocksH * previewCell) / 2 - minPy * previewCell;

        for (int py = 0; py < 4; ++py)
            for (int px = 0; px < 4; ++px)
                if (preview.shape[py][px]) {
                    sf::RectangleShape rect(sf::Vector2f(previewCell-2, previewCell-2));
                    rect.setFillColor(getPieceColor(preview.type));
                    rect.setPosition(baseX + px * previewCell, baseY + py * previewCell);
                    window.draw(rect);
                }
    }
}

void drawScorePanel(
    sf::RenderWindow& window, const sf::Font& font,
    int score, int level, int lines,
    int panelX, int panelY,
    int panelW, int panelH
)
{
    const int border = 8;
    int cellH = (panelH - 2 * border) / 3;

    // Biała ramka + tło panelu (ciemny)
    sf::RectangleShape outer(sf::Vector2f(panelW, panelH));
    outer.setPosition(panelX, panelY);
    outer.setFillColor(sf::Color::White);
    window.draw(outer);

    sf::RectangleShape bg(sf::Vector2f(panelW - 2 * border, panelH - 2 * border));
    bg.setPosition(panelX + border, panelY + border);
    bg.setFillColor(sf::Color(35, 35, 35));
    window.draw(bg);

    std::vector<std::string> labels = {"SCORE", "LEVEL", "LINES"};
    std::vector<int> values = {score, level, lines};

    for (int i = 0; i < 3; ++i) {
        int y = panelY + border + i * cellH;

        // SZARY pasek pod napisem (jak HOLD/NEXT)
        int barH = 36;
        sf::RectangleShape bar(sf::Vector2f(panelW - 2 * border, barH));
        bar.setPosition(panelX + border, y);
        bar.setFillColor(sf::Color(85, 85, 85));
        window.draw(bar);

        // NAPIS – duży, biały, wyśrodkowany, bez tła!
        sf::Text label(labels[i], font, 30);
        label.setStyle(sf::Text::Bold);
        label.setFillColor(sf::Color::White);
        sf::FloatRect lRect = label.getLocalBounds();
        float lx = panelX + border + ((panelW - 2 * border) - lRect.width) / 2 - lRect.left;
        float ly = y + (barH - lRect.height) / 2 - 5;
        label.setPosition(lx, ly);
        window.draw(label);

        // CZARNE tło tylko pod liczbą!
        int numH = cellH * 0.52;
        sf::RectangleShape cell(sf::Vector2f(panelW - 2 * border, numH));
        cell.setPosition(panelX + border, y + barH);
        cell.setFillColor(sf::Color(18,18,18));
        window.draw(cell);

        // LICZBA – duża, wyśrodkowana na czarnym polu
        sf::Text val(std::to_string(values[i]), font, 38);
        val.setStyle(sf::Text::Bold);
        val.setFillColor(sf::Color(220, 220, 220));
        sf::FloatRect vRect = val.getLocalBounds();
        float vx = panelX + border + ((panelW - 2 * border) - vRect.width) / 2 - vRect.left;
        float vy = y + barH + (numH - vRect.height) / 2 - 7;
        val.setPosition(vx, vy);
        window.draw(val);
    }
}

void resetGame(sf::RenderWindow& window) 
{
    if (holdPiece) { delete holdPiece; holdPiece = nullptr; }
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
    //sf::RenderWindow window(sf::VideoMode(2400, 1200), "Tetris!", sf::Style::Default);  //testowanie
    cellSize = std::min(window.getSize().x / (boardWidth + 10), window.getSize().y / (boardHeight + 2));

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
        scoreText.setString(std::to_string(score));
        levelText.setString(std::to_string(level));
        linesText.setString(std::to_string(linesClearedTotal));
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

            const int panelWidth = boardHeight * cellSize / 4;
            const int panelHeight = boardHeight * cellSize / 2;
            
            const int boardBorder = 6;
            sf::RectangleShape gridFrame(sf::Vector2f(gridWidth + 2 * boardBorder, gridHeight + 2 * boardBorder));
            gridFrame.setPosition(offsetX - boardBorder, offsetY - boardBorder);
            gridFrame.setFillColor(sf::Color::White);
            window.draw(gridFrame);

            for (int y = 0; y < boardHeight; ++y)
            {
                for (int x = 0; x < boardWidth; ++x) 
                {
                    sf::RectangleShape cell(sf::Vector2f(cellSize-1, cellSize-1));
                    cell.setPosition(offsetX + x * cellSize, offsetY + y * cellSize);
                    if (board[y][x]) {
                        sf::Color color = getPieceColor((TetrominoType)(board[y][x] - 1));
                        cell.setFillColor(color);
                    } 
                    else 
                    {
                        cell.setFillColor(sf::Color(30,30,30));
                    }
                    window.draw(cell);
                }
            }
            for (int x = 0; x <= boardWidth; ++x) 
            {
                sf::RectangleShape line(sf::Vector2f(2, boardHeight * cellSize));
                line.setPosition(offsetX + x * cellSize - 1, offsetY);
                line.setFillColor(sf::Color(60, 60, 60));
                window.draw(line);
            }
            for (int y = 0; y <= boardHeight; ++y) 
            {
                sf::RectangleShape line(sf::Vector2f(boardWidth * cellSize, 2));
                line.setPosition(offsetX, offsetY + y * cellSize - 1);
                line.setFillColor(sf::Color(60, 60, 60));
                window.draw(line);
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
                            sf::Color color = getPieceColor(currentPiece.type);
                            drawRoundedCell(window, sf::Vector2f(offsetX + bx * cellSize, offsetY + by * cellSize), cellSize-1, color);
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
                        int by = ghostPiece.y + py;
                        int bx = ghostPiece.x + px;
                        if (by >= 0 && by < boardHeight && bx >= 0 && bx < boardWidth)
                        {
                            sf::RectangleShape ghostCell(sf::Vector2f(cellSize - 1, cellSize - 1));
                            ghostCell.setPosition(offsetX + bx * cellSize, offsetY + by * cellSize);
                            sf::Color color = getPieceColor(ghostPiece.type);
                            color.a = 90;
                            ghostCell.setFillColor(color);
                            window.draw(ghostCell);
                        }
                    }
                }
            }
            drawNextPanel(
                window, font, nextPieces, cellSize,
                offsetX + gridWidth + 60,
                offsetY + 40,
                panelWidth,    
                panelHeight       
            );

            sf::Text scoreShadow = scoreText;
            scoreShadow.setFillColor(sf::Color(0,0,0,120)); // czarny cień
            scoreShadow.setPosition(scoreText.getPosition().x + 3, scoreText.getPosition().y + 3);

            // oblicz pozycje analogicznie jak panel NEXT, blisko planszy po lewej
            int panelW = boardHeight * cellSize / 4;
            int holdPanelH = boardHeight * cellSize / 4 + 40;
            int scorePanelH = 320;

            int panelX = offsetX - panelW - 60;

            // HOLD na wysokości NEXT (czyli wysokość offsetY + 40)
            int holdPanelY = offsetY + 40;

            // SCORE dużo niżej – praktycznie przy dole planszy
            int scorePanelY = offsetY + gridHeight - scorePanelH - 30; // 24px odstęp od dołu

            drawHoldPanel(window, font, holdPiece ? holdPiece->type : TetrominoType(-1),
                cellSize, panelX, holdPanelY, panelW, holdPanelH);

            drawScorePanel(window, font, score, level, linesClearedTotal,
                panelX, scorePanelY, panelW, scorePanelH);
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