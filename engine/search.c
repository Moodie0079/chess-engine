#include "defs.h"

#define CHECKMATE 1000000  // Score for checkmate, safely above any real material score (max ~8100)
#define INFINITY  2000000  // Used to initialize best/alpha/beta so any real score beats it immediately

// Answers: "what is the best score the side to move can achieve from this position?"
// It does NOT pick a move, it just returns a score.
//
// How it works:
//   - Try every legal move, recurse one level deeper with depth-1
//   - At depth 0, stop recursing and call evaluate() to score the position directly
//   - The score bubbles back up, each level picks the best score from all moves below it
//   - "Best" always means best for the side moving at that level
//
// Assuming both sides play perfectly, the final score represents, 
//   "If both sides make their best moves for `depth` half-moves, this is the resulting score for the side to move."
//
// Alpha-beta pruning:
//   Alpha = the best score the current side is guaranteed so far across all branches
//   Beta  = the best score the opponent is guaranteed so far across all branches
//   If we find a move that scores >= beta, the opponent already has a better option elsewhere
//   and will never let us reach this position — so we stop searching this branch immediately.
//   Same answer as without pruning, just faster — letting us search deeper in the same time.
static int negamax(Board *board, int depth, int alpha, int beta) {
    // Depth 0: no more moves to look ahead so score the position as-is.
    // evaluate() always returns from white's perspective, so negate for black.
    if (depth == 0)
        return (board->side == WHITE) ? evaluate(board) : -evaluate(board);

    MoveList moves;
    generate_legal_moves(board, &moves);

    // No legal moves means the game is over at this node
    if (moves.count == 0) {
        int king_piece = (board->side == WHITE) ? K : k;
        int king_sq    = get_lsb_index(board->bitboards[king_piece]);
        if (is_square_attacked(board, king_sq, board->side ^ 1))
            return -CHECKMATE; // King is in check with no escape = checkmate
        return 0;              // No check, no moves = stalemate
    }

    int best = -INFINITY; // Will be replaced by the first real score we find 

    for (int i = 0; i < moves.count; i++) {
        Board copy = *board;
        make_move(&copy, moves.moves[i]);

        // Recursively call negamax with the opponent now to move, so negate their score to get ours.
        // Also swap and negate alpha/beta, their alpha is our beta and vice versa.
        int score = -negamax(&copy, depth - 1, -beta, -alpha);

        if (score > best)  best  = score; // New best score found at this level
        if (score > alpha) alpha = score; // Tighten our guarantee
        if (alpha >= beta) break;         // Opponent already has better elsewhere so stop searching this branch
    }

    return best;
}

// Entry point for the search. Tries every legal move from the root position,
// uses negamax to score each one (assuming both sides play perfectly for `depth` half-moves),
// and returns the move that produced the highest score.
int search(Board *board, int depth) {
    MoveList moves;
    generate_legal_moves(board, &moves);

    int best_move  = 0;
    int best_score = -INFINITY;
    int alpha      = -INFINITY;
    int beta       =  INFINITY;

    for (int i = 0; i < moves.count; i++) {
        Board copy = *board;
        make_move(&copy, moves.moves[i]);

        int score = -negamax(&copy, depth - 1, -beta, -alpha);

        // Track which root move produced the best score that's what we return
        if (score > best_score) {
            best_score = score;
            best_move  = moves.moves[i]; // storing which move produced the best score
        }
        if (score > alpha) alpha = score;
    }

    return best_move;
}
