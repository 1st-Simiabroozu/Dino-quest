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

#define clear() printf("\033[H\033[J")
#define SCREEN_WIDTH 50
#define SCREEN_HEIGHT 10
#define PLAYER_COL 4
#define PLAYER_ROW 1
#define MAX_LIVES 3
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
    int score;          // Score du joueur
} GameSettings;

typedef struct {
    char* pseudo;  // Nom du joueur
    int score;     // Score du joueur
} profile;

void save_score(char* pseudo, int score) {
    FILE* f = fopen("score", "r");

    if (f == NULL) {
        // Si le fichier n'existe pas, créez-le et ajoutez le premier score
        f = fopen("score", "w");
        fprintf(f, "%s - %d\n", pseudo, score);
        fclose(f);
    } else {
        // Lire les scores existants
        fclose(f);
        profile* profiles;
        int lines = read_file(&profiles);

        // Insérer le nouveau score à la bonne position
        int i = 0;
        while (i < lines && score < profiles[i].score) i++;

        for (int j = lines; j > i; j--) {
            profiles[j] = profiles[j - 1];
        }

        profiles[i] = (profile){strdup(pseudo), score};  // Copier pseudo dynamiquement

        // Réécrire les scores dans le fichier
        f = fopen("score", "w");
        for (int j = 0; j < lines + 1; j++) {
            fprintf(f, "%s - %d\n", profiles[j].pseudo, profiles[j].score);
            free(profiles[j].pseudo);  // Libérer la mémoire de chaque pseudo
        }
        fclose(f);

        free(profiles);  // Libérer la mémoire du tableau de profils
    }
}

int read_file(profile** profiles) {
    FILE* f = fopen("score", "r");
    if (f == NULL) return 0;

    char* t_pseudo = malloc(sizeof(char) * 13);
    int t_score;
    int lines = 0;

    while (fscanf(f, "%s - %d\n", t_pseudo, &t_score) == 2) lines++;
    fclose(f);

    // Allocation de mémoire pour les profils
    *profiles = malloc(sizeof(profile) * (lines + 1));

    // Relire le fichier et remplir les profils
    f = fopen("score", "r");
    for (int i = 0; fscanf(f, "%s - %d\n", t_pseudo, &t_score) == 2 && i < lines; i++) {
        (*profiles)[i] = (profile){strdup(t_pseudo), t_score};  // Copie dynamique du pseudo
    }
    fclose(f);

    free(t_pseudo);  // Libérer le buffer temporaire
    return lines;    // Retourner le nombre de lignes
}


/// Variables Globales pour gestion du temps : vous pouvez ajuster selon besoins.
int acquisition_time ;
double gravity_time ;
pid_t pid_musique = -1;

#define DIFFICULTY_EASY 'a'
#define DIFFICULTY_MEDIUM 'b'
#define DIFFICULTY_HARD 'c'

void init_game_state(GameState* state) {
    state->score = 0;
    state->lives = MAX_LIVES;
    state->current_speed = 50000;  // Faster initial speed
    state->is_invincible = false;
}

void display_game_stats(GameState* state) {
    printf("\033[H\033[20;0H"); // Move cursor to bottom of screen
    printf("\033[K"); // Clear line
    printf("Score: %d | Lives: %d | Speed: %d\n", state->score, state->lives, state->current_speed);
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
                if (board[i][j] != 'I' && board[i][j] != 'D') {
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
        execlp("mpg123", "mpg123", "--loop", "-1", "-q", fichier_audio, NULL);

        
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
    for (int i = height; i < height + 6; i++) { 
        for (int j = pos - 2; j <= pos + 2; j++) { 
            if (i >= 0 && i < row && j >= 0 && j < col) {
                if (board[i][j] != 'I' && board[i][j] != 'D') { 
                    board[i][j] = ' '; 
                }
            }
        }
    }
}








bool check_collision_precise(char board[SCREEN_HEIGHT][SCREEN_WIDTH], 
                             enum Player_class player, 
                             int player_col, 
                             int player_height, 
                             bool is_on_ground,
                             GameState* game_state) {
   
    int offsets[][2] = {
        {0, 0}, {1, 0}, {1, -1}, {1, 1},  // Torse et bras
        {2, -1}, {2, 1}, {2, 0},          // Jambes
    };

    // Si le personnage est au sol, on ajuste les offsets
    if (is_on_ground) {
        offsets[0][0] = 1; offsets[0][1] = 0;  // Pieds
        offsets[1][0] = 1; offsets[1][1] = -1; 
        offsets[2][0] = 1; offsets[2][1] = 1;
        offsets[3][0] = 0; offsets[3][1] = 0;  // Torse
    }

    int count = sizeof(offsets) / sizeof(offsets[0]);

    // Vérification de chaque zone
    for (int i = 0; i < count; i++) {
        int check_row = player_height + offsets[i][0];
        int check_col = player_col + offsets[i][1];

        // Vérifie si les coordonnées sont valides
        if (check_row >= 0 && check_row < SCREEN_HEIGHT && 
            check_col >= 0 && check_col < SCREEN_WIDTH) {
            // Vérifie la collision avec un obstacle
            if (board[check_row][check_col] == 'I' || board[check_row][check_col - 1] == 'I') {
                
                // Vérifie si le joueur est invincible
                if (!game_state->is_invincible || (time(NULL) - game_state->invincibility_start) >= 2) {
                    // Gère la collision
                    if (!game_state->is_invincible) {
                        game_state->lives--;  // Perte de vie
                        game_state->is_invincible = true;  // Active l'invincibilité
                        game_state->invincibility_start = time(NULL);  // Enregistre le moment où l'invincibilité commence

                        // Si plus de vies, fin de jeu
                        if (game_state->lives <= 0) {
                            game_state->game_over = true;
                            return true;
                        }
                        printf("Invincible pendant 2 secondes\n");
                    }
                }

                // Vérifie si l'invincibilité a expiré
                if (game_state->is_invincible && (time(NULL) - game_state->invincibility_start) >= 2) {
                    game_state->is_invincible = false;  // Désactive l'invincibilité après 2 secondes
                }
            }
        }
    }

    return false;  // Pas de collision
}




void next_level(GameSettings *settings, GameState *game_state) {
    
    if (settings->score > 0 && settings->score % 50 == 0) {
        int level = settings->score / 50;
        printf("\n=== NIVEAU %d ===\n", level);
        printf("Préparez-vous pour un défi plus grand !\n");
        usleep(2000000); // Pause de 2 secondes entre les niveaux
        settings->acceleration_rate += 100;  // Augmenter la vitesse d'accélération
        settings->obstacle_frequency++;     // Ajouter plus d'obstacles
    }
}








#define MAX_SCORES 10
#define MAX_NAME_LENGTH 50

typedef struct {
    char name[MAX_NAME_LENGTH];
    int score;
} HighScore;

void save_high_score(const char* filename, const char* player_name, int score) {
    HighScore scores[MAX_SCORES + 1]; 
    int num_scores = 0;

    // Charger les scores existants
    FILE* file = fopen(filename, "r");
    if (file != NULL) {
        while (num_scores < MAX_SCORES && 
               fscanf(file, "%49[^,],%d\n", scores[num_scores].name, &scores[num_scores].score) == 2) {
            num_scores++;
        }
        fclose(file);
    }

    // Ajouter le nouveau score
    strncpy(scores[num_scores].name, player_name, sizeof(scores[num_scores].name) - 1);
    scores[num_scores].name[sizeof(scores[num_scores].name) - 1] = '\0';  // Assurer la terminaison nulle
    scores[num_scores].score = score;
    num_scores++;

    // Trier les scores par ordre décroissant
    for (int i = 0; i < num_scores - 1; i++) {
        for (int j = 0; j < num_scores - i - 1; j++) {
            if (scores[j].score < scores[j + 1].score) {
                HighScore temp = scores[j];
                scores[j] = scores[j + 1];
                scores[j + 1] = temp;
            }
        }
    }

    // Sauvegarder uniquement les meilleurs scores
    file = fopen(filename, "a");
    if (file != NULL) {
        for (int i = 0; i < MAX_SCORES && i < num_scores; i++) {
            fprintf(file, "%s,%d\n", scores[i].name, scores[i].score);
        }
        fclose(file);
    } else {
        perror("Erreur lors de l'ouverture du fichier pour sauvegarder les scores.");
    }
}




void display_high_scores(const char* filename) {
    HighScore scores[MAX_SCORES];
    int num_scores = 0;

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Aucun score enregistré.\n");
        return;
    }

    clear();
    printf("=== TABLEAU DES MEILLEURS SCORES ===\n");
    printf("-------------------------------------\n");

    while (num_scores < MAX_SCORES && 
           fscanf(file, "%49[^,],%d\n", 
           scores[num_scores].name, 
           &scores[num_scores].score) == 2) {
        printf("%d. %s - %d points\n", 
               num_scores + 1, 
               scores[num_scores].name, 
               scores[num_scores].score);
        num_scores++;
    }
    fclose(file);

    printf("\nAppuyez sur une touche pour continuer...");
    getchar();
}







void save_game(const char* filename,char* player_name, GameSettings* settings, GameState* game_state) {
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        printf("Erreur lors de l'ouverture du fichier pour sauvegarder.\n");
        return;
    }

    fprintf(file, "%s, %d", player_name ,game_state->score);
    
    fclose(file);

    save_high_score("high_scores.txt", player_name, game_state->score);

    printf("GAME OVER DOMMAGE !!!! (Si vous voulez éviter les obstacles les plus difficiles maintenez w, mais arriverez-vous à le faire ? :)) )\n");
    printf("Jeu sauvegardé avec succès.\n");
}




void load_game(const char* filename, GameSettings* settings, GameState* game_state) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Aucune sauvegarde trouvée. Nouveau jeu commencé.\n");
        return;
    }

    fscanf(file, "%d", &game_state->score);
    fscanf(file, "%d", &settings->initial_scroll_delay);
    fscanf(file, "%d", &settings->obstacle_frequency);

    settings->current_scroll_delay = settings->initial_scroll_delay;

    fclose(file);
    printf("Jeu chargé avec succès.\n");
}

void get_player_name(char* player_name, int max_length) {
    system("clear");
    char input[max_length];
    
    printf("=== Entrez votre nom ===\n");
    printf("Nom du joueur : ");
    scanf("%s", player_name);

}

char choose_game_action() { 
char action;
bool valid_choice = false;
printf("=== Choisissez une action ===\n"); 
    printf("a. Nouvelle Partie\n"); 
    printf("b. Voir les Meilleurs Scores\n"); 
    printf("c. Quitter\n"); 
    printf("\nVotre choix (a-c): "); 
do
{
    printf("\b \b");; 
    

    scanf(" %c", &action); 

    if (action == 'a' || action == 'b' || action == 'c') { valid_choice = true; } 
    else { }
  
}while (action != 'a' && action != 'b' && action != 'c');

  
return action;


}


char choose_difficulty() {
    clear();
    printf("Pour Sauter appuyez sur w\n");
    printf("\n=== Choisissez la difficulté ===\n");
    printf("a. Facile (Débutant)\n");
    printf("b. Moyen (Intermédiaire)\n");
    printf("c. Difficile (Expert)\n");
    printf("\nVotre choix (1-3): ");
    
    char choice;
    bool valid_choice = false;    
    do
    {
        printf("\b \b");; 
    

        scanf(" %c", &choice); 

    if (choice == 'a' || choice == 'b' || choice == 'c') { valid_choice = true; } 
    else { }
  
}while (choice != 'a' && choice != 'b' && choice != 'c');

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
    time_t last_refresh, now;
    GameSettings settings;

    init_board(SCREEN_HEIGHT, SCREEN_WIDTH, board);
    init_game_state(&game_state);
    enum Player_class Class;
    Afficher_la_musique("MAIN.mp3");
    Display_Title();

    char filename[] = "player_data.txt";  
    char player_name[100];
    get_player_name(player_name, sizeof(player_name));

    system("clear");

    char action = choose_game_action();
    
    if (action == 'a') {
        printf("Nouvelle Partie commence...\n");
        init_game_state(&game_state);
    } 
    else if (action == 'b') {
        display_high_scores("high_scores.txt");
    } 
    else if(action == 'c'){
        stop_music();
        return 0;
    }

    char difficulty = choose_difficulty();
    if (difficulty == 'a')
    {
        init_game_settings(&settings, difficulty);
    }else if (difficulty == 'b')
    {
        init_game_settings(&settings, difficulty);
    }
    
    else if(difficulty == 'c'){
       init_game_settings(&settings, difficulty);
    }
    
    
    
    
    system("clear");
    Class = Choice_the_character(Class);
    stop_music();

    system("clear");
    Afficher_la_musique("Level.mp3");

    init_character(SCREEN_HEIGHT, SCREEN_WIDTH, board, Class, PLAYER_COL, 1);
    print_board(row, col, board);
    time(&last_refresh);

    int input;

    while (input != 'q') {
        move_track(board, &game_state);
        next_level(&settings, &game_state);
          
        time(&now);
        if (now - last_refresh > gravity_time) {
            gravity(row, col, board);
            print_board(row, col, board);
            last_refresh = now;
        }

        handle_jump(&jump_state);
        
        jump_character(board, Class, &jump_state);
        
        bool is_on_ground = !jump_state.is_jumping && !jump_state.is_falling;
        if (check_collision_precise(board, Class, PLAYER_COL, jump_state.ground_level + jump_state.jump_height, is_on_ground, &game_state)) {
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

        usleep(50000);  
    }
    stop_music();
    game_over_musique();
    sleep(4);
    clear();   
    save_game("high_scores.txt", player_name, &settings, &game_state); 
    save_score(player_name, game_state.score);
    sleep(6);
    
    clear();    
    return 0;
    }
    
    
