/*
 *  Randomix - Enhanced Random Number Generator for SA-MP Servers
 *  Copyright (c) 2025, Fanorisky
 *  GitHub: github.com/Fanorisky/PawnRandomix
 */
//#include "platform.h"
#include "plugin.h"
#include "amx/amx.h"
#include "amx/amx2.h"
#include "randomix.hpp"
#include <chrono>
#include <cmath>
#include <mutex>
#include <vector>

// Plugin data
extern void *pAMXFunctions;
typedef void (*logprintf_t)(const char* format, ...);
logprintf_t logprintf;

// Plugin Info
PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
    return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** ppData) {
    pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
    logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
    
    // Initialize random generators with system time
    uint64_t seed = static_cast<uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    
    RandomixGenerators::SeedPRNG(seed);
    RandomixGenerators::SeedCSPRNG(seed);
    
    logprintf("");
    logprintf("  Randomix Plugin Loaded");
    logprintf("  Version: v%s", RANDOMIX_VERSION);
    logprintf("  Author: Fanorisky");
    logprintf("  GitHub: github.com/Fanorisky/PawnRandomix");
    logprintf("");
    
    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
    logprintf("Randomix Plugin Unloaded");
}

// Helper function to get array address
static inline cell* GetArrayAddress(AMX* amx, cell address) {
    cell* phys_addr;
    amx_GetAddr(amx, address, &phys_addr);
    return phys_addr;
}

// native PRandRange(min, max);
static cell AMX_NATIVE_CALL n_PRandRange(AMX* amx, cell* params) {
    int min = static_cast<int>(params[1]);
    int max = static_cast<int>(params[2]);
    
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return min + static_cast<cell>(RandomixGenerators::GetPRNG().next_bounded(range + 1));
}

// native CSPRandRange(min, max);
static cell AMX_NATIVE_CALL n_CSPRandRange(AMX* amx, cell* params) {
    int min = static_cast<int>(params[1]);
    int max = static_cast<int>(params[2]);
    
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return min + static_cast<cell>(RandomixGenerators::GetCSPRNG().next_bounded(range + 1));
}

// native Float:PRandFloatRange(Float:min, Float:max);
static cell AMX_NATIVE_CALL n_PRandFloatRange(AMX* amx, cell* params) {
    float min = amx_ctof(params[1]);
    float max = amx_ctof(params[2]);
    
    if (min > max) std::swap(min, max);
    if (min == max) return params[1];
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    float result = min + RandomixGenerators::GetPRNG().next_float() * (max - min);
    return amx_ftoc(result);
}

// native Float:CSPRandFloatRange(Float:min, Float:max);
static cell AMX_NATIVE_CALL n_CSPRandFloatRange(AMX* amx, cell* params) {
    float min = amx_ctof(params[1]);
    float max = amx_ctof(params[2]);
    
    if (min > max) std::swap(min, max);
    if (min == max) return params[1];
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    float result = min + RandomixGenerators::GetCSPRNG().next_float() * (max - min);
    return amx_ftoc(result);
}

// native SeedPRNG(seed);
static cell AMX_NATIVE_CALL n_SeedPRNG(AMX* amx, cell* params) {
    RandomixGenerators::SeedPRNG(static_cast<uint64_t>(params[1]));
    return 1;
}

// native SeedCSPRNG(seed);
static cell AMX_NATIVE_CALL n_SeedCSPRNG(AMX* amx, cell* params) {
    RandomixGenerators::SeedCSPRNG(static_cast<uint64_t>(params[1]));
    return 1;
}

// native bool:PRandBool(Float:probability);
static cell AMX_NATIVE_CALL n_PRandBool(AMX* amx, cell* params) {
    float probability = amx_ctof(params[1]);
    
    if (probability <= 0.0f) return 0;
    if (probability >= 1.0f) return 1;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return RandomixGenerators::GetPRNG().next_float() < probability ? 1 : 0;
}

// native bool:CSPRandBool(Float:probability);
static cell AMX_NATIVE_CALL n_CSPRandBool(AMX* amx, cell* params) {
    float probability = amx_ctof(params[1]);
    
    if (probability <= 0.0f) return 0;
    if (probability >= 1.0f) return 1;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return RandomixGenerators::GetCSPRNG().next_float() < probability ? 1 : 0;
}

// native bool:PRandBoolWeighted(trueWeight, falseWeight);
static cell AMX_NATIVE_CALL n_PRandBoolWeighted(AMX* amx, cell* params) {
    int trueW = static_cast<int>(params[1]);
    int falseW = static_cast<int>(params[2]);
    
    if (trueW <= 0) return 0;
    if (falseW <= 0) return 1;
    
    uint32_t total = static_cast<uint32_t>(trueW + falseW);
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return RandomixGenerators::GetPRNG().next_bounded(total) < static_cast<uint32_t>(trueW) ? 1 : 0;
}

// native PRandWeighted(const weights[], count);
static cell AMX_NATIVE_CALL n_PRandWeighted(AMX* amx, cell* params) {
    int count = static_cast<int>(params[2]);
    if (count <= 0) return 0;
    
    cell* weights = GetArrayAddress(amx, params[1]);
    if (!weights) return 0;
    
    uint32_t total = 0;
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0) {
            total += static_cast<uint32_t>(weights[i]);
        }
    }
    
    if (total == 0) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    uint32_t rand = RandomixGenerators::GetPRNG().next_bounded(total);
    uint32_t sum = 0;
    
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0) {
            sum += static_cast<uint32_t>(weights[i]);
            if (rand < sum) return i;
        }
    }
    
    return count - 1;
}

// native bool:PRandShuffle(array[], count);
static cell AMX_NATIVE_CALL n_PRandShuffle(AMX* amx, cell* params) {
    int count = static_cast<int>(params[2]);
    if (count <= 1) return 1;
    
    cell* array = GetArrayAddress(amx, params[1]);
    if (!array) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    for (int i = count - 1; i > 0; i--) {
        int j = static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(i + 1));
        std::swap(array[i], array[j]);
    }
    
    return 1;
}

// native bool:PRandShuffleRange(array[], start, end);
static cell AMX_NATIVE_CALL n_PRandShuffleRange(AMX* amx, cell* params) {
    cell* array = GetArrayAddress(amx, params[1]);
    if (!array) return 0;
    
    int start = static_cast<int>(params[2]);
    int end = static_cast<int>(params[3]);
    
    if (start > end) std::swap(start, end);
    if (end - start < 1) return 1;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    for (int i = end; i > start; i--) {
        int j = start + RandomixGenerators::GetPRNG().next_bounded(i - start + 1);
        std::swap(array[i], array[j]);
    }
    
    return 1;
}

// native PRandGaussian(Float:mean, Float:stddev);
static cell AMX_NATIVE_CALL n_PRandGaussian(AMX* amx, cell* params) {
    float mean = amx_ctof(params[1]);
    float stddev = amx_ctof(params[2]);
    
    if (stddev <= 0.0f) return static_cast<cell>(mean);
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float u1 = RandomixGenerators::GetPRNG().next_float();
    float u2 = RandomixGenerators::GetPRNG().next_float();
    
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
    float result = mean + z0 * stddev;
    
    return static_cast<cell>(result < 0.0f ? 0.0f : result);
}

// native PRandDice(sides, count);
static cell AMX_NATIVE_CALL n_PRandDice(AMX* amx, cell* params) {
    int sides = static_cast<int>(params[1]);
    int count = static_cast<int>(params[2]);
    
    if (sides <= 0 || count <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    uint32_t total = 0;
    uint32_t uSides = static_cast<uint32_t>(sides);
    
    for (int i = 0; i < count; i++) {
        total += RandomixGenerators::GetPRNG().next_bounded(uSides) + 1;
    }
    
    return static_cast<cell>(total);
}

// native CSPRandToken(length);
static cell AMX_NATIVE_CALL n_CSPRandToken(AMX* amx, cell* params) {
    int length = static_cast<int>(params[1]);
    if (length <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    uint32_t token = 0;
    int actualLength = (length > 8) ? 8 : length;
    
    for (int i = 0; i < actualLength; i++) {
        token = (token << 4) | (RandomixGenerators::GetCSPRNG().next_bounded(16));
    }
    
    return static_cast<cell>(token);
}

// native bool:CSPRandBytes(dest[], length);
static cell AMX_NATIVE_CALL n_CSPRandBytes(AMX* amx, cell* params) {
    int length = static_cast<int>(params[2]);
    if (length <= 0) return 0;
    
    cell* dest = GetArrayAddress(amx, params[1]);
    if (!dest) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    for (int i = 0; i < length; i++) {
        dest[i] = static_cast<cell>(
            RandomixGenerators::GetCSPRNG().next_uint32() & 0xFF
        );
    }
    
    return 1;
}

// native bool:CSPRandUUID(dest[]);
static cell AMX_NATIVE_CALL n_CSPRandUUID(AMX* amx, cell* params) {
    cell* out = GetArrayAddress(amx, params[1]);
    if (!out) return 0;
    
    uint8_t bytes[16];
    {
        std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
        RandomixGenerators::GetCSPRNG().next_bytes(bytes, 16);
    }
    
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;
    
    static const char* hex = "0123456789abcdef";
    int p = 0;
    
    auto write = [&](uint8_t b) {
        out[p++] = hex[b >> 4];
        out[p++] = hex[b & 0xF];
    };
    
    for (int i = 0; i < 16; i++) {
        write(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9)
            out[p++] = '-';
    }
    out[p] = '\0';
    
    return 1;
}

// ============================================================================
// NEW 2D POINT FUNCTIONS (DARI OPEN.MP) - FIXED VERSION
// ============================================================================

// native bool:PRandPointInCircle(Float:centerX, Float:centerY, Float:radius, &Float:outX, &Float:outY);
static cell AMX_NATIVE_CALL n_PRandPointInCircle(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radius = amx_ctof(params[3]);
    
    if (radius <= 0.0f) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[4], &outX);
    amx_GetAddr(amx, params[5], &outY);
    
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float angle = RandomixGenerators::GetPRNG().next_float() * 6.28318530718f;
    float r = radius * sqrtf(RandomixGenerators::GetPRNG().next_float());
    
    // FIXED: Store in temporary float first
    float resultX = centerX + r * cosf(angle);
    float resultY = centerY + r * sinf(angle);
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    
    return 1;
}

// native bool:CSPRandPointInCircle(Float:centerX, Float:centerY, Float:radius, &Float:outX, &Float:outY);
static cell AMX_NATIVE_CALL n_CSPRandPointInCircle(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radius = amx_ctof(params[3]);
    
    if (radius <= 0.0f) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[4], &outX);
    amx_GetAddr(amx, params[5], &outY);
    
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    float angle = RandomixGenerators::GetCSPRNG().next_float() * 6.28318530718f;
    float r = radius * sqrtf(RandomixGenerators::GetCSPRNG().next_float());
    
    // FIXED
    float resultX = centerX + r * cosf(angle);
    float resultY = centerY + r * sinf(angle);
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    
    return 1;
}

// native bool:PRandPointOnCircle(Float:centerX, Float:centerY, Float:radius, &Float:outX, &Float:outY);
static cell AMX_NATIVE_CALL n_PRandPointOnCircle(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radius = amx_ctof(params[3]);
    
    if (radius <= 0.0f) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[4], &outX);
    amx_GetAddr(amx, params[5], &outY);
    
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float angle = RandomixGenerators::GetPRNG().next_float() * 6.28318530718f;
    
    // FIXED
    float resultX = centerX + radius * cosf(angle);
    float resultY = centerY + radius * sinf(angle);
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    
    return 1;
}

// native bool:PRandPointInRect(Float:minX, Float:minY, Float:maxX, Float:maxY, &Float:outX, &Float:outY);
static cell AMX_NATIVE_CALL n_PRandPointInRect(AMX* amx, cell* params) {
    float minX = amx_ctof(params[1]);
    float minY = amx_ctof(params[2]);
    float maxX = amx_ctof(params[3]);
    float maxY = amx_ctof(params[4]);
    
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // FIXED
    float resultX = minX + RandomixGenerators::GetPRNG().next_float() * (maxX - minX);
    float resultY = minY + RandomixGenerators::GetPRNG().next_float() * (maxY - minY);
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    
    return 1;
}

// native bool:PRandPointInRing(Float:centerX, Float:centerY, Float:innerRadius, Float:outerRadius, &Float:outX, &Float:outY);
static cell AMX_NATIVE_CALL n_PRandPointInRing(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float innerRadius = amx_ctof(params[3]);
    float outerRadius = amx_ctof(params[4]);
    
    if (innerRadius < 0.0f || outerRadius <= 0.0f || innerRadius >= outerRadius) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float angle = RandomixGenerators::GetPRNG().next_float() * 6.28318530718f;
    float innerRadiusSq = innerRadius * innerRadius;
    float outerRadiusSq = outerRadius * outerRadius;
    float r = sqrtf(innerRadiusSq + RandomixGenerators::GetPRNG().next_float() * (outerRadiusSq - innerRadiusSq));
    
    // FIXED
    float resultX = centerX + r * cosf(angle);
    float resultY = centerY + r * sinf(angle);
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    
    return 1;
}

// native bool:PRandPointInEllipse(Float:centerX, Float:centerY, Float:radiusX, Float:radiusY, &Float:outX, &Float:outY);
static cell AMX_NATIVE_CALL n_PRandPointInEllipse(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radiusX = amx_ctof(params[3]);
    float radiusY = amx_ctof(params[4]);
    
    if (radiusX <= 0.0f || radiusY <= 0.0f) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float angle = RandomixGenerators::GetPRNG().next_float() * 6.28318530718f;
    float r = sqrtf(RandomixGenerators::GetPRNG().next_float());
    
    // FIXED
    float resultX = centerX + radiusX * r * cosf(angle);
    float resultY = centerY + radiusY * r * sinf(angle);
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    
    return 1;
}

// native bool:PRandPointInTriangle(Float:x1, Float:y1, Float:x2, Float:y2, Float:x3, Float:y3, &Float:outX, &Float:outY);
static cell AMX_NATIVE_CALL n_PRandPointInTriangle(AMX* amx, cell* params) {
    float x1 = amx_ctof(params[1]);
    float y1 = amx_ctof(params[2]);
    float x2 = amx_ctof(params[3]);
    float y2 = amx_ctof(params[4]);
    float x3 = amx_ctof(params[5]);
    float y3 = amx_ctof(params[6]);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[7], &outX);
    amx_GetAddr(amx, params[8], &outY);
    
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float r1 = RandomixGenerators::GetPRNG().next_float();
    float r2 = RandomixGenerators::GetPRNG().next_float();
    
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    
    float r3 = 1.0f - r1 - r2;
    
    // FIXED
    float resultX = r1 * x1 + r2 * x2 + r3 * x3;
    float resultY = r1 * y1 + r2 * y2 + r3 * y3;
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    
    return 1;
}

// ============================================================================
// NEW 3D POINT FUNCTIONS (DARI OPEN.MP) - FIXED VERSION
// ============================================================================

// native bool:PRandPointInSphere(Float:centerX, Float:centerY, Float:centerZ, Float:radius, &Float:outX, &Float:outY, &Float:outZ);
static cell AMX_NATIVE_CALL n_PRandPointInSphere(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float centerZ = amx_ctof(params[3]);
    float radius = amx_ctof(params[4]);
    
    if (radius <= 0.0f) return 0;
    
    cell *outX, *outY, *outZ;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    amx_GetAddr(amx, params[7], &outZ);
    
    if (!outX || !outY || !outZ) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float x, y, z, sq;
    do {
        x = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        y = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        z = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        sq = x * x + y * y + z * z;
    } while (sq > 1.0f || sq == 0.0f);
    
    float scale = radius * cbrtf(RandomixGenerators::GetPRNG().next_float()) / sqrtf(sq);
    
    // FIXED
    float resultX = centerX + x * scale;
    float resultY = centerY + y * scale;
    float resultZ = centerZ + z * scale;
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    *outZ = amx_ftoc(resultZ);
    
    return 1;
}

// native bool:CSPRandPointInSphere(Float:centerX, Float:centerY, Float:centerZ, Float:radius, &Float:outX, &Float:outY, &Float:outZ);
static cell AMX_NATIVE_CALL n_CSPRandPointInSphere(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float centerZ = amx_ctof(params[3]);
    float radius = amx_ctof(params[4]);
    
    if (radius <= 0.0f) return 0;
    
    cell *outX, *outY, *outZ;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    amx_GetAddr(amx, params[7], &outZ);
    
    if (!outX || !outY || !outZ) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    float x, y, z, sq;
    do {
        x = RandomixGenerators::GetCSPRNG().next_float() * 2.0f - 1.0f;
        y = RandomixGenerators::GetCSPRNG().next_float() * 2.0f - 1.0f;
        z = RandomixGenerators::GetCSPRNG().next_float() * 2.0f - 1.0f;
        sq = x * x + y * y + z * z;
    } while (sq > 1.0f || sq == 0.0f);
    
    float scale = radius * cbrtf(RandomixGenerators::GetCSPRNG().next_float()) / sqrtf(sq);
    
    // FIXED
    float resultX = centerX + x * scale;
    float resultY = centerY + y * scale;
    float resultZ = centerZ + z * scale;
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    *outZ = amx_ftoc(resultZ);
    
    return 1;
}

// native bool:PRandPointOnSphere(Float:centerX, Float:centerY, Float:centerZ, Float:radius, &Float:outX, &Float:outY, &Float:outZ);
static cell AMX_NATIVE_CALL n_PRandPointOnSphere(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float centerZ = amx_ctof(params[3]);
    float radius = amx_ctof(params[4]);
    
    if (radius <= 0.0f) return 0;
    
    cell *outX, *outY, *outZ;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    amx_GetAddr(amx, params[7], &outZ);
    
    if (!outX || !outY || !outZ) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float u, v, s;
    do {
        u = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        v = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    float multiplier = 2.0f * sqrtf(1.0f - s);
    
    // FIXED
    float resultX = centerX + radius * u * multiplier;
    float resultY = centerY + radius * v * multiplier;
    float resultZ = centerZ + radius * (1.0f - 2.0f * s);
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    *outZ = amx_ftoc(resultZ);
    
    return 1;
}

// native bool:PRandPointInBox(Float:minX, Float:minY, Float:minZ, Float:maxX, Float:maxY, Float:maxZ, &Float:outX, &Float:outY, &Float:outZ);
static cell AMX_NATIVE_CALL n_PRandPointInBox(AMX* amx, cell* params) {
    float minX = amx_ctof(params[1]);
    float minY = amx_ctof(params[2]);
    float minZ = amx_ctof(params[3]);
    float maxX = amx_ctof(params[4]);
    float maxY = amx_ctof(params[5]);
    float maxZ = amx_ctof(params[6]);
    
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    if (minZ > maxZ) std::swap(minZ, maxZ);
    
    cell *outX, *outY, *outZ;
    amx_GetAddr(amx, params[7], &outX);
    amx_GetAddr(amx, params[8], &outY);
    amx_GetAddr(amx, params[9], &outZ);
    
    if (!outX || !outY || !outZ) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // FIXED
    float resultX = minX + RandomixGenerators::GetPRNG().next_float() * (maxX - minX);
    float resultY = minY + RandomixGenerators::GetPRNG().next_float() * (maxY - minY);
    float resultZ = minZ + RandomixGenerators::GetPRNG().next_float() * (maxZ - minZ);
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    *outZ = amx_ftoc(resultZ);
    
    return 1;
}

// native bool:PRandPointInPolygon(const Float:vertices[], vertexCount, &Float:outX, &Float:outY);
static cell AMX_NATIVE_CALL n_PRandPointInPolygon(AMX* amx, cell* params) {
    int vertexCount = static_cast<int>(params[2]);
    if (vertexCount < 3) return 0;
    
    cell* verticesPtr = GetArrayAddress(amx, params[1]);
    cell *outX, *outY;
    amx_GetAddr(amx, params[3], &outX);
    amx_GetAddr(amx, params[4], &outY);
    
    if (!verticesPtr || !outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // Calculate total area using triangulation from first vertex
    float totalArea = 0.0f;
    std::vector<float> triangleAreas;
    
    for (int i = 1; i < vertexCount - 1; i++) {
        float x1 = amx_ctof(verticesPtr[0]);
        float y1 = amx_ctof(verticesPtr[1]);
        float x2 = amx_ctof(verticesPtr[i * 2]);
        float y2 = amx_ctof(verticesPtr[i * 2 + 1]);
        float x3 = amx_ctof(verticesPtr[(i + 1) * 2]);
        float y3 = amx_ctof(verticesPtr[(i + 1) * 2 + 1]);
        
        float area = fabsf((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)) * 0.5f;
        triangleAreas.push_back(area);
        totalArea += area;
    }
    
    if (totalArea <= 0.0f) return 0;
    
    // Select random triangle based on area weights
    float rand = RandomixGenerators::GetPRNG().next_float() * totalArea;
    float sum = 0.0f;
    int selectedTriangle = 0;
    
    for (size_t i = 0; i < triangleAreas.size(); i++) {
        sum += triangleAreas[i];
        if (rand < sum) {
            selectedTriangle = static_cast<int>(i);
            break;
        }
    }
    
    // Generate point in selected triangle
    float x1 = amx_ctof(verticesPtr[0]);
    float y1 = amx_ctof(verticesPtr[1]);
    float x2 = amx_ctof(verticesPtr[(selectedTriangle + 1) * 2]);
    float y2 = amx_ctof(verticesPtr[(selectedTriangle + 1) * 2 + 1]);
    float x3 = amx_ctof(verticesPtr[(selectedTriangle + 2) * 2]);
    float y3 = amx_ctof(verticesPtr[(selectedTriangle + 2) * 2 + 1]);
    
    float r1 = RandomixGenerators::GetPRNG().next_float();
    float r2 = RandomixGenerators::GetPRNG().next_float();
    
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    
    float r3 = 1.0f - r1 - r2;
    
    // FIXED
    float resultX = r1 * x1 + r2 * x2 + r3 * x3;
    float resultY = r1 * y1 + r2 * y2 + r3 * y3;
    *outX = amx_ftoc(resultX);
    *outY = amx_ftoc(resultY);
    
    return 1;
}

// ============================================================================
// NATIVE FUNCTIONS TABLE - DIPERBARUI DENGAN NATIVE BARU
// ============================================================================

static const AMX_NATIVE_INFO native_list[] = {
    // Range Functions
    {"PRandRange", n_PRandRange},
    {"CSPRandRange", n_CSPRandRange},
    {"PRandFloatRange", n_PRandFloatRange},
    {"CSPRandFloatRange", n_CSPRandFloatRange},
    
    // Seed Functions
    {"SeedPRNG", n_SeedPRNG},
    {"SeedCSPRNG", n_SeedCSPRNG},
    
    // Boolean Functions
    {"PRandBool", n_PRandBool},
    {"CSPRandBool", n_CSPRandBool},
    {"PRandBoolWeighted", n_PRandBoolWeighted},
    
    // Array Functions
    {"PRandWeighted", n_PRandWeighted},
    {"PRandShuffle", n_PRandShuffle},
    {"PRandShuffleRange", n_PRandShuffleRange},
    
    // Distribution Functions
    {"PRandGaussian", n_PRandGaussian},
    {"PRandDice", n_PRandDice},
    
    // Cryptographic Functions
    {"CSPRandToken", n_CSPRandToken},
    {"CSPRandBytes", n_CSPRandBytes},
    {"CSPRandUUID", n_CSPRandUUID},
    
    // =============== NEW 2D POINT FUNCTIONS ===============
    {"PRandPointInCircle", n_PRandPointInCircle},
    {"CSPRandPointInCircle", n_CSPRandPointInCircle},
    {"PRandPointOnCircle", n_PRandPointOnCircle},
    {"PRandPointInRect", n_PRandPointInRect},
    {"PRandPointInRing", n_PRandPointInRing},
    {"PRandPointInEllipse", n_PRandPointInEllipse},
    {"PRandPointInTriangle", n_PRandPointInTriangle},
    
    // =============== NEW 3D POINT FUNCTIONS ===============
    {"PRandPointInSphere", n_PRandPointInSphere},
    {"CSPRandPointInSphere", n_CSPRandPointInSphere},
    {"PRandPointOnSphere", n_PRandPointOnSphere},
    {"PRandPointInBox", n_PRandPointInBox},
    
    // =============== NEW ADVANCED GEOMETRIC FUNCTION ===============
    {"PRandPointInPolygon", n_PRandPointInPolygon},
    
    {NULL, NULL}
};

// Register natives
PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
    return amx_Register(amx, native_list, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
    return AMX_ERR_NONE;
}