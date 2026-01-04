/*
 *  Randomix - Enhanced Random Number Generator for open.mp Servers
 *  Copyright (c) 2025, Fanorisky
 *  GitHub: github.com/Fanorisky/PawnRandomix
 */

#include "randomix.hpp"
#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>

#include <chrono>
#include <cmath>

// Helper Functions for Pawn
static inline cell* GetArrayPtr(AMX* amx, cell param) {
    cell* addr = nullptr;
    amx_GetAddr(amx, param, &addr);
    return addr;
}

// Main Component
class RandomixComponent final : public IComponent, public PawnEventHandler {
private:
    ICore* core_ = nullptr;
    IPawnComponent* pawn_ = nullptr;
    
public:
    PROVIDE_UID(0x4D52616E646F6D69);
    
    ~RandomixComponent() {
        if (pawn_) {
            pawn_->getEventDispatcher().removeEventHandler(this);
        }
    }
    
    StringView componentName() const override {
        return "Randomix";
    }
    
    SemanticVersion componentVersion() const override {
        return SemanticVersion(1, 2, 0, 0);
    }
    
    void onLoad(ICore* c) override {
        core_ = c;
        
        uint64_t seed = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        
        RandomixGenerators::SeedPRNG(seed);
        RandomixGenerators::SeedCSPRNG(seed);
        
        core_->printLn("");
        core_->printLn("  Randomix Component Loaded");
        core_->printLn("  Version: v%s", RANDOMIX_VERSION);
        core_->printLn("  Author: Fanorisky");
        core_->printLn("  GitHub: github.com/Fanorisky/PawnRandomix");
        core_->printLn("");
        
        setAmxLookups(core_);
    }
    
    void onInit(IComponentList* components) override {
        pawn_ = components->queryComponent<IPawnComponent>();
        
        if (pawn_) {
            setAmxFunctions(pawn_->getAmxFunctions());
            setAmxLookups(components);
            pawn_->getEventDispatcher().addEventHandler(this);
        }
    }
    
    void onAmxLoad(IPawnScript& script) override {
        pawn_natives::AmxLoad(script.GetAMX());
    }
    
    void onAmxUnload(IPawnScript& script) override {}
    
    void onReady() override {}
    
    void onFree(IComponent* component) override {
        if (component == pawn_) {
            if (pawn_) {
                pawn_->getEventDispatcher().removeEventHandler(this);
            }
            pawn_ = nullptr;
            setAmxFunctions();
            setAmxLookups();
        }
    }
    
    void free() override {
        delete this;
    }
    
    void reset() override {
        uint64_t seed = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        
        RandomixGenerators::SeedPRNG(seed);
        RandomixGenerators::SeedCSPRNG(seed);
    }
};

COMPONENT_ENTRY_POINT() {
    return new RandomixComponent();
}


// Random within specific range (PRNG)
SCRIPT_API(PRandRange, int(int min, int max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return min + static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(range + 1));
}

// Random within specific range (CSPRNG)
SCRIPT_API(CSPRandRange, int(int min, int max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return min + static_cast<int>(RandomixGenerators::GetCSPRNG().next_bounded(range + 1));
}

// Random float within range (PRNG)
SCRIPT_API(PRandFloatRange, float(float min, float max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return min + RandomixGenerators::GetPRNG().next_float() * (max - min);
}

// Random float within range (CSPRNG)
SCRIPT_API(CSPRandFloatRange, float(float min, float max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return min + RandomixGenerators::GetCSPRNG().next_float() * (max - min);
}

// Set seed for PRNG
SCRIPT_API(SeedPRNG, int(int seed)) {
    RandomixGenerators::SeedPRNG(static_cast<uint64_t>(seed));
    return 1;
}

// Set seed for CSPRNG
SCRIPT_API(SeedCSPRNG, int(int seed)) {
    RandomixGenerators::SeedCSPRNG(static_cast<uint64_t>(seed));
    return 1;
}

// Random boolean with probability (PRNG)
SCRIPT_API(PRandBool, bool(float probability)) {
    if (probability <= 0.0f) return false;
    if (probability >= 1.0f) return true;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return RandomixGenerators::GetPRNG().next_float() < probability;
}

// Random boolean with probability (CSPRNG)
SCRIPT_API(CSPRandBool, bool(float probability)) {
    if (probability <= 0.0f) return false;
    if (probability >= 1.0f) return true;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return RandomixGenerators::GetCSPRNG().next_float() < probability;
}

// Random boolean with weights (trueWeight vs falseWeight)
SCRIPT_API(PRandBoolWeighted, bool(int trueW, int falseW)) {
    if (trueW <= 0) return false;
    if (falseW <= 0) return true;

    uint32_t total = static_cast<uint32_t>(trueW + falseW);
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return RandomixGenerators::GetPRNG().next_bounded(total) < static_cast<uint32_t>(trueW);
}

// Weighted random selection
SCRIPT_API(PRandWeighted, int(cell weightsAddr, int count)) {
    if (count <= 0) return 0;
    
    cell* weights = GetArrayPtr(GetAMX(), weightsAddr);
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

// Shuffle array (Fisher-Yates algorithm)
SCRIPT_API(PRandShuffle, bool(cell arrayAddr, int count)) {
    if (count <= 1) return true;
    
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    for (int i = count - 1; i > 0; i--) {
        int j = static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(i + 1));
        std::swap(array[i], array[j]);
    }
    
    return true;
}

// Shuffle part of array (specific range)
SCRIPT_API(PRandShuffleRange, bool(cell arrayAddr, int start, int end)) {
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return false;

    if (start > end) std::swap(start, end);
    if (end - start < 1) return true;

    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);

    for (int i = end; i > start; i--) {
        int j = start + RandomixGenerators::GetPRNG().next_bounded(i - start + 1);
        std::swap(array[i], array[j]);
    }
    return true;
}

// Gaussian/Normal distribution (Box-Muller transform)
SCRIPT_API(PRandGaussian, int(float mean, float stddev)) {
    if (stddev <= 0.0f) return static_cast<int>(mean);
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float u1 = RandomixGenerators::GetPRNG().next_float();
    float u2 = RandomixGenerators::GetPRNG().next_float();
    
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
    float result = mean + z0 * stddev;
    
    return static_cast<int>(result < 0.0f ? 0.0f : result);
}

// D&D style dice roll (e.g., 3d6 = 3 dice with 6 sides each)
SCRIPT_API(PRandDice, int(int sides, int count)) {
    if (sides <= 0 || count <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    uint32_t total = 0;
    uint32_t uSides = static_cast<uint32_t>(sides);
    
    for (int i = 0; i < count; i++) {
        total += RandomixGenerators::GetPRNG().next_bounded(uSides) + 1;
    }
    
    return static_cast<int>(total);
}

// Generate hexadecimal token
SCRIPT_API(CSPRandToken, int(int length)) {
    if (length <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    uint32_t token = 0;
    int actualLength = (length > 8) ? 8 : length;
    
    for (int i = 0; i < actualLength; i++) {
        token = (token << 4) | (RandomixGenerators::GetCSPRNG().next_bounded(16));
    }
    
    return static_cast<int>(token);
}

// Generate random bytes for cryptographic purposes
SCRIPT_API(CSPRandBytes, bool(cell destAddr, int length)) {
    if (length <= 0) return false;

    cell* dest = GetArrayPtr(GetAMX(), destAddr);
    if (!dest) return false;

    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);

    for (int i = 0; i < length; i++) {
        dest[i] = static_cast<cell>(
            RandomixGenerators::GetCSPRNG().next_uint32() & 0xFF
        );
    }
    return true;
}

// Generate UUID v4 (Universally Unique Identifier)
SCRIPT_API(CSPRandUUID, bool(cell destAddr)) {
    cell* out = GetArrayPtr(GetAMX(), destAddr);
    if (!out) return false;

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
    return true;
}

// ============================================
// 2D POINT FUNCTIONS - FIXED VERSION
// ============================================

/**
 * Generate random point in circle (uniform distribution)
 * Uses polar rejection method for true uniform distribution
 */
SCRIPT_API(PRandPointInCircle, bool(float centerX, float centerY, float radius, cell outX, cell outY)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // Square root method for uniform distribution
    float angle = RandomixGenerators::GetPRNG().next_float() * 6.28318530718f; // 2π
    float r = radius * sqrtf(RandomixGenerators::GetPRNG().next_float());
    
    *reinterpret_cast<float*>(xAddr) = centerX + r * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + r * sinf(angle);
    
    return true;
}

/**
 * CSPRNG version
 */
SCRIPT_API(CSPRandPointInCircle, bool(float centerX, float centerY, float radius, cell outX, cell outY)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    float angle = RandomixGenerators::GetCSPRNG().next_float() * 6.28318530718f;
    float r = radius * sqrtf(RandomixGenerators::GetCSPRNG().next_float());
    
    *reinterpret_cast<float*>(xAddr) = centerX + r * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + r * sinf(angle);
    
    return true;
}

/**
 * Generate random point on circle edge (circumference)
 */
SCRIPT_API(PRandPointOnCircle, bool(float centerX, float centerY, float radius, cell outX, cell outY)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float angle = RandomixGenerators::GetPRNG().next_float() * 6.28318530718f;
    
    *reinterpret_cast<float*>(xAddr) = centerX + radius * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + radius * sinf(angle);
    
    return true;
}

/**
 * Generate random point in rectangle
 */
SCRIPT_API(PRandPointInRect, bool(float minX, float minY, float maxX, float maxY, cell outX, cell outY)) {
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    *reinterpret_cast<float*>(xAddr) = minX + RandomixGenerators::GetPRNG().next_float() * (maxX - minX);
    *reinterpret_cast<float*>(yAddr) = minY + RandomixGenerators::GetPRNG().next_float() * (maxY - minY);
    
    return true;
}

/**
 * Generate random point in ring (donut shape)
 */
SCRIPT_API(PRandPointInRing, bool(float centerX, float centerY, float innerRadius, float outerRadius, cell outX, cell outY)) {
    if (innerRadius < 0.0f || outerRadius <= 0.0f || innerRadius >= outerRadius) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float angle = RandomixGenerators::GetPRNG().next_float() * 6.28318530718f;
    
    // Uniform distribution in ring
    float innerRadiusSq = innerRadius * innerRadius;
    float outerRadiusSq = outerRadius * outerRadius;
    float r = sqrtf(innerRadiusSq + RandomixGenerators::GetPRNG().next_float() * (outerRadiusSq - innerRadiusSq));
    
    *reinterpret_cast<float*>(xAddr) = centerX + r * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + r * sinf(angle);
    
    return true;
}

/**
 * Generate random point in ellipse
 */
SCRIPT_API(PRandPointInEllipse, bool(float centerX, float centerY, float radiusX, float radiusY, cell outX, cell outY)) {
    if (radiusX <= 0.0f || radiusY <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    float angle = RandomixGenerators::GetPRNG().next_float() * 6.28318530718f;
    float r = sqrtf(RandomixGenerators::GetPRNG().next_float());
    
    *reinterpret_cast<float*>(xAddr) = centerX + radiusX * r * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + radiusY * r * sinf(angle);
    
    return true;
}

/**
 * Generate random point in triangle
 */
SCRIPT_API(PRandPointInTriangle, bool(float x1, float y1, float x2, float y2, float x3, float y3, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // Barycentric coordinate method
    float r1 = RandomixGenerators::GetPRNG().next_float();
    float r2 = RandomixGenerators::GetPRNG().next_float();
    
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    
    float r3 = 1.0f - r1 - r2;
    
    *reinterpret_cast<float*>(xAddr) = r1 * x1 + r2 * x2 + r3 * x3;
    *reinterpret_cast<float*>(yAddr) = r1 * y1 + r2 * y2 + r3 * y3;
    
    return true;
}

// ============================================
// 3D POINT FUNCTIONS - FIXED VERSION
// ============================================

/**
 * Generate random point in sphere (uniform distribution)
 * Fixed: Correct method for uniform distribution in sphere
 */
SCRIPT_API(PRandPointInSphere, bool(float centerX, float centerY, float centerZ, float radius, cell outX, cell outY, cell outZ)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    
    if (!xAddr || !yAddr || !zAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // Rejection method for uniform distribution in sphere
    float x, y, z, sq;
    do {
        x = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        y = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        z = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        sq = x * x + y * y + z * z;
    } while (sq > 1.0f || sq == 0.0f);
    
    // Scale to uniform distribution within sphere
    float scale = radius * cbrtf(RandomixGenerators::GetPRNG().next_float()) / sqrtf(sq);
    
    *reinterpret_cast<float*>(xAddr) = centerX + x * scale;
    *reinterpret_cast<float*>(yAddr) = centerY + y * scale;
    *reinterpret_cast<float*>(zAddr) = centerZ + z * scale;
    
    return true;
}

/**
 * CSPRNG version
 */
SCRIPT_API(CSPRandPointInSphere, bool(float centerX, float centerY, float centerZ, float radius, cell outX, cell outY, cell outZ)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    
    if (!xAddr || !yAddr || !zAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    float x, y, z, sq;
    do {
        x = RandomixGenerators::GetCSPRNG().next_float() * 2.0f - 1.0f;
        y = RandomixGenerators::GetCSPRNG().next_float() * 2.0f - 1.0f;
        z = RandomixGenerators::GetCSPRNG().next_float() * 2.0f - 1.0f;
        sq = x * x + y * y + z * z;
    } while (sq > 1.0f || sq == 0.0f);
    
    float scale = radius * cbrtf(RandomixGenerators::GetCSPRNG().next_float()) / sqrtf(sq);
    
    *reinterpret_cast<float*>(xAddr) = centerX + x * scale;
    *reinterpret_cast<float*>(yAddr) = centerY + y * scale;
    *reinterpret_cast<float*>(zAddr) = centerZ + z * scale;
    
    return true;
}

/**
 * Generate random point on sphere surface
 * Fixed: Correct method for uniform distribution on sphere surface
 */
SCRIPT_API(PRandPointOnSphere, bool(float centerX, float centerY, float centerZ, float radius, cell outX, cell outY, cell outZ)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    
    if (!xAddr || !yAddr || !zAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // Marsaglia's method for uniform distribution on sphere surface
    float u, v, s;
    do {
        u = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        v = RandomixGenerators::GetPRNG().next_float() * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    float multiplier = 2.0f * sqrtf(1.0f - s);
    
    *reinterpret_cast<float*>(xAddr) = centerX + radius * u * multiplier;
    *reinterpret_cast<float*>(yAddr) = centerY + radius * v * multiplier;
    *reinterpret_cast<float*>(zAddr) = centerZ + radius * (1.0f - 2.0f * s);
    
    return true;
}

/**
 * Generate random point in box (cuboid)
 */
SCRIPT_API(PRandPointInBox, bool(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, cell outX, cell outY, cell outZ)) {
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    if (minZ > maxZ) std::swap(minZ, maxZ);
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    
    if (!xAddr || !yAddr || !zAddr) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    *reinterpret_cast<float*>(xAddr) = minX + RandomixGenerators::GetPRNG().next_float() * (maxX - minX);
    *reinterpret_cast<float*>(yAddr) = minY + RandomixGenerators::GetPRNG().next_float() * (maxY - minY);
    *reinterpret_cast<float*>(zAddr) = minZ + RandomixGenerators::GetPRNG().next_float() * (maxZ - minZ);
    
    return true;
}

// ============================================
// ADVANCED GEOMETRIC FUNCTIONS - FIXED VERSION
// ============================================

/**
 * Generate random point in convex polygon (2D)
 * Uses triangulation method
 */
SCRIPT_API(PRandPointInPolygon, bool(cell verticesAddr, int vertexCount, cell outX, cell outY)) {
    if (vertexCount < 3) return false;
    
    cell* verticesPtr = GetArrayPtr(GetAMX(), verticesAddr);
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!verticesPtr || !xAddr || !yAddr) return false;
    
    // Cast to float pointer for easier access
    float* vertices = reinterpret_cast<float*>(verticesPtr);
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // Calculate total area using triangulation from first vertex
    float totalArea = 0.0f;
    std::vector<float> triangleAreas;
    
    for (int i = 1; i < vertexCount - 1; i++) {
        float x1 = vertices[0];
        float y1 = vertices[1];
        float x2 = vertices[i * 2];
        float y2 = vertices[i * 2 + 1];
        float x3 = vertices[(i + 1) * 2];
        float y3 = vertices[(i + 1) * 2 + 1];
        
        // Triangle area using cross product
        float area = fabsf((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)) * 0.5f;
        triangleAreas.push_back(area);
        totalArea += area;
    }
    
    if (totalArea <= 0.0f) return false;
    
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
    float x1 = vertices[0];
    float y1 = vertices[1];
    float x2 = vertices[(selectedTriangle + 1) * 2];
    float y2 = vertices[(selectedTriangle + 1) * 2 + 1];
    float x3 = vertices[(selectedTriangle + 2) * 2];
    float y3 = vertices[(selectedTriangle + 2) * 2 + 1];
    
    float r1 = RandomixGenerators::GetPRNG().next_float();
    float r2 = RandomixGenerators::GetPRNG().next_float();
    
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    
    float r3 = 1.0f - r1 - r2;
    
    *reinterpret_cast<float*>(xAddr) = r1 * x1 + r2 * x2 + r3 * x3;
    *reinterpret_cast<float*>(yAddr) = r1 * y1 + r2 * y2 + r3 * y3;
    
    return true;
}