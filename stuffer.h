#if !defined(STUFFER_H_INCLUDED)
#define STUFFER_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <span>

struct VLC {
    uint16_t code;
    uint8_t len;
    constexpr VLC() : code(0), len(0) {}
    constexpr VLC(int value, int length) : code(value), len(length) {}
//    constexpr std::string toString() const {
//        return std::format("{:#0{}b}", unsigned(code), int(len) + 2);
//    }
};

struct ByteStuffer {
    ByteStuffer() : _storage() { }
    constexpr ByteStuffer(std::span<uint8_t> output)
        : _storage(output), _ptr(output.data()) { }
    constexpr uint8_t* begin() const { return _storage.data(); }
    constexpr uint8_t* end() const { return _ptr; }
    constexpr size_t size() const { return end() - begin(); }
    constexpr size_t done() const { return size(); }

    constexpr void wr4(uint32_t x) {
        __builtin_memcpy(_ptr, &x, sizeof(x));
        _ptr += sizeof(x);
    }
    constexpr void wr3(uint32_t x) {
        // Note deliberate tail corruption here.
        __builtin_memcpy(_ptr, &x, sizeof(x));
        _ptr += 3;
    }
    constexpr void wr2(uint16_t x) {
        __builtin_memcpy(_ptr, &x, sizeof(x));
        _ptr += sizeof(x);
    }
    constexpr void wr1(uint8_t x) {
        *_ptr++ = x;
    }
    constexpr void wr(std::span<const uint8_t> x) {
        __builtin_memcpy(_ptr, x.data(), x.size());
        _ptr += x.size();
    }
   protected:
    std::span<uint8_t> _storage;
    uint8_t* _ptr = nullptr;
};

struct BitStuffer : private ByteStuffer {
    constexpr BitStuffer(std::span<uint8_t> output) : ByteStuffer(output) {}
    constexpr BitStuffer(ByteStuffer const& bs) : ByteStuffer(bs) {}
    using ByteStuffer::begin;
    constexpr uint8_t* end() const { return _ptr + ((_bit + 7) >> 3); }
    constexpr size_t size() const { return end() - begin(); }
    constexpr size_t done() {
        sync();
        assert(_bit == 0);
        return ByteStuffer::done();
    }
    constexpr void sync() {
        while (_bit >= 8) {
            wr1(uint8_t(_reg));
            _reg >>= 8;
            _bit -= 8;
        }
    }

    constexpr void wr(int len, uint64_t bits) {
        _reg |= bits << _bit;
        _bit += len;
        if (std::is_constant_evaluated()) {
            // Avoid the type-aliasing memcpy() operation
            while (_bit >= 8) {
                wr1(uint8_t(_reg));
                _reg >>= 8;
                _bit -= 8;
            }
        } else {
            if (_bit >= 32) {
                wr4(uint32_t(_reg));
                _reg >>= 32;
                _bit -= 32;
            }
        }
    }
    constexpr void wr(VLC const& sym) {
        wr(sym.len, sym.code);
    }
   private:
    uint64_t _reg = 0;
    int _bit = 0;
};

#endif  // !defined(STUFFER_H_INCLUDED)
