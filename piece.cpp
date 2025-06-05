#include "piece.hpp"
#include <cstdlib> // rand()

// Definicje kształtów
static const int shapes[7][4][4] = {
    // O
    { {1,1,0,0},
      {1,1,0,0},
      {0,0,0,0},
      {0,0,0,0} },
    // I
    { {0,0,0,0},
      {1,1,1,1},
      {0,0,0,0},
      {0,0,0,0} },
    // T
    { {0,1,0,0},
      {1,1,1,0},
      {0,0,0,0},
      {0,0,0,0} },
    // S
    { {0,1,1,0},
      {1,1,0,0},
      {0,0,0,0},
      {0,0,0,0} },
    // Z
    { {1,1,0,0},
      {0,1,1,0},
      {0,0,0,0},
      {0,0,0,0} },
    // J
    { {1,0,0,0},
      {1,1,1,0},
      {0,0,0,0},
      {0,0,0,0} },
    // L
    { {0,0,1,0},
      {1,1,1,0},
      {0,0,0,0},
      {0,0,0,0} }
};

Piece::Piece(TetrominoType t)
{
    x = 4; // start na środku
    y = 0;
    type = t;
    int idx = static_cast<int>(t);
    for (int py = 0; py < 4; ++py)
        for (int px = 0; px < 4; ++px)
            shape[py][px] = shapes[idx][py][px];
}

TetrominoType randomTetromino()
{
    return static_cast<TetrominoType>(rand() % 7);
}

void Piece::rotateRight() 
{
    int tmp[4][4];
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            tmp[x][3 - y] = shape[y][x];
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            shape[y][x] = tmp[y][x];
}

void Piece::rotateLeft() 
{
    int tmp[4][4];
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            tmp[3 - x][y] = shape[y][x];
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            shape[y][x] = tmp[y][x];
}
