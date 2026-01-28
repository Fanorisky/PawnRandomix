# Pawn Randomix

Cryptographic-grade random number generator for **SA-MP** and **open.mp** servers.  
Powered by **ChaCha20** stream cipher - the same algorithm used in TLS 1.3 and WireGuard.

## Features

- **Cryptographic Security**: ChaCha20 CSPRNG (not predictable like standard `random()`)
- **Game Ready**: Dice, shuffling, weighted picks, Gaussian distributions
- **Geometric Utilities**: Random points in circles, spheres, polygons, rings
- **Token Generation**: UUID v4, hex patterns, random strings

## Installation

### open.mp
1. Download `Randomix.dll` (Windows) or `Randomix.so` (Linux)
2. Place in server `components/` directory
3. Copy `Randomix.inc` to `qawno/include/` or your Pawn include directory
4. Add `#include <Randomix>` to your scripts
5. Recompile

### SA-MP (Plugin)
1. Download `randomix.dll` / `randomix.so`
2. Place in server `plugins/` directory
3. Add `randomix` to `server.cfg` (plugins line)
4. Copy `Randomix.inc` to include directory
5. Add `#include <Randomix>` to your scripts
6. Recompile

## API Reference

### Core Random Functions
```pawn
RandRange(min, max)              // Integer in range [min, max]
RandFloatRange(Float:min, Float:max)  // Float in range [min, max)
RandBool(Float:probability)      // Boolean with probability (0.0-1.0)
RandBoolWeighted(trueW, falseW)  // Boolean with custom weights
RandWeighted(weights[], count)   // Weighted array index selection
SeedRNG(seed)                    // Set seed (testing only)
```

### Array & Shuffling
```pawn
RandShuffle(array[], count)           // Fisher-Yates shuffle
RandShuffleRange(array[], start, end) // Shuffle specific range
RandPick(array[], count)              // Pick 1 random element (O(1))
```

### String & Token Generation
```pawn
RandFormat(dest[], pattern[])    // Generate by pattern (see below)
RandBytes(dest[], length)        // Cryptographic random bytes
RandUUID(uuid[37])               // UUID v4 (RFC 4122 compliant)
```

**RandFormat Patterns:**
- `X` = Uppercase (A-Z)
- `x` = Lowercase (a-z)
- `9` = Digit (0-9)
- `A` = Alphanumeric (A-Z, a-z, 0-9)
- `!` = Symbol (!@#$%^&*...)
- `\X` = Literal character (escape)
- Others = Copied literally

```pawn
new code[16];
RandFormat(code, "PROMO-XXXX-9999");  // "PROMO-KJQM-4829"
RandFormat(code, "LICENSE-9999-x");   // "LICENSE-4823-m"
RandFormat(code, "v\\1.9A");          // "v1.9k" (literal \1)
```

### Statistical Distributions
```pawn
RandGaussian(Float:mean, Float:stddev)  // Normal distribution
RandDice(sides, count)                  // D&D style (e.g., 2d6, 1d20)
```

### 2D Geometric Distributions
```pawn
RandPointInCircle(Float:cx, Float:cy, Float:r, &Float:x, &Float:y)
RandPointOnCircle(Float:cx, Float:cy, Float:r, &Float:x, &Float:y)
RandPointInRect(Float:minX, Float:minY, Float:maxX, Float:maxY, &Float:x, &Float:y)
RandPointInRing(Float:cx, Float:cy, Float:innerR, Float:outerR, &Float:x, &Float:y)
RandPointInEllipse(Float:cx, Float:cy, Float:rx, Float:ry, &Float:x, &Float:y)
RandPointInTriangle(Float:x1, Float:y1, Float:x2, Float:y2, Float:x3, Float:y3, &Float:x, &Float:y)
RandPointInPolygon(const Float:verts[], count, &Float:x, &Float:y)
```

### 3D Geometric Distributions
```pawn
RandPointInSphere(Float:cx, Float:cy, Float:cz, Float:r, &Float:x, &Float:y, &Float:z)
RandPointOnSphere(Float:cx, Float:cy, Float:cz, Float:r, &Float:x, &Float:y, &Float:z)
RandPointInBox(Float:minX, Float:minY, Float:minZ, Float:maxX, Float:maxY, Float:maxZ, &Float:x, &Float:y, &Float:z)
```

## Usage Examples

### Basic Random
```pawn
#include <Randomix>

public OnPlayerConnect(playerid)
{
    // Random spawn
    new Float:x, Float:y;
    RandPointInCircle(0.0, 0.0, 100.0, x, y);
    SetPlayerPos(playerid, x, y, 5.0);
    
    // Random weapon (1-5 slots)
    GivePlayerWeapon(playerid, RandRange(22, 34), 100);
}
```

### Loot System
```pawn
GiveRandomLoot(playerid)
{
    new lootTable[] = {301, 302, 303, 304}; // Item IDs
    new item = RandPick(lootTable, sizeof(lootTable));
    
    // Or with weighted rarity
    new weights[] = {60, 25, 10, 5}; // Common to Rare
    new idx = RandWeighted(weights, sizeof(weights));
    item = lootTable[idx];
    
    GivePlayerItem(playerid, item);
}
```

### Secure Tokens
```pawn
GeneratePromoCode(playerid)
{
    new code[16];
    RandFormat(code, "PROMO-XXXX-9999");
    // Result: "PROMO-K9QM-4829"
    
    SendClientMessage(playerid, -1, code);
}
```

### Geometric Spawning
```pawn
// Spawn enemies in ring (not too close, not too far)
new Float:x, Float:y;
RandPointInRing(0.0, 0.0, 50.0, 150.0, x, y);
CreateEnemy(x, y, 0.0);

// Random point in triangle zone
new Float:verts[] = {0.0, 0.0, 100.0, 0.0, 50.0, 100.0};
RandPointInPolygon(verts, 3, x, y);
```

### Statistical Rolls
```pawn
// Gaussian distribution for damage (critical hits rare, average common)
new damage = RandGaussian(50.0, 10.0); // Mean 50, SD 10

// D&D style dice
new dmg = RandDice(6, 2);  // Roll 2d6
new crit = RandDice(20, 1); // Roll 1d20
```

## Helper Macros (Stock)

```pawn
Random(max)                    // 0 to max-1
RandFloat(max)                 // 0.0 to max
RandomColor()                  // 0xRRGGBB
RandExcept(min, max, except)   // Range with exclusion
RandomChance(25)               // 25% boolean
CoinFlip()                     // 50/50
RollD6() / RollD20() / RollD100()  // Standard dice
ShuffleArray(array)            // Macro for RandShuffle
RandomElement(array, size)     // Random index access
```

## Technical Details

- **Algorithm**: ChaCha20 (20 rounds)
- **Security**: 256-bit key, 64-bit nonce
- **Reseeding**: Automatic after 1GB of output
- **Memory**: ~150 bytes per instance (singleton)
- **Threading**: Mutex-protected (safe for multi-thread)

## Credits

- **ChaCha20**: Daniel J. Bernstein
- **Implementation**: Fanorisky
- **SA-MP/open.mp Plugin SDK**: SA-MP Team, open.mp Contributors

## License

MIT License - See LICENSE file for details

GitHub: https://github.com/Fanorisky/PawnRandomix
```
