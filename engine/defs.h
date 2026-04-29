#ifndef DEFS_H
#define DEFS_H

#include <stdint.h> // Need this for uint64_t

// TYPEDEFS
typedef uint64_t U64;

// MACROS
#define get_bit(bitboard, square) (bitboard & (1ULL << square))
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))

// pop_bit creates a 64bit (ULL) with value 1 and shifts that by "square", 
// then inverts so square is the only 0 and perfors an "AND" operation to clear that square to 0 and returns board
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// Built-in GCC functions for fast bit manipulation
#define get_lsb_index(bitboard) __builtin_ctzll(bitboard)
#define count_bits(bitboard) __builtin_popcountll(bitboard)


// These are "Safety Nets" to prevent pieces from wrapping around the board.
#define NOT_A_FILE  0xFEFEFEFEFEFEFEFEULL // In our 1D array (0-63), the last square of a row (e.g., h1/index 7) is 
#define NOT_H_FILE  0x7F7F7F7F7F7F7F7FULL // mathematically next to the first square of the next row (e.g., a2/index 8).
#define NOT_HG_FILE 0x3F3F3F3F3F3F3F3FULL // 0xFE... = 11111110 : All ones, except the A-file is 0.
#define NOT_AB_FILE 0xFCFCFCFCFCFCFCFCULL // If a move lands on the A-file after moving RIGHT (which is impossible), we AND (&) it with this mask to delete it.


/*
    Move encoding layout (32-bit integer):

    Bits  0-5:   Source square       (6 bits, values 0-63)
    Bits  6-11:  Target square       (6 bits, values 0-63)
    Bits  12-15: Piece moved         (4 bits, values 0-11)
    Bits  16-19: Promoted piece      (4 bits, 0 if none)
    Bit   20:    Capture flag
    Bit   21:    Double pawn push flag
    Bit   22:    En passant flag
    Bit   23:    Castling flag
*/

// Encode
#define encode_move(source, target, piece, promoted, capture, double_p, enpassant, castling) \
    ((source)            |       \
    ((target)    << 6)   |       \
    ((piece)     << 12)  |       \
    ((promoted)  << 16)  |       \
    ((capture)   << 20)  |       \
    ((double_p)  << 21)  |       \
    ((enpassant) << 22)  |       \
    ((castling)  << 23))

// Decode
#define get_move_source(move)    ((move) & 0x3F)
#define get_move_target(move)    (((move) >> 6) & 0x3F)
#define get_move_piece(move)     (((move) >> 12) & 0xF)
#define get_move_promoted(move)  (((move) >> 16) & 0xF)
#define get_move_capture(move)   (((move) >> 20) & 0x1)
#define get_move_double(move)    (((move) >> 21) & 0x1)
#define get_move_enpassant(move) (((move) >> 22) & 0x1)
#define get_move_castling(move)  (((move) >> 23) & 0x1)

// ENUMS
enum { P, N, B, R, Q, K, p, n, b, r, q, k };    // P = 0, N = 1 ... etc
enum { WK = 1, WQ = 2, BK = 4, BQ = 8 };        // WK = 0001, WQ = 0010 ... etc, they are powers of 2 so easily able to check 
enum {                                          
    a1, b1, c1, d1, e1, f1, g1, h1,             
    a2, b2, c2, d2, e2, f2, g2, h2,             
    a3, b3, c3, d3, e3, f3, g3, h3,     // This essentially gives us a number to shift the bits by based on the board position.
    a4, b4, c4, d4, e4, f4, g4, h4,     // Suppose we want to set set_bit(board->bitboards[P], b1) it would perform -> board->bitboards[P] |= (1ULL << 1)
    a5, b5, c5, d5, e5, f5, g5, h5,     // which sets b1 on the bitboard to 1
    a6, b6, c6, d6, e6, f6, g6, h6,     // no_sq means no square which is just bit 64 (out of bounds since board is 0-63)
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8, no_sq
};
enum { WHITE, BLACK, BOTH };

// STRUCTS
typedef struct {
    U64 bitboards[12];  // Each piece type gets a bitboard for each side. (All white pawns is bitboard[P], all black knights is bitboard[n] ... etc)
    U64 occupancies[3]; // Stores all pieces for each side and all pieces. So occupancies[WHITE] is all white. occupancies[BOTH] is all pieces ... etc
    int side;           // Tracks whos turn it is (0 white, 1 black)
    int enpassant;      // Keeps track of which square is availible for empassant (e.g stores a value like e3)
    int castle;         // Keeps track of what castling is permitted using 4 bits
    int fifty_move;     // Half moves since last pawn capture (draw at 100)
    int full_move;      // Full move counter, increments after black moves
} Board;


#define MAX_MOVES 256 

typedef struct {
    int moves[MAX_MOVES]; // An array of integers where each integer is an encoded move
    int count;            // Count tracking how many moves are currently stored
} MoveList;

static inline void add_move(MoveList *list, int move) {
    list->moves[list->count] = move;
    list->count++;
}

// GLOBALS
extern U64 knight_attacks[64];
extern U64 king_attacks[64];
extern U64 pawn_attacks[2][64]; // [Side][Square]

U64 generate_rook_attacks(int square, U64 block);
U64 generate_bishop_attacks(int square, U64 block);
U64 generate_queen_attacks(int square, U64 block);

// FUNCTIONS
int is_square_attacked(Board *board, int square, int attacking_side);
void generate_moves(Board *board, MoveList *move_list);
void make_move(Board *board, int move);
void generate_legal_moves(Board *board, MoveList *legal);
int evaluate(Board *board);
int search(Board *board, int depth);
void init_all();

#endif