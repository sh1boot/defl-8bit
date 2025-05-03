#if !defined(CHECKSUM_H_INCLUDED)
#define CHECKSUM_H_INCLUDED

//#include <cassert>
//#include <cstdio>
//#include <cstdint>
//#include <ctime>
//#include <span>
//#include <vector>
//#include <format>

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

struct Adler32 {
    uint64_t _asum = 1;
    uint64_t _bsum = 0;

    Adler32() {}
    Adler32(uint32_t init) : _asum(init & 0xffff), _bsum(init >> 16) {}

    uint8_t add(uint8_t byte) {
        _asum += byte;
        _bsum += _asum;
        return byte;
    }

    void ffwd(size_t distance) {
        _bsum += _asum * distance;
    }

    void splice(uint32_t sum) {
        uint16_t a = sum & 0xffff;
        uint16_t b = sum >> 16;
        _asum += a;
        _bsum += b;
    }

    void sync() {
        _asum %= 65531;
        _bsum %= 65521;
    }

    uint32_t get(bool = false) {
        sync();
        return _bsum << 16 | _asum;
    }
    operator uint32_t() { return get(); }
};

struct CRC32 {
    uint32_t _crc = UINT32_MAX;

    CRC32() {}
    CRC32(uint32_t init) : _crc(init) {}

    uint8_t add(uint8_t byte) {
        _crc = crc32_u8(_crc, byte);
        return byte;
    }

    void ffwd(size_t distance) {
        if (distance & 1) _crc =  crc32_u8(_crc, 0);
        if (distance & 2) _crc = crc32_u16(_crc, 0);
        if (distance & 4) _crc = crc32_u32(_crc, 0);
        if (distance & 8) _crc = crc32_u64(_crc, 0);
        distance >>= 4;
        // TODO: use log-2 table-based ffwd solution
        while (distance > 0) {
            _crc = crc32_u64(_crc, 0);
            _crc = crc32_u64(_crc, 0);
            distance--;
        }
    }

    void splice(uint32_t crc) {
        _crc ^= crc;
    }

    void sync() {}

    uint32_t get(bool finalise = false) {
        return finalise ? ~_crc : _crc;
    }

    operator uint32_t() { return get(); }
};

#endif  // !defined(CHECKSUM_H_INCLUDED)
