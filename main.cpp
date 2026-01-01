// Randomix - Enhanced Random Number Generator for open.mp Servers
// Features: Fast PRNG for game mechanics, Cryptographic RNG for security

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
#include <cstring>

// OS-specific headers for system entropy
#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__)
    #include <sys/syscall.h>
    #include <unistd.h>
    #include <fcntl.h>
#elif defined(__unix__) || defined(__APPLE__)
    #include <unistd.h>
    #include <fcntl.h>
#endif

// PCG32 - Fast Random Generator
class PCG32 {
private:
    uint64_t state;
    uint64_t inc;
    
    static constexpr uint64_t MULTIPLIER = 6364136223846793005ULL;
    static constexpr uint64_t INCREMENT = 1442695040888963407ULL;
    
public:
    PCG32(uint64_t seed = 0) {
        // If no seed provided, use system time
        if (seed == 0) {
            seed = static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count()
            );
        }
        
        // Initialize state
        state = 0;
        inc = (INCREMENT << 1u) | 1u;
        next_uint32();
        state += seed;
        next_uint32();
    }
    
    void seed(uint64_t seed) {
        // Reset and set new seed
        state = 0;
        inc = (INCREMENT << 1u) | 1u;
        next_uint32();
        state += seed;
        next_uint32();
    }
    
    uint32_t next_uint32() {
        // Generate 32-bit random number
        uint64_t oldstate = state;
        state = oldstate * MULTIPLIER + inc;
        
        uint32_t xorshifted = static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
        uint32_t rot = static_cast<uint32_t>(oldstate >> 59u);
        
        return (xorshifted >> rot) | (xorshifted << ((~rot + 1) & 31u));
    }
    
    float next_float() {
        // Float between 0.0 and 1.0
        return static_cast<float>(next_uint32()) / 4294967296.0f;
    }
    
    uint32_t next_bounded(uint32_t bound) {
        // Random number within bounds (0 to bound-1)
        if (bound == 0) return 0;
        
        // Lemire's method for unbiased results
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

// ChaChaRNG - Cryptographic Random
class ChaChaRNG {
private:
    static constexpr int ROUNDS = 20;
    std::array<uint32_t, 16> state;
    uint32_t block[16];
    int position;
    uint64_t counter;
    uint64_t bytes_generated;
    static constexpr uint64_t RESEED_THRESHOLD = 32 * 1024 * 1024; // Reseed every 32MB
    
    // ChaCha20 constants
    static constexpr uint32_t CONSTANTS[4] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574
    };
    
    // Bit rotation left
    static inline uint32_t rotl32(uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    }
    
    // Quarter round operation (ChaCha20 core)
    void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = rotl32(d, 16);
        c += d; b ^= c; b = rotl32(b, 12);
        a += b; d ^= a; d = rotl32(d, 8);
        c += d; b ^= c; b = rotl32(b, 7);
    }
    
    // Get entropy from operating system
    uint64_t get_os_entropy() {
        uint64_t entropy = 0;
        
        #ifdef _WIN32
            // Windows: use built-in security API
            HMODULE advapi = LoadLibraryA("ADVAPI32.DLL");
            if (advapi) {
                typedef BOOLEAN (WINAPI *RtlGenRandomFunc)(PVOID, ULONG);
                RtlGenRandomFunc func = (RtlGenRandomFunc)GetProcAddress(advapi, "SystemFunction036");
                if (func) {
                    func(&entropy, sizeof(entropy));
                }
                FreeLibrary(advapi);
            }
        #elif defined(__linux__)
            // Linux: try getrandom syscall first
            if (syscall(SYS_getrandom, &entropy, sizeof(entropy), 0) != sizeof(entropy)) {
                // Fallback to /dev/urandom
                int fd = open("/dev/urandom", O_RDONLY);
                if (fd >= 0) {
                    read(fd, &entropy, sizeof(entropy));
                    close(fd);
                }
            }
        #elif defined(__unix__) || defined(__APPLE__)
            // Unix/Mac: use /dev/urandom directly
            int fd = open("/dev/urandom", O_RDONLY);
            if (fd >= 0) {
                read(fd, &entropy, sizeof(entropy));
                close(fd);
            }
        #endif
        
        return entropy;
    }
    
    // Generate one block of data (64 bytes)
    void generate_block() {
        std::copy(state.begin(), state.end(), block);
        
        // 20 rounds of ChaCha20 (10 double rounds)
        for (int i = 0; i < ROUNDS; i += 2) {
            // Column rounds
            quarter_round(block[0], block[4], block[8], block[12]);
            quarter_round(block[1], block[5], block[9], block[13]);
            quarter_round(block[2], block[6], block[10], block[14]);
            quarter_round(block[3], block[7], block[11], block[15]);
            
            // Diagonal rounds
            quarter_round(block[0], block[5], block[10], block[15]);
            quarter_round(block[1], block[6], block[11], block[12]);
            quarter_round(block[2], block[7], block[8], block[13]);
            quarter_round(block[3], block[4], block[9], block[14]);
        }
        
        // Add initial state
        for (int i = 0; i < 16; ++i) {
            block[i] += state[i];
        }
        
        // Increment counter
        counter++;
        state[12] = static_cast<uint32_t>(counter);
        state[13] = static_cast<uint32_t>(counter >> 32);
        
        bytes_generated += 64;
        position = 0;
    }
    
    // Check if reseed is needed
    void check_reseed() {
        if (bytes_generated >= RESEED_THRESHOLD) {
            uint64_t os_entropy = get_os_entropy();
            if (os_entropy != 0) {
                // Mix system entropy with current state
                uint64_t current_seed = (static_cast<uint64_t>(state[4]) << 32) | state[5];
                seed(current_seed ^ os_entropy);
                bytes_generated = 0;
            }
        }
    }
    
    // Expand seed into longer key
    void expand_seed(uint64_t seed, uint32_t* output, size_t count) {
        std::array<uint32_t, 16> temp_state;
        std::copy(CONSTANTS, CONSTANTS + 4, temp_state.begin());
        
        // Key from seed
        temp_state[4] = static_cast<uint32_t>(seed);
        temp_state[5] = static_cast<uint32_t>(seed >> 32);
        temp_state[6] = static_cast<uint32_t>(seed ^ 0x5A5A5A5A);
        temp_state[7] = static_cast<uint32_t>((seed >> 32) ^ 0xA5A5A5A5);
        
        // Nonce from high-precision time
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        uint64_t nanos = static_cast<uint64_t>(now.count());
        temp_state[8] = static_cast<uint32_t>(nanos);
        temp_state[9] = static_cast<uint32_t>(nanos >> 32);
        temp_state[10] = static_cast<uint32_t>(nanos ^ seed);
        temp_state[11] = static_cast<uint32_t>((nanos >> 32) ^ (seed >> 32));
        
        // Initial counter
        temp_state[12] = 0;
        temp_state[13] = 0;
        temp_state[14] = 0;
        temp_state[15] = 0;
        
        uint32_t temp_block[16];
        size_t generated = 0;
        
        while (generated < count) {
            std::copy(temp_state.begin(), temp_state.end(), temp_block);
            
            // ChaCha20 processing for key derivation
            for (int i = 0; i < ROUNDS; i += 2) {
                quarter_round(temp_block[0], temp_block[4], temp_block[8], temp_block[12]);
                quarter_round(temp_block[1], temp_block[5], temp_block[9], temp_block[13]);
                quarter_round(temp_block[2], temp_block[6], temp_block[10], temp_block[14]);
                quarter_round(temp_block[3], temp_block[7], temp_block[11], temp_block[15]);
                
                quarter_round(temp_block[0], temp_block[5], temp_block[10], temp_block[15]);
                quarter_round(temp_block[1], temp_block[6], temp_block[11], temp_block[12]);
                quarter_round(temp_block[2], temp_block[7], temp_block[8], temp_block[13]);
                quarter_round(temp_block[3], temp_block[4], temp_block[9], temp_block[14]);
            }
            
            for (int i = 0; i < 16; ++i) {
                temp_block[i] += temp_state[i];
            }
            
            size_t to_copy = std::min(count - generated, static_cast<size_t>(16));
            std::copy(temp_block, temp_block + to_copy, output + generated);
            generated += to_copy;
            
            // Increment counter
            temp_state[12]++;
            if (temp_state[12] == 0) temp_state[13]++;
        }
        
        // Clear sensitive data from memory
        std::fill(temp_state.begin(), temp_state.end(), 0);
        std::fill(temp_block, temp_block + 16, 0);
    }
    
public:
    ChaChaRNG(uint64_t seed = 0) {
        bytes_generated = 0;
        
        // If no seed provided, get from system
        if (seed == 0) {
            uint64_t os_entropy = get_os_entropy();
            
            if (os_entropy != 0) {
                seed = os_entropy;
            } else {
                // Fallback: combine multiple time sources
                auto sys_time = std::chrono::system_clock::now().time_since_epoch();
                auto hi_time = std::chrono::high_resolution_clock::now().time_since_epoch();
                
                uint64_t t1 = static_cast<uint64_t>(sys_time.count());
                uint64_t t2 = static_cast<uint64_t>(hi_time.count());
                
                seed = t1 ^ (t2 << 21) ^ (t2 >> 11);
                seed ^= reinterpret_cast<uint64_t>(&seed);
            }
        }
        
        counter = 0;
        std::copy(CONSTANTS, CONSTANTS + 4, state.begin());
        
        // Expand seed into key and nonce
        uint32_t expanded[12];
        expand_seed(seed, expanded, 12);
        
        // 256-bit key
        std::copy(expanded, expanded + 8, state.begin() + 4);
        
        // 64-bit nonce
        state[14] = expanded[8];
        state[15] = expanded[9];
        
        // Initial counter
        state[12] = 0;
        state[13] = 0;
        
        // Clear expanded seed
        std::fill(expanded, expanded + 12, 0);
        
        position = 16; // Force first block generation
    }
    
    void seed(uint64_t seed) {
        // Reset with new seed
        counter = 0;
        bytes_generated = 0;
        std::copy(CONSTANTS, CONSTANTS + 4, state.begin());
        
        uint32_t expanded[12];
        expand_seed(seed, expanded, 12);
        
        std::copy(expanded, expanded + 8, state.begin() + 4);
        state[14] = expanded[8];
        state[15] = expanded[9];
        state[12] = 0;
        state[13] = 0;
        
        std::fill(expanded, expanded + 12, 0);
        position = 16;
    }
    
    uint32_t next_uint32() {
        check_reseed(); // Check for auto-reseed
        if (position >= 16) {
            generate_block();
        }
        return block[position++];
    }
    
    float next_float() {
        // 24-bit precision float
        uint32_t val = next_uint32() >> 8;
        return static_cast<float>(val) / 16777216.0f; // 2^24
    }
    
    uint32_t next_bounded(uint32_t bound) {
        if (bound == 0) return 0;
        if (bound == 1) return 0;
        
        // Lemire's method with rejection sampling
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
    
    // Generate random bytes for cryptographic use
    void next_bytes(uint8_t* buffer, size_t length) {
        for (size_t i = 0; i < length; i += 4) {
            uint32_t val = next_uint32();
            size_t to_copy = std::min(static_cast<size_t>(4), length - i);
            std::memcpy(buffer + i, &val, to_copy);
        }
    }
    
    ~ChaChaRNG() {
        std::fill(state.begin(), state.end(), 0);
        std::fill(block, block + 16, 0);
        counter = 0;
        bytes_generated = 0;
        position = 0;
    }
};

// Global Random Generators
namespace RandomixGenerators {
    std::mutex prng_mutex;     // For PRNG (PCG32)
    std::mutex csprng_mutex;   // For CSPRNG (ChaChaRNG)
    
    // Singleton instance for PRNG
    PCG32& GetPRNG() {
        static PCG32 instance(0);
        return instance;
    }
    
    // Singleton instance for CSPRNG
    ChaChaRNG& GetCSPRNG() {
        static ChaChaRNG instance(0);
        return instance;
    }
    
    // Function to set PRNG seed
    void SeedPRNG(uint64_t seed) {
        std::lock_guard<std::mutex> lock(prng_mutex);
        GetPRNG().seed(seed);
    }
    
    // Function to set CSPRNG seed
    void SeedCSPRNG(uint64_t seed) {
        std::lock_guard<std::mutex> lock(csprng_mutex);
        GetCSPRNG().seed(seed);
    }
}

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
        
        // Seed with system time
        uint64_t seed = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        
        RandomixGenerators::SeedPRNG(seed);
        RandomixGenerators::SeedCSPRNG(seed);
        
        core_->printLn(" ");
        core_->printLn("Randomix Loaded - by Fanorisky");
        core_->printLn("Version 1.2.0 (https://github.com/Fanorisky/PawnRandomix)");
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
        // Reset seed with new time
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

// Basic PRNG (PCG32) - for game mechanics
SCRIPT_API(PRandom, int(int max)) {
    if (max < 0) return 0;
    std::lock_guard<std::mutex> lock(RandomixGenerators::prng_mutex);
    return static_cast<int>(RandomixGenerators::GetPRNG().next_bounded(static_cast<uint32_t>(max)));
}

// Basic CSPRNG (ChaCha20) - for security operations
SCRIPT_API(CSPRandom, int(int max)) {
    if (max < 0) return 0;
    std::lock_guard<std::mutex> lock(RandomixGenerators::csprng_mutex);
    return static_cast<int>(RandomixGenerators::GetCSPRNG().next_bounded(static_cast<uint32_t>(max)));
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
    
    // Calculate total weight
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
    
    // Find index based on weight
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
    
    // Fisher-Yates shuffle algorithm
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
    
    // Box-Muller transform
    float u1 = RandomixGenerators::GetPRNG().next_float();
    float u2 = RandomixGenerators::GetPRNG().next_float();
    
    // Avoid log(0)
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
    float result = mean + z0 * stddev;
    
    // Ensure non-negative result
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
    
    // Generate token as hexadecimal
    uint32_t token = 0;
    int actualLength = (length > 8) ? 8 : length; // Max 8 hex digits
    
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

    // UUID v4 format (RFC 4122)
    bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4
    bytes[8] = (bytes[8] & 0x3F) | 0x80; // RFC 4122 variant

    // Convert to hexadecimal string
    static const char* hex = "0123456789abcdef";
    int p = 0;

    auto write = [&](uint8_t b) {
        out[p++] = hex[b >> 4];
        out[p++] = hex[b & 0xF];
    };

    // Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    for (int i = 0; i < 16; i++) {
        write(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9)
            out[p++] = '-';
    }
    out[p] = '\0';
    return true;
}