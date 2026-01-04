#include "randomix.hpp"
#include <chrono>
#include <algorithm>
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

// PCG32 Implementation
PCG32::PCG32(uint64_t seed) {
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

void PCG32::seed(uint64_t seed) {
    state = 0;
    inc = (INCREMENT << 1u) | 1u;
    next_uint32();
    state += seed;
    next_uint32();
}

uint32_t PCG32::next_uint32() {
    uint64_t oldstate = state;
    state = oldstate * MULTIPLIER + inc;
    
    uint32_t xorshifted = static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = static_cast<uint32_t>(oldstate >> 59u);
    
    return (xorshifted >> rot) | (xorshifted << ((~rot + 1) & 31u));
}

float PCG32::next_float() {
    return static_cast<float>(next_uint32()) / 4294967296.0f;
}

uint32_t PCG32::next_bounded(uint32_t bound) {
    if (bound == 0) return 0;
    
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

// ChaChaRNG Implementation
inline uint32_t ChaChaRNG::rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

void ChaChaRNG::quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
    a += b; d ^= a; d = rotl32(d, 16);
    c += d; b ^= c; b = rotl32(b, 12);
    a += b; d ^= a; d = rotl32(d, 8);
    c += d; b ^= c; b = rotl32(b, 7);
}

uint64_t ChaChaRNG::get_os_entropy() {
    uint64_t entropy = 0;
    
    #ifdef _WIN32
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
        if (syscall(SYS_getrandom, &entropy, sizeof(entropy), 0) != sizeof(entropy)) {
            int fd = open("/dev/urandom", O_RDONLY);
            if (fd >= 0) {
                read(fd, &entropy, sizeof(entropy));
                close(fd);
            }
        }
    #elif defined(__unix__) || defined(__APPLE__)
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd >= 0) {
            read(fd, &entropy, sizeof(entropy));
            close(fd);
        }
    #endif
    
    return entropy;
}

void ChaChaRNG::generate_block() {
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
    
    counter++;
    state[12] = static_cast<uint32_t>(counter);
    state[13] = static_cast<uint32_t>(counter >> 32);
    
    bytes_generated += 64;
    position = 0;
}

void ChaChaRNG::check_reseed() {
    if (bytes_generated >= RESEED_THRESHOLD) {
        uint64_t os_entropy = get_os_entropy();
        if (os_entropy != 0) {
            uint64_t current_seed = (static_cast<uint64_t>(state[4]) << 32) | state[5];
            seed(current_seed ^ os_entropy);
            bytes_generated = 0;
        }
    }
}

void ChaChaRNG::expand_seed(uint64_t seed, uint32_t* output, size_t count) {
    std::array<uint32_t, 16> temp_state;
    std::copy(CONSTANTS, CONSTANTS + 4, temp_state.begin());
    
    temp_state[4] = static_cast<uint32_t>(seed);
    temp_state[5] = static_cast<uint32_t>(seed >> 32);
    temp_state[6] = static_cast<uint32_t>(seed ^ 0x5A5A5A5A);
    temp_state[7] = static_cast<uint32_t>((seed >> 32) ^ 0xA5A5A5A5);
    
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    uint64_t nanos = static_cast<uint64_t>(now.count());
    temp_state[8] = static_cast<uint32_t>(nanos);
    temp_state[9] = static_cast<uint32_t>(nanos >> 32);
    temp_state[10] = static_cast<uint32_t>(nanos ^ seed);
    temp_state[11] = static_cast<uint32_t>((nanos >> 32) ^ (seed >> 32));
    
    temp_state[12] = 0;
    temp_state[13] = 0;
    temp_state[14] = 0;
    temp_state[15] = 0;
    
    uint32_t temp_block[16];
    size_t generated = 0;
    
    while (generated < count) {
        std::copy(temp_state.begin(), temp_state.end(), temp_block);
        
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
        
        temp_state[12]++;
        if (temp_state[12] == 0) temp_state[13]++;
    }
    
    std::fill(temp_state.begin(), temp_state.end(), 0);
    std::fill(temp_block, temp_block + 16, 0);
}

ChaChaRNG::ChaChaRNG(uint64_t seed) {
    bytes_generated = 0;
    
    if (seed == 0) {
        uint64_t os_entropy = get_os_entropy();
        
        if (os_entropy != 0) {
            seed = os_entropy;
        } else {
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

void ChaChaRNG::seed(uint64_t seed) {
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

uint32_t ChaChaRNG::next_uint32() {
    check_reseed();
    if (position >= 16) {
        generate_block();
    }
    return block[position++];
}

float ChaChaRNG::next_float() {
    uint32_t val = next_uint32() >> 8;
    return static_cast<float>(val) / 16777216.0f;
}

uint32_t ChaChaRNG::next_bounded(uint32_t bound) {
    if (bound == 0) return 0;
    if (bound == 1) return 0;
    
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

void ChaChaRNG::next_bytes(uint8_t* buffer, size_t length) {
    for (size_t i = 0; i < length; i += 4) {
        uint32_t val = next_uint32();
        size_t to_copy = std::min(static_cast<size_t>(4), length - i);
        std::memcpy(buffer + i, &val, to_copy);
    }
}

ChaChaRNG::~ChaChaRNG() {
    std::fill(state.begin(), state.end(), 0);
    std::fill(block, block + 16, 0);
    counter = 0;
    bytes_generated = 0;
    position = 0;
}

// Global Random Generators Implementation
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