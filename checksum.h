#if !defined(CHECKSUM_H_INCLUDED)
#define CHECKSUM_H_INCLUDED

// TODO: constexpr implementation of CRC functions
#if defined(__ARM_FEATURE_CRC32)
#include <arm_acle.h>
#include <arm_neon.h>
#define crc32_u8(crc, x) __crc32b(crc, x)
#define crc32_u16(crc, x) __crc32h(crc, x)
#define crc32_u32(crc, x) __crc32w(crc, x)
#define crc32_u64(crc, x) __crc32d(crc, x)
#elif defined(__amd64__) && defined(__CRC32__)
#include <wmmintrin.h>
#define crc32_u8(crc, x) _mm_crc32_u8(crc, x)
#define crc32_u16(crc, x) _mm_crc32_u16(crc, x)
#define crc32_u32(crc, x) _mm_crc32_u32(crc, x)
#define crc32_u64(crc, x) _mm_crc32_u64(crc, x)
#elif 0 // TODO: test for RISCV, etc.
#endif

namespace {
namespace CRCTools {
static constexpr class CRCTables {
    template <size_t N = 8>
    static constexpr uint32_t basic_crc(uint32_t crc, uint64_t x = 0) {
        x &= ~(uint64_t(-2) << (N - 1));
        for (size_t i = 0; i < N; ++i) {
            if ((i & 31) == 0) {
                crc ^= x;
                x >>= 32;
            }
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
        }
        return crc;
    }
    template <size_t N = 8>
    static constexpr uint32_t basic_rev_crc(uint32_t crc) {
        for (size_t i = 0; i < N; ++i) {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? 0xedb88320 * 2 + 1 : 0);
        }
        return crc;
    }

    public:
    constexpr CRCTables() {
        for (int i = 0; i < 8; ++i) {
            for (uint64_t j = 0; j < 256; ++j) {
                crcbyte_[i][j] = basic_crc<64>(0, j << (i * 8));
            }
        }
        constexpr uint32_t init = basic_rev_crc<64>(1);
        uint32_t seq = init;
        for (int i = 0; i < kFfwdTableSize; ++i) {
            ffwd_[i] = seq;
            seq = basic_crc<8>(seq);
        }
    }

    static constexpr size_t kFfwdTableSize = 260;
    uint32_t crcbyte_[8][256];
    uint32_t ffwd_[kFfwdTableSize];
} tables;

static constexpr uint64_t clmul(uint32_t x, uint32_t y) {
#if defined(__ARM_FEATURE_CRYPTO)
    if (!std::is_constant_evaluated()) {
        auto x64 = vcreate_p64(x);
        auto y64 = vcreate_p64(y);
        // TODO: why does vmull_p64 use weird types?
        auto out = vmull_p64((poly64_t)x64, (poly64_t)y64);
        return vgetq_lane_u64((uint64x2_t)out, 0);
    }
#endif
#if defined(__AVX__)
    // Avoid hardware intrinsic when computing compile-time constants.
    if (!std::is_constant_evaluated()) {
        auto x128 = _mm_cvtsi32_si128(x);
        auto y128 = _mm_cvtsi32_si128(y);
        auto r128 = _mm_clmulepi64_si128(x128, y128, 0x00);
        return _mm_cvtsi128_si64(r128);
    }
#endif

    uint64_t r = 0;
    for (int i = 0; i < 32; ++i) {
        r <<= 1;
        if (y & 0x80000000) r ^= x;
        y <<= 1;
    }
    return r;
}

template <size_t N = 8>
uint32_t crc(uint32_t crc, uint64_t x = 0) {
    switch (N) {
#ifdef crc32_u8
        case 8: return crc32_u8(crc, x);
#endif
#ifdef crc32_u16
        case 16: return crc32_u16(crc, x);
#endif
#ifdef crc32_u32
        case 32: return crc32_u32(crc, x);
#endif
#ifdef crc32_u64
        case 64: return crc32_u64(crc, x);
#endif
    }
    x ^= crc;
    crc = 0;
    for (int i = 0; i < N / 8; ++i) {
        crc ^= tables.crcbyte_[(64 - N) / 8 + i][(x >> (i * 8)) & 255];
    }
    return crc;
}

uint32_t ffwd(uint32_t crc, uint32_t n) {
    return CRCTools::crc<64>(0, clmul(crc, tables.ffwd_[n]));
}

}  // namespace CRCTools
}  // namespace

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
    uint32_t crc_;

    constexpr CRC32(uint32_t init = UINT32_MAX) : crc_(init) {}

    constexpr uint8_t add(uint8_t byte) {
        crc_ = CRCTools::crc<8>(crc_, byte);
        return byte;
    }

    constexpr void ffwd(size_t distance) {
        crc_ = CRCTools::ffwd(crc_, distance);
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
