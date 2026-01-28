/*
 *  Randomix - Cryptographic Random Number Generator for open.mp
 *  Version: 2.0.0 (CSPRNG Only - ChaCha20)
 */

#include "randomix.hpp"
#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>

#include <chrono>
#include <cmath>
#include <algorithm>

static inline cell* GetArrayPtr(AMX* amx, cell param) {
    cell* addr = nullptr;
    amx_GetAddr(amx, param, &addr);
    return addr;
}

// ============================================================
// CORE RANDOM FUNCTIONS (Unified CSPRNG)
// ============================================================

SCRIPT_API(RandRange, int(int min, int max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return min + static_cast<int>(Randomix::GetRNG().next_bounded(range + 1));
}

SCRIPT_API(RandFloatRange, float(float min, float max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return min + Randomix::GetRNG().next_float() * (max - min);
}

SCRIPT_API(SeedRNG, int(int seed)) {
    Randomix::Seed(static_cast<uint64_t>(seed));
    return 1;
}

SCRIPT_API(RandBool, bool(float probability)) {
    if (probability <= 0.0f) return false;
    if (probability >= 1.0f) return true;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return Randomix::GetRNG().next_float() < probability;
}

SCRIPT_API(RandBoolWeighted, bool(int trueWeight, int falseWeight)) {
    if (trueWeight <= 0) return false;
    if (falseWeight <= 0) return true;

    uint32_t total = static_cast<uint32_t>(trueWeight + falseWeight);
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return Randomix::GetRNG().next_bounded(total) < static_cast<uint32_t>(trueWeight);
}

SCRIPT_API(RandWeighted, int(cell weightsAddr, int count)) {
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

SCRIPT_API(RandShuffle, bool(cell arrayAddr, int count)) {
    if (count <= 1) return true;
    
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    for (int i = count - 1; i > 0; i--) {
        int j = static_cast<int>(Randomix::GetRNG().next_bounded(i + 1));
        std::swap(array[i], array[j]);
    }
    
    return true;
}

SCRIPT_API(RandShuffleRange, bool(cell arrayAddr, int start, int end)) {
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return false;

    if (start > end) std::swap(start, end);
    if (end - start < 1) return true;

    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);

    for (int i = end; i > start; i--) {
        int j = start + Randomix::GetRNG().next_bounded(i - start + 1);
        std::swap(array[i], array[j]);
    }
    return true;
}

SCRIPT_API(RandGaussian, int(float mean, float stddev)) {
    if (stddev <= 0.0f) return static_cast<int>(mean);
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float u1 = Randomix::GetRNG().next_float();
    float u2 = Randomix::GetRNG().next_float();
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
    float result = mean + z0 * stddev;
    
    return static_cast<int>(result < 0.0f ? 0.0f : result);
}

SCRIPT_API(RandDice, int(int sides, int count)) {
    if (sides <= 0 || count <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    uint32_t total = 0;
    uint32_t uSides = static_cast<uint32_t>(sides);
    
    for (int i = 0; i < count; i++) {
        total += Randomix::GetRNG().next_bounded(uSides) + 1;
    }
    
    return static_cast<int>(total);
}

SCRIPT_API(RandPick, int(cell arrayAddr, int count)) {
    if (count <= 0) return 0;
    
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    uint32_t idx = Randomix::GetRNG().next_bounded(static_cast<uint32_t>(count));
    return array[idx];
}

SCRIPT_API(RandFormat, bool(cell destAddr, cell patternAddr, int destSize)) {
    if (destSize <= 0) return false;
    
    cell* dest = GetArrayPtr(GetAMX(), destAddr);
    cell* pattern = GetArrayPtr(GetAMX(), patternAddr);
    if (!dest || !pattern) return false;
    
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
    return true;
}

// ============================================================
// CRYPTOGRAPHIC FUNCTIONS
// ============================================================

SCRIPT_API(RandBytes, bool(cell destAddr, int length)) {
    if (length <= 0) return false;

    cell* dest = GetArrayPtr(GetAMX(), destAddr);
    if (!dest) return false;

    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);

    for (int i = 0; i < length; i++) {
        dest[i] = static_cast<cell>(Randomix::GetRNG().next_uint32() & 0xFF);
    }
    return true;
}

SCRIPT_API(RandUUID, bool(cell destAddr)) {
    cell* out = GetArrayPtr(GetAMX(), destAddr);
    if (!out) return false;

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
    return true;
}

// ============================================================
// 2D GEOMETRY
// ============================================================

SCRIPT_API(RandPointInCircle, bool(float centerX, float centerY, float radius, cell outX, cell outY)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * 6.28318530718f;
    float r = radius * sqrtf(Randomix::GetRNG().next_float());
    
    *reinterpret_cast<float*>(xAddr) = centerX + r * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + r * sinf(angle);
    return true;
}

SCRIPT_API(RandPointOnCircle, bool(float centerX, float centerY, float radius, cell outX, cell outY)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * 6.28318530718f;
    
    *reinterpret_cast<float*>(xAddr) = centerX + radius * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + radius * sinf(angle);
    return true;
}

SCRIPT_API(RandPointInRect, bool(float minX, float minY, float maxX, float maxY, cell outX, cell outY)) {
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    *reinterpret_cast<float*>(xAddr) = minX + Randomix::GetRNG().next_float() * (maxX - minX);
    *reinterpret_cast<float*>(yAddr) = minY + Randomix::GetRNG().next_float() * (maxY - minY);
    return true;
}

SCRIPT_API(RandPointInRing, bool(float centerX, float centerY, float innerRadius, float outerRadius, cell outX, cell outY)) {
    if (innerRadius < 0.0f || outerRadius <= 0.0f || innerRadius >= outerRadius) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * 6.28318530718f;
    float innerSq = innerRadius * innerRadius;
    float outerSq = outerRadius * outerRadius;
    float r = sqrtf(innerSq + Randomix::GetRNG().next_float() * (outerSq - innerSq));
    
    *reinterpret_cast<float*>(xAddr) = centerX + r * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + r * sinf(angle);
    return true;
}

SCRIPT_API(RandPointInEllipse, bool(float centerX, float centerY, float radiusX, float radiusY, cell outX, cell outY)) {
    if (radiusX <= 0.0f || radiusY <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * 6.28318530718f;
    float r = sqrtf(Randomix::GetRNG().next_float());
    
    *reinterpret_cast<float*>(xAddr) = centerX + radiusX * r * cosf(angle);
    *reinterpret_cast<float*>(yAddr) = centerY + radiusY * r * sinf(angle);
    return true;
}

SCRIPT_API(RandPointInTriangle, bool(float x1, float y1, float x2, float y2, float x3, float y3, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!xAddr || !yAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float r1 = Randomix::GetRNG().next_float();
    float r2 = Randomix::GetRNG().next_float();
    
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    float r3 = 1.0f - r1 - r2;
    
    *reinterpret_cast<float*>(xAddr) = r1 * x1 + r2 * x2 + r3 * x3;
    *reinterpret_cast<float*>(yAddr) = r1 * y1 + r2 * y2 + r3 * y3;
    return true;
}

// ============================================================
// 3D GEOMETRY
// ============================================================

SCRIPT_API(RandPointInSphere, bool(float centerX, float centerY, float centerZ, float radius, cell outX, cell outY, cell outZ)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    
    if (!xAddr || !yAddr || !zAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float x, y, z, sq;
    do {
        x = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        y = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        z = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        sq = x * x + y * y + z * z;
    } while (sq > 1.0f || sq == 0.0f);
    
    float scale = radius * cbrtf(Randomix::GetRNG().next_float()) / sqrtf(sq);
    
    *reinterpret_cast<float*>(xAddr) = centerX + x * scale;
    *reinterpret_cast<float*>(yAddr) = centerY + y * scale;
    *reinterpret_cast<float*>(zAddr) = centerZ + z * scale;
    return true;
}

SCRIPT_API(RandPointOnSphere, bool(float centerX, float centerY, float centerZ, float radius, cell outX, cell outY, cell outZ)) {
    if (radius <= 0.0f) return false;
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    
    if (!xAddr || !yAddr || !zAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float u, v, s;
    do {
        u = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        v = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    float multiplier = 2.0f * sqrtf(1.0f - s);
    
    *reinterpret_cast<float*>(xAddr) = centerX + radius * u * multiplier;
    *reinterpret_cast<float*>(yAddr) = centerY + radius * v * multiplier;
    *reinterpret_cast<float*>(zAddr) = centerZ + radius * (1.0f - 2.0f * s);
    return true;
}

SCRIPT_API(RandPointInBox, bool(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, cell outX, cell outY, cell outZ)) {
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    if (minZ > maxZ) std::swap(minZ, maxZ);
    
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    
    if (!xAddr || !yAddr || !zAddr) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    *reinterpret_cast<float*>(xAddr) = minX + Randomix::GetRNG().next_float() * (maxX - minX);
    *reinterpret_cast<float*>(yAddr) = minY + Randomix::GetRNG().next_float() * (maxY - minY);
    *reinterpret_cast<float*>(zAddr) = minZ + Randomix::GetRNG().next_float() * (maxZ - minZ);
    return true;
}

// ============================================================
// ADVANCED GEOMETRY
// ============================================================

SCRIPT_API(RandPointInPolygon, bool(cell verticesAddr, int vertexCount, cell outX, cell outY)) {
    if (vertexCount < 3) return false;
    
    cell* verticesPtr = GetArrayPtr(GetAMX(), verticesAddr);
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!verticesPtr || !xAddr || !yAddr) return false;
    
    float* vertices = reinterpret_cast<float*>(verticesPtr);
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float totalArea = 0.0f;
    std::vector<float> triangleAreas;
    
    for (int i = 1; i < vertexCount - 1; i++) {
        float x1 = vertices[0];
        float y1 = vertices[1];
        float x2 = vertices[i * 2];
        float y2 = vertices[i * 2 + 1];
        float x3 = vertices[(i + 1) * 2];
        float y3 = vertices[(i + 1) * 2 + 1];
        
        float area = fabsf((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)) * 0.5f;
        triangleAreas.push_back(area);
        totalArea += area;
    }
    
    if (totalArea <= 0.0f) return false;
    
    float rand = Randomix::GetRNG().next_float() * totalArea;
    float sum = 0.0f;
    int selectedTriangle = 0;
    
    for (size_t i = 0; i < triangleAreas.size(); i++) {
        sum += triangleAreas[i];
        if (rand < sum) {
            selectedTriangle = static_cast<int>(i);
            break;
        }
    }
    
    float x1 = vertices[0];
    float y1 = vertices[1];
    float x2 = vertices[(selectedTriangle + 1) * 2];
    float y2 = vertices[(selectedTriangle + 1) * 2 + 1];
    float x3 = vertices[(selectedTriangle + 2) * 2];
    float y3 = vertices[(selectedTriangle + 2) * 2 + 1];
    
    float r1 = Randomix::GetRNG().next_float();
    float r2 = Randomix::GetRNG().next_float();
    
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    
    float r3 = 1.0f - r1 - r2;
    
    *reinterpret_cast<float*>(xAddr) = r1 * x1 + r2 * x2 + r3 * x3;
    *reinterpret_cast<float*>(yAddr) = r1 * y1 + r2 * y2 + r3 * y3;
    
    return true;
}

// ============================================================
// COMPONENT CLASS
// ============================================================

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
    
    StringView componentName() const override { return "Randomix"; }
    SemanticVersion componentVersion() const override { return SemanticVersion(2, 0, 0, 0); }
    
    void onLoad(ICore* c) override {
        core_ = c;
        
        uint64_t seed = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        Randomix::Seed(seed);
        
        core_->printLn("");
        core_->printLn("  Randomix v2.0 Loaded");
        core_->printLn("  Algorithm: ChaCha20 (Cryptographic)");
        core_->printLn("  Security: CSPRNG Mode Only");
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
    
    void free() override { delete this; }
    
    void reset() override {
        uint64_t seed = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        Randomix::Seed(seed);
    }
};

COMPONENT_ENTRY_POINT() {
    return new RandomixComponent();
}