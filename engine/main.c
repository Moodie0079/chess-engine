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

void print_board(Board *board) {
    static const char piece_chars[] = "PNBRQKpnbrqk";
    printf("\n");
    for (int rank = 7; rank >= 0; rank--) {
        printf(" %d  ", rank + 1);
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            char piece = '.';
            for (int i = P; i <= k; i++) {
                if (get_bit(board->bitboards[i], square)) {
                    piece = piece_chars[i];
                    break;
                }
            }
            printf("%c ", piece);
        }
        printf("\n");
    }
    printf("\n     a b c d e f g h\n\n");
}

// Prints a move in UCI format (e.g. "e2e4", "e7e8q")
void print_move(int move) {
    int src = get_move_source(move);
    int tgt = get_move_target(move);
    int promoted = get_move_promoted(move);

    // Indexed by piece enum (0-11); only the promotion slots (N/B/R/Q and n/b/r/q) are used
    static const char promo_chars[] = " nbrq  nbrq ";

    if (promoted)
        printf("%c%c%c%c%c", 'a' + (src % 8), '1' + (src / 8),
                              'a' + (tgt % 8), '1' + (tgt / 8),
                              promo_chars[promoted]);
    else
        printf("%c%c%c%c", 'a' + (src % 8), '1' + (src / 8),
                           'a' + (tgt % 8), '1' + (tgt / 8));
}

void print_move_list(MoveList *list) {
    for (int i = 0; i < list->count; i++) {
        printf("  ");
        print_move(list->moves[i]);
        printf("\n");
    }
    printf("  Total: %d moves\n", list->count);
}

int main(void) {
    init_all();

    Board board;
    MoveList moves;

    printf("\n=== Test 1: Starting position (expect 20) ===\n");
    parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &board);
    print_board(&board);
    generate_legal_moves(&board, &moves);
    print_move_list(&moves);

    printf("\n=== Test 2: En passant (white e5 pawn can capture on d6) ===\n");
    parse_fen("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", &board);
    print_board(&board);
    generate_legal_moves(&board, &moves);
    print_move_list(&moves);

    printf("\n=== Test 3: Promotion (white pawn on a7, expect 4 promotion moves) ===\n");
    parse_fen("8/P7/8/8/8/8/8/4K2k w - - 0 1", &board);
    print_board(&board);
    generate_legal_moves(&board, &moves);
    print_move_list(&moves);

    printf("\n=== Test 4: Castling (white can castle both sides) ===\n");
    parse_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", &board);
    print_board(&board);
    generate_legal_moves(&board, &moves);
    print_move_list(&moves);

    printf("\n=== Test 5: King in check (expect 3 — king must escape rook on h1) ===\n");
    // White king e1 checked by black rook h1; only d2, e2, f2 escape the rank-1 rook
    parse_fen("4k3/8/8/8/8/8/8/4K2r w - - 0 1", &board);
    print_board(&board);
    generate_legal_moves(&board, &moves);
    print_move_list(&moves);

    printf("\n=== Test 6: Pinned rook (expect 9 — rook pinned on e-file, king has 4 escapes) ===\n");
    // White rook e2 pinned by black rook e7; can only move along e-file (e3-e7)
    parse_fen("4k3/4r3/8/8/8/8/4R3/4K3 w - - 0 1", &board);
    print_board(&board);
    generate_legal_moves(&board, &moves);
    print_move_list(&moves);

    return 0;
}
