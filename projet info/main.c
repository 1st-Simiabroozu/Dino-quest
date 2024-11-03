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
  int pos = 4 ;

  for (int i = 1; i < row; i++) {
    
    for (int j = pos+1; j < col; j++)
      if (board[i][j] == 'O') {
	      board[i][j] = ' ' ;

        if (i > 1){
	        board[i-1][j] = 'O' ;
        }
      }
  }
}


// Initialize the board.
void init_board(int row, int col, char board[row][col]) {
  for (int j = 0; j < col; j++) {
    board[0][j] = 'D' ;
  }
  
  for (int i = 1; i < row; i++) {
    
    for (int j = 0; j < col; j++){
      board[i][j] = ' ' ;
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

int main () {
  // Global Variable For Timers
  gravity_time = 0.1 ;
  acquisition_time = 50 ;
  enum Player_class Class;
  char *tile_audio = "MAIN.mp3";
  char *level_audio = "Level.mp3";


  // Init Board
  char board[10][150] ;
  int row = 10 ;
  int col = 150;
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
    int n  = getch () - 48;
    
    if (0 <= n && n < col) {
      board[row_obs + 1][n+pos] = 'O' ;
      print_board(row, col, board) ;
    }



    time(&now) ;
    
    if (now - last_refresh > gravity_time) {
      gravity(row, col, board) ;
      print_board(row, col, board) ;
      last_refresh = now ;
    }

    

  } 
     
}