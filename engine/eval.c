#include "defs.h"

// Material values indexed by piece enum (P=0 through k=11)
// King has no value since it can never be captured
static const int piece_value[12] = {
    100,    // P
    320,    // N
    330,    // B
    500,    // R
    900,    // Q
    0,      // K
    100,    // p
    320,    // n
    330,    // b
    500,    // r
    900,    // q
    0,      // k
};

// Returns the material score from white's perspective.
// Positive = white is ahead, negative = black is ahead.
int evaluate(Board *board) {
    int score = 0;

    for (int piece = P; piece <= K; piece++)
        score += count_bits(board->bitboards[piece]) * piece_value[piece];

    for (int piece = p; piece <= k; piece++)
        score -= count_bits(board->bitboards[piece]) * piece_value[piece];

    return score;
}
