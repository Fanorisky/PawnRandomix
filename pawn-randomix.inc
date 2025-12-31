#if defined _pawnrandomix_included
    #endinput
#endif
#define _pawnrandomix_included

/*
    PawnRandomix
    Modern Random Generator for Open.MP

    - PRNG  : PCG32 (fast, unbiased)
    - CSPRNG : ChaCha20 (cryptographically secure)

    Author : Component PawnRandomix
    Version: 1.1.0
*/

// =======================================================
// BASIC PRNG (PCG32)
// =======================================================

// Random integer 0 .. max
native PRandom(max);

// Random integer min .. max
native PRandRange(min, max);

// Random float min .. max
native Float:PRandFloatRange(Float:min, Float:max);

// Seed PRNG (32-bit seed)
native SeedPRNG(seed);

// =======================================================
// CRYPTO PRNG (ChaCha20)
// =======================================================

// Random integer 0 .. max (secure)
native CSPRandom(max);

// Random integer min .. max (secure)
native CSPRandRange(min, max);

// Random float min .. max (secure)
native Float:CSPRandFloatRange(Float:min, Float:max);

// Seed CSPRNG
native SeedCSPRNG(seed);

// =======================================================
// BOOLEAN / PROBABILITY
// =======================================================

// True dengan probabilitas 0.0 .. 1.0
native bool:PRandBool(Float:probability = 0.5);

// Secure version
native bool:CSPRandBool(Float:probability = 0.5);

// =======================================================
// ADVANCED RANDOM
// =======================================================

// Weighted random
// return: index terpilih
native PRandWeighted(weights[], count);

// Shuffle array in-place
native bool:PRandShuffle(array[], count);

// Gaussian / normal distribution
native PRandGaussian(Float:mean, Float:stddev);

// =======================================================
// POSITION GENERATORS
// =======================================================

// Random position in 2D circle
native bool:PRandPosition2D(
    Float:centerX,
    Float:centerY,
    Float:radius,
    &Float:outX,
    &Float:outY
);

// Random position in 3D sphere
native bool:PRandPosition3D(
    Float:centerX,
    Float:centerY,
    Float:centerZ,
    Float:radius,
    &Float:outX,
    &Float:outY,
    &Float:outZ
);

// =======================================================
// GAME-STYLE UTILITIES
// =======================================================

// Roll dice (e.g. 3d6)
native PRandDice(sides, count = 1);

// Cryptographic token (hex-ish, up to 32-bit)
native CSPRandToken(length);

// =======================================================
// CONVENIENCE MACROS
// =======================================================

#define PRand01()          PRandFloatRange(0.0, 1.0)
#define CSPRand01()         CSPRandFloatRange(0.0, 1.0)
#define PRandChance(p)    PRandBool(p)
#define CSPRandChance(p)   CSPRandBool(p)
