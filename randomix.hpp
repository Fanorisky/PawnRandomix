#pragma once

#include <cstdint>
#include <array>
#include <mutex>

// PCG32 - Fast Random Generator
class PCG32 {
private:
    uint64_t state;
    uint64_t inc;
    
    static constexpr uint64_t MULTIPLIER = 6364136223846793005ULL;
    static constexpr uint64_t INCREMENT = 1442695040888963407ULL;
    
public:
    PCG32(uint64_t seed = 0);
    void seed(uint64_t seed);
    uint32_t next_uint32();
    float next_float();
    uint32_t next_bounded(uint32_t bound);
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
    static constexpr uint64_t RESEED_THRESHOLD = 32 * 1024 * 1024;
    
    static constexpr uint32_t CONSTANTS[4] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574
    };
    
    static inline uint32_t rotl32(uint32_t x, int n);
    void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d);
    uint64_t get_os_entropy();
    void generate_block();
    void check_reseed();
    void expand_seed(uint64_t seed, uint32_t* output, size_t count);
    
public:
    ChaChaRNG(uint64_t seed = 0);
    ~ChaChaRNG();
    void seed(uint64_t seed);
    uint32_t next_uint32();
    float next_float();
    uint32_t next_bounded(uint32_t bound);
    void next_bytes(uint8_t* buffer, size_t length);
};

// Global Random Generators
namespace RandomixGenerators {
    extern std::mutex prng_mutex;
    extern std::mutex csprng_mutex;
    
    PCG32& GetPRNG();
    ChaChaRNG& GetCSPRNG();
    void SeedPRNG(uint64_t seed);
    void SeedCSPRNG(uint64_t seed);
}