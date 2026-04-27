#include "defs.h"

/*
    Determines if a given square is attacked by any piece of the specified side.
    Uses reverse lookup: generates attacks FROM the target square for each piece
    type and checks if any enemy piece of that type occupies a reachable square.
*/
int is_square_attacked(Board *board, int square, int attacking_side) {
    U64 both = board->occupancies[BOTH];

    // Pawn attacks are asymmetric — use the opposite side's pattern to find
    // where an attacking pawn would need to be to cover this square
    if (pawn_attacks[attacking_side ^ 1][square] &
        board->bitboards[attacking_side == WHITE ? P : p])
        return 1;

    if (knight_attacks[square] &
        board->bitboards[attacking_side == WHITE ? N : n])
        return 1;

    // Bishops and queens share diagonal movement
    U64 bishop_atks = generate_bishop_attacks(square, both);
    if (bishop_atks & (board->bitboards[attacking_side == WHITE ? B : b] |
                       board->bitboards[attacking_side == WHITE ? Q : q]))
        return 1;

    // Rooks and queens share orthogonal movement
    U64 rook_atks = generate_rook_attacks(square, both);
    if (rook_atks & (board->bitboards[attacking_side == WHITE ? R : r] |
                     board->bitboards[attacking_side == WHITE ? Q : q]))
        return 1;

    if (king_attacks[square] &
        board->bitboards[attacking_side == WHITE ? K : k])
        return 1;

    return 0;
}

// Lookup by square index (0=a1 ... 63=h8). After every move we do:
// board->castle &= castling_rights[src] & castling_rights[tgt]
// 15 (1111) leaves all rights intact. The six home squares of kings/rooks
// have values with specific bits zeroed to revoke the right they guard.
static const int castling_rights[64] = {
  // squares 0-7  = rank 1 (white's home rank, first in the array because square 0 = a1)
    13, 15, 15, 15, 12, 15, 15, 14,   // a1=13→clears WQ(0010), e1=12→clears WK+WQ(0011), h1=14→clears WK(0001)
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,   // ranks 2-7: no kings or rooks start here, nothing to revoke
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11,   // squares 56-63 = rank 8 (black's home rank, last in the array because square 63 = h8)
                                    // a8=7→clears BQ(1000), e8=3→clears BK+BQ(1100), h8=11→clears BK(0100)
};

void make_move(Board *board, int move) {
    int src    = get_move_source(move);
    int tgt    = get_move_target(move);
    int piece  = get_move_piece(move);
    int promo  = get_move_promoted(move);
    int cap    = get_move_capture(move);
    int dbl    = get_move_double(move);
    int ep     = get_move_enpassant(move);
    int castle = get_move_castling(move);
    int side   = board->side;

    // Move the piece
    pop_bit(board->bitboards[piece], src);
    set_bit(board->bitboards[piece], tgt);

    // Regular capture: find and remove the enemy piece on the target square, loops through the correct sides pieces until target square piece found
    if (cap && !ep) {
        int enemy_start = (side == WHITE) ? p : P;
        int enemy_end   = (side == WHITE) ? k : K;
        for (int pc = enemy_start; pc <= enemy_end; pc++) {
            if (get_bit(board->bitboards[pc], tgt)) {
                pop_bit(board->bitboards[pc], tgt);
                break;
            }
        }
    }

    // En passant: the captured pawn sits one rank behind the target square
    if (ep) {
        int captured_sq = (side == WHITE) ? tgt - 8 : tgt + 8;
        pop_bit(board->bitboards[side == WHITE ? p : P], captured_sq);
    }

    // Promotion: replace the pawn on the target square with the promoted piece
    if (promo) {
        pop_bit(board->bitboards[piece], tgt);
        set_bit(board->bitboards[promo], tgt);
    }

    // Castling: also reposition the rook
    if (castle) {
        switch (tgt) {
            case g1: pop_bit(board->bitboards[R], h1); set_bit(board->bitboards[R], f1); break;
            case c1: pop_bit(board->bitboards[R], a1); set_bit(board->bitboards[R], d1); break;
            case g8: pop_bit(board->bitboards[r], h8); set_bit(board->bitboards[r], f8); break;
            case c8: pop_bit(board->bitboards[r], a8); set_bit(board->bitboards[r], d8); break;
        }
    }

    // Fifty-move clock: reset on pawn move or capture
    if (piece == P || piece == p || cap)
        board->fifty_move = 0;
    else
        board->fifty_move++;

    // Full-move counter increments after Black's move
    if (side == BLACK)
        board->full_move++;

    // Reset en passant; set a new target square for en passant if pawn double pushes
    board->enpassant = no_sq;
    if (dbl)
        board->enpassant = (side == WHITE) ? tgt - 8 : tgt + 8;

    // castling_rights[sq] holds a bitmask, ANDing it with board->castle zeroes out the bits for any rights that sq invalidates
    // e.g. king moves from e1: castling_rights[e1]=12(1100), so 1111 & 1100 = 1100, clearing WK (White kingside) and WQ (white queenside)
    board->castle &= castling_rights[src];
    board->castle &= castling_rights[tgt];

    // Flip side to move
    board->side ^= 1;

    // Rebuild occupancy bitboards
    board->occupancies[WHITE] = 0;
    board->occupancies[BLACK] = 0;
    for (int pc = P; pc <= K; pc++) board->occupancies[WHITE] |= board->bitboards[pc];
    for (int pc = p; pc <= k; pc++) board->occupancies[BLACK] |= board->bitboards[pc];
    board->occupancies[BOTH] = board->occupancies[WHITE] | board->occupancies[BLACK];
}

// Filters pseudo-legal moves to those that don't leave our king in check
void generate_legal_moves(Board *board, MoveList *legal) {
    MoveList pseudo;
    generate_moves(board, &pseudo);
    legal->count = 0;

    int our_side   = board->side;
    int king_piece = (our_side == WHITE) ? K : k;

    for (int i = 0; i < pseudo.count; i++) {
        Board copy = *board;
        make_move(&copy, pseudo.moves[i]);
        // After make_move, copy.side is the opponent, exactly who we ask about attacks
        int king_sq = get_lsb_index(copy.bitboards[king_piece]);
        if (!is_square_attacked(&copy, king_sq, copy.side))
            add_move(legal, pseudo.moves[i]);
    }
}

// Generates all pseudo-legal moves for the side to move
void generate_moves(Board *board, MoveList *move_list) {
    move_list->count = 0;

    int source_square, target_square;
    int side = board->side;
    U64 bitboard, attacks;

    // --- Pawns and castling ---

    if (side == WHITE) {
        bitboard = board->bitboards[P];

        while (bitboard) {
            source_square = get_lsb_index(bitboard);
            target_square = source_square + 8;

            // Quiet push
            if (!get_bit(board->occupancies[BOTH], target_square)) {
                // Promotion: source on rank 7 (squares 48-55), target lands on rank 8
                if (source_square >= a7) {
                    add_move(move_list, encode_move(source_square, target_square, P, Q, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, R, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, B, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, N, 0, 0, 0, 0));
                } else {
                    add_move(move_list, encode_move(source_square, target_square, P, 0, 0, 0, 0, 0));

                    // Double push: source on rank 2 (squares 8-15), second square must also be empty
                    int double_push = target_square + 8;
                    if (source_square <= h2 && !get_bit(board->occupancies[BOTH], double_push))
                        add_move(move_list, encode_move(source_square, double_push, P, 0, 0, 1, 0, 0));
                }
            }

            // Diagonal captures
            attacks = pawn_attacks[WHITE][source_square] & board->occupancies[BLACK];
            while (attacks) {
                target_square = get_lsb_index(attacks);
                if (source_square >= a7) {
                    add_move(move_list, encode_move(source_square, target_square, P, Q, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, R, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, B, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, N, 1, 0, 0, 0));
                } else {
                    add_move(move_list, encode_move(source_square, target_square, P, 0, 1, 0, 0, 0));
                }
                pop_bit(attacks, target_square);
            }

            // En passant: check if this pawn's attack pattern covers the en passant target square
            if (board->enpassant != no_sq) {
                U64 ep_attacks = pawn_attacks[WHITE][source_square] & (1ULL << board->enpassant);
                if (ep_attacks) {
                    target_square = get_lsb_index(ep_attacks);
                    add_move(move_list, encode_move(source_square, target_square, P, 0, 1, 0, 1, 0));
                }
            }

            pop_bit(bitboard, source_square);
        }

        // King-side: f1 and g1 empty, king and both transit squares not attacked by black
        if ((board->castle & WK) &&
            !get_bit(board->occupancies[BOTH], f1) &&
            !get_bit(board->occupancies[BOTH], g1) &&
            !is_square_attacked(board, e1, BLACK) &&
            !is_square_attacked(board, f1, BLACK) &&
            !is_square_attacked(board, g1, BLACK))
            add_move(move_list, encode_move(e1, g1, K, 0, 0, 0, 0, 1));

        // Queen-side: b1, c1, d1 empty, king and both transit squares not attacked by black
        if ((board->castle & WQ) &&
            !get_bit(board->occupancies[BOTH], d1) &&
            !get_bit(board->occupancies[BOTH], c1) &&
            !get_bit(board->occupancies[BOTH], b1) &&
            !is_square_attacked(board, e1, BLACK) &&
            !is_square_attacked(board, d1, BLACK) &&
            !is_square_attacked(board, c1, BLACK))
            add_move(move_list, encode_move(e1, c1, K, 0, 0, 0, 0, 1));

    } else {
        bitboard = board->bitboards[p];

        while (bitboard) {
            source_square = get_lsb_index(bitboard);
            target_square = source_square - 8;

            // Quiet push
            if (!get_bit(board->occupancies[BOTH], target_square)) {
                // Promotion: source on rank 2 (squares 8-15), target lands on rank 1
                if (source_square <= h2) {
                    add_move(move_list, encode_move(source_square, target_square, p, q, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, r, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, b, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, n, 0, 0, 0, 0));
                } else {
                    add_move(move_list, encode_move(source_square, target_square, p, 0, 0, 0, 0, 0));

                    // Double push: source on rank 7 (squares 48-55), second square must also be empty
                    int double_push = target_square - 8;
                    if (source_square >= a7 && !get_bit(board->occupancies[BOTH], double_push))
                        add_move(move_list, encode_move(source_square, double_push, p, 0, 0, 1, 0, 0));
                }
            }

            // Diagonal captures
            attacks = pawn_attacks[BLACK][source_square] & board->occupancies[WHITE];
            while (attacks) {
                target_square = get_lsb_index(attacks);
                if (source_square <= h2) {
                    add_move(move_list, encode_move(source_square, target_square, p, q, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, r, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, b, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, n, 1, 0, 0, 0));
                } else {
                    add_move(move_list, encode_move(source_square, target_square, p, 0, 1, 0, 0, 0));
                }
                pop_bit(attacks, target_square);
            }

            // En passant: check if this pawn's attack pattern covers the en passant target square
            if (board->enpassant != no_sq) {
                U64 ep_attacks = pawn_attacks[BLACK][source_square] & (1ULL << board->enpassant);
                if (ep_attacks) {
                    target_square = get_lsb_index(ep_attacks);
                    add_move(move_list, encode_move(source_square, target_square, p, 0, 1, 0, 1, 0));
                }
            }

            pop_bit(bitboard, source_square);
        }

        // King-side: f8 and g8 empty, king and both transit squares not attacked by white
        if ((board->castle & BK) &&
            !get_bit(board->occupancies[BOTH], f8) &&
            !get_bit(board->occupancies[BOTH], g8) &&
            !is_square_attacked(board, e8, WHITE) &&
            !is_square_attacked(board, f8, WHITE) &&
            !is_square_attacked(board, g8, WHITE))
            add_move(move_list, encode_move(e8, g8, k, 0, 0, 0, 0, 1));

        // Queen-side: b8, c8, d8 empty, king and both transit squares not attacked by white
        if ((board->castle & BQ) &&
            !get_bit(board->occupancies[BOTH], d8) &&
            !get_bit(board->occupancies[BOTH], c8) &&
            !get_bit(board->occupancies[BOTH], b8) &&
            !is_square_attacked(board, e8, WHITE) &&
            !is_square_attacked(board, d8, WHITE) &&
            !is_square_attacked(board, c8, WHITE))
            add_move(move_list, encode_move(e8, c8, k, 0, 0, 0, 0, 1));
    }

    // --- knights, bishops, rooks, queens, kings ---
    int start_piece = (side == WHITE) ? N : n;
    int end_piece   = (side == WHITE) ? K : k;

    for (int piece = start_piece; piece <= end_piece; piece++) {
        bitboard = board->bitboards[piece];

        while (bitboard) {
            source_square = get_lsb_index(bitboard);

            if      (piece == N || piece == n) attacks = knight_attacks[source_square];
            else if (piece == B || piece == b) attacks = generate_bishop_attacks(source_square, board->occupancies[BOTH]);
            else if (piece == R || piece == r) attacks = generate_rook_attacks(source_square, board->occupancies[BOTH]);
            else if (piece == Q || piece == q) attacks = generate_queen_attacks(source_square, board->occupancies[BOTH]);
            else                               attacks = king_attacks[source_square];

            // Mask out squares occupied by friendly pieces
            attacks &= ~board->occupancies[side];

            while (attacks) {
                target_square = get_lsb_index(attacks);

                int capture = get_bit(board->occupancies[side ^ 1], target_square) ? 1 : 0;
                add_move(move_list, encode_move(source_square, target_square, piece, 0, capture, 0, 0, 0));

                pop_bit(attacks, target_square);
            }

            pop_bit(bitboard, source_square);
        }
    }
}
