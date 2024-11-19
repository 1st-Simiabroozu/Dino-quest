
#include <stdio.h>

enum Player_class{Paladin,Druid,Berserker}; 

void init_character(int row, int col, char board[row][col], enum Player_class player, int pos, int height) {
    // Efface la zone autour de la position du personnage pour ne pas avoir de résidus
    clear_character(row, col, board, pos, height);

    // Vérifie si les indices sont valides avant de dessiner
    if (pos < 0 || pos >= col || height < 0 || height >= row) {
        return;
    }

    switch (player) {
        case 0:  // Paladin
            // Dessin complet du Paladin
            board[height + 2][pos] = 'O';    // Tête
            board[height + 1][pos] = '\\';   // Corps (partie gauche)
            board[height + 1][pos + 1] = '|'; // Corps (torse)
            board[height + 3][pos - 1] = '_'; // Bas du corps (gauche)
            board[height + 3][pos] = '_';    // Bas du corps (milieu)
            board[height + 1][pos - 1] = '0'; // Bras gauche
            board[height][pos - 1] = '/';    // Jambes gauche
            board[height][pos + 1] = '\\';   // Jambes droite
            break;

        case 1:  // Druid
            // Dessin complet du Druid
            board[height + 2][pos + 1] = '\\'; // Tête
            board[height + 2][pos - 1] = '/';  // Tête
            board[height + 2][pos] = 'O';      // Tête
            board[height + 1][pos] = '\\';     // Corps (partie gauche)
            board[height + 1][pos + 1] = '|';  // Corps (torse)
            board[height + 3][pos] = '_';      // Bas du corps
            board[height + 1][pos + 1] = '0';  // Bras droit
            board[height][pos - 1] = '/';     // Jambes gauche
            board[height][pos + 1] = '\\';    // Jambes droite
            break;

        case 2:  // Berserker
            // Dessin complet du Berserker
            board[height + 2][pos + 1] = '_';  // Casque (partie droite)
            board[height + 2][pos - 1] = '|';  // Casque (partie gauche)
            board[height + 2][pos + 1] = '|';  // Casque (partie droite)
            board[height + 4][pos] = '|';      // Ceinture
            board[height + 5][pos] = 'O';      // Corps (partie supérieure)
            board[height + 4][pos + 1] = '\\'; // Bras droit
            board[height + 4][pos - 1] = '/'; // Bras gauche
            board[height + 3][pos + 1] = '/'; // Bras droit
            board[height + 3][pos - 1] = '\\'; // Bras gauche
            board[height + 2][pos] = 'O';     // Tête
            board[height + 1][pos] = '\\';    // Torse (gauche)
            board[height + 1][pos + 1] = '|'; // Torse (droite)
            board[height + 3][pos] = '_';     // Bas du corps
            board[height + 1][pos + 1] = '*'; // Épée ?
            board[height][pos - 1] = '/';     // Jambes gauche
            board[height][pos + 1] = '\\';    // Jambes droite
            break;
    }
}




