# include <stdio.h>
# include <stdlib.h>
# include <stdint.h>
# include <string.h>
# include "defs.h"

void parse_fen(char *fen, Board *board) {
    memset(board->bitboards, 0, sizeof(board->bitboards));      // Goes to bitboards in memory and sets all bytes in bitboards array to 0
    memset(board->occupancies, 0, sizeof(board->occupancies));  // Needs the third argument (size) since pointer is just a memory address and doesnt know how much memory lives there
    board->side = 0;
    board->enpassant = no_sq;
    board->castle = 0;        
    board->fifty_move = 0;
    board->full_move = 0;

    for (int rank = 7; rank >= 0; rank--) {   // FEN string starts from top left, so loop starts at highest rank and counts down
        int file = 0;
        while (file < 8) {  
            char square_value = *fen;

            if ((square_value >= 'a' && square_value <= 'z') || (square_value >= 'A' && square_value <= 'Z')) { // If current FEN character is a letter (any piece)
                int square = rank * 8 + file;   // Converts the rank and file into a square index (0-63)

                switch (square_value) {         // Sets bitboards for pieces accordingly
                    case 'P': set_bit(board->bitboards[P], square); break;
                    case 'N': set_bit(board->bitboards[N], square); break;
                    case 'B': set_bit(board->bitboards[B], square); break;
                    case 'R': set_bit(board->bitboards[R], square); break;
                    case 'Q': set_bit(board->bitboards[Q], square); break;
                    case 'K': set_bit(board->bitboards[K], square); break;
                    
                    case 'p': set_bit(board->bitboards[p], square); break;
                    case 'n': set_bit(board->bitboards[n], square); break;
                    case 'b': set_bit(board->bitboards[b], square); break;
                    case 'r': set_bit(board->bitboards[r], square); break;
                    case 'q': set_bit(board->bitboards[q], square); break;
                    case 'k': set_bit(board->bitboards[k], square); break;
                }
                fen++;
                file++;
            }

            else if (square_value >= '1' && square_value <= '8') {  // If FEN character is a numbers from 1 to 8
                int offset = square_value - '0';                    // Convert char digit to int ('3' -> 3)
                file += offset;                                     // Skipping amount specfied by FEN (no pieces on these squares)
                fen++;
            }

            else if (square_value == '/') {   // The / in FEN seperates ranks, so we just skip it
                fen++;
            }

            else {          // Catches any unexpected character (like a space at the end of the board section) and skips it too. 
                fen++;
            }
        }
    }

    // At this point we are done populating the board

    fen++;
    board->side = (*fen == 'w') ? 0 : 1;    // FEN format uses w or b to indicate whose turn it is
    fen += 2;                               // += 2 skips space

    while (*fen != ' ') {                   // Castling section of FEN loops until space is hit
        switch (*fen) {
            case 'K': board->castle |= WK; break; // Add White King-side
            case 'Q': board->castle |= WQ; break; // Add White Queen-side
            case 'k': board->castle |= BK; break; // Add Black King-side
            case 'q': board->castle |= BQ; break; // Add Black Queen-side
            case '-': break; // Do nothing
       }
       fen++;
    }
    fen++;

    if (*fen != '-') {    // Enpassant section of FEN
        int file = fen[0] - 'a';
        int rank = fen[1] - '1';
        
        board->enpassant = rank * 8 + file;
        
        fen += 3;
    } 
    
    else {
        board->enpassant = no_sq;
        fen += 2; // Skip "- "
    }

    // Parse fifty-move counter 
    while (*fen != ' ') {
        int digit = *fen - '0';                                 // Convert char to int
        board->fifty_move = board->fifty_move * 10 + digit;     // Shift left and add digit
        fen++;
    }
    fen++; 

    // Parse Full Move Counter
    while (*fen != ' ' && *fen != '\0') {
        int digit = *fen - '0';
        board->full_move = board->full_move * 10 + digit;
        fen++;
    }

    /* Now all bitboards are populated, occupancies bitboards must be done */

    // Loop over White Pieces (0-5) (Uppercase P N B R Q K are enums for white)
    for (int piece = P; piece <= K; piece++) {
        board->occupancies[WHITE] |= board->bitboards[piece];
    }

    // Loop over Black Pieces (6-11) (Lowercase p n b r q k are enums for black)
    for (int piece = p; piece <= k; piece++) {
        board->occupancies[BLACK] |= board->bitboards[piece];
    }

    // Combine them for "Both"
    board->occupancies[BOTH] = board->occupancies[WHITE] | board->occupancies[BLACK];
}

void print_bitboard(U64 bitboard) {
    printf("\n");

    for (int rank = 7; rank >= 0; rank--)
    {
        for (int file = 0; file <= 7; file++) {
            int square = rank * 8 + file;
      
            printf(" %c ", get_bit(bitboard, square) ? '1' : '0');
        }
        printf("\n");
    }
    printf("\n");
}


void print_test(char *desc, U64 bitboard) {
    printf("\n%s\n", desc);
    print_bitboard(bitboard);
}

// argc count of arguments, argv is an array of strings containing them. argv[0] program name, argv[1] is first argument I pass
int main(int argc, char *argv[]) {
    init_all();
   
    if (argc < 2) {  
        printf("Error: No FEN string provided\n");
        return 1;  
    }

    char *fen = argv[1];
    Board game_board;

    parse_fen(fen, &game_board);

    return 0;
}

 
