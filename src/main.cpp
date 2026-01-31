/*
 *  Randomix - Cryptographic Random Number Generator for open.mp
 *  Version: 2.0.1 (CSPRNG Only - ChaCha20)
 */

#include "randomix.hpp"
#include "randomix_impl.hpp"
#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>

#include <chrono>
#include <cstring>

static inline cell* GetArrayPtr(AMX* amx, cell param) {
    cell* addr = nullptr;
    amx_GetAddr(amx, param, &addr);
    return addr;
}

// Core random functions

SCRIPT_API(RandRange, int(int min, int max)) {
    return ImplRandRange(min, max);
}

SCRIPT_API(RandFloatRange, float(float min, float max)) {
    return ImplRandFloatRange(min, max);
}

SCRIPT_API(SeedRNG, int(int seed)) {
    Randomix::Seed(static_cast<uint64_t>(seed));
    return 1;
}

SCRIPT_API(RandBool, bool(float probability)) {
    return ImplRandBool(probability);
}

SCRIPT_API(RandBoolWeighted, bool(int trueWeight, int falseWeight)) {
    return ImplRandBoolWeighted(trueWeight, falseWeight);
}

SCRIPT_API(RandWeighted, int(cell weightsAddr, int count)) {
    if (count <= 0) return 0;
    
    cell* weights = GetArrayPtr(GetAMX(), weightsAddr);
    if (!weights) return 0;
    
    return ImplRandWeighted(reinterpret_cast<int*>(weights), count);
}

SCRIPT_API(RandShuffle, bool(cell arrayAddr, int count)) {
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return false;
    
    return ImplRandShuffle(reinterpret_cast<int*>(array), count);
}

SCRIPT_API(RandShuffleRange, bool(cell arrayAddr, int start, int end)) {
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return false;

    return ImplRandShuffleRange(reinterpret_cast<int*>(array), start, end);
}

SCRIPT_API(RandGaussian, int(float mean, float stddev)) {
    return ImplRandGaussian(mean, stddev);
}

SCRIPT_API(RandDice, int(int sides, int count)) {
    return ImplRandDice(sides, count);
}

SCRIPT_API(RandPick, int(cell arrayAddr, int count)) {
    if (count <= 0) return 0;
    
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return 0;
    
    return ImplRandPick(reinterpret_cast<int*>(array), count);
}

SCRIPT_API(RandFormat, bool(cell destAddr, cell patternAddr, int destSize)) {
    cell* dest = GetArrayPtr(GetAMX(), destAddr);
    cell* pattern = GetArrayPtr(GetAMX(), patternAddr);
    if (!dest || !pattern) return false;
    
    // Convert cell pattern to char buffer with bounds checking
    static thread_local char patternBuf[256];
    int i;
    for (i = 0; i < 255; i++) {
        cell c;
        amx_GetAddr(GetAMX(), patternAddr + i, reinterpret_cast<cell**>(&c));
        if (c == 0) break;
        patternBuf[i] = static_cast<char>(c);
    }
    patternBuf[i] = '\0';
    
    char destBuf[1024];
    if (!ImplRandFormat(destBuf, patternBuf, sizeof(destBuf))) {
        return false;
    }
    
    // Copy back to AMX
    for (i = 0; destBuf[i] != '\0' && i < destSize - 1; i++) {
        dest[i] = static_cast<cell>(destBuf[i]);
    }
    dest[i] = '\0';
    return true;
}

// Cryptographic functions

SCRIPT_API(RandBytes, bool(cell destAddr, int length)) {
    if (length <= 0) return false;

    cell* dest = GetArrayPtr(GetAMX(), destAddr);
    if (!dest) return false;

    static thread_local uint8_t buffer[65536];
    int len = (length > 65536) ? 65536 : length;
    
    if (!ImplRandBytes(buffer, len)) return false;
    
    for (int i = 0; i < len; i++) {
        dest[i] = static_cast<cell>(buffer[i]);
    }
    return true;
}

SCRIPT_API(RandUUID, bool(cell destAddr)) {
    cell* out = GetArrayPtr(GetAMX(), destAddr);
    if (!out) return false;

    char uuidBuf[40];
    if (!ImplRandUUID(uuidBuf)) return false;
    
    for (int i = 0; uuidBuf[i] != '\0'; i++) {
        out[i] = static_cast<cell>(uuidBuf[i]);
    }
    return true;
}

// 2D geometry

SCRIPT_API(RandPointInCircle, bool(float centerX, float centerY, float radius, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    if (!xAddr || !yAddr) return false;
    
    float x, y;
    if (!ImplRandPointInCircle(centerX, centerY, radius, x, y)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    return true;
}

SCRIPT_API(RandPointOnCircle, bool(float centerX, float centerY, float radius, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    if (!xAddr || !yAddr) return false;
    
    float x, y;
    if (!ImplRandPointOnCircle(centerX, centerY, radius, x, y)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    return true;
}

SCRIPT_API(RandPointInRect, bool(float minX, float minY, float maxX, float maxY, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    if (!xAddr || !yAddr) return false;
    
    float x, y;
    if (!ImplRandPointInRect(minX, minY, maxX, maxY, x, y)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    return true;
}

SCRIPT_API(RandPointInRing, bool(float centerX, float centerY, float innerRadius, float outerRadius, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    if (!xAddr || !yAddr) return false;
    
    float x, y;
    if (!ImplRandPointInRing(centerX, centerY, innerRadius, outerRadius, x, y)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    return true;
}

SCRIPT_API(RandPointInEllipse, bool(float centerX, float centerY, float radiusX, float radiusY, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    if (!xAddr || !yAddr) return false;
    
    float x, y;
    if (!ImplRandPointInEllipse(centerX, centerY, radiusX, radiusY, x, y)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    return true;
}

SCRIPT_API(RandPointInTriangle, bool(float x1, float y1, float x2, float y2, float x3, float y3, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    if (!xAddr || !yAddr) return false;
    
    float x, y;
    if (!ImplRandPointInTriangle(x1, y1, x2, y2, x3, y3, x, y)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    return true;
}

// RandPointInArc - random point in arc/circular sector

SCRIPT_API(RandPointInArc, bool(float centerX, float centerY, float radius, float startAngle, float endAngle, cell outX, cell outY)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    if (!xAddr || !yAddr) return false;
    
    float x, y;
    if (!ImplRandPointInArc(centerX, centerY, radius, startAngle, endAngle, x, y)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    return true;
}

// 3D geometry

SCRIPT_API(RandPointInSphere, bool(float centerX, float centerY, float centerZ, float radius, cell outX, cell outY, cell outZ)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    if (!xAddr || !yAddr || !zAddr) return false;
    
    float x, y, z;
    if (!ImplRandPointInSphere(centerX, centerY, centerZ, radius, x, y, z)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    *reinterpret_cast<float*>(zAddr) = z;
    return true;
}

SCRIPT_API(RandPointOnSphere, bool(float centerX, float centerY, float centerZ, float radius, cell outX, cell outY, cell outZ)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    if (!xAddr || !yAddr || !zAddr) return false;
    
    float x, y, z;
    if (!ImplRandPointOnSphere(centerX, centerY, centerZ, radius, x, y, z)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    *reinterpret_cast<float*>(zAddr) = z;
    return true;
}

SCRIPT_API(RandPointInBox, bool(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, cell outX, cell outY, cell outZ)) {
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    cell* zAddr = GetArrayPtr(GetAMX(), outZ);
    if (!xAddr || !yAddr || !zAddr) return false;
    
    float x, y, z;
    if (!ImplRandPointInBox(minX, minY, minZ, maxX, maxY, maxZ, x, y, z)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    *reinterpret_cast<float*>(zAddr) = z;
    return true;
}

// Advanced geometry

SCRIPT_API(RandPointInPolygon, bool(cell verticesAddr, int vertexCount, cell outX, cell outY)) {
    if (vertexCount < 3) return false;
    
    cell* verticesPtr = GetArrayPtr(GetAMX(), verticesAddr);
    cell* xAddr = GetArrayPtr(GetAMX(), outX);
    cell* yAddr = GetArrayPtr(GetAMX(), outY);
    
    if (!verticesPtr || !xAddr || !yAddr) return false;
    
    // Bounds check for vertex count
    if (vertexCount > MAX_POLYGON_VERTICES) return false;
    
    float x, y;
    if (!ImplRandPointInPolygon(reinterpret_cast<float*>(verticesPtr), vertexCount, x, y)) return false;
    
    *reinterpret_cast<float*>(xAddr) = x;
    *reinterpret_cast<float*>(yAddr) = y;
    return true;
}

// Component class

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
    SemanticVersion componentVersion() const override { return SemanticVersion(2, 0, 1, 0); }
    
    void onLoad(ICore* c) override {
        core_ = c;
        
        uint64_t seed = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        Randomix::Seed(seed);
        
        core_->printLn("");
        core_->printLn("  Randomix v2.0.1 Loaded");
        core_->printLn("  Algorithm: ChaCha20 (Cryptographic)");
        core_->printLn("  Author: Fanorisky (https://github.com/Fanorisky/PawnRandomix)");
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
            pawn_->getEventDispatcher().removeEventHandler(this);
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
