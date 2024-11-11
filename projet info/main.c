#include "skin.h"
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <termios.h>
#include <sys/types.h>
#include <signal.h>

/// Alternative plus 'trans-plateforme' que 'system("clear")'
#define clear() printf("\033[H\033[J")


/// Variables Globales pour gestion du temps : vous pouvez ajuster selon besoins.
int acquisition_time ;
double gravity_time ;
pid_t pid_musique = -1;


/// /!\ Ne pas toucher à cette fonction, je ne la comprend pas.
// Gets a char from STDIN
// Returns immediatly, even if STDIN is empty
// in which case it returns ?
int getch() 
{ 
    int ch;
    struct termios oldattr, newattr;

    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~ICANON;
    newattr.c_lflag &= ~ECHO;
    newattr.c_cc[VMIN] = 1;
    newattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI };
    
    if( poll(&mypoll, 1, acquisition_time) )
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);

    return ch; 
}


void print_board(int row, int col, char board[row][col]) {
  clear();
  for (int i = row-1; i >= 0 ; i--) {
    for (int j = 0; j < col; j++)
      printf("%c", board[i][j]) ;
    printf("\n");
  }
}

void gravity(int row, int col, char board[row][col]) {
    int pos = 4;
    int height = 1;

    for (int i = height + 3; i >= 1; i--) {
        if (board[i][pos] == 'O' && board[i - 1][pos] == ' ') {
            board[i][pos] = ' ';
            board[i - 1][pos] = 'O';
        }
    }
}

// Initialize the board.
void init_board(int row, int col, char board[row][col]) {
  for (int j = 0; j < col; j++) {
    board[0][j] = 'D' ;
  }
  for (int i = 1; i < row; i++) {
    for (int j = 0; j < col; j++)
      board[i][j] = ' ' ;
  }
}

const int PLAYER_ROW = 1;  // Colonne fixe pour le personnage

// Fonction mise à jour pour conserver le personnage et créer des obstacles franchissables
void move_track(int row, int col, char board[row][col]) {
    int start_row = 1;
    int end_row = 2;
    int player_column = 4;  // Colonne du personnage

    for (int i = start_row; i < end_row; i++) {
        for (int j = 0; j < col - 1; j++) {
            if (j < player_column - 1 || j > player_column + 1 || i < PLAYER_ROW - 3 || i > PLAYER_ROW + 3) {
                board[i][j] = board[i][j + 1];
            }
        }
    }

    // Ajouter des obstacles dans la dernière colonne
    for (int i = start_row; i <= end_row; i++) {
        if (rand() % 15 == 0) {
            board[i][col - 1] = 'O';
        } else {
            board[i][col - 1] = ' ';
        }
    }

    for (int i = row-1 ; i >= row-2; i--) {
        if (rand() % 20 == 0) {
            board[i][col - 1] = 'X';
        } else {
            board[i][col - 1] = ' ';
        }
    }
}



void Afficher_la_musique(char *fichier_audio) {
    pid_musique = fork(); // Créer un nouveau processus
    if (pid_musique == 0) { // Dans le processus fils
        // Rediriger la sortie standard et l'erreur standard vers /dev/null
        freopen("/dev/null", "w", stdout);  
        freopen("/dev/null", "w", stderr);  
        
        // Exécuter la commande pour jouer le fichier audio
        execlp("play", "play", "-v", "1.0", "-t", "mp3", fichier_audio, "repeat", "10000", NULL);
        
        // Si execlp échoue, sortir avec un code d'erreur
        exit(1);
    } else if (pid_musique < 0) {
        // En cas d'erreur lors de fork
        perror("Erreur lors de la création du processus");
    }
}

// Fonction pour arrêter la musique
void stop_music() {
    if (pid_musique > 0) { // Vérifie si un processus a été lancé
        kill(pid_musique, SIGKILL); // Arrête le processus fils
        pid_musique = -1; // Réinitialise la variable PID
    } else {
        printf("Aucune musique en cours de lecture.\n");
    }
}


void clear_character(int row, int col, char board[row][col], int pos, int height) {
    for (int i = height; i < height + 6; i++) {
        for (int j = pos - 2; j <= pos + 2; j++) {
            if (i >= 0 && i < row && j >= 0 && j < col) {
                board[i][j] = ' ';
            }
        }
    }
}


// Fonction pour gérer le saut amélioré du personnage
void jump_character(int row, int col, char board[row][col], enum Player_class player, int pos) {
    int initial_height = 1;  // Hauteur initiale
    int jump_heights[] = {initial_height + 1, initial_height + 2, initial_height + 3};  // Étapes de saut
    int num_stages = sizeof(jump_heights) / sizeof(jump_heights[0]);

    // Monter progressivement
    for (int i = 0; i < num_stages; i++) {
        clear_character(row, col, board, pos, initial_height);
        
        // Vérifier la collision avec un obstacle pendant le saut
        if (board[jump_heights[i]][pos] == 'O' || board[jump_heights[i]][pos] == 'X') {
            printf("Collision détectée ! Le saut est interrompu.\n");
            break;  // Arrête le saut en cas de collision
        }

        // Placer le personnage à la hauteur de saut actuelle
        switch (player) {
            case Paladin:
                board[jump_heights[i] + 2][pos] = 'O';
                board[jump_heights[i] + 1][pos] = '\\';
                board[jump_heights[i] + 1][pos + 1] = '|';
                board[jump_heights[i] + 3][pos - 1] = '_';
                board[jump_heights[i] + 3][pos] = '_';
                board[jump_heights[i] + 1][pos - 1] = '0';
                board[jump_heights[i]][pos - 1] = '/';
                board[jump_heights[i]][pos + 1] = '\\';
                break;

            case Druid:
                board[jump_heights[i] + 2][pos + 1] = '\\';
                board[jump_heights[i] + 2][pos - 1] = '/';
                board[jump_heights[i] + 2][pos] = 'O';
                board[jump_heights[i] + 1][pos] = '\\';
                board[jump_heights[i] + 1][pos + 1] = '|';
                board[jump_heights[i] + 3][pos] = '_';
                board[jump_heights[i] + 1][pos + 1] = '0';
                board[jump_heights[i]][pos - 1] = '/';
                board[jump_heights[i]][pos + 1] = '\\';
                break;

            case Berserker:
                board[jump_heights[i] + 2][pos + 1] = '_';
                board[jump_heights[i] + 2][pos - 1] = '|';
                board[jump_heights[i] + 2][pos + 1] = '|';
                board[jump_heights[i] + 4][pos] = '|';
                board[jump_heights[i] + 5][pos] = 'O';
                board[jump_heights[i] + 4][pos + 1] = '\\';
                board[jump_heights[i] + 4][pos - 1] = '/';
                board[jump_heights[i] + 3][pos + 1] = '/';
                board[jump_heights[i] + 3][pos - 1] = '\\';
                board[jump_heights[i] + 2][pos] = 'O';
                board[jump_heights[i] + 1][pos] = '\\';
                board[jump_heights[i] + 1][pos + 1] = '|';
                board[jump_heights[i] + 3][pos] = '_';
                board[jump_heights[i] + 1][pos + 1] = '*';
                board[jump_heights[i]][pos - 1] = '/';
                board[jump_heights[i]][pos + 1] = '\\';
                break;
        }

        print_board(row, col, board);  // Afficher le plateau mis à jour
        usleep(100000);  // Pause pour la durée du saut

        // Effacer le personnage de la position actuelle avant de redescendre
        clear_character(row, col, board, pos, jump_heights[i]);
    }

    // Redescendre progressivement
    for (int i = num_stages - 1; i >= 0; i--) {
        // Redessiner le personnage pour chaque étape de descente
        switch (player) {
            case Paladin:
                board[jump_heights[i] + 2][pos] = 'O';
                board[jump_heights[i] + 1][pos] = '\\';
                board[jump_heights[i] + 1][pos + 1] = '|';
                board[jump_heights[i] + 3][pos - 1] = '_';
                board[jump_heights[i] + 3][pos] = '_';
                board[jump_heights[i] + 1][pos - 1] = '0';
                board[jump_heights[i]][pos - 1] = '/';
                board[jump_heights[i]][pos + 1] = '\\';
                break;
                
            case Druid:
                board[jump_heights[i] + 2][pos + 1] = '\\';   // Bras droit
                board[jump_heights[i] + 2][pos - 1] = '/';    // Bras gauche
                board[jump_heights[i] + 2][pos] = 'O';        // Tête
                board[jump_heights[i] + 1][pos] = '\\';       // Corps
                board[jump_heights[i] + 1][pos + 1] = '|';    // Bras droit
                board[jump_heights[i] + 3][pos] = '_';        // Pieds
                board[jump_heights[i] + 1][pos + 1] = '0';    // Main droite
                board[jump_heights[i]][pos - 1] = '/';        // Pied gauche
                board[jump_heights[i]][pos + 1] = '\\';       // Pied droit
                break;
                
            case Berserker:
                board[jump_heights[i] + 2][pos + 1] = '_';   // Bras droit
                board[jump_heights[i] + 2][pos - 1] = '|';   // Bras gauche
                board[jump_heights[i] + 2][pos + 1] = '|';   // Bras droit (encore)
                board[jump_heights[i] + 4][pos] = '|';       // Ceinture
                board[jump_heights[i] + 5][pos] = 'O';       // Tête
                board[jump_heights[i] + 4][pos + 1] = '\\';  // Jambe droite
                board[jump_heights[i] + 4][pos - 1] = '/';   // Jambe gauche
                board[jump_heights[i] + 3][pos + 1] = '/';   // Bras droit
                board[jump_heights[i] + 3][pos - 1] = '\\';  // Bras gauche
                board[jump_heights[i] + 2][pos] = 'O';       // Corps
                board[jump_heights[i] + 1][pos] = '\\';      // Torse
                board[jump_heights[i] + 1][pos + 1] = '|';   // Torse
                board[jump_heights[i] + 3][pos] = '_';       // Pieds
                board[jump_heights[i] + 1][pos + 1] = '*';   // Main droite
                board[jump_heights[i]][pos - 1] = '/';       // Pied gauche
                board[jump_heights[i]][pos + 1] = '\\';      // Pied droit
                break;
        }
        
        print_board(row, col, board);  // Afficher la grille mise à jour pendant la descente
        usleep(100000);  // Pause pour la durée de la descente

        // Effacer le personnage pour chaque étape de la descente
        clear_character(row, col, board, pos, jump_heights[i]);
    }

    // Restaurer le personnage à la position initiale
    init_character(row, col, board, player);
    print_board(row, col, board);
}












int main () {
  // Global Variable For Timers
  gravity_time = 0.1 ;
  acquisition_time = 50 ;
  enum Player_class Class;
  char *tile_audio = "MAIN.mp3";
  char *level_audio = "Level.mp3";


  // Init Board
  char board[10][210] ;
  int row = 10 ;
  int col = 210;
  int row_obs = 0;
  int col_obs = 45;
  int pos = 6 ;
  int n = 0 ;
  time_t last_refresh, now; 
  
  Afficher_la_musique(tile_audio);
  Display_Title();
  system("clear");
  Class = Choice_the_character(Class);
  stop_music();

  system("clear");
  Afficher_la_musique(level_audio);
  init_board(row, col, board);
  init_character(row, col, board,Class);
  print_board(row, col, board) ;
  time(&last_refresh) ;

  //Game Loop
  while (n != -1) {
        // Déplacer la ligne de course
        move_track(row, col, board);
        // Afficher l'écran mis à jour
        print_board(row, col, board);
        // Gestion du temps de la gravité ou autres éléments de jeu
        time(&now);
        if (now - last_refresh > gravity_time) {
            gravity(row, col, board);
            print_board(row, col, board);
            last_refresh = now;
        }

        // Lire l'entrée de l'utilisateur (si nécessaire)
        int n = getch();
        if (n == 'w') {  // 'w' pour sauter
            jump_character(row, col, board, Class, pos);
        } 
     
  }
}
