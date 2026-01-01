# Pawn Randomix

Enhanced random number generator and utility for open.mp servers (SA-MP support planned), providing a PRNG (PCG32) for game mechanics and a CSPRNG (ChaCha20) for cryptographic security.

## Installation

1. Download component binary for your platform (Windows `.dll`, Linux `.so`)
2. Place in server `components/` directory
3. Copy `Randomix.inc` to Pawn include directory
4. Add `#include <Randomix>` to your scripts
5. Recompile

## Functions

### PRNG (Fast Game Mechanics)
```pawn
PRandom(max)                     // 0 to max-1
PRandRange(min, max)             // Range integer
PRandFloatRange(Float:min, Float:max)  // Range float
PRandBool(Float:probability)     // Probability boolean
PRandWeighted(weights[], count)  // Weighted selection
PRandShuffle(array[], count)     // Fisher-Yates shuffle
PRandGaussian(Float:mean, Float:stddev) // Normal distribution
PRandDice(sides, count)          // Dice roll (2d6, 1d20, etc.)
SeedPRNG(seed)                   // Set PRNG seed
```

### CSPRNG (Cryptographic Security)
```pawn
CSPRandom(max)                   // Secure 0 to max-1
CSPRandRange(min, max)           // Secure range
CSPRandFloatRange(Float:min, Float:max) // Secure float
CSPRandBool(Float:probability)   // Secure boolean
CSPRandToken(length)             // Hex token (1-8 digits)
CSPRandBytes(dest[], length)     // Cryptographic bytes
CSPRandUUID(uuid[])              // UUID v4 string
SeedCSPRNG(seed)                 // Set CSPRNG seed (testing only)
```

## Usage

```pawn
#include <pawn-randomix>

// Game mechanics (fast PRNG)
new damage = PRandom(100);
new position = PRandRange(10, 50);
new lootIndex = PRandWeighted(lootWeights, 4);
ShuffleArray(players);

// Security operations (CSPRNG)
new token = CSPRandToken(8);
new uuid[37];
CSPRandUUID(uuid);
```

## Helper Macros

```pawn
RandomChance(25)         // 25% chance
CoinFlip()              // 50/50 chance
RollD20()               // 1d20 dice roll
RandomElement(array, size) // Random array element
SecureRandomElement(array, size) // Secure random element
ShuffleArray(array)     // Shuffle entire array
```

## Credits
- PCG Random by Melissa O'Neill
- ChaCha20 by Daniel J. Bernstein
- open.mp Component SDK by AmyrAhmady
- Fanorisky (me)

GitHub: https://github.com/Fanorisky/pawn-randomix