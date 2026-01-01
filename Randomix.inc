// ==============================
// Randomix Include File
// Version: 1.2.0
// Author: Fanorisky
// ==============================
// PRNG (PCG32): Fast statistical random
// CSPRNG (ChaCha20): Cryptographically secure random
// ==============================

#if defined _randomix_included
    #endinput
#endif
#define _randomix_included

// PRNG Functions (PCG32)

/**
 * Generate random integer from 0 to max-1 (exclusive)
 * @param max Maximum value (exclusive)
 * @return Random integer [0, max-1]
 */
native PRandom(max);

/**
 * Generate random integer in range [min, max-1] (exclusive)
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 * @return Random integer [min, max-1]
 */
native PRandRange(min, max);

/**
 * Generate random float in range [min, max)
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 * @return Random float [min, max)
 */
native Float:PRandFloatRange(Float:min, Float:max);

/**
 * Generate random boolean with probability
 * @param probability Chance of returning true [0.0 - 1.0)
 * @return true or false based on probability
 * @example PRandBool(0.3) // 30% chance of true
 */
native bool:PRandBool(Float:probability = 0.5);

/**
 * Random boolean with weighted chance
 * @param trueWeight Weight for true result
 * @param falseWeight Weight for false result
 * @return true or false based on weights
 * @example PRandBoolWeighted(75, 25) // 75% true, 25% false
 */
native bool:PRandBoolWeighted(trueWeight, falseWeight);

/**
 * Weighted random selection from array
 * @param weights[] Array of weights (higher = more likely)
 * @param count Number of elements in array
 * @return Selected index based on weights [0, count-1]
 * @example
 *   new weights[] = {10, 30, 60}; // 10%, 30%, 60% chance
 *   new result = PRandWeighted(weights, 3);
 */
native PRandWeighted(const weights[], count = sizeof weights);

/**
 * Shuffle array randomly (in-place) using Fisher-Yates algorithm
 * @param array[] Array to shuffle
 * @param count Number of elements
 * @return true on success
 * @example
 *   new players[10] = {0,1,2,3,4,5,6,7,8,9};
 *   PRandShuffle(players, 10);
 */
native bool:PRandShuffle(array[], count = sizeof array);

/**
 * Shuffle part of array (specific range)
 * @param array[] Array to shuffle
 * @param start Starting index (inclusive)
 * @param end Ending index (exclusive)
 * @return true on success
 * @example
 *   new players[10] = {0,1,2,3,4,5,6,7,8,9};
 *   PRandShuffleRange(players, 3, 7); // Shuffle indices 3-6 only
 */
native bool:PRandShuffleRange(array[], start, end);

/**
 * Generate random number with Gaussian/Normal distribution
 * @param mean Center of distribution
 * @param stddev Standard deviation (spread)
 * @return Random integer following normal distribution
 * @example PRandGaussian(100.0, 15.0) // Average 100, ±15 spread
 */
native PRandGaussian(Float:mean, Float:stddev);

/**
 * Roll dice (D&D style)
 * @param sides Number of sides on die [1-6, 8, 10, 12, 20, 100]
 * @param count Number of dice to roll
 * @return Sum of all dice rolls
 * @example
 *   new damage = PRandDice(6, 2); // Roll 2d6
 *   new d20 = PRandDice(20, 1);   // Roll 1d20
 */
native PRandDice(sides, count = 1);

/**
 * Seed the PRNG generator
 * @param seed Seed value (use same seed for reproducible results)
 * @return 1 on success
 * @note Use this for debugging or replay systems
 */
native SeedPRNG(seed);

// CSPRNG Functions (ChaCha20)

/**
 * Generate cryptographically secure random integer [0, max-1]
 * @param max Maximum value (exclusive)
 * @return Random integer [0, max-1]
 * @note Slower than PRNG but unpredictable
 */
native CSPRandom(max);

/**
 * Generate cryptographically secure random in range [min, max-1]
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 * @return Random integer [min, max-1]
 */
native CSPRandRange(min, max);

/**
 * Generate cryptographically secure random float [min, max)
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 * @return Random float [min, max)
 */
native Float:CSPRandFloatRange(Float:min, Float:max);

/**
 * Generate cryptographically secure random boolean
 * @param probability Chance of returning true [0.0 - 1.0)
 * @return true or false based on probability
 */
native bool:CSPRandBool(Float:probability = 0.5);

/**
 * Generate cryptographic token
 * @param length Token length in hexadecimal digits (1-8)
 * @return Random token value
 * @note Use for session IDs, temporary passwords, etc.
 * @example new token = CSPRandToken(8); // 8-digit hex token
 */
native CSPRandToken(length);

/**
 * Fill array with cryptographically secure random bytes
 * @param dest[] Destination array
 * @param length Number of bytes to generate
 * @return true on success
 * @note Use for generating encryption keys, nonces, etc.
 */
native bool:CSPRandBytes(dest[], length);

/**
 * Generate UUID v4 (Universally Unique Identifier)
 * @param uuid[] Array to store UUID string (must be at least 37 cells)
 * @return true on success
 * @note UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 * @example 
 *   new uuid[37];
 *   CSPRandUUID(uuid);
 *   printf("Generated UUID: %s", uuid);
 */
native bool:CSPRandUUID(uuid[]);

/**
 * Seed the CSPRNG generator
 * @param seed Seed value
 * @return 1 on success
 * @warning Only use for testing! In production, let it auto-seed from OS
 */
native SeedCSPRNG(seed);

// ==============================
// Helper Macros
// ==============================

/**
 * Random chance helper - returns true X% of the time
 * @example if(RandomChance(25)) // 25% chance
 */
#define RandomChance(%0) PRandBool(Float:(%0) / 100.0)

/**
 * Cryptographically secure random chance
 * @example if(SecureRandomChance(25)) // 25% chance with CSPRNG
 */
#define SecureRandomChance(%0) CSPRandBool(Float:(%0) / 100.0)

/**
 * Flip a coin (50/50 chance)
 * @example if(CoinFlip()) // 50% chance
 */
#define CoinFlip() PRandBool(0.5)

/**
 * Secure coin flip (cryptographically secure)
 */
#define SecureCoinFlip() CSPRandBool(0.5)

/**
 * Random element from array
 * @example new item = RandomElement(items, sizeof(items));
 */
#define RandomElement(%0,%1) %0[PRandom(%1)]

/**
 * Secure random element from array
 */
#define SecureRandomElement(%0,%1) %0[CSPRandom(%1)]

/**
 * Roll standard D&D dice (fast PRNG)
 */
#define RollD4()  PRandRange(1, 5)  // 1-4
#define RollD6()  PRandRange(1, 7)  // 1-6
#define RollD8()  PRandRange(1, 9)  // 1-8
#define RollD10() PRandRange(1, 11) // 1-10
#define RollD12() PRandRange(1, 13) // 1-12
#define RollD20() PRandRange(1, 21) // 1-20
#define RollD100() PRandRange(1, 101) // 1-100

/**
 * Roll dice with cryptographically secure RNG
 */
#define SecureRollD4()  CSPRandRange(1, 5)   // 1-4
#define SecureRollD6()  CSPRandRange(1, 7)   // 1-6
#define SecureRollD8()  CSPRandRange(1, 9)   // 1-8
#define SecureRollD10() CSPRandRange(1, 11)  // 1-10
#define SecureRollD12() CSPRandRange(1, 13)  // 1-12
#define SecureRollD20() CSPRandRange(1, 21)  // 1-20
#define SecureRollD100() CSPRandRange(1, 101) // 1-100

/**
 * Pick random item from comma-separated list (for 2 items)
 * @example new color = RandomPick(0xFF0000, 0x00FF00); // 50% each
 */
#define RandomPick(%0,%1) (PRandom(2) == 0 ? (%0) : (%1))

/**
 * Shuffle array with optional range
 * @example 
 *   ShuffleArray(players); // Shuffle entire array
 *   ShuffleArrayRange(players, 3, 7); // Shuffle only indices 3-6
 */
#define ShuffleArray(%0) PRandShuffle(%0, sizeof(%0))
#define ShuffleArrayRange(%0,%1,%2) PRandShuffleRange(%0, %1, %2)

/**
 * Create weighted chance table
 * @example
 *   new item = WeightedRandom(
 *       {30, 0},   // 30% chance for item 0
 *       {50, 1},   // 50% chance for item 1
 *       {20, 2}    // 20% chance for item 2
 *   );
 */
stock WeightedRandom(...) {
    new count = numargs();
    new weights[128], items[128];
    new total = 0;
    
    for(new i = 0; i < count; i += 2) {
        weights[i/2] = getarg(i);
        items[i/2] = getarg(i + 1);
        total += weights[i/2];
    }
    
    new rand = PRandom(total);
    new sum = 0;
    
    for(new i = 0; i < count/2; i++) {
        sum += weights[i];
        if(rand < sum) return items[i];
    }
    
    return items[count/2 - 1];
}

/**
 * Generate random string with specified characters
 * @param dest[] Destination string
 * @param len Length of string to generate
 * @param charset[] Character set to use (default: alphanumeric)
 * @return Generated string
 */
stock RandomString(dest[], len = sizeof dest, const charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") {
    new charset_len = strlen(charset);
    for(new i = 0; i < len; i++) {
        dest[i] = charset[PRandom(charset_len)];
    }
    dest[len] = EOS;
    return 1;
}

/**
 * Generate secure random string (cryptographically secure)
 */
stock SecureRandomString(dest[], len = sizeof dest, const charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") {
    new charset_len = strlen(charset);
    for(new i = 0; i < len; i++) {
        dest[i] = charset[CSPRandom(charset_len)];
    }
    dest[len] = EOS;
    return 1;
}

/**
 * Get random color (RGB format)
 * @return Random color (0xRRGGBB)
 */
stock RandomColor() {
    return (PRandom(256) << 16) | (PRandom(256) << 8) | PRandom(256);
}

/**
 * Get secure random color
 */
stock SecureRandomColor() {
    return (CSPRandom(256) << 16) | (CSPRandom(256) << 8) | CSPRandom(256);
}