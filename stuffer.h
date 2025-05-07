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
    ByteStuffer() : storage_() { }
    constexpr ByteStuffer(std::span<uint8_t> output)
        : storage_(output), ptr_(output.data()) { }
    constexpr uint8_t* begin() const { return storage_.data(); }
    constexpr uint8_t* end() const { return ptr_; }
    constexpr size_t size() const { return end() - begin(); }
    constexpr size_t done() const { return size(); }
    constexpr void clear() { ptr_ = storage_.data(); }

    constexpr void wr4(uint32_t x) {
        __builtin_memcpy(ptr_, &x, sizeof(x));
        ptr_ += sizeof(x);
    }
    constexpr void wr3(uint32_t x) {
        // Note deliberate tail corruption here.
        __builtin_memcpy(ptr_, &x, sizeof(x));
        ptr_ += 3;
    }
    constexpr void wr2(uint16_t x) {
        __builtin_memcpy(ptr_, &x, sizeof(x));
        ptr_ += sizeof(x);
    }
    constexpr void wr1(uint8_t x) {
        *ptr_++ = x;
    }
    constexpr void wr(std::span<const uint8_t> x) {
        __builtin_memcpy(ptr_, x.data(), x.size());
        ptr_ += x.size();
    }
   protected:
    std::span<uint8_t> storage_;
    uint8_t* ptr_ = nullptr;
};

struct BitStuffer : private ByteStuffer {
    constexpr BitStuffer(std::span<uint8_t> output) : ByteStuffer(output) {}
    constexpr BitStuffer(ByteStuffer const& bs) : ByteStuffer(bs) {}
    using ByteStuffer::begin;
    constexpr uint8_t* end() const { return ptr_ + ((bit_ + 7) >> 3); }
    constexpr size_t size() const { return end() - begin(); }
    constexpr size_t done() {
        sync();
        assert(bit_ == 0);
        return ByteStuffer::done();
    }
    constexpr void sync() {
        while (bit_ >= 8) {
            wr1(uint8_t(reg_));
            reg_ >>= 8;
            bit_ -= 8;
        }
    }

    constexpr void wr(int len, uint64_t bits) {
        reg_ |= bits << bit_;
        bit_ += len;
        if (std::is_constant_evaluated()) {
            // Avoid the type-aliasing memcpy() operation
            while (bit_ >= 8) {
                wr1(uint8_t(reg_));
                reg_ >>= 8;
                bit_ -= 8;
            }
        } else {
            if (bit_ >= 32) {
                wr4(uint32_t(reg_));
                reg_ >>= 32;
                bit_ -= 32;
            }
        }
    }
    constexpr void wr(VLC const& sym) {
        wr(sym.len, sym.code);
    }
   private:
    uint64_t reg_ = 0;
    int bit_ = 0;
};

#endif  // !defined(STUFFER_H_INCLUDED)
