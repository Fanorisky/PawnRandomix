#pragma once

#include <cstdint>
#include <array>
#include <mutex>

class ChaChaRNG {
private:
    static constexpr int ROUNDS = 20;
    std::array<uint32_t, 16> state;
    uint32_t block[16];
    int position;
    uint64_t counter;
    uint64_t bytes_generated;
    static constexpr uint64_t RESEED_THRESHOLD = 1024ULL * 1024ULL * 1024ULL; // 1GB
    
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
    explicit ChaChaRNG(uint64_t seed = 0);
    ~ChaChaRNG();
    
    // Non-copyable for security
    ChaChaRNG(const ChaChaRNG&) = delete;
    ChaChaRNG& operator=(const ChaChaRNG&) = delete;
    ChaChaRNG(ChaChaRNG&&) = delete;
    ChaChaRNG& operator=(ChaChaRNG&&) = delete;
    
    void seed(uint64_t seed);
    uint32_t next_uint32() noexcept;
    float next_float() noexcept;
    uint32_t next_bounded(uint32_t bound);
    void next_bytes(uint8_t* buffer, size_t length);
};

// Global Singleton
namespace Randomix {
    extern std::mutex rng_mutex;
    ChaChaRNG& GetRNG();
    void Seed(uint64_t seed);
}