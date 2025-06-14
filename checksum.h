#if !defined(CHECKSUM_H_INCLUDED)
#define CHECKSUM_H_INCLUDED

// TODO: constexpr implementation of CRC functions
#include <arm_acle.h>
#if defined(__ARM_FEATURE_CRC32)
#define crc32_u8(crc, x) __crc32b(crc, x)
#define crc32_u16(crc, x) __crc32h(crc, x)
#define crc32_u32(crc, x) __crc32w(crc, x)
#define crc32_u64(crc, x) __crc32d(crc, x)
#elif 1 // TODO: proper preprocessor test
#include <crc32intrin.h>
#define crc32_u8(crc, x) _mm_crc32_u8(crc, x)
#define crc32_u16(crc, x) _mm_crc32_u16(crc, x)
#define crc32_u32(crc, x) _mm_crc32_u32(crc, x)
#define crc32_u64(crc, x) _mm_crc32_u64(crc, x)
#elif 0 // TODO: test for RISCV, etc.
#endif

struct NullChecksum {
    constexpr NullChecksum() {}
    // constexpr NullChecksum(uint32_t) {}

    constexpr uint8_t add(uint8_t byte) { return byte; }
    constexpr void ffwd(size_t distance) {}
    constexpr void splice(uint32_t sum) {}

    constexpr void sync() {}

    constexpr uint32_t get(bool = false) const { return 0; }
    constexpr operator uint32_t() const { return get(); }
};

struct Adler32 : private NullChecksum {
    uint64_t asum_ = 1;
    uint64_t bsum_ = 0;

    constexpr Adler32() {}
    constexpr Adler32(uint32_t init) : asum_(init & 0xffff), bsum_(init >> 16) {}

    constexpr uint8_t add(uint8_t byte) {
        asum_ += byte;
        bsum_ += asum_;
        return byte;
    }

    constexpr void ffwd(size_t distance) {
        bsum_ += asum_ * distance;
        sync();
    }

    constexpr void splice(uint32_t sum) {
        uint16_t a = sum & 0xffff;
        uint16_t b = sum >> 16;
        asum_ += a;
        bsum_ += b;
        sync();
    }

    constexpr void sync() {
        asum_ %= 65531;
        bsum_ %= 65521;
    }

    constexpr uint32_t get(bool = false) const {
        //sync();
        return (bsum_ % 65521) << 16 | (asum_ % 65521);
    }
    constexpr operator uint32_t() const { return get(); }
};

struct CRC32 : private NullChecksum {
    uint32_t crc_ = UINT32_MAX;

    constexpr CRC32() {}
    constexpr CRC32(uint32_t init) : crc_(init) {}

    constexpr uint8_t add(uint8_t byte) {
        crc_ = crc32_u8(crc_, byte);
        return byte;
    }

    constexpr void ffwd(size_t distance) {
        if (distance & 1) crc_ =  crc32_u8(crc_, 0);
        if (distance & 2) crc_ = crc32_u16(crc_, 0);
        if (distance & 4) crc_ = crc32_u32(crc_, 0);
        if (distance & 8) crc_ = crc32_u64(crc_, 0);
        distance >>= 4;
        // TODO: use log-2 table-based ffwd solution
        while (distance > 0) {
            crc_ = crc32_u64(crc_, 0);
            crc_ = crc32_u64(crc_, 0);
            distance--;
        }
    }

    constexpr void splice(uint32_t crc) {
        crc_ ^= crc;
    }

    constexpr void sync() {}

    constexpr uint32_t get(bool finalise = false) const {
        return finalise ? ~crc_ : crc_;
    }

    constexpr operator uint32_t() const { return get(); }
};

#endif  // !defined(CHECKSUM_H_INCLUDED)
