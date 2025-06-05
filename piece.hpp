#pragma once

enum class TetrominoType { O, I, T, S, Z, J, L };

struct Piece 
{
    int x, y;
    int shape[4][4];
    TetrominoType type;
    Piece(TetrominoType t);          // Konstruktor główny
    Piece(const Piece&) = default;   // Konstruktor kopiujący - DODAJ TO!
    Piece& operator=(const Piece&) = default; // Operator przypisania - też przyda się!
    void rotateLeft();
    void rotateRight();
};
TetrominoType randomTetromino();
