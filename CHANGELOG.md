# Changelog

## [2.0.1] - 2026-01-31

### Added
- New native `RandPointInArc()` - Generate random point in circular sector
- New stock functions:
  - `RandInt(min, max)` - Alias for RandRange
  - `RandAngle()` - Random angle [0, 2π) in radians
  - `RandSign()` - Random +1 or -1
  - `RandColorAlpha(alpha)` - RGBA color with alpha
  - `RandVecInRange(min, max, &x, &y, &z)` - Random 3D vector
  - `RandLoot(itemIds, weights, count)` - Weighted loot table selection
  - `RandPointInCircleAroundPlayer(playerid, radius, &x, &y)` - Spawn around player
- New macros:
  - `RandD2()` through `RandD100()` - D&D dice macros
  - `RandShuf(array)` - Shuffle array shorthand
  - `RandShufRange(array, start, end)` - Shuffle range shorthand
  - `RandElem(array, size)` - Random element accessor
  - `RandEither(opt1, opt2)` - Pick one of two options
  - `RandChance(percent)` - Percentage chance shorthand
- Mathematical constants: `RANDIX_PI`, `RANDIX_TWO_PI`, `RANDIX_HALF_PI`
- Angle conversion macros: `Deg2Rad()`, `Rad2Deg()`
- `[[nodiscard]]` attributes on ChaChaRNG methods
- `.dockerignore` file for faster Docker builds
- Docker updated to Ubuntu 22.04 with Clang 14

### Changed
- **BREAKING**: `RandRange(min, max)` now returns [min, max) instead of [min, max]
  - Now matches SA-MP's half-open range convention
  - `Random(10)` and `RandRange(0, 10)` both return 0-9
- Code deduplication: Created `randomix_impl.hpp` shared header
  - Eliminated ~90% duplicate code between SA-MP and open.mp builds
- Renamed stocks for consistency:
  - `RandomColor()` → `RandColor()`
  - `RandomColorAlpha()` → `RandColorAlpha()`
  - `RandomSign()` → `RandSign()`
  - `RandExcept()` → `RandExc()`
  - `RandExceptMany()` → `RandExcMany()`
  - `RandHexString()` → `RandHex()`
  - `RandString()` → `RandStr()`
  - `ShuffleArray()` → `RandShuf()`
  - `ShuffleArrayRange()` → `RandShufRange()`
  - `RandomElement()` → `RandElem()`
  - `RandomPick()` → `RandEither()`
  - `RandomChance()` → `RandChance()`
  - `RollD*()` → `RandD*()`
  - `DegToRad()` → `Deg2Rad()`
  - `RadToDeg()` → `Rad2Deg()`
- Simplified code comments (removed `// ===` blocks)
- Updated load/unload messages with version info
- CI/CD now builds both open.mp and SA-MP variants

### Fixed
- `RandPointInPolygon`: Replaced heap allocation with stack buffer
  - Fixed performance issue with `std::vector<float>`
- Added comprehensive bounds checking:
  - NaN/Inf detection for all float inputs
  - Integer overflow protection in weighted functions
  - Sanity limits on array sizes and iterations
- Fixed typo in documentation: "untuk" → "for"
- Fixed redundant null check in `onFree()` handler
- Fixed CMake compatibility with `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`
- Removed unused `minSize` parameter from `CheckArrayBounds()`
- Fixed `RandLoot` sizeof reliability issue

### Security
- ChaCha20 CSPRNG with 256-bit key, 64-bit nonce
- Auto-reseed after 1GB of generated data
- Thread-safe with mutex protection
- Secure memory scrubbing in destructor

## [2.0.0] - 2026-01-28

### Initial Release
- ChaCha20-based CSPRNG
- SA-MP and open.mp support
- Core functions: `RandRange`, `RandFloatRange`, `RandBool`, `RandWeighted`
- Array operations: `RandShuffle`, `RandPick`
- String/Token: `RandFormat`, `RandBytes`, `RandUUID`
- Statistics: `RandGaussian`, `RandDice`
- 2D Geometry: Circle, Ring, Ellipse, Triangle, Rectangle, Polygon
- 3D Geometry: Sphere, Box
- Helper stocks: `Random`, `RandomFloat`, `RandExcept`, `RandomColor`, etc.
