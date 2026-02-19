#include "defs.h"

/*
    Determines if a given square is attacked by any piece of the specified side.
    Uses reverse lookup: generates attacks FROM the target square for each piece
    type and checks if any enemy piece of that type occupies a reachable square.
*/
int is_square_attacked(Board *board, int square, int attacking_side) {
    U64 both = board->occupancies[BOTH];

    // Pawn attacks are asymmetrical, so use opposite side's attack pattern to find where an attacking pawn would be
    if (pawn_attacks[attacking_side ^ 1][square] &
        board->bitboards[attacking_side == WHITE ? P : p])
        return 1;

    // Generate attacks from target square, AND with attacking side's piece bitboard to check for overlap
    if (knight_attacks[square] &
        board->bitboards[attacking_side == WHITE ? N : n])
        return 1;

    // Bishops and queens share diagonal movement, so check both with one set of attacks
    U64 bishop_atks = generate_bishop_attacks(square, both);
    if (bishop_atks & (board->bitboards[attacking_side == WHITE ? B : b] |
                       board->bitboards[attacking_side == WHITE ? Q : q]))
        return 1;

    // Rooks and queens share orthogonal movement, so check both with one set of attacks
    U64 rook_atks = generate_rook_attacks(square, both);
    if (rook_atks & (board->bitboards[attacking_side == WHITE ? R : r] |
                     board->bitboards[attacking_side == WHITE ? Q : q]))
        return 1;

    if (king_attacks[square] &
        board->bitboards[attacking_side == WHITE ? K : k])
        return 1;

    return 0;
}