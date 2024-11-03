
enum Player_class{Paladin,Druid,Berserker}; 

void init_character(int row, int col, char board[row][col], enum Player_class player){
    int pos = 4 ;
    int height = 1 ;
    switch (player)
    {
    
        case 0:
            board[height+2][pos] = 'O' ;
            board[height+1][pos] = '\\' ;
            board[height+1][pos+1] = '|' ;
            board[height+3][pos-1] = '_' ;
            board[height+3][pos] = '_' ;
            board[height+1][pos-1] = '0' ;
            board[height][pos-1] = '/' ;
            board[height][pos+1] = '\\' ;
        break;
        
        case 1:
            board[height+2][pos + 1] = '\\' ;
            board[height + 2][pos - 1] = '/' ;
            board[height+2][pos] = 'O' ;
            board[height+1][pos] = '\\' ;
            board[height+1][pos+1] = '|' ;
            board[height+3][pos] = '_' ;
            board[height+1][pos +1] = '0' ;
            board[height][pos-1] = '/' ;
            board[height][pos+1] = '\\' ;
        break;
        
        case 2:
            board[height+2][pos + 1] = '_' ;
            board[height + 2][pos - 1] = '|' ;
            board[height + 2][pos + 1] = '|' ;
            board[height + 4][pos] = '|' ;
            board[height + 5][pos] = 'O' ;
            board[height + 4][pos + 1] = '\\' ;
            board[height + 4][pos - 1] = '/' ;
            board[height + 3][pos + 1] = '/' ;
            board[height + 3][pos - 1] = '\\' ;
            board[height+2][pos] = 'O' ;
            board[height+1][pos] = '\\' ;
            board[height+1][pos+1] = '|' ;
            board[height+3][pos] = '_' ;
            board[height+1][pos +1] = '*' ;
            board[height][pos-1] = '/' ;
            board[height][pos+1] = '\\' ;
        break;

    }
}
