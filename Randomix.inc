// ==============================
// Randomix Include File
// Version: 2.0.0 (CSPRNG Only)
// Author: Fanorisky
// Algorithm: ChaCha20 (Cryptographic)
// ==============================

#if defined _randomix_included
    #endinput
#endif
#define _randomix_included

// ==============================
// 1. CORE RANDOM FUNCTIONS (ChaCha20 CSPRNG)
// ==============================

/**
 * Seed the RNG generator
 * @param seed Seed value
 * @return 1 on success
 * @warning Seeding compromises forward secrecy. Use only for testing/replay.
 * @note By default, auto-seeds from OS entropy on plugin load
 */
native SeedRNG(seed);

/**
 * Generate cryptographically secure random integer in range [min, max] (inclusive)
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random integer [min, max]
 * @example RandRange(1, 6) returns 1-6 (secure dice roll)
 * @note Uses rejection sampling for unbiased distribution
 */
native RandRange(min, max);

/**
 * Generate cryptographically secure random float in range [min, max) (max exclusive)
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 * @return Random float [min, max)
 */
native Float:RandFloatRange(Float:min, Float:max);

/**
 * Generate cryptographically secure random boolean with probability
 * @param probability Chance of returning true [0.0 - 1.0]
 * @return true or false based on probability
 * @example RandBool(0.3) has 30% chance of true
 */
native bool:RandBool(Float:probability = 0.5);

/**
 * Random boolean with weighted chance
 * @param trueWeight Weight for true result
 * @param falseWeight Weight for false result
 * @return true or false based on weights
 * @example RandBoolWeighted(75, 25) has 75% true, 25% false
 */
native bool:RandBoolWeighted(trueWeight, falseWeight);

/**
 * Weighted random selection from array
 * @param weights[] Array of weights (higher = more likely, must be > 0)
 * @param count Number of elements
 * @return Selected index based on weights [0, count-1]
 * @example new weights[] = {10, 30, 60}; new result = RandWeighted(weights, 3);
 */
native RandWeighted(const weights[], count = sizeof weights);

/**
 * Shuffle array randomly (in-place) using Fisher-Yates
 * @param array[] Array to shuffle
 * @param count Number of elements
 * @return true on success
 */
native bool:RandShuffle(array[], count = sizeof array);

/**
 * Shuffle part of array (specific range)
 * @param array[] Array to shuffle
 * @param start Starting index (inclusive)
 * @param end Ending index (inclusive)
 * @return true on success
 */
native bool:RandShuffleRange(array[], start, end);

/**
 * Generate random number with Gaussian/Normal distribution
 * @param mean Center of distribution
 * @param stddev Standard deviation (spread, must be > 0)
 * @return Random integer following normal distribution (clamped >= 0)
 * @example RandGaussian(100.0, 15.0) typically returns 85-115
 */
native RandGaussian(Float:mean, Float:stddev);

/**
 * Roll dice (D&D style) with secure RNG
 * @param sides Number of sides (must be > 0)
 * @param count Number of dice (must be > 0)
 * @return Sum of all dice [count, count*sides]
 * @example RandDice(6, 2) rolls 2d6 (2-12)
 */
native RandDice(sides, count = 1);

// ==============================
// 2. UTILITY FUNCTIONS (NEW)
// ==============================

/**
 * Pick random element from array (without modifying array)
 * @param array[] Source array (integer or float treated as cell)
 * @param count Number of elements
 * @return Random element from array
 * @example 
 *   new weapons[] = {24, 25, 31, 34};
 *   new gun = RandPick(weapons, sizeof(weapons));
 * @note More efficient than RandShuffle if you only need 1 item
 * @note Uses uniform distribution
 */
native RandPick(const array[], count = sizeof array);

/**
 * Generate random string based on pattern template
 * @param dest[] Destination string array
 * @param pattern[] Pattern template:
 *   X = Uppercase A-Z
 *   x = Lowercase a-z  
 *   9 = Digit 0-9
 *   A = Alphanumeric A-Z, a-z, 0-9
 *   ! = Symbol !@#$%^&*()_+-=[]{}|;:,.<>?
 *   \X = Literal character X (escape with backslash)
 *   other chars = Copied literally (e.g., "-" stays "-")
 * @param maxLen Maximum length of destination array (sizeof)
 * @return 1 on success, 0 on failure (null pointer or overflow)
 * @example
 *   new code[16];
 *   RandFormat(code, "PROMO-XXXX-9999");    // "PROMO-KJQM-4829"
 *   RandFormat(code, "LICENSE-9999-x");     // "LICENSE-4823-m"  
 *   RandFormat(code, "v\\1.9A");            // "v1.9k" (literal \1)
 *   RandFormat(code, "KEY_!9A");            // "KEY_@4p"
 * @note Pattern length must be < maxLen to avoid truncation
 * @note Uses CSPRNG untuk setiap karakter generated
 */
native bool:RandFormat(dest[], const pattern[], maxLen = sizeof dest);

// ==============================
// 3. CRYPTOGRAPHIC FUNCTIONS
// ==============================

/**
 * Fill array with cryptographically secure random bytes
 * @param dest[] Destination array
 * @param length Number of bytes to generate
 * @return true on success
 * @note Use for encryption keys, nonces, salts, session tokens
 * @example new key[32]; RandBytes(key, 32); // 256-bit key
 */
native bool:RandBytes(dest[], length);

/**
 * Generate UUID v4 (Universally Unique Identifier)
 * @param uuid[] Array to store UUID string (minimum 37 cells)
 * @return true on success
 * @note UUID format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx (RFC 4122)
 * @warning Array must be at least 37 cells (36 chars + null)
 */
native bool:RandUUID(uuid[37]);

// ==============================
// 4. 2D GEOMETRIC DISTRIBUTIONS
// ==============================

/**
 * Generate random point inside circle (uniform distribution)
 * @param centerX Circle center X
 * @param centerY Circle center Y
 * @param radius Circle radius (> 0)
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @return true on success
 * @note Uses square root method for true uniform distribution (not naive r^2)
 */
native bool:RandPointInCircle(Float:centerX, Float:centerY, Float:radius, &Float:x, &Float:y);

/**
 * Generate random point ON circle edge (circumference only)
 * @param centerX Circle center X
 * @param centerY Circle center Y
 * @param radius Circle radius (> 0)
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @return true on success
 */
native bool:RandPointOnCircle(Float:centerX, Float:centerY, Float:radius, &Float:x, &Float:y);

/**
 * Generate random point in rectangle
 * @param minX Minimum X
 * @param minY Minimum Y
 * @param maxX Maximum X
 * @param maxY Maximum Y
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @return true on success
 * @note If min > max, values are swapped automatically
 */
native bool:RandPointInRect(Float:minX, Float:minY, Float:maxX, Float:maxY, &Float:x, &Float:y);

/**
 * Generate random point in ring/donut (between two circles)
 * @param centerX Center X
 * @param centerY Center Y
 * @param innerRadius Inner radius (>= 0)
 * @param outerRadius Outer radius (> innerRadius)
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @return true on success
 */
native bool:RandPointInRing(Float:centerX, Float:centerY, Float:innerRadius, Float:outerRadius, &Float:x, &Float:y);

/**
 * Generate random point in ellipse
 * @param centerX Center X
 * @param centerY Center Y
 * @param radiusX Horizontal radius (> 0)
 * @param radiusY Vertical radius (> 0)
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @return true on success
 */
native bool:RandPointInEllipse(Float:centerX, Float:centerY, Float:radiusX, Float:radiusY, &Float:x, &Float:y);

/**
 * Generate random point in triangle
 * @param x1,y1 First vertex
 * @param x2,y2 Second vertex
 * @param x3,y3 Third vertex
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @return true on success
 * @note Uses barycentric coordinates for uniform distribution
 */
native bool:RandPointInTriangle(Float:x1, Float:y1, Float:x2, Float:y2, Float:x3, Float:y3, &Float:x, &Float:y);

// ==============================
// 5. 3D GEOMETRIC DISTRIBUTIONS
// ==============================

/**
 * Generate random point inside sphere (uniform volume)
 * @param centerX Sphere center X
 * @param centerY Sphere center Y
 * @param centerZ Sphere center Z
 * @param radius Sphere radius (> 0)
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @param z Output Z (by reference)
 * @return true on success
 */
native bool:RandPointInSphere(Float:centerX, Float:centerY, Float:centerZ, Float:radius, &Float:x, &Float:y, &Float:z);

/**
 * Generate random point ON sphere surface (shell)
 * @param centerX Sphere center X
 * @param centerY Sphere center Y
 * @param centerZ Sphere center Z
 * @param radius Sphere radius (> 0)
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @param z Output Z (by reference)
 * @return true on success
 * @note Uses Marsaglia's method for uniform distribution on surface
 */
native bool:RandPointOnSphere(Float:centerX, Float:centerY, Float:centerZ, Float:radius, &Float:x, &Float:y, &Float:z);

/**
 * Generate random point in box/cuboid
 * @param minX,minY,minZ Minimum corner
 * @param maxX,maxY,maxZ Maximum corner
 * @param x,y,z Output coordinates (by reference)
 * @return true on success
 */
native bool:RandPointInBox(Float:minX, Float:minY, Float:minZ, Float:maxX, Float:maxY, Float:maxZ, &Float:x, &Float:y, &Float:z);

// ==============================
// 6. ADVANCED GEOMETRY
// ==============================

/**
 * Generate random point in convex polygon (2D)
 * @param vertices[] Flat array {x1, y1, x2, y2, ...}
 * @param vertexCount Number of vertices (>= 3)
 * @param x Output X (by reference)
 * @param y Output Y (by reference)
 * @return true on success
 * @note Polygon must be convex for uniform distribution
 * @note Uses triangulation method weighted by area
 */
native bool:RandPointInPolygon(const Float:vertices[], vertexCount, &Float:x, &Float:y);

// ==============================
// 7. CONVENIENCE STOCK FUNCTIONS
// ==============================

/**
 * Random integer [0, max-1]
 * @param max Maximum value (exclusive)
 * @return Random integer [0, max-1]
 */
stock Random(max) { 
    return RandRange(0, max - 1); 
}

/**
 * Random float [0.0, max)
 * @param max Maximum value (exclusive)
 * @return Random float [0.0, max)
 */
stock Float:RandFloat(Float:max) { 
    return RandFloatRange(0.0, max); 
}

/**
 * Random integer in range with excluded value
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @param except Value to exclude
 * @return Random integer [min, max] except 'except'
 * @note Efficient implementation without retry loop
 */
stock RandExcept(min, max, except) {
    if (min > max) {
        new temp = min;
        min = max;
        max = temp;
    }
    if (min == max) return min;
    if (except < min || except > max) return RandRange(min, max);
    
    new result = RandRange(min, max - 1);
    return (result >= except) ? (result + 1) : result;
}

/**
 * Random RGB color (24-bit)
 * @return Random color integer (0xRRGGBB)
 * @example SetPlayerColor(playerid, RandomColor());
 */
stock RandomColor() {
    return (Random(256) << 16) | (Random(256) << 8) | Random(256);
}

/**
 * Random hex string
 * @param dest[] Destination string
 * @param len Length of hex string
 * @return 1 on success, 0 if len <= 0
 * @example new hex[9]; RandHexString(hex, 8); // "A3F5D2B1"
 */
stock RandHexString(dest[], len) {
    if (len <= 0) return 0;
    static const hex[] = "0123456789ABCDEF";
    for (new i = 0; i < len; i++) {
        dest[i] = hex[Random(16)];
    }
    dest[len] = EOS;
    return 1;
}

/**
 * Random alphanumeric string
 * @param dest[] Destination string
 * @param len Desired length (NOT including null terminator)
 * @param charset[] Character set to use (default: alphanumeric)
 * @return 1 on success, 0 if charset empty
 * @note Destination must be at least len+1 cells
 */
stock RandString(dest[], len, const charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") {
    new charset_len = strlen(charset);
    if (charset_len == 0) return 0;
    for (new i = 0; i < len; i++) {
        dest[i] = charset[Random(charset_len)];
    }
    dest[len] = EOS;
    return 1;
}

/**
 * Pick random element from float array
 * @param array[] Float array
 * @param count Number of elements
 * @return Random float from array
 * @note Workaround since RandPick works on cells
 */
stock Float:RandPickFloat(const Float:array[], count = sizeof array) {
    new idx = RandRange(0, count - 1);
    return array[idx];
}

// ==============================
// 8. USEFUL MACROS
// ==============================

/**
 * Random chance X%
 * @example if(RandomChance(25)) // 25% chance
 */
#define RandomChance(%0) RandBool(Float:(%0) / 100.0)

/**
 * Flip a coin (50/50)
 */
#define CoinFlip() RandBool(0.5)

/**
 * Random element from array
 * @example new item = RandomElement(items, sizeof(items));
 */
#define RandomElement(%0,%1) %0[Random(%1)]

/**
 * Pick random from two options
 * @example new color = RandomPick(0xFF0000, 0x00FF00);
 */
#define RandomPick(%0,%1) (RandBool(0.5) ? (%0) : (%1))

/**
 * Shuffle entire array
 * @example ShuffleArray(players);
 */
#define ShuffleArray(%0) RandShuffle(%0, sizeof(%0))

/**
 * Shuffle array range
 * @example ShuffleArrayRange(players, 3, 7);
 */
#define ShuffleArrayRange(%0,%1,%2) RandShuffleRange(%0, %1, %2)

/**
 * Standard D&D dice macros
 */
#define RollD4()  RandRange(1, 4)
#define RollD6()  RandRange(1, 6)
#define RollD8()  RandRange(1, 8)
#define RollD10() RandRange(1, 10)
#define RollD12() RandRange(1, 12)
#define RollD20() RandRange(1, 20)
#define RollD100() RandRange(1, 100)