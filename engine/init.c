# include "defs.h"

// Allocating memory for tables
U64 knight_attacks[64];
U64 king_attacks[64];
U64 pawn_attacks[2][64];

U64 mask_knight_attacks(int square) {
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    set_bit(bitboard, square);

    // 2 Up (+16), 1 Right (+1) = +17
    // If we move Right, ensure we didn't wrap to A-file
    if ((bitboard << 17) & NOT_A_FILE) attacks |= (bitboard << 17);
    
    // 2 Up (+16), 1 Left (-1) = +15
    // If we move Left, ensure we didn't wrap to H-file
    if ((bitboard << 15) & NOT_H_FILE) attacks |= (bitboard << 15);
    
    // 2 Down (-16), 1 Right (+1) = -15 (which is >> 15)
    if ((bitboard >> 15) & NOT_A_FILE) attacks |= (bitboard >> 15);
    
    // 2 Down (-16), 1 Left (-1) = -17 (which is >> 17)
    if ((bitboard >> 17) & NOT_H_FILE) attacks |= (bitboard >> 17);
    
    // 1 Up (+8), 2 Right (+2) = +10
    // Long jump Right: Check NOT_AB (Files 0 and 1) to be safe
    if ((bitboard << 10) & NOT_AB_FILE) attacks |= (bitboard << 10);
    
    // 1 Up (+8), 2 Left (-2) = +6
    // Long jump Left: Check NOT_GH
    if ((bitboard << 6) & NOT_HG_FILE) attacks |= (bitboard << 6);
    
    // 1 Down (-8), 2 Right (+2) = -6 (>> 6)
    if ((bitboard >> 6) & NOT_AB_FILE) attacks |= (bitboard >> 6);
    
    // 1 Down (-8), 2 Left (-2) = -10 (>> 10)
    if ((bitboard >> 10) & NOT_HG_FILE) attacks |= (bitboard >> 10);

    return attacks;
}

U64 mask_king_attacks(int square) {
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    set_bit(bitboard, square);

    attacks |= (bitboard >> 8); // South
    attacks |= (bitboard << 8); // North
    
    // East (+1) - Avoid wrapping to A
    if ((bitboard << 1) & NOT_A_FILE) attacks |= (bitboard << 1);
    
    // West (-1) - Avoid wrapping to H
    if ((bitboard >> 1) & NOT_H_FILE) attacks |= (bitboard >> 1);
    
    // North East (+9)
    if ((bitboard << 9) & NOT_A_FILE) attacks |= (bitboard << 9);
    
    // North West (+7)
    if ((bitboard << 7) & NOT_H_FILE) attacks |= (bitboard << 7);
    
    // South East (-7)
    if ((bitboard >> 7) & NOT_A_FILE) attacks |= (bitboard >> 7);
    
    // South West (-9)
    if ((bitboard >> 9) & NOT_H_FILE) attacks |= (bitboard >> 9);

    return attacks;
}

U64 mask_pawn_attacks(int side, int square) {
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    set_bit(bitboard, square);

    // White Pawns capture North (+7, +9)
    if (side == WHITE) {
        // Capture Right (North East +9) - Avoid wrapping to A
        if ((bitboard << 9) & NOT_A_FILE) attacks |= (bitboard << 9);
        // Capture Left (North West +7) - Avoid wrapping to H
        if ((bitboard << 7) & NOT_H_FILE) attacks |= (bitboard << 7);
    }
    // Black Pawns capture South (-7, -9)
    else {
        // Capture Left - South East -7
        if ((bitboard >> 7) & NOT_A_FILE) attacks |= (bitboard >> 7);
        // Capture Right - South West -9
        if ((bitboard >> 9) & NOT_H_FILE) attacks |= (bitboard >> 9);
    }
    
    return attacks;
}

// Generate Bishop attacks
U64 generate_bishop_attacks(int square, U64 block) {
    U64 attacks = 0ULL;
    
    int target_rank = square / 8;
    int target_file = square % 8;
    
    int r, f;
    
    // North East (Rank +, File +)
    for (r = target_rank + 1, f = target_file + 1; r <= 7 && f <= 7; r++, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break; // Blocked
    }
    
    // North West (Rank +, File -)
    for (r = target_rank + 1, f = target_file - 1; r <= 7 && f >= 0; r++, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break; // Blocked
    }
    
    // South East (Rank -, File +)
    for (r = target_rank - 1, f = target_file + 1; r >= 0 && f <= 7; r--, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break; // Blocked
    }
    
    // South West (Rank -, File -)
    for (r = target_rank - 1, f = target_file - 1; r >= 0 && f >= 0; r--, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break; // Blocked
    }
    
    return attacks;
}

// Generate Rook attacks
U64 generate_rook_attacks(int square, U64 block) {
    U64 attacks = 0ULL;
    
    int target_rank = square / 8;
    int target_file = square % 8;
    
    int r, f;
    
    // North (Rank +)
    for (r = target_rank + 1; r <= 7; r++) {
        attacks |= (1ULL << (r * 8 + target_file));
        if ((1ULL << (r * 8 + target_file)) & block) break;
    }
    
    // South (Rank -)
    for (r = target_rank - 1; r >= 0; r--) {
        attacks |= (1ULL << (r * 8 + target_file));
        if ((1ULL << (r * 8 + target_file)) & block) break;
    }
    
    // East (File +)
    for (f = target_file + 1; f <= 7; f++) {
        attacks |= (1ULL << (target_rank * 8 + f));
        if ((1ULL << (target_rank * 8 + f)) & block) break;
    }
    
    // West (File -)
    for (f = target_file - 1; f >= 0; f--) {
        attacks |= (1ULL << (target_rank * 8 + f));
        if ((1ULL << (target_rank * 8 + f)) & block) break;
    }
    
    return attacks;
}

// Generate Queen attacks (Rook + Bishop)
U64 generate_queen_attacks(int square, U64 block) {
    U64 rook_attacks = generate_rook_attacks(square, block);
    U64 bishop_attacks = generate_bishop_attacks(square, block);
    
    return rook_attacks | bishop_attacks;
}

// Finds all possible moves for leaping pieces on every square and stores them appropriately
void init_leapers() {
    for (int square = 0; square < 64; square++) {
        knight_attacks[square] = mask_knight_attacks(square);
        king_attacks[square] = mask_king_attacks(square);
        pawn_attacks[WHITE][square] = mask_pawn_attacks(WHITE, square);
        pawn_attacks[BLACK][square] = mask_pawn_attacks(BLACK, square);
    }
}

void init_all() {
    init_leapers();
}