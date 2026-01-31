/*
 *  Randomix - Cryptographic Random Number Generator for SA-MP
 *  Version: 2.0.1 (CSPRNG Only - ChaCha20)
 */

#include "plugin.h"
#include "amx/amx.h"
#include "amx/amx2.h"
#include "randomix.hpp"
#include "randomix_impl.hpp"
#include <chrono>
#include <cstring>

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
    logprintf("  Randomix v2.0.1 Loaded");
    logprintf("  Algorithm: ChaCha20 (Cryptographic)");
    logprintf("  Author: Fanorisky (https://github.com/Fanorisky/PawnRandomix)");
    logprintf("");
    
    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
    logprintf("");
    logprintf("  Randomix v2.0.1 Unloaded");
    logprintf("");
}

static inline cell* GetAddr(AMX* amx, cell address) {
    cell* phys_addr;
    amx_GetAddr(amx, address, &phys_addr);
    return phys_addr;
}

// Core random functions

static cell AMX_NATIVE_CALL n_RandRange(AMX* amx, cell* params) {
    int min = static_cast<int>(params[1]);
    int max = static_cast<int>(params[2]);
    return static_cast<cell>(ImplRandRange(min, max));
}

static cell AMX_NATIVE_CALL n_RandFloatRange(AMX* amx, cell* params) {
    float min = amx_ctof(params[1]);
    float max = amx_ctof(params[2]);
    float result = ImplRandFloatRange(min, max);
    return amx_ftoc(result);
}

static cell AMX_NATIVE_CALL n_SeedRNG(AMX* amx, cell* params) {
    Randomix::Seed(static_cast<uint64_t>(params[1]));
    return 1;
}

static cell AMX_NATIVE_CALL n_RandBool(AMX* amx, cell* params) {
    float probability = amx_ctof(params[1]);
    return ImplRandBool(probability) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_RandBoolWeighted(AMX* amx, cell* params) {
    int trueW = static_cast<int>(params[1]);
    int falseW = static_cast<int>(params[2]);
    return ImplRandBoolWeighted(trueW, falseW) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_RandWeighted(AMX* amx, cell* params) {
    int count = static_cast<int>(params[2]);
    if (count <= 0) return 0;
    
    cell* weights = GetAddr(amx, params[1]);
    if (!weights) return 0;
    
    // Copy to int array for safety
    static thread_local int intWeights[65536];
    int actualCount = (count > 65536) ? 65536 : count;
    for (int i = 0; i < actualCount; i++) {
        intWeights[i] = static_cast<int>(weights[i]);
    }
    
    return static_cast<cell>(ImplRandWeighted(intWeights, actualCount));
}

static cell AMX_NATIVE_CALL n_RandShuffle(AMX* amx, cell* params) {
    int count = static_cast<int>(params[2]);
    if (count <= 1) return 1;
    
    cell* array = GetAddr(amx, params[1]);
    if (!array) return 0;
    
    return ImplRandShuffle(reinterpret_cast<int*>(array), count) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_RandShuffleRange(AMX* amx, cell* params) {
    cell* array = GetAddr(amx, params[1]);
    if (!array) return 0;
    
    int start = static_cast<int>(params[2]);
    int end = static_cast<int>(params[3]);
    
    return ImplRandShuffleRange(reinterpret_cast<int*>(array), start, end) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_RandGaussian(AMX* amx, cell* params) {
    float mean = amx_ctof(params[1]);
    float stddev = amx_ctof(params[2]);
    return static_cast<cell>(ImplRandGaussian(mean, stddev));
}

static cell AMX_NATIVE_CALL n_RandDice(AMX* amx, cell* params) {
    int sides = static_cast<int>(params[1]);
    int count = static_cast<int>(params[2]);
    return static_cast<cell>(ImplRandDice(sides, count));
}

static cell AMX_NATIVE_CALL n_RandPick(AMX* amx, cell* params) {
    int count = static_cast<int>(params[2]);
    if (count <= 0) return 0;
    
    cell* array = GetAddr(amx, params[1]);
    if (!array) return 0;
    
    static thread_local int intArray[65536];
    int actualCount = (count > 65536) ? 65536 : count;
    for (int i = 0; i < actualCount; i++) {
        intArray[i] = static_cast<int>(array[i]);
    }
    
    return static_cast<cell>(ImplRandPick(intArray, actualCount));
}

static cell AMX_NATIVE_CALL n_RandFormat(AMX* amx, cell* params) {
    int destSize = static_cast<int>(params[3]);
    if (destSize <= 0) return 0;
    
    cell* dest = GetAddr(amx, params[1]);
    cell* pattern = GetAddr(amx, params[2]);
    if (!dest || !pattern) return 0;
    
    // Bounds check destSize
    if (destSize > 65536) destSize = 65536;
    
    // Convert pattern to char string
    static thread_local char patternBuf[256];
    int i;
    for (i = 0; i < 255 && pattern[i] != 0; i++) {
        patternBuf[i] = static_cast<char>(pattern[i]);
    }
    patternBuf[i] = '\0';
    
    static thread_local char destBuf[65536];
    if (!ImplRandFormat(destBuf, patternBuf, destSize)) return 0;
    
    // Copy back to AMX
    for (i = 0; destBuf[i] != '\0' && i < destSize - 1; i++) {
        dest[i] = static_cast<cell>(destBuf[i]);
    }
    dest[i] = 0;
    return 1;
}

// Cryptographic functions

static cell AMX_NATIVE_CALL n_RandBytes(AMX* amx, cell* params) {
    int length = static_cast<int>(params[2]);
    if (length <= 0) return 0;
    
    cell* dest = GetAddr(amx, params[1]);
    if (!dest) return 0;
    
    // Bounds check
    if (length > 65536) length = 65536;
    
    static thread_local uint8_t buffer[65536];
    if (!ImplRandBytes(buffer, length)) return 0;
    
    for (int i = 0; i < length; i++) {
        dest[i] = static_cast<cell>(buffer[i]);
    }
    return 1;
}

static cell AMX_NATIVE_CALL n_RandUUID(AMX* amx, cell* params) {
    cell* out = GetAddr(amx, params[1]);
    if (!out) return 0;
    
    char uuidBuf[40];
    if (!ImplRandUUID(uuidBuf)) return 0;
    
    for (int i = 0; uuidBuf[i] != '\0'; i++) {
        out[i] = static_cast<cell>(uuidBuf[i]);
    }
    return 1;
}

// 2D geometry

static cell AMX_NATIVE_CALL n_RandPointInCircle(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radius = amx_ctof(params[3]);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[4], &outX);
    amx_GetAddr(amx, params[5], &outY);
    if (!outX || !outY) return 0;
    
    float x, y;
    if (!ImplRandPointInCircle(centerX, centerY, radius, x, y)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointOnCircle(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radius = amx_ctof(params[3]);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[4], &outX);
    amx_GetAddr(amx, params[5], &outY);
    if (!outX || !outY) return 0;
    
    float x, y;
    if (!ImplRandPointOnCircle(centerX, centerY, radius, x, y)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInRect(AMX* amx, cell* params) {
    float minX = amx_ctof(params[1]);
    float minY = amx_ctof(params[2]);
    float maxX = amx_ctof(params[3]);
    float maxY = amx_ctof(params[4]);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    if (!outX || !outY) return 0;
    
    float x, y;
    if (!ImplRandPointInRect(minX, minY, maxX, maxY, x, y)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInRing(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float innerR = amx_ctof(params[3]);
    float outerR = amx_ctof(params[4]);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    if (!outX || !outY) return 0;
    
    float x, y;
    if (!ImplRandPointInRing(centerX, centerY, innerR, outerR, x, y)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInEllipse(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radiusX = amx_ctof(params[3]);
    float radiusY = amx_ctof(params[4]);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    if (!outX || !outY) return 0;
    
    float x, y;
    if (!ImplRandPointInEllipse(centerX, centerY, radiusX, radiusY, x, y)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
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
    
    float x, y;
    if (!ImplRandPointInTriangle(x1, y1, x2, y2, x3, y3, x, y)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    return 1;
}

// RandPointInArc - random point in arc/circular sector

static cell AMX_NATIVE_CALL n_RandPointInArc(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float radius = amx_ctof(params[3]);
    float startAngle = amx_ctof(params[4]);
    float endAngle = amx_ctof(params[5]);
    
    cell *outX, *outY;
    amx_GetAddr(amx, params[6], &outX);
    amx_GetAddr(amx, params[7], &outY);
    if (!outX || !outY) return 0;
    
    float x, y;
    if (!ImplRandPointInArc(centerX, centerY, radius, startAngle, endAngle, x, y)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    return 1;
}

// 3D geometry

static cell AMX_NATIVE_CALL n_RandPointInSphere(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float centerZ = amx_ctof(params[3]);
    float radius = amx_ctof(params[4]);
    
    cell *outX, *outY, *outZ;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    amx_GetAddr(amx, params[7], &outZ);
    if (!outX || !outY || !outZ) return 0;
    
    float x, y, z;
    if (!ImplRandPointInSphere(centerX, centerY, centerZ, radius, x, y, z)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    *outZ = amx_ftoc(z);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointOnSphere(AMX* amx, cell* params) {
    float centerX = amx_ctof(params[1]);
    float centerY = amx_ctof(params[2]);
    float centerZ = amx_ctof(params[3]);
    float radius = amx_ctof(params[4]);
    
    cell *outX, *outY, *outZ;
    amx_GetAddr(amx, params[5], &outX);
    amx_GetAddr(amx, params[6], &outY);
    amx_GetAddr(amx, params[7], &outZ);
    if (!outX || !outY || !outZ) return 0;
    
    float x, y, z;
    if (!ImplRandPointOnSphere(centerX, centerY, centerZ, radius, x, y, z)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    *outZ = amx_ftoc(z);
    return 1;
}

static cell AMX_NATIVE_CALL n_RandPointInBox(AMX* amx, cell* params) {
    float minX = amx_ctof(params[1]);
    float minY = amx_ctof(params[2]);
    float minZ = amx_ctof(params[3]);
    float maxX = amx_ctof(params[4]);
    float maxY = amx_ctof(params[5]);
    float maxZ = amx_ctof(params[6]);
    
    cell *outX, *outY, *outZ;
    amx_GetAddr(amx, params[7], &outX);
    amx_GetAddr(amx, params[8], &outY);
    amx_GetAddr(amx, params[9], &outZ);
    if (!outX || !outY || !outZ) return 0;
    
    float x, y, z;
    if (!ImplRandPointInBox(minX, minY, minZ, maxX, maxY, maxZ, x, y, z)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    *outZ = amx_ftoc(z);
    return 1;
}

// Advanced geometry

static cell AMX_NATIVE_CALL n_RandPointInPolygon(AMX* amx, cell* params) {
    int vertexCount = static_cast<int>(params[2]);
    if (vertexCount < 3 || vertexCount > MAX_POLYGON_VERTICES) return 0;
    
    cell* verticesPtr = GetAddr(amx, params[1]);
    cell *outX, *outY;
    amx_GetAddr(amx, params[3], &outX);
    amx_GetAddr(amx, params[4], &outY);
    
    if (!verticesPtr || !outX || !outY) return 0;
    
    float x, y;
    if (!ImplRandPointInPolygon(reinterpret_cast<float*>(verticesPtr), vertexCount, x, y)) return 0;
    
    *outX = amx_ftoc(x);
    *outY = amx_ftoc(y);
    return 1;
}

// Native registration

AMX_NATIVE_INFO PluginNatives[] = {
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
    {"RandPick", n_RandPick},
    {"RandFormat", n_RandFormat},
    {"RandBytes", n_RandBytes},
    {"RandUUID", n_RandUUID},
    {"RandPointInCircle", n_RandPointInCircle},
    {"RandPointOnCircle", n_RandPointOnCircle},
    {"RandPointInRect", n_RandPointInRect},
    {"RandPointInRing", n_RandPointInRing},
    {"RandPointInEllipse", n_RandPointInEllipse},
    {"RandPointInTriangle", n_RandPointInTriangle},
    {"RandPointInArc", n_RandPointInArc},
    {"RandPointInSphere", n_RandPointInSphere},
    {"RandPointOnSphere", n_RandPointOnSphere},
    {"RandPointInBox", n_RandPointInBox},
    {"RandPointInPolygon", n_RandPointInPolygon},
    {0, 0}
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx) {
    return amx_Register(amx, PluginNatives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX* amx) {
    return AMX_ERR_NONE;
}
