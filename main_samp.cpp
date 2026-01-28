/*
 *  Randomix - Cryptographic Random Number Generator for SA-MP
 *  Version: 2.0.0 (CSPRNG Only - ChaCha20)
 */

#include "plugin.h"
#include "amx/amx.h"
#include "amx/amx2.h"
#include "randomix.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <vector>

extern void *pAMXFunctions;
typedef void (*logprintf_t)(const char* format, ...);
logprintf_t logprintf;

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
    return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** ppData) {
    pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
    logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
    
    uint64_t seed = static_cast<uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    Randomix::Seed(seed);
    
    logprintf("");
    logprintf("  Randomix Plugin Loaded");
    logprintf("  Version: v2.0.0 (CSPRNG Mode)");
    logprintf("  Algorithm: ChaCha20");
    logprintf("  Security: Cryptographic Grade");
    logprintf("");
    
    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
    logprintf("Randomix Plugin Unloaded");
}

static inline cell* GetAddr(AMX* amx, cell address) {
    cell* phys_addr;
    amx_GetAddr(amx, address, &phys_addr);
    return phys_addr;
}

// ============================================================
// CORE RANDOM FUNCTIONS
// ============================================================

static cell AMX_NATIVE_CALL n_RandRange(AMX* amx, cell* params) {
    int min = static_cast<int>(params[1]);
    int max = static_cast<int>(params[2]);
    
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return min + static_cast<cell>(Randomix::GetRNG().next_bounded(range + 1));
}

static cell AMX_NATIVE_CALL n_RandFloatRange(AMX* amx, cell* params) {
    float min = amx_ctof(params[1]);
    float max = amx_ctof(params[2]);
    
    if (min > max) std::swap(min, max);
    if (min == max) return params[1];
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float result = min + Randomix::GetRNG().next_float() * (max - min);
    return amx_ftoc(result);
}

static cell AMX_NATIVE_CALL n_SeedRNG(AMX* amx, cell* params) {
    Randomix::Seed(static_cast<uint64_t>(params[1]));
    return 1;
}

static cell AMX_NATIVE_CALL n_RandBool(AMX* amx, cell* params) {
    float probability = amx_ctof(params[1]);
    
    if (probability <= 0.0f) return 0;
    if (probability >= 1.0f) return 1;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return Randomix::GetRNG().next_float() < probability ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_RandBoolWeighted(AMX* amx, cell* params) {
    int trueW = static_cast<int>(params[1]);
    int falseW = static_cast<int>(params[2]);
    
    if (trueW <= 0) return 0;
    if (falseW <= 0) return 1;
    
    uint32_t total = static_cast<uint32_t>(trueW + falseW);
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return Randomix::GetRNG().next_bounded(total) < static_cast<uint32_t>(trueW) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_RandWeighted(AMX* amx, cell* params) {
    int count = static_cast<int>(params[2]);
    if (count <= 0) return 0;
    
    cell* weights = GetAddr(amx, params[1]);
    if (!weights) return 0;
    
    uint32_t total = 0;
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0) total += static_cast<uint32_t>(weights[i]);
    }
    
    if (total == 0) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    uint32_t rand = Randomix::GetRNG().next_bounded(total);
    uint32_t sum = 0;
    
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0) {
            sum += static_cast<uint32_t>(weights[i]);
            if (rand < sum) return i;
        }
    }
    return count - 1;
}

static cell AMX_NATIVE_CALL n_RandShuffle(AMX* amx, cell* params) {
    int count = static_cast<int>(params[2]);
    if (count <= 1) return 1;
    
    cell* array = GetAddr(amx, params[1]);
    if (!array) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    for (int i = count - 1; i > 0; i--) {
        int j = static_cast<int>(Randomix::GetRNG().next_bounded(i + 1));
        std::swap(array[i], array[j]);
    }
    return 1;
}

static cell AMX_NATIVE_CALL n_RandShuffleRange(AMX* amx, cell* params) {
    cell* array = GetAddr(amx, params[1]);
    if (!array) return 0;
    
    int start = static_cast<int>(params[2]);
    int end = static_cast<int>(params[3]);
    
    if (start > end) std::swap(start, end);
    if (end - start < 1) return 1;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    for (int i = end; i > start; i--) {
        int j = start + Randomix::GetRNG().next_bounded(i - start + 1);
        std::swap(array[i], array[j]);
    }
    return 1;
}

static cell AMX_NATIVE_CALL n_RandGaussian(AMX* amx, cell* params) {
    float mean = amx_ctof(params[1]);
    float stddev = amx_ctof(params[2]);
    
    if (stddev <= 0.0f) return static_cast<cell>(mean);
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float u1 = Randomix::GetRNG().next_float();
    float u2 = Randomix::GetRNG().next_float();
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
    float result = mean + z0 * stddev;
    
    return static_cast<cell>(result < 0.0f ? 0.0f : result);
}

static cell AMX_NATIVE_CALL n_RandDice(AMX* amx, cell* params) {
    int sides = static_cast<int>(params[1]);
    int count = static_cast<int>(params[2]);
    
    if (sides <= 0 || count <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    uint32_t total = 0;
    uint32_t uSides = static_cast<uint32_t>(sides);
    
    for (int i = 0; i < count; i++) {
        total += Randomix::GetRNG().next_bounded(uSides) + 1;
    }
    return static_cast<cell>(total);
}

// ============================================================
// NEW: RandPick & RandFormat
// ============================================================

static cell AMX_NATIVE_CALL n_RandPick(AMX* amx, cell* params) {
    int count = static_cast<int>(params[2]);
    if (count <= 0) return 0;
    
    cell* array = GetAddr(amx, params[1]);
    if (!array) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    uint32_t idx = Randomix::GetRNG().next_bounded(static_cast<uint32_t>(count));
    return array[idx];
}

static cell AMX_NATIVE_CALL n_RandFormat(AMX* amx, cell* params) {
    int destSize = static_cast<int>(params[3]);
    if (destSize <= 0) return 0;
    
    cell* dest = GetAddr(amx, params[1]);
    cell* pattern = GetAddr(amx, params[2]);
    if (!dest || !pattern) return 0;
    
    static const char* upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const char* lower = "abcdefghijklmnopqrstuvwxyz";
    static const char* digit = "0123456789";
    static const char* alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static const char* symbol = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    auto& rng = Randomix::GetRNG();
    
    int outPos = 0;
    for (int i = 0; pattern[i] != '\0' && outPos < destSize - 1; i++) {
        char c = static_cast<char>(pattern[i]);
        const char* charset = nullptr;
        size_t len = 0;
        
        switch (c) {
            case 'X': charset = upper; len = 26; break;
            case 'x': charset = lower; len = 26; break;
            case '9': charset = digit; len = 10; break;
            case 'A': charset = alpha; len = 62; break;
            case '!': charset = symbol; len = 25; break;
            default:
                if (c == '\\' && pattern[i+1] != '\0') {
                    i++;
                    dest[outPos++] = pattern[i];
                } else {
                    dest[outPos++] = pattern[i];
                }
                continue;
        }
        
        if (charset && len > 0) {
            uint32_t idx = rng.next_bounded(static_cast<uint32_t>(len));
            dest[outPos++] = static_cast<cell>(charset[idx]);
        }
    }
    
    dest[outPos] = '\0';
    return 1;
}

// ============================================================
// CRYPTOGRAPHIC FUNCTIONS
// ============================================================

static cell AMX_NATIVE_CALL n_RandBytes(AMX* amx, cell* params) {
    int length = static_cast<int>(params[2]);
    if (length <= 0) return 0;
    
    cell* dest = GetAddr(amx, params[1]);
    if (!dest) return 0;
 
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    for (int i = 0; i < length; i++) {
        dest[i] = static_cast<cell>(Randomix::GetRNG().next_uint32() & 0xFF);
    }
    return 1;
}

static cell AMX_NATIVE_CALL n_RandUUID(AMX* amx, cell* params) {
    cell* out = GetAddr(amx, params[1]);
    if (!out) return 0;
    
    uint8_t bytes[16];
    {
        std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
        Randomix::GetRNG().next_bytes(bytes, 16);
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

// ============================================================
// 2D GEOMETRY
// ============================================================

static cell AMX_NATIVE_CALL n_RandPointInCircle(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radius = amx_ctof(params[3]);
    
    if (radius <= 0.0f) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[4], &outX);
    amx_GetAddr(amx, params[5], &outY);
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * 6.28318530718f;
    float r = radius * sqrtf(Randomix::GetRNG().next_float());
    
    float resX = centerX + r * cosf(angle);
    float resY = centerY + r * sinf(angle);
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointOnCircle(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radius = amx_ctof(params[3]);
    
    if (radius <= 0.0f) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[4], &outX);
    amx_GetAddr(amx, params[5], &outY);
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * 6.28318530718f;
    
    float resX = centerX + radius * cosf(angle);
    float resY = centerY + radius * sinf(angle);
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInRect(AMX* amx, cell* params) {
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
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float resX = minX + Randomix::GetRNG().next_float() * (maxX - minX);
    float resY = minY + Randomix::GetRNG().next_float() * (maxY - minY);
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInRing(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float innerR = amx_ctof(params[3]);
    float outerR = amx_ctof(params[4]);
    
    if (innerR < 0.0f || outerR <= 0.0f || innerR >= outerR) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * 6.28318530718f;
    float innerSq = innerR * innerR;
    float outerSq = outerR * outerR;
    float r = sqrtf(innerSq + Randomix::GetRNG().next_float() * (outerSq - innerSq));
    
    float resX = centerX + r * cosf(angle);
    float resY = centerY + r * sinf(angle);
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInEllipse(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radiusX = amx_ctof(params[3]);
    float radiusY = amx_ctof(params[4]);
    
    if (radiusX <= 0.0f || radiusY <= 0.0f) return 0;
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * 6.28318530718f;
    float r = sqrtf(Randomix::GetRNG().next_float());
    
    float resX = centerX + radiusX * r * cosf(angle);
    float resY = centerY + radiusY * r * sinf(angle);
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInTriangle(AMX* amx, cell* params) {
    float x1 = amx_ctof(params[1]), y1 = amx_ctof(params[2]);
    float x2 = amx_ctof(params[3]), y2 = amx_ctof(params[4]);
    float x3 = amx_ctof(params[5]), y3 = amx_ctof(params[6]);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[7], &outX);
    amx_GetAddr(amx, params[8], &outY);
    if (!outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float r1 = Randomix::GetRNG().next_float();
    float r2 = Randomix::GetRNG().next_float();
    
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    float r3 = 1.0f - r1 - r2;
    
    float resX = r1 * x1 + r2 * x2 + r3 * x3;
    float resY = r1 * y1 + r2 * y2 + r3 * y3;
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    return 1;
}

// ============================================================
// 3D GEOMETRY
// ============================================================

static cell AMX_NATIVE_CALL n_RandPointInSphere(AMX* amx, cell* params) {
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
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float x, y, z, sq;
    do {
        x = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        y = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        z = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        sq = x * x + y * y + z * z;
    } while (sq > 1.0f || sq == 0.0f);
    
    float scale = radius * cbrtf(Randomix::GetRNG().next_float()) / sqrtf(sq);
    
    float resX = centerX + x * scale;
    float resY = centerY + y * scale;
    float resZ = centerZ + z * scale;
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    *outZ = amx_ftoc(resZ);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointOnSphere(AMX* amx, cell* params) {
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
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float u, v, s;
    do {
        u = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        v = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    float multiplier = 2.0f * sqrtf(1.0f - s);
    
    float resX = centerX + radius * u * multiplier;
    float resY = centerY + radius * v * multiplier;
    float resZ = centerZ + radius * (1.0f - 2.0f * s);
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    *outZ = amx_ftoc(resZ);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInBox(AMX* amx, cell* params) {
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
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float resX = minX + Randomix::GetRNG().next_float() * (maxX - minX);
    float resY = minY + Randomix::GetRNG().next_float() * (maxY - minY);
    float resZ = minZ + Randomix::GetRNG().next_float() * (maxZ - minZ);
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    *outZ = amx_ftoc(resZ);
    return 1;
}

// ============================================================
// ADVANCED GEOMETRY
// ============================================================

static cell AMX_NATIVE_CALL n_RandPointInPolygon(AMX* amx, cell* params) {
    int vertexCount = static_cast<int>(params[2]);
    if (vertexCount < 3) return 0;
    
    cell* verticesPtr = GetAddr(amx, params[1]);
    cell *outX, *outY;
    amx_GetAddr(amx, params[3], &outX);
    amx_GetAddr(amx, params[4], &outY);
    
    if (!verticesPtr || !outX || !outY) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float totalArea = 0.0f;
    std::vector<float> areas;
    areas.reserve(vertexCount - 2);
    
    float x0 = amx_ctof(verticesPtr[0]);
    float y0 = amx_ctof(verticesPtr[1]);
    
    for (int i = 1; i < vertexCount - 1; i++) {
        float x1 = amx_ctof(verticesPtr[i * 2]);
        float y1 = amx_ctof(verticesPtr[i * 2 + 1]);
        float x2 = amx_ctof(verticesPtr[(i + 1) * 2]);
        float y2 = amx_ctof(verticesPtr[(i + 1) * 2 + 1]);
        
        float area = fabsf((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)) * 0.5f;
        areas.push_back(area);
        totalArea += area;
    }
    
    if (totalArea <= 0.0f) return 0;
    
    float rand = Randomix::GetRNG().next_float() * totalArea;
    float sum = 0.0f;
    int idx = 0;
    for (size_t i = 0; i < areas.size(); i++) {
        sum += areas[i];
        if (rand < sum) {
            idx = static_cast<int>(i);
            break;
        }
    }
    
    float x0v = amx_ctof(verticesPtr[0]);
    float y0v = amx_ctof(verticesPtr[1]);
    float x1 = amx_ctof(verticesPtr[(idx + 1) * 2]);
    float y1 = amx_ctof(verticesPtr[(idx + 1) * 2 + 1]);
    float x2 = amx_ctof(verticesPtr[(idx + 2) * 2]);
    float y2 = amx_ctof(verticesPtr[(idx + 2) * 2 + 1]);
    
    float r1 = Randomix::GetRNG().next_float();
    float r2 = Randomix::GetRNG().next_float();
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    float r3 = 1.0f - r1 - r2;
    
    float resX = r1 * x0v + r2 * x1 + r3 * x2;
    float resY = r1 * y0v + r2 * y1 + r3 * y2;
    *outX = amx_ftoc(resX);
    *outY = amx_ftoc(resY);
    return 1;
}

// ============================================================
// NATIVE REGISTRATION
// ============================================================

static const AMX_NATIVE_INFO native_list[] = {
    {"RandRange", n_RandRange},
    {"RandFloatRange", n_RandFloatRange},
    {"SeedRNG", n_SeedRNG},
    {"RandBool", n_RandBool},
    {"RandBoolWeighted", n_RandBoolWeighted},
    {"RandWeighted", n_RandWeighted},
    {"RandShuffle", n_RandShuffle},
    {"RandShuffleRange", n_RandShuffleRange},
    {"RandGaussian", n_RandGaussian},
    {"RandDice", n_RandDice},
    {"RandBytes", n_RandBytes},
    {"RandUUID", n_RandUUID},
    {"RandPick", n_RandPick},
    {"RandFormat", n_RandFormat},
    
    // 2D
    {"RandPointInCircle", n_RandPointInCircle},
    {"RandPointOnCircle", n_RandPointOnCircle},
    {"RandPointInRect", n_RandPointInRect},
    {"RandPointInRing", n_RandPointInRing},
    {"RandPointInEllipse", n_RandPointInEllipse},
    {"RandPointInTriangle", n_RandPointInTriangle},
    
    // 3D
    {"RandPointInSphere", n_RandPointInSphere},
    {"RandPointOnSphere", n_RandPointOnSphere},
    {"RandPointInBox", n_RandPointInBox},
    
    // Advanced
    {"RandPointInPolygon", n_RandPointInPolygon},
    
    {NULL, NULL}
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
    return amx_Register(amx, native_list, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
    return AMX_ERR_NONE;
}