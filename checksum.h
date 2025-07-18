#if !defined(CHECKSUM_H_INCLUDED)
#define CHECKSUM_H_INCLUDED

#if defined(__ARM_FEATURE_CRYPTO)
#include <arm_neon.h>
#elif defined(__amd64__) && defined(__PCLMUL__)
#include <smmintrin.h>
#include <wmmintrin.h>
#endif

#if defined(__ARM_FEATURE_CRC32)
#include <arm_acle.h>
#endif

namespace {
namespace CRCTools {
static constexpr struct CRCTables {
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

static constexpr uint64_t clmul(uint64_t x, uint64_t y) {
    if (!std::is_constant_evaluated()) {
#if defined(__ARM_FEATURE_CRYPTO)
        auto x64 = vget_lane_p64(vcreate_p64(x), 0);
        auto y64 = vget_lane_p64(vcreate_p64(y), 0);
        auto out = vmull_p64(x64, y64);
        return vgetq_lane_u64(vreinterpretq_u64_p128(out), 0);
#elif defined(__PCLMUL__)
        auto x128 = _mm_cvtsi64_si128(x);
        auto y128 = _mm_cvtsi64_si128(y);
        auto r128 = _mm_clmulepi64_si128(x128, y128, 0x00);
        return _mm_cvtsi128_si64(r128);
#endif
    }
    uint64_t r = 0;
    for (int i = 0; i < 64; ++i) {
        r <<= 1;
        if (y & 0x8000000000000000) r ^= x;
        y <<= 1;
    }
    return r;
}

static constexpr uint64_t clmul_high(uint64_t x, uint64_t y) {
    if (!std::is_constant_evaluated()) {
#if defined(__ARM_FEATURE_CRYPTO)
        auto x64 = vget_lane_p64(vcreate_p64(x), 0);
        auto y64 = vget_lane_p64(vcreate_p64(y), 0);
        auto out = vmull_p64(x64, y64);
        return vgetq_lane_u64(vreinterpretq_u64_p128(out), 1);
#elif defined(__PCLMUL__)
        auto x128 = _mm_cvtsi64_si128(x);
        auto y128 = _mm_cvtsi64_si128(y);
        auto r128 = _mm_clmulepi64_si128(x128, y128, 0x00);
        return _mm_extract_epi64(r128, 1);
#endif
    }

    uint64_t r = 0;
    for (int i = 0; i < 64; ++i) {
        x >>= 1;
        if (y & 0x8000000000000000) r ^= x;
        y <<= 1;
    }
    return r;
}

template <size_t N = 8>
static constexpr uint32_t crc(uint32_t crc, uint64_t x = 0) {
    if (!std::is_constant_evaluated()) {
        switch (N) {
#if defined(__ARM_FEATURE_CRC32)
        case 8: return __crc32b(crc, x);
        case 16: return __crc32h(crc, x);
        case 32: return __crc32w(crc, x);
        case 64: return __crc32d(crc, x);
#elif __has_builtin(__builtin_rev_crc32_data8)
        case 8 : return __builtin_rev_crc32_data8(crc, x, 0x04c11db7);
        case 16: return __builtin_rev_crc32_data16(crc, x, 0x04c11db7);
        case 32: return __builtin_rev_crc32_data32(crc, x, 0x04c11db7);
        case 64: return uint32_t(__builtin_rev_crc32_data32(uint32_t(crc), x, 0x04c11db700000000));
#endif
            default: break;
        }
    }
    constexpr uint64_t mask = ~(-uint64_t(2) << (N - 1));
#if 1  // TODO: seems to compile badly on x86
    constexpr uint64_t k1 = 0xb4e5b025f7011641;   // bitrev64(2^128 / 0x104c11db7)
    constexpr uint64_t k2 = 0x1db710640;          // bitrev64(0x04c11db7) * 2

    x ^= crc;
    uint64_t a = clmul(x, k1 & mask) & mask;
    if (N <= 32) {
        uint64_t b = clmul(a, k2);
        if (N < 32) b ^= x;
        return b >> N;
    }
    uint64_t b = clmul_high(a, k2);
    return b;
#else
    constexpr uint64_t k1 = 0xb4e5b025f7011641 & (mask & 0xffffffff);   // 2^128 / 0x104c11db7
    constexpr uint64_t k2 = 0x1db710640;                                // bitrev64(0x04c11db7) * 2

    x ^= crc;
    uint64_t a = clmul(x, k1) & (mask & 0xffffffff);
    uint64_t b = clmul(a, k2);
    b ^= x;
    if (N <= 32) {
        crc = b >> N;
    } else {
        x = b >> 32;
        a = clmul(x, k1) & (mask >> 32);
        b = clmul(a, k2);
        b ^= x;
        crc = b >> (N - 32);
    }
    return crc;
#endif
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

    static constexpr uint32_t check(uint32_t init, auto&& s) {
        return 0;
    }
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

    static constexpr uint32_t check(uint32_t init, auto&& s) {
        Adler32 check{init};
        for (auto c : s) check.add(c);
        return check;
    }
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

    static constexpr uint32_t check(uint32_t init, auto&& s) {
        CRC32 check{init};
        for (auto c : s) check.add(c);
        return check;
    }
};

#endif  // !defined(CHECKSUM_H_INCLUDED)
