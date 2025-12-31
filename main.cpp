// PawnRandomix - FIXED VERSION (No Singleton) dengan semua fungsi advanced
// Mengikuti pola BigInt yang work

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>

#include <cstdint>
#include <chrono>
#include <array>
#include <algorithm>
#include <limits>
#include <mutex>
#include <random>
#include <cmath>

// ==============================
// PCG32 Implementation
// ==============================
class PCG32 {
private:
    uint64_t state;
    uint64_t inc;
    
    static constexpr uint64_t MULTIPLIER = 6364136223846793005ULL;
    static constexpr uint64_t INCREMENT = 1442695040888963407ULL;
    
public:
    PCG32(uint64_t seed = 0) {
        if (seed == 0) {
            seed = static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count()
            );
        }
        
        state = 0;
        inc = (INCREMENT << 1u) | 1u;
        next_uint32();
        state += seed;
        next_uint32();
    }
    
    void seed(uint64_t seed) {
        state = 0;
        inc = (INCREMENT << 1u) | 1u;
        next_uint32();
        state += seed;
        next_uint32();
    }
    
    uint32_t next_uint32() {
        uint64_t oldstate = state;
        state = oldstate * MULTIPLIER + inc;
        
        uint32_t xorshifted = static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
        uint32_t rot = static_cast<uint32_t>(oldstate >> 59u);
        
        return (xorshifted >> rot) | (xorshifted << ((~rot + 1) & 31u));
    }
    
    float next_float() {
        return static_cast<float>(next_uint32()) / 4294967296.0f;
    }
    
    uint32_t next_bounded(uint32_t bound) {
        if (bound == 0) return 0;
        
        // Lemire's method untuk unbiased random
        uint64_t m = static_cast<uint64_t>(next_uint32()) * static_cast<uint64_t>(bound);
        uint32_t leftover = static_cast<uint32_t>(m);
        
        if (leftover < bound) {
            uint32_t threshold = (0u - bound) % bound;
            while (leftover < threshold) {
                m = static_cast<uint64_t>(next_uint32()) * static_cast<uint64_t>(bound);
                leftover = static_cast<uint32_t>(m);
            }
        }
        
        return static_cast<uint32_t>(m >> 32);
    }
};

// ==============================
// ChaChaRNG Implementation
// ==============================
class ChaChaRNG {
private:
    static constexpr int ROUNDS = 20;
    std::array<uint32_t, 16> state;
    uint32_t block[16];
    int position;
    
    static constexpr uint32_t CONSTANTS[4] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574
    };
    
    static inline uint32_t rotl32(uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    }
    
    void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = rotl32(d, 16);
        c += d; b ^= c; b = rotl32(b, 12);
        a += b; d ^= a; d = rotl32(d, 8);
        c += d; b ^= c; b = rotl32(b, 7);
    }
    
    void generate_block() {
        std::copy(state.begin(), state.end(), block);
        
        for (int i = 0; i < ROUNDS; i += 2) {
            quarter_round(block[0], block[4], block[8], block[12]);
            quarter_round(block[1], block[5], block[9], block[13]);
            quarter_round(block[2], block[6], block[10], block[14]);
            quarter_round(block[3], block[7], block[11], block[15]);
            
            quarter_round(block[0], block[5], block[10], block[15]);
            quarter_round(block[1], block[6], block[11], block[12]);
            quarter_round(block[2], block[7], block[8], block[13]);
            quarter_round(block[3], block[4], block[9], block[14]);
        }
        
        for (int i = 0; i < 16; ++i) {
            block[i] += state[i];
        }
        
        state[12]++;
        position = 0;
    }
    
public:
    ChaChaRNG(uint64_t seed = 0) {
        if (seed == 0) {
            seed = static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count()
            );
        }
        
        std::copy(CONSTANTS, CONSTANTS + 4, state.begin());
        
        uint32_t seed_parts[8];
        seed_parts[0] = static_cast<uint32_t>(seed);
        seed_parts[1] = static_cast<uint32_t>(seed >> 32);
        
        for (int i = 2; i < 8; ++i) {
            seed_parts[i] = seed_parts[i - 2] ^ (seed_parts[i - 1] << 13) ^ 0x9E3779B9u;
        }
        
        std::copy(seed_parts, seed_parts + 8, state.begin() + 4);
        
        state[12] = 0;
        state[13] = 0;
        state[14] = 0xDEADBEEF;
        state[15] = 0xCAFEBABE;
        
        position = 16;
    }
    
    void seed(uint64_t seed) {
        std::copy(CONSTANTS, CONSTANTS + 4, state.begin());
        
        uint32_t seed_parts[8];
        seed_parts[0] = static_cast<uint32_t>(seed);
        seed_parts[1] = static_cast<uint32_t>(seed >> 32);
        
        for (int i = 2; i < 8; ++i) {
            seed_parts[i] = seed_parts[i - 2] ^ (seed_parts[i - 1] << 13) ^ 0x9E3779B9u;
        }
        
        std::copy(seed_parts, seed_parts + 8, state.begin() + 4);
        
        state[12] = 0;
        state[13] = 0;
        state[14] = 0xDEADBEEF;
        state[15] = 0xCAFEBABE;
        
        position = 16;
    }
    
    uint32_t next_uint32() {
        if (position >= 16) {
            generate_block();
        }
        return block[position++];
    }
    
    float next_float() {
        return static_cast<float>(next_uint32()) / 4294967296.0f;
    }
    
    uint32_t next_bounded(uint32_t bound) {
        if (bound == 0) return 0;
        
        // Lemire's method untuk unbiased random
        uint64_t m = static_cast<uint64_t>(next_uint32()) * static_cast<uint64_t>(bound);
        uint32_t leftover = static_cast<uint32_t>(m);
        
        if (leftover < bound) {
            uint32_t threshold = (0u - bound) % bound;
            while (leftover < threshold) {
                m = static_cast<uint64_t>(next_uint32()) * static_cast<uint64_t>(bound);
                leftover = static_cast<uint32_t>(m);
            }
        }
        
        return static_cast<uint32_t>(m >> 32);
    }
};

// ==============================
// Global Generators (No Singleton Pattern)
// ==============================
namespace RandomixGenerators {
    std::mutex prng_mutex;
    std::mutex csprng_mutex;
    
    PCG32& GetPRNG() {
        static PCG32 instance(0);
        return instance;
    }
    
    ChaChaRNG& GetCSPRNG() {
        static ChaChaRNG instance(0);
        return instance;
    }
    
    void SeedPRNG(uint64_t seed) {
        std::lock_guard<std::mutex> lock(prng_mutex);
        GetPRNG().seed(seed);
    }
    
    void SeedCSPRNG(uint64_t seed) {
        std::lock_guard<std::mutex> lock(csprng_mutex);
        GetCSPRNG().seed(seed);
    }
}

// ==============================
// Helper Functions untuk Array Access
// ==============================
static inline cell* GetArrayPtr(AMX* amx, cell param)
{
    cell* addr = nullptr;
    amx_GetAddr(amx, param, &addr);
    return addr;
}

// ==============================
// Main Randomix Component
// ==============================
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
        return "PawnRandomix";
    }
    
    SemanticVersion componentVersion() const override {
        return SemanticVersion(1, 1, 0, 0);
    }
    
    void onLoad(ICore* c) override {
        core_ = c;
        
        // Seed dengan waktu sistem
        uint64_t seed = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        
        RandomixGenerators::SeedPRNG(seed);
        RandomixGenerators::SeedCSPRNG(seed);
        
        core_->printLn(" ");
        core_->printLn("  PawnRandomix Component Loaded! [v1.1.0 - Fixed]");
        core_->printLn("  PRNG: PCG32 (Fast, unbiased statistical)");
        core_->printLn("  CSPRNG: ChaCha20 (Cryptographically secure)");
        core_->printLn("  Fixed: Modulo Bias + Float Consistency");
        core_->printLn("  Advanced functions: Shuffle, Gaussian, Dice, Weighted, etc.");
        core_->printLn(" ");
        
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
        // Re-seed dengan waktu baru
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

// ==============================
// Basic Pawn Native Functions
// ==============================

// PRNG functions (PCG)
SCRIPT_API(PRandom, int(int max)) {
    if (max < 0) return 0;
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(static_cast<uint32_t>(max) + 1));
}

SCRIPT_API(PRandRange, int(int min, int max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return min + static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(range + 1));
}

SCRIPT_API(PRandFloatRange, float(float min, float max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return min + RandomixGenerators::GetPRNG().next_float() * (max - min);
}

// CSPRNG functions (ChaCha)
SCRIPT_API(CSPRandom, int(int max)) {
    if (max < 0) return 0;
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return static_cast<int>(RandomixGenerators::GetCSPRNG().next_bounded(static_cast<uint32_t>(max) + 1));
}

SCRIPT_API(CSPRandRange, int(int min, int max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return min + static_cast<int>(RandomixGenerators::GetCSPRNG().next_bounded(range + 1));
}

SCRIPT_API(CSPRandFloatRange, float(float min, float max)) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return min + RandomixGenerators::GetCSPRNG().next_float() * (max - min);
}

// Seed functions
SCRIPT_API(SeedPRNG, int(int seed)) {
    RandomixGenerators::SeedPRNG(static_cast<uint64_t>(seed));
    return 1;
}

SCRIPT_API(SeedCSPRNG, int(int seed)) {
    RandomixGenerators::SeedCSPRNG(static_cast<uint64_t>(seed));
    return 1;
}

// ==============================
// ADVANCED NATIVE FUNCTIONS
// ==============================

// Random boolean dengan probability
SCRIPT_API(PRandBool, bool(float probability)) {
    if (probability <= 0.0f) return false;
    if (probability >= 1.0f) return true;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return RandomixGenerators::GetPRNG().next_float() < probability;
}

SCRIPT_API(CSPRandBool, bool(float probability)) {
    if (probability <= 0.0f) return false;
    if (probability >= 1.0f) return true;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return RandomixGenerators::GetCSPRNG().next_float() < probability;
}

// Weighted random selection
SCRIPT_API(PRandWeighted, int(cell weightsAddr, int count)) {
    if (count <= 0) return 0;
    
    cell* weights = GetArrayPtr(GetAMX(), weightsAddr);
    if (!weights) return 0;
    
    // Hitung total weight
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

// Shuffle array in-place (Fisher-Yates algorithm)
SCRIPT_API(PRandShuffle, bool(cell arrayAddr, int count)) {
    if (count <= 1) return true;
    
    cell* array = GetArrayPtr(GetAMX(), arrayAddr);
    if (!array) return false;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // Fisher-Yates shuffle
    for (int i = count - 1; i > 0; i--) {
        int j = static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(i + 1));
        std::swap(array[i], array[j]);
    }
    
    return true;
}

// Gaussian/Normal distribution (Box-Muller transform)
SCRIPT_API(PRandGaussian, int(float mean, float stddev)) {
    if (stddev <= 0.0f) return static_cast<int>(mean);
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    // Box-Muller transform
    float u1 = RandomixGenerators::GetPRNG().next_float();
    float u2 = RandomixGenerators::GetPRNG().next_float();
    
    // Hindari log(0)
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
    float result = mean + z0 * stddev;
    
    // Kembalikan integer positif
    return static_cast<int>(result < 0.0f ? 0.0f : result);
}

// Roll dice (D&D style)
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

// Cryptographic token generation (hexadecimal)
SCRIPT_API(CSPRandToken, int(int length)) {
    if (length <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    // Generate token sebagai hexadecimal
    uint32_t token = 0;
    int actualLength = (length > 8) ? 8 : length; // Maksimal 8 hex digits (32-bit)
    
    for (int i = 0; i < actualLength; i++) {
        token = (token << 4) | (RandomixGenerators::GetCSPRNG().next_bounded(16));
    }
    
    return static_cast<int>(token);
}

// Random string dari karakter set
SCRIPT_API(PRandString, int(cell destAddr, int maxLen, const std::string& charset)) {
    if (maxLen <= 0 || charset.empty()) return 0;
    
    cell* dest = GetArrayPtr(GetAMX(), destAddr);
    if (!dest) return 0;
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    
    int length = static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(maxLen)) + 1;
    int charsetSize = static_cast<int>(charset.size());
    
    for (int i = 0; i < length; i++) {
        int idx = static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(charsetSize));
        dest[i] = static_cast<cell>(charset[idx]);
    }
    
    dest[length] = 0; // Null terminator
    return length;
}

// Generate random UUID (version 4)
SCRIPT_API(CSPRandUUID, int(OutputOnlyString& str)) {
    // Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    // dimana x adalah random hex digit, y adalah 8, 9, A, atau B
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    std::string uuid;
    const char hex[] = "0123456789abcdef";
    
    for (int i = 0; i < 32; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            uuid += '-';
        }
        
        if (i == 12) {
            uuid += '4'; // Version 4 UUID
        } else if (i == 16) {
            uint32_t rnd = RandomixGenerators::GetCSPRNG().next_bounded(4);
            uuid += hex[8 + rnd]; // 8, 9, a, atau b
        } else {
            uint32_t rnd = RandomixGenerators::GetCSPRNG().next_bounded(16);
            uuid += hex[rnd];
        }
    }
    
    str = uuid;
    return std::get<StringView>(str).length();
}

// Random password generator
SCRIPT_API(CSPRandPassword, int(OutputOnlyString& str, int length, int flags)) {
    if (length <= 0) length = 12;
    
    const std::string lowercase = "abcdefghijklmnopqrstuvwxyz";
    const std::string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const std::string digits = "0123456789";
    const std::string symbols = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    std::string charset;
    
    // Build charset berdasarkan flags
    if (flags & 1) charset += lowercase;      // 1 = lowercase
    if (flags & 2) charset += uppercase;      // 2 = uppercase
    if (flags & 4) charset += digits;         // 4 = digits
    if (flags & 8) charset += symbols;        // 8 = symbols
    
    // Default: lowercase + uppercase + digits
    if (charset.empty()) {
        charset = lowercase + uppercase + digits;
    }
    
    if (charset.empty()) {
        str = "";
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    
    std::string password;
    int charsetSize = static_cast<int>(charset.size());
    
    for (int i = 0; i < length; i++) {
        int idx = static_cast<int>(RandomixGenerators::GetCSPRNG().next_bounded(charsetSize));
        password += charset[idx];
    }
    
    str = password;
    return std::get<StringView>(str).length();
}