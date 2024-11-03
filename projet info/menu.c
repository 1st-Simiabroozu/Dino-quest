#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include "skin.h"

void set_terminal_mode(bool enable) {
    struct termios t;
    if (tcgetattr(STDIN_FILENO, &t) == -1) {
        perror("Erreur lors de la récupération des attributs du terminal");
        return;
    }

    if (!enable) {
        t.c_lflag &= ~(ICANON | ECHO);  // Désactive le mode canonique et l'écho
    } else {
        t.c_lflag |= (ICANON | ECHO);   // Active le mode canonique et l'écho
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &t) == -1) {
        perror("Erreur lors de la configuration des attributs du terminal");
    }
}

void set_nonblocking_input() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

bool key_pressed() {
    struct timeval tv = {0, 0};  // Pas de délai d'attente
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    return select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv) > 0;
}

void Display_Title() {
    int col = 40;
    int row = 100;
    bool s = false;
    bool running = true;

    char title[100][100] = {"DINO QUEST", "CREDIT : DJOSS MAEZE SANGWA", "APPUYEZ SUR START"};
    char p[col][row];
    int pos_y = 15;
    int pos_x = 27;
    int move = 0;  
    
    set_nonblocking_input();

    while (running)
    {
    s = false;
    move = 0;
    system("clear");
    if (key_pressed()) {
            int c = getchar();  
            if (c == '\n' || c == '\n') {
                running = false; 
                break; 
            }
        }
    for (int i = 0; i < col; i++) {
        for (int j = 0; j < row; j++) {
            if (i < 18 && i != 1 || i > 22 && i != 24) {
                p[i][j] = ' ';
                if (i == 32 || (i == 33 && j >= 6)) {
                    p[32][6] = '/'; 
                    p[32][7] = '\\'; 
                    p[33][6] = '\\'; 
                    p[33][7] = '/'; 
                }
                if (i == 6 || (i == 7 && j >= 94)) {
                    p[6][94] = '/'; 
                    p[6][95] = '\\'; 
                    p[7][94] = '\\'; 
                    p[7][95] = '/'; 
                }
            } else {
                if (i == 18 || i == 19 || i == 21 || i == 22) {
                    if (j >= 27 && j <= 78) {
                        p[i][j] = '=';
                    } else {
                        p[i][j] = ' ';
                    }             
                } else if (i == 20) {
                    if (j >= 48 && j <= 57) {
                        p[i][j] = title[0][j - 48];
                    } else {
                        p[i][j] = ' ';
                    }
                } else if (i == 1) {
                    int len_credit = strlen(title[1]);
                    if (j >= 1 && j <= len_credit) {
                        p[i][j] = title[1][j - 1];
                    }
                } else if (i == 24) {
                    int len_start = strlen(title[2]);
                    if (j >= 44 && j <= 61) {
                        p[i][j] = title[2][j - 44];
                    } else {
                        p[i][j] = ' ';
                    }
                }
            }
        }
    }

    system("clear");
    for (int i = 0; i < col; i++) {
        for (int j = 0; j < row; j++) {
            printf("%c ", p[i][j]);
        }
        printf("\n");
    }

    while (s != true) {
        if (key_pressed()) {
            int c = getchar();  
            if (c == '\n' || c == '\n') {
                running = false; 
                break; 
            }
        }
        if (move > 0) {
            printf("\033[%d;%dH ", pos_y + 1, (pos_x + move - 1) * 2);
            printf("\033[%d;%dH ", pos_y + 2, (pos_x + move - 1) * 2);
            printf("\033[%d;%dH ", pos_y + 2, (pos_x + move) * 2);
            printf("\033[%d;%dH ", pos_y + 3, (pos_x + move) * 2);
            printf("\033[%d;%dH ", pos_y + 3, (pos_x + move - 1) * 2);
        }

        if (move < 53) {
            printf("\033[%d;%dHO", pos_y + 1, (pos_x + move) * 2);
            printf("\033[%d;%dH|", pos_y + 2, (pos_x + move) * 2);
            printf("\033[%d;%dH-", pos_y + 2, (pos_x + 1 + move) * 2);
            printf("\033[%d;%dH\\", pos_y + 3, (pos_x + 1 + move) * 2);
            printf("\033[%d;%dH|", pos_y + 3, (pos_x + move) * 2);
            move++;
        }

        

        fflush(stdout);
        if (move == 53)
        {
            s = true;
        }
        if (key_pressed()) {
            int c = getchar();  
            if (c == '\n' || c == '\n') {
                running = false; 
                break; 
            }
        }
    
        usleep(100000);  // Ajuste le délai pour contrôler la vitesse du personnage
    }
    if (key_pressed()) {
            int c = getchar();  
            if (c == '\n' || c == '\n') {
                running = false; 
                break; 
            }
        }
    

    if (running == false)
    {
        break;
    }
    
    
    
    sleep(5);
  }
}


enum Player_class Choice_the_character(enum Player_class pl) {
    int col = 40;
    int row = 150;
    char choice_char[] = "CHOISISSEZ UN PERSONNAGE";
    char paladin[2][1000] = {"PALADIN", "Appuyez sur A"};
    char druid[2][1000] = {"DRUID", "Appuyez sur B"};
    char berserker[2][1000] = {"BERSERKER", "Appuyez sur Z"};
    char p[col][row];

    // Initialisation de la grille avec des espaces
    for (int i = 0; i < col; i++) {
        for (int j = 0; j < row; j++) {
            p[i][j] = ' ';
        }
    }

    // DRUID
    p[20][40] = 'O';
    p[20 + 1][40] = '\\';
    p[20 + 1][41] = '|';
    p[20 + 2][40] = '_';
    p[20 + 1][40 - 1] = '0';
    p[20 - 1][40 - 1] = '/';
    p[20 - 1][40 + 1] = '\\';
    
    for (int i = 0; i < strlen(druid[0]); i++)
    {
        p[17][i + 38] = druid[0][i];
    }

    for (int i = 0; i < strlen(druid[1]); i++)
    {
        p[25][i + 34] = druid[1][i];
    }


    //BERSERKER
    p[20 + 2][78 + 1] = '\\' ;
    p[20 + 2][78 - 1] = '/' ;
    p[20 - 2][78] = 'O' ;
    p[20+1][78] = '\\' ;
    p[20 - 1][78] = '_' ;
    p[20][78] = '0' ;
    p[20][78-1] = '/' ;
    p[20][78+1] = '\\' ;

   
    for (int i = 0; i < strlen(berserker[0]); i++)
    {
        p[17][i + 74] = berserker[0][i];
    }

    for (int i = 0; i < strlen(berserker[1]); i++)
    {
        p[25][i + 72] = berserker[1][i];
    }

    //PALADIN
    p[20][120 + 1] = '_' ;
    p[20][120 - 1] = '|' ;
    p[20][120 + 1] = '|' ;
    p[20 - 2][120] = '|' ;
    p[20 - 3][120] = 'O' ;
    p[20 - 2][120 + 1] = '\\' ;
    p[20 - 2][120 - 1] = '/' ;
    p[20 - 1][120 + 1] = '/' ;
    p[20 - 1][120 - 1] = '\\' ;
    p[20][120] = 'O' ;
    p[20 + 1][120] = '\\' ;
    p[20 + 1][120 + 1] = '|' ;
    p[20 - 1][120] = '_' ;
    p[20 + 1][120 + 1] = '*' ;
    p[20 + 2][120 - 1] = '/' ;
    p[20 + 2][120 + 1] = '\\' ;

    for (int i = 0; i < strlen(paladin[0]); i++)
    {
        p[16][i + 117] = paladin[0][i];
    }

    for (int i = 0; i < strlen(paladin[1]); i++)
    {
        p[25][i + 114] = paladin[1][i];
    }
  
    //TITRE 
    for (int j = 0; j < strlen(choice_char); j++) {
        p[10][67 + j] = choice_char[j];
    }

    // Affichage de la grille
    for (int i = 0; i < col; i++) {
        for (int j = 0; j < row; j++) {
            printf("%c", p[i][j]);
        }
        printf("\n");
    }

    fflush(stdout);
    while (true)
    {
        if (key_pressed()) {
            int c = getchar();  
            if (c == 'a' || c == 'A') {
                pl = Paladin;
                return pl;
            }
            if (c == 'b' || c == 'B') {
                pl = Druid;
                return pl;
            }
            if (c == 'z' || c == 'Z') {
                pl = Berserker;
                return pl;
            }
        }
    }
    
}
