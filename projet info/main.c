#include "skin.h"
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <termios.h>
#include<sys/types.h>
#include <signal.h>
#include <sys/time.h>  
#include <string.h>
#include <math.h>

#define SCROLL_DELAY 50000  
#define JUMP_DELAY 100000   
#define POWER_UP_CHANCE 20  // Chance d'apparition d'un power-up (1-100)

#define clear() printf("\033[H\033[J")
#define SCREEN_WIDTH 50
#define SCREEN_HEIGHT 10
#define PLAYER_COL 4
#define MAX_LIVES 1
#define INVINCIBILITY_DURATION 2  // Seconds of invincibility after hit

typedef struct {
    bool game_over;
    int score;
    int lives;
    int current_speed;
    bool is_invincible;
    time_t invincibility_start;
} GameState;


typedef struct {
    bool is_jumping;
    int jump_height;
    int max_jump_height;
    bool is_falling;
    int ground_level;
} PlayerJumpState;


typedef enum { NONE, SPEED, SHIELD, DOUBLE_SCORE } PowerUpType;

typedef struct {
    PowerUpType type;
    int position;
} PowerUp;

typedef struct {
    bool is_jumping;
    int jump_stage;
    int initial_height;
    int jump_direction; 
} JumpState;

typedef struct {
    int initial_scroll_delay;    // Délai initial en microsecondes
    int current_scroll_delay;    // Délai actuel
    int min_scroll_delay;        // Délai minimum
    int acceleration_rate;       // Taux d'accélération
    int obstacle_frequency;      // Fréquence d'apparition des obstacles (1-10)
    int score;                   // Score du joueur
} GameSettings;

/// Variables Globales pour gestion du temps : vous pouvez ajuster selon besoins.
int acquisition_time ;
double gravity_time ;
pid_t pid_musique = -1;

#define DIFFICULTY_EASY 1
#define DIFFICULTY_MEDIUM 2
#define DIFFICULTY_HARD 3

void init_game_state(GameState* state) {
    state->score = 0;
    state->lives = MAX_LIVES;
    state->current_speed = 20000;  // Faster initial speed
    state->is_invincible = false;
}

void display_game_stats(GameState* state) {
    printf("\033[H\033[20;0H"); // Move cursor to bottom of screen
    printf("\033[K"); // Clear line
    printf("Score: %d | Lives: %d | Speed: %d\n", 
           state->score, state->lives, state->current_speed);
}

// Fonction pour gérer la fin de partie
void handle_game_over(GameSettings* settings) {
    clear();
    printf("\n\n=== GAME OVER ===\n");
    printf("Score final: %d\n", settings->score);
    printf("Appuyez sur une touche pour continuer...\n");
    while (getchar() != '\n');
}

void handle_jump(PlayerJumpState* jump_state) {
    if (jump_state->is_jumping) {
        if (!jump_state->is_falling) {
            jump_state->jump_height++;
            if (jump_state->jump_height >= jump_state->max_jump_height) {
                jump_state->is_falling = true;
            }
        } else {
            jump_state->jump_height--;
            if (jump_state->jump_height <= 0) {
                jump_state->is_jumping = false;
                jump_state->is_falling = false;
                jump_state->jump_height = 0;
            }
        }
    }
}



/// /!\ Ne pas toucher à cette fonction, je ne la comprend pas.
// Gets a char from STDIN
// Returns immediatly, even if STDIN is empty
// in which case it returns ?
// Fonction pour récupérer une touche sans bloquer
int getch() {
    struct termios oldattr, newattr;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~ICANON; // Mode non-canonique
    newattr.c_lflag &= ~ECHO;   // Désactiver l'écho
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);

    struct pollfd mypoll = { STDIN_FILENO, POLLIN | POLLPRI };
    int ch = -1;
    if (poll(&mypoll, 1, 50) > 0) { // 50ms timeout
        ch = getchar();
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

void init_game_settings(GameSettings* settings, int difficulty) {
    switch(difficulty) {
        case DIFFICULTY_EASY:
            settings->initial_scroll_delay = 50000;  // Plus rapide
            settings->min_scroll_delay = 20000;
            settings->acceleration_rate = 500;       // Accélération plus rapide
            settings->obstacle_frequency = 2;        // Moins d'obstacles
            break;
        case DIFFICULTY_MEDIUM:
            settings->initial_scroll_delay = 30000;
            settings->min_scroll_delay = 10000;
            settings->acceleration_rate = 1000;
            settings->obstacle_frequency = 3;
            break;
        case DIFFICULTY_HARD:
            settings->initial_scroll_delay = 20000;
            settings->min_scroll_delay = 5000;
            settings->acceleration_rate = 2000;
            settings->obstacle_frequency = 4;
            break;
    }
    settings->current_scroll_delay = settings->initial_scroll_delay;
    settings->score = 0;
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
        if (board[i][pos] == 'I' && board[i - 1][pos] == ' ') {
            board[i][pos] = ' ';
            board[i - 1][pos] = 'I';
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



void move_track(char board[SCREEN_HEIGHT][SCREEN_WIDTH], GameState* game_state) {
    // Déplacement du terrain vers la gauche
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
    for (int j = 0; j < SCREEN_WIDTH - 1; j++) {
            board[i][j] = board[i][j + 1];
        
    }
    board[i][SCREEN_WIDTH - 1] = ' '; // Garder la dernière colonne vide
}

    // Maintien du sol
    for (int j = 0; j < SCREEN_WIDTH; j++) {
        board[0][j] = 'D';
    }

    // Génération d'obstacles
    static int obstacle_gap = 0;
    obstacle_gap--;

    if (obstacle_gap <= 0) {
        if (rand() % 4 == 0) {  // 25% de chances de générer un obstacle
            for (int h = 1; h <= 2; h++) {
                if (h < SCREEN_HEIGHT) {
                    board[h][SCREEN_WIDTH - 1] = 'I';
                }
            }
        }
        obstacle_gap = 5 + (rand() % 3);  // Écart entre les obstacles
    }

    game_state->score++;
}





void jump_character(char board[SCREEN_HEIGHT][SCREEN_WIDTH], enum Player_class player, PlayerJumpState* jump_state) {
    // Effacer l'ancienne position du personnage
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        for (int j = PLAYER_COL - 2; j <= PLAYER_COL + 2; j++) {
            if (j >= 0 && j < SCREEN_WIDTH && i > 0) {
                if (board[i][j] != 'I') {
                      board[i][j] = ' ';
                }
                }
        }
    }

    // Calculer la nouvelle position verticale
    int current_height = jump_state->ground_level + jump_state->jump_height;
    
    // Dessiner le personnage à la nouvelle position
    init_character(SCREEN_HEIGHT, SCREEN_WIDTH, board, player, PLAYER_COL, current_height);
}



void Afficher_la_musique(char *fichier_audio) {
    pid_musique = fork(); // Créer un nouveau processus
    if (pid_musique == 0) { // Dans le processus fils
        // Rediriger la sortie standard et l'erreur standard vers /dev/null
        freopen("/dev/null", "w", stdout);  
        freopen("/dev/null", "w", stderr);  
        
        // Exécuter la commande mpg123 pour jouer le fichier audio
        execlp("mpg123", "mpg123", "-v", "1.0", fichier_audio,"loop","-1",NULL);
        
        // Si execlp échoue, sortir avec un code d'erreur
        exit(1);
    } else if (pid_musique < 0) {
        // En cas d'erreur lors de fork
        perror("Erreur lors de la création du processus");
    }
}

void game_over_musique() {
    pid_musique = fork(); // Créer un nouveau processus
    if (pid_musique == 0) { // Dans le processus fils
        // Rediriger la sortie standard et l'erreur standard vers /dev/null
        freopen("/dev/null", "w", stdout);  
        freopen("/dev/null", "w", stderr);  
        
        // Exécuter la commande mpg123 pour jouer le fichier audio
        execlp("mpg123", "mpg123", "-v", "1.0", "game_over.mp3", NULL);
        
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
    for (int i = height; i < height + 6; i++) { // Supposant que la hauteur max est 6
        for (int j = pos - 2; j <= pos + 2; j++) { // Supposant que la largeur max est 2
            if (i >= 0 && i < row && j >= 0 && j < col) {
                board[i][j] = ' '; // Effacer la case
            }
        }
    }
}




int choose_difficulty() {
    clear();
    printf("\n=== Choisissez la difficulté ===\n");
    printf("1. Facile (Débutant)\n");
    printf("2. Moyen (Intermédiaire)\n");
    printf("3. Difficile (Expert)\n");
    printf("\nVotre choix (1-3): ");
    
    int choice;
    scanf("%d", &choice);
    while(getchar() != '\n'); // Vider le buffer
    
    if(choice < 1 || choice > 3) {
        choice = DIFFICULTY_MEDIUM; // Par défaut
    }
    return choice;
}


bool check_collision(char board[SCREEN_HEIGHT][SCREEN_WIDTH], PlayerJumpState* jump_state) {
    
    return false;
}




void generate_power_up(int row, int col, char board[row][col], PowerUp *power_up) {
    if (rand() % 100 < POWER_UP_CHANCE) {
        // Générer un power-up sur une position aléatoire
        int pos = col - 1;  // Dernière colonne
        int height = rand() % row;
        int type = rand() % 3;
        
        power_up->type = type;
        power_up->position = pos;
        
        // Choisir le type de power-up
        if (type == 0) {
            board[height][pos] = 'S';  // Vitesse
        } else if (type == 1) {
            board[height][pos] = 'B';  // Bouclier
        } else {
            board[height][pos] = 'D';  // Double Score
        }
    }
}

void collect_power_up(GameSettings *settings, PowerUp *power_up) {
    // Si le personnage est sur le power-up
    if (power_up->position == 4 && rand() % 10 == 0) {
        switch (power_up->type) {
            case SPEED:
                settings->current_scroll_delay -= 2000;  // Augmenter la vitesse
                break;
            case SHIELD:
                // Activer le bouclier
                break;
            case DOUBLE_SCORE:
                settings->score *= 2;  // Doubler le score
                break;
            default:
                break;
        }
        power_up->type = NONE;  // Désactiver le power-up après collecte
    }
}

void next_level(GameSettings *settings, GameState *game_state) {
    if (settings->score > 0 && settings->score % 50 == 0) {
        int level = settings->score / 50;
        clear();
        printf("\n=== NIVEAU %d ===\n", level);
        printf("Préparez-vous pour un défi plus grand !\n");
        usleep(2000000); // Pause de 2 secondes entre les niveaux
        settings->acceleration_rate += 100;  // Augmenter la vitesse d'accélération
        settings->obstacle_frequency++;     // Ajouter plus d'obstacles
    }
}



void save_game(const char* filename, GameSettings* settings, GameState* game_state) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erreur lors de l'ouverture du fichier pour sauvegarder.\n");
        return;
    }

    // Sauvegarde des paramètres de jeu
    fprintf(file, "Le score du jeu obtenu lors de la précédente partie est : %d", game_state->score);
    
    fclose(file);
    printf("GAME OVER DOMMAGE !!!!\n");
    printf("Jeu sauvegardé avec succès.\n");
}

void load_game(const char* filename, GameSettings* settings, GameState* game_state) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Aucune sauvegarde trouvée. Nouveau jeu commencé.\n");
        return;
    }

    // Chargement des paramètres sauvegardés
    fscanf(file, "%d", &game_state->score);
    fscanf(file, "%d", &settings->initial_scroll_delay);
    fscanf(file, "%d", &settings->obstacle_frequency);

    // Mise à jour des autres paramètres basés sur la difficulté
    settings->current_scroll_delay = settings->initial_scroll_delay;

    fclose(file);
    printf("Jeu chargé avec succès.\n");
}


void ask_for_player_name(char* filename) {
    char name[100];  // Tableau pour le nom du joueur

    clear();
    printf("=== Entrez votre nom ===\n");
    printf("Nom du joueur: ");
    fgets(name, sizeof(name), stdin);
    
    // Enlever le '\n' à la fin du nom, si présent
    name[strcspn(name, "\n")] = 0;
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erreur lors de la sauvegarde du nom.\n");
        return;
    }
    
    fprintf(file, "Nom du joueur: %s\n", name);
    fclose(file);

    printf("Nom du joueur sauvegardé: %s\n", name);
    usleep(1000000);  // Pause d'une seconde avant de retourner au menu
}


int choose_game_action() {
    clear();
    printf("=== Choisissez une action ===\n");
    printf("1. Nouvelle Partie\n");
    printf("2. Charger une Partie Sauvegardée\n");
    printf("3. Quitter\n");
    printf("\nVotre choix (1-3): ");
    
    int choice;
    scanf("%d", &choice);
    while(getchar() != '\n'); // Vider le buffer
    
    if (choice < 1 || choice > 3) {
        choice = 1; // Par défaut, commencer une nouvelle partie
    }

    return choice;
}




int main() {
    int row = 10, col = 50;
    srand(time(NULL));
    
    char board[SCREEN_HEIGHT][SCREEN_WIDTH];
    GameState game_state;
    PlayerJumpState jump_state = {
        .is_jumping = false,
        .jump_height = 0,
        .max_jump_height = 4,
        .is_falling = false,
        .ground_level = 1
    };
    int pos = 6;
    int n = 0;
    time_t last_refresh, now;
    GameSettings settings;

    init_board(SCREEN_HEIGHT, SCREEN_WIDTH, board);
    init_game_state(&game_state);
    enum Player_class Class;
    Afficher_la_musique("MAIN.mp3");
    Display_Title();

    char filename[] = "player_data.txt";  
    char player_name[100]; 

    ask_for_player_name(filename);
    sleep(10);
    
    int action = choose_game_action();

    if (action == 1) {
        printf("Nouvelle Partie commence...\n");
        init_game_state(&game_state);
        int difficulty = choose_difficulty();
        init_game_settings(&settings, difficulty);
    } 
    else if (action == 2) {
        printf("Chargement de la partie...\n");
        load_game("save_game.txt", &settings, &game_state);
    } 
    else {
        printf("Quitter le jeu.\n");
        return 0;
    }

    int difficulty = choose_difficulty();
    init_game_settings(&settings, difficulty);
    
    system("clear");
    Class = Choice_the_character(Class);
    stop_music();

    system("clear");
    Afficher_la_musique("Level.mp3");

    init_character(SCREEN_HEIGHT, SCREEN_WIDTH, board, Class, PLAYER_COL, 1);
    print_board(row, col, board);
    time(&last_refresh);

    int input;
    clock_t last_update = clock();

    while (input != 'q') {
        move_track(board, &game_state);
          
        time(&now);
        if (now - last_refresh > gravity_time) {
            gravity(row, col, board);
            print_board(row, col, board);
            last_refresh = now;
        }

        handle_jump(&jump_state);
        
        // Mise à jour de la position du personnage
        jump_character(board, Class, &jump_state);
        
        if (check_collision(board, &game_state)) {
            break;
        }
        
        input = getch();
        if (input == 'w' && !jump_state.is_jumping) {
            jump_state.is_jumping = true;
            jump_state.jump_height = 0;
            jump_state.is_falling = false;
        }

        print_board(SCREEN_HEIGHT, SCREEN_WIDTH, board);
        display_game_stats(&game_state);
        
        usleep(50000);  // Smooth game loop timing
    }
    stop_music();
    game_over_musique();
    sleep(4);
    clear();   
    save_game("save_game.txt", &settings, &game_state); 
    sleep(6);
    clear();    
    return 0;
    }
    
    
