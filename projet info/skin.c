#include <stdio.h>

enum Player_class { Paladin, Druid, Berserker };

void init_character(int row, int col, char board[row][col], enum Player_class player, int pos, int height) {
    // Efface la zone autour de la position du personnage
    clear_character(row, col, board, pos, height);

    // Vérifie si les indices sont valides avant de dessiner
    if (pos < 0 || pos >= col || height < 0 || height >= row) {
        return;
    }

    // Dessin du personnage basé sur sa classe
    switch (player) {
        case Paladin: // Classe Paladin
            if (board[height + 2][pos] == ' ') board[height + 2][pos] = 'O';    // Tête
            if (board[height + 1][pos] == ' ') board[height + 1][pos] = '\\';   // Corps (partie gauche)
            if (board[height + 1][pos + 1] == ' ') board[height + 1][pos + 1] = '|'; // Corps (torse)
            if (board[height + 3][pos - 1] == ' ') board[height + 3][pos - 1] = '_'; // Bas du corps (gauche)
            if (board[height + 3][pos] == ' ') board[height + 3][pos] = '_';    // Bas du corps (milieu)
            if (board[height + 1][pos - 1] == ' ') board[height + 1][pos - 1] = '0'; // Bras gauche
            if (board[height][pos - 1] == ' ') board[height][pos - 1] = '/';    // Jambes gauche
            if (board[height][pos + 1] == ' ') board[height][pos + 1] = '\\';   // Jambes droite
            break;

        case Druid: // Classe Druid
            if (board[height + 2][pos + 1] == ' ') board[height + 2][pos + 1] = '\\'; // Tête (droite)
            if (board[height + 2][pos - 1] == ' ') board[height + 2][pos - 1] = '/';  // Tête (gauche)
            if (board[height + 2][pos] == ' ') board[height + 2][pos] = 'O';          // Tête (centre)
            if (board[height + 1][pos] == ' ') board[height + 1][pos] = '\\';         // Corps (partie gauche)
            if (board[height + 1][pos + 1] == ' ') board[height + 1][pos + 1] = '|';  // Corps (torse)
            if (board[height + 3][pos] == ' ') board[height + 3][pos] = '_';          // Bas du corps
            if (board[height + 1][pos + 1] == ' ') board[height + 1][pos + 1] = '0';  // Bras droit
            if (board[height][pos - 1] == ' ') board[height][pos - 1] = '/';         // Jambes gauche
            if (board[height][pos + 1] == ' ') board[height][pos + 1] = '\\';        // Jambes droite
            break;

        case Berserker: // Classe Berserker
            if (board[height + 2][pos + 1] == ' ') board[height + 2][pos + 1] = '_';  // Casque (droite)
            if (board[height + 2][pos - 1] == ' ') board[height + 2][pos - 1] = '|';  // Casque (gauche)
            if (board[height + 4][pos] == ' ') board[height + 4][pos] = '|';          // Ceinture
            if (board[height + 5][pos] == ' ') board[height + 5][pos] = 'O';          // Corps (supérieur)
            if (board[height + 4][pos + 1] == ' ') board[height + 4][pos + 1] = '\\'; // Bras droit
            if (board[height + 4][pos - 1] == ' ') board[height + 4][pos - 1] = '/';  // Bras gauche
            if (board[height + 3][pos + 1] == ' ') board[height + 3][pos + 1] = '/';  // Bras droit (bas)
            if (board[height + 3][pos - 1] == ' ') board[height + 3][pos - 1] = '\\'; // Bras gauche (bas)
            if (board[height + 2][pos] == ' ') board[height + 2][pos] = 'O';          // Tête
            if (board[height + 1][pos] == ' ') board[height + 1][pos] = '\\';         // Torse (gauche)
            if (board[height + 1][pos + 1] == ' ') board[height + 1][pos + 1] = '|';  // Torse (droite)
            if (board[height + 3][pos] == ' ') board[height + 3][pos] = '_';          // Bas du corps
            if (board[height + 1][pos + 1] == ' ') board[height + 1][pos + 1] = '*';  // Épée
            if (board[height][pos - 1] == ' ') board[height][pos - 1] = '/';          // Jambes gauche
            if (board[height][pos + 1] == ' ') board[height][pos + 1] = '\\';         // Jambes droite
            break;
    }
}
