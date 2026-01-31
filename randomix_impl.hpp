/*
 *  Randomix - Shared Implementation Header
 *  Version: 2.0.1
 * 
 *  This header contains shared implementations for both SA-MP and open.mp
 *  to avoid code duplication between main.cpp and main_samp.cpp
 */

#pragma once

#include "randomix.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <climits>

// Constants

static constexpr float PI = 3.14159265359f;
static constexpr float TWO_PI = 6.28318530718f;
static constexpr int MAX_POLYGON_VERTICES = 128;  // Stack buffer size for polygon triangulation

// Bounds checking utilities

template<typename T>
inline bool CheckArrayBounds(T* ptr, size_t minSize = 1) {
    return ptr != nullptr;
}

inline bool CheckRangeValid(int min, int max) {
    return min <= max;
}

inline bool CheckFloatRangeValid(float min, float max) {
    if (std::isnan(min) || std::isnan(max)) return false;
    return min <= max;
}

inline bool CheckPositive(float value) {
    return value > 0.0f && !std::isnan(value) && !std::isinf(value);
}

inline bool CheckNonNegative(float value) {
    return value >= 0.0f && !std::isnan(value) && !std::isinf(value);
}

inline bool CheckValidProbability(float prob) {
    return !std::isnan(prob) && !std::isinf(prob);
}

// Core random functions

inline int ImplRandRange(int min, int max) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    uint32_t range = static_cast<uint32_t>(max - min);
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return min + static_cast<int>(Randomix::GetRNG().next_bounded(range + 1));
}

inline float ImplRandFloatRange(float min, float max) {
    if (min > max) std::swap(min, max);
    if (min == max) return min;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return min + Randomix::GetRNG().next_float() * (max - min);
}

inline bool ImplRandBool(float probability) {
    if (probability <= 0.0f) return false;
    if (probability >= 1.0f) return true;
    if (!CheckValidProbability(probability)) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return Randomix::GetRNG().next_float() < probability;
}

inline bool ImplRandBoolWeighted(int trueWeight, int falseWeight) {
    if (trueWeight <= 0) return false;
    if (falseWeight <= 0) return true;
    
    if (trueWeight > INT_MAX - falseWeight) {
        trueWeight /= 2;
        falseWeight /= 2;
    }
    
    uint32_t total = static_cast<uint32_t>(trueWeight + falseWeight);
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    return Randomix::GetRNG().next_bounded(total) < static_cast<uint32_t>(trueWeight);
}

inline int ImplRandWeighted(const int* weights, int count) {
    if (count <= 0 || weights == nullptr) return 0;
    if (count > 65536) return 0;
    
    uint32_t total = 0;
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0) {
            if (weights[i] > static_cast<int>(UINT32_MAX - total)) {
                return 0;
            }
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

inline bool ImplRandShuffle(int* array, int count) {
    if (count <= 1) return true;
    if (array == nullptr) return false;
    if (count > 10000000) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    for (int i = count - 1; i > 0; i--) {
        int j = static_cast<int>(Randomix::GetRNG().next_bounded(i + 1));
        std::swap(array[i], array[j]);
    }
    
    return true;
}

inline bool ImplRandShuffleRange(int* array, int start, int end) {
    if (array == nullptr) return false;
    if (start > end) std::swap(start, end);
    if (end - start < 1) return true;
    if (start < 0) return false;
    if (end > 10000000) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    for (int i = end; i > start; i--) {
        int j = start + static_cast<int>(Randomix::GetRNG().next_bounded(i - start + 1));
        std::swap(array[i], array[j]);
    }
    return true;
}

inline int ImplRandGaussian(float mean, float stddev) {
    if (stddev <= 0.0f) return static_cast<int>(mean);
    if (!CheckPositive(stddev)) return static_cast<int>(mean);
    if (std::isnan(mean) || std::isinf(mean)) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float u1 = Randomix::GetRNG().next_float();
    float u2 = Randomix::GetRNG().next_float();
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float z0 = std::sqrt(-2.0f * std::log(u1)) * std::cos(TWO_PI * u2);
    float result = mean + z0 * stddev;
    
    return static_cast<int>(result < 0.0f ? 0.0f : result);
}

inline int ImplRandDice(int sides, int count) {
    if (sides <= 0 || count <= 0) return 0;
    if (sides > 10000 || count > 10000) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    uint32_t total = 0;
    uint32_t uSides = static_cast<uint32_t>(sides);
    
    for (int i = 0; i < count; i++) {
        total += Randomix::GetRNG().next_bounded(uSides) + 1;
    }
    
    return static_cast<int>(total);
}

inline int ImplRandPick(const int* array, int count) {
    if (count <= 0 || array == nullptr) return 0;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    uint32_t idx = Randomix::GetRNG().next_bounded(static_cast<uint32_t>(count));
    return array[idx];
}

// String & token functions

inline bool ImplRandFormat(char* dest, const char* pattern, int destSize) {
    if (destSize <= 0 || dest == nullptr || pattern == nullptr) return false;
    if (destSize > 65536) return false;
    
    static const char* upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const char* lower = "abcdefghijklmnopqrstuvwxyz";
    static const char* digit = "0123456789";
    static const char* alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static const char* symbol = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    auto& rng = Randomix::GetRNG();
    
    int outPos = 0;
    int patternLen = static_cast<int>(std::strlen(pattern));
    
    for (int i = 0; i < patternLen && outPos < destSize - 1; i++) {
        char c = pattern[i];
        const char* charset = nullptr;
        size_t len = 0;
        
        switch (c) {
            case 'X': charset = upper; len = 26; break;
            case 'x': charset = lower; len = 26; break;
            case '9': charset = digit; len = 10; break;
            case 'A': charset = alpha; len = 62; break;
            case '!': charset = symbol; len = 25; break;
            default:
                if (c == '\\' && i + 1 < patternLen) {
                    i++;
                    dest[outPos++] = pattern[i];
                } else {
                    dest[outPos++] = c;
                }
                continue;
        }
        
        if (charset && len > 0) {
            uint32_t idx = rng.next_bounded(static_cast<uint32_t>(len));
            dest[outPos++] = charset[idx];
        }
    }
    
    dest[outPos] = '\0';
    return true;
}

inline bool ImplRandBytes(uint8_t* buffer, int length) {
    if (length <= 0 || buffer == nullptr) return false;
    if (length > 65536) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    for (int i = 0; i < length; i++) {
        buffer[i] = static_cast<uint8_t>(Randomix::GetRNG().next_uint32() & 0xFF);
    }
    return true;
}

inline bool ImplRandUUID(char* out) {
    if (out == nullptr) return false;
    
    uint8_t bytes[16];
    {
        std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
        Randomix::GetRNG().next_bytes(bytes, 16);
    }
    
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;
    
    static const char* hex = "0123456789abcdef";
    int p = 0;
    
    for (int i = 0; i < 16; i++) {
        out[p++] = hex[bytes[i] >> 4];
        out[p++] = hex[bytes[i] & 0xF];
        if (i == 3 || i == 5 || i == 7 || i == 9)
            out[p++] = '-';
    }
    out[p] = '\0';
    return true;
}

// 2D geometry functions

inline bool ImplRandPointInCircle(float centerX, float centerY, float radius, float& outX, float& outY) {
    if (!CheckPositive(radius)) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * TWO_PI;
    float r = radius * std::sqrt(Randomix::GetRNG().next_float());
    
    outX = centerX + r * std::cos(angle);
    outY = centerY + r * std::sin(angle);
    return true;
}

inline bool ImplRandPointOnCircle(float centerX, float centerY, float radius, float& outX, float& outY) {
    if (!CheckPositive(radius)) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * TWO_PI;
    
    outX = centerX + radius * std::cos(angle);
    outY = centerY + radius * std::sin(angle);
    return true;
}

inline bool ImplRandPointInRect(float minX, float minY, float maxX, float maxY, float& outX, float& outY) {
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    
    if (std::isnan(minX) || std::isnan(maxX) || std::isnan(minY) || std::isnan(maxY)) return false;
    if (std::isinf(minX) || std::isinf(maxX) || std::isinf(minY) || std::isinf(maxY)) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    outX = minX + Randomix::GetRNG().next_float() * (maxX - minX);
    outY = minY + Randomix::GetRNG().next_float() * (maxY - minY);
    return true;
}

inline bool ImplRandPointInRing(float centerX, float centerY, float innerRadius, float outerRadius, float& outX, float& outY) {
    if (!CheckNonNegative(innerRadius)) return false;
    if (!CheckPositive(outerRadius)) return false;
    if (innerRadius >= outerRadius) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * TWO_PI;
    float innerSq = innerRadius * innerRadius;
    float outerSq = outerRadius * outerRadius;
    float r = std::sqrt(innerSq + Randomix::GetRNG().next_float() * (outerSq - innerSq));
    
    outX = centerX + r * std::cos(angle);
    outY = centerY + r * std::sin(angle);
    return true;
}

inline bool ImplRandPointInEllipse(float centerX, float centerY, float radiusX, float radiusY, float& outX, float& outY) {
    if (!CheckPositive(radiusX)) return false;
    if (!CheckPositive(radiusY)) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float angle = Randomix::GetRNG().next_float() * TWO_PI;
    float r = std::sqrt(Randomix::GetRNG().next_float());
    
    outX = centerX + radiusX * r * std::cos(angle);
    outY = centerY + radiusY * r * std::sin(angle);
    return true;
}

inline bool ImplRandPointInTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float& outX, float& outY) {
    float area2 = std::abs((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1));
    if (area2 < 1e-10f) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    float r1 = Randomix::GetRNG().next_float();
    float r2 = Randomix::GetRNG().next_float();
    
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    float r3 = 1.0f - r1 - r2;
    
    outX = r1 * x1 + r2 * x2 + r3 * x3;
    outY = r1 * y1 + r2 * y2 + r3 * y3;
    return true;
}

// RandPointInArc - generate random point in arc/circular sector

inline bool ImplRandPointInArc(float centerX, float centerY, float radius, 
                                float startAngle, float endAngle,
                                float& outX, float& outY) {
    if (!CheckPositive(radius)) return false;
    
    while (startAngle < 0.0f) startAngle += TWO_PI;
    while (endAngle < 0.0f) endAngle += TWO_PI;
    while (startAngle >= TWO_PI) startAngle -= TWO_PI;
    while (endAngle >= TWO_PI) endAngle -= TWO_PI;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float angleRange;
    if (endAngle >= startAngle) {
        angleRange = endAngle - startAngle;
    } else {
        angleRange = (TWO_PI - startAngle) + endAngle;
    }
    
    if (angleRange <= 0.0f) return false;
    
    float angle = startAngle + Randomix::GetRNG().next_float() * angleRange;
    if (angle >= TWO_PI) angle -= TWO_PI;
    
    float r = radius * std::sqrt(Randomix::GetRNG().next_float());
    
    outX = centerX + r * std::cos(angle);
    outY = centerY + r * std::sin(angle);
    return true;
}

// 3D geometry functions

inline bool ImplRandPointInSphere(float centerX, float centerY, float centerZ, float radius, 
                                   float& outX, float& outY, float& outZ) {
    if (!CheckPositive(radius)) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float x, y, z, sq;
    int attempts = 0;
    do {
        x = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        y = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        z = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        sq = x * x + y * y + z * z;
        if (++attempts > 10000) return false;
    } while (sq > 1.0f || sq == 0.0f);
    
    float scale = radius * std::cbrt(Randomix::GetRNG().next_float()) / std::sqrt(sq);
    
    outX = centerX + x * scale;
    outY = centerY + y * scale;
    outZ = centerZ + z * scale;
    return true;
}

inline bool ImplRandPointOnSphere(float centerX, float centerY, float centerZ, float radius,
                                   float& outX, float& outY, float& outZ) {
    if (!CheckPositive(radius)) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float u, v, s;
    int attempts = 0;
    do {
        u = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        v = Randomix::GetRNG().next_float() * 2.0f - 1.0f;
        s = u * u + v * v;
        if (++attempts > 10000) return false;
    } while (s >= 1.0f || s == 0.0f);
    
    float multiplier = 2.0f * std::sqrt(1.0f - s);
    
    outX = centerX + radius * u * multiplier;
    outY = centerY + radius * v * multiplier;
    outZ = centerZ + radius * (1.0f - 2.0f * s);
    return true;
}

inline bool ImplRandPointInBox(float minX, float minY, float minZ,
                                float maxX, float maxY, float maxZ,
                                float& outX, float& outY, float& outZ) {
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    if (minZ > maxZ) std::swap(minZ, maxZ);
    
    if (std::isnan(minX) || std::isnan(maxX) || std::isnan(minY) || 
        std::isnan(maxY) || std::isnan(minZ) || std::isnan(maxZ)) return false;
    if (std::isinf(minX) || std::isinf(maxX) || std::isinf(minY) || 
        std::isinf(maxY) || std::isinf(minZ) || std::isinf(maxZ)) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    outX = minX + Randomix::GetRNG().next_float() * (maxX - minX);
    outY = minY + Randomix::GetRNG().next_float() * (maxY - minY);
    outZ = minZ + Randomix::GetRNG().next_float() * (maxZ - minZ);
    return true;
}

// RandPointInPolygon - fixed: no heap allocation

inline bool ImplRandPointInPolygon(const float* vertices, int vertexCount, float& outX, float& outY) {
    if (vertexCount < 3 || vertices == nullptr) return false;
    if (vertexCount > MAX_POLYGON_VERTICES) return false;
    
    float areas[MAX_POLYGON_VERTICES - 2];
    float totalArea = 0.0f;
    int areaCount = 0;
    
    float x0 = vertices[0];
    float y0 = vertices[1];
    
    for (int i = 1; i < vertexCount - 1 && areaCount < MAX_POLYGON_VERTICES - 2; i++) {
        float x1 = vertices[i * 2];
        float y1 = vertices[i * 2 + 1];
        float x2 = vertices[(i + 1) * 2];
        float y2 = vertices[(i + 1) * 2 + 1];
        
        float area = std::abs((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)) * 0.5f;
        areas[areaCount++] = area;
        totalArea += area;
    }
    
    if (totalArea <= 0.0f) return false;
    
    std::lock_guard<std::mutex> lock(Randomix::rng_mutex);
    
    float rand = Randomix::GetRNG().next_float() * totalArea;
    float sum = 0.0f;
    int selectedTriangle = 0;
    
    for (int i = 0; i < areaCount; i++) {
        sum += areas[i];
        if (rand < sum) {
            selectedTriangle = i;
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
    
    outX = r1 * x1 + r2 * x2 + r3 * x3;
    outY = r1 * y1 + r2 * y2 + r3 * y3;
    
    return true;
}
