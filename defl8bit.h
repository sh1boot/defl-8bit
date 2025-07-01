#if !defined(DEFL8BIT_H_INCLUDED)
#define DEFL8BIT_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

#include "stuffer.h"
#include "checksum.h"
#include "huffman.h"

struct EncoderLiteral {
    using LPIndex = uint16_t;
    template <typename T>
    constexpr EncoderLiteral(T const& lit, std::span<const uint8_t> storage, int index)
        : literal(storage.subspan(lit.literal_offset, lit.literal_length)),
        checksum(lit.checksum),
        length(lit.length),
        i(index) {}

    std::span<const uint8_t> literal;
    uint32_t checksum;
    uint16_t length;
    LPIndex i;
};

template <typename T_cksum = NullChecksum>
struct EncoderBase {
    EncoderBase() {}
    EncoderBase(std::span<uint8_t> out) : out_(out) {}
    void reset() { out_.clear(); }
    void reset(std::span<uint8_t> dest) { out_ = dest; }
    void seed(uint64_t seed) { prng_ = seed; }
    uint8_t* begin() const { return out_.begin(); }
    uint8_t* end() const { return out_.end(); }
    size_t size() const { return out_.size(); }
    std::tuple<uint32_t, uint32_t> tell() const {
        return {position_, checksum_.get()};
    }

    constexpr void block_head() {}
    constexpr void block_tail() {}

    static constexpr uint32_t encode_literal(std::vector<uint8_t>& out, std::string_view s) {
        out.insert(out.end(), s.begin(), s.end());
        return 0;
    }

    // Should probably be virtual:
    constexpr void backref(uint16_t length, uint16_t distance) {
        if (distance < out_.size()) {
            out_.wr(std::span(out_.end() - distance, out_.end()));
        } else {
            constexpr uint8_t oops[] = "###out of bounds backref###";
            out_.wr(oops);
        }
        // checksum & length are caller's responsibility
    }

    // Should probably be virtual:
    constexpr void literal(std::span<const uint8_t> s) {
        out_.wr(s);
        // checksum & length are caller's responsibility
    }

    // Should probably be virtual:
    constexpr void blob(EncoderLiteral blob) {
        literal(blob.literal);
        position_ += blob.length;
        checksum_.ffwd(blob.length);
        checksum_.splice(blob.checksum);
    }

    // Should probably be virtual (but that sounds costly?):
    constexpr void byte(uint8_t byte) {
        out_.wr1(byte);
        position_++;
        checksum_.add(byte);
    }

    // Should probably be virtual:
    constexpr void integer(uint32_t value) {
        uint8_t tmp[16], *end = tmp + 16, *p = end;
        do {
            int d = value % 10;
            value /= 10;
            *--p = 0x30 + d;
        } while (value > 0);
        position_ += end - p;
        for (auto digit : std::span(p, end)) byte(digit);
    }

   protected:
    struct {
        void assign(size_t, uint32_t) {}
    } last_use_;
    ByteStuffer out_;
    uint32_t position_ = 0;
    uint64_t prng_ = 1;
    T_cksum checksum_;

    uint32_t randint(uint32_t range, uint32_t start = 0) {
       uint64_t z = prng_;
       prng_ += UINT64_C(0x9e3779b97f4a7c15);
       z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
       z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
       return (((z >> 32) * range) >> 32) + start;
    }
};

using RawData = EncoderBase<NullChecksum>;

template <typename T_cksum = Adler32>
class Defl8bit : public EncoderBase<T_cksum> {
    using super = EncoderBase<T_cksum>;

   public:
    static constexpr uint32_t kIllegalOffset = UINT32_MAX /*- 65536*/;
    std::tuple<uint32_t, uint32_t> tell() const {
        return {position_, checksum_.get()};
    }

    constexpr void block_head() {
        out_.wr(hufftable.header_blob_);
    }

    constexpr void block_tail() {
        assert(hufftable.litcodelen_[256] == 8);
        out_.wr1(uint8_t(hufftable.litcode_[256]));
    }

    static constexpr uint32_t encode_literal(std::vector<uint8_t>& out, std::string_view s) {
        T_cksum check(0);
        int utf_shift = 0;
        uint64_t utf_chunk = 0;
        for (uint8_t byte : s) {
            int code = hufftable.litcode_[byte];
            int code_len = hufftable.litcodelen_[byte];
            check.add(byte);
            if (byte < 0x80) {
                assert(code_len == 8);
                out.push_back(code);
            } else if (byte < 0xc0) {
                utf_chunk |= uint64_t(code) << utf_shift;
                utf_shift += code_len;
                if ((utf_shift & 7) == 0) {
                    // Could try: out.wr(span(reinterpret_cast<uint8_t*>(&utf_chunk), utf_shift >> 3));
                    while (utf_shift > 0) {
                        out.push_back(utf_chunk & 255);
                        utf_chunk >>= 8;
                        utf_shift -= 8;
                    }
                }
            } else if (byte < 0xf8) {
                // Can avoid setting a lengh counter, here, because we
                // stop on a multiple of 8 bits and never visit another
                // multiple on the way:
                //  - 0xc0-0xdf: code lengths: 14, 24
                //  - 0xe0-0xef: code lengths: 12, 22, 32
                //  - 0xf0-0xf7: code lengths: 10, 20, 30, 40
                utf_chunk = code;
                utf_shift = code_len;
            }
        }
        return check;
    }

    constexpr void backref(uint16_t length, uint16_t distance) {
        while (length >= 3) {
            int run = length;
            if (run > 258) {
                run = 258;
                if (length - run == 1) run--;
                if (length - run == 2) run--;
            }

            uint32_t bits = hufftable.match_table_[run] | (uint32_t(hufftable.dist_table_[distance]) << 9);
            out_.wr3(bits);
            length -= run;
        }
        // checksum & length are caller's responsibility
    }

    // Should probably be virtual:
    constexpr void literal(std::span<const uint8_t> s) {
        out_.wr(s);
        // checksum & length are caller's responsibility
    }

    constexpr void blob(EncoderLiteral blob) {
        int i = blob.i;
        if (i >= last_use_.size()) {
            last_use_.resize(i + 16, kIllegalOffset);
        }
        uint32_t last_use = last_use_[i];
        last_use_[i] = position_;
        uint32_t distance = position_ - last_use;
        bool hit = blob.length >= 3 && last_use != kIllegalOffset && distance <= 32768;

        if (hit) {
            backref(blob.length, distance);
        } else {
            literal(blob.literal);
        }
        position_ += blob.length;
        checksum_.ffwd(blob.length);
        checksum_.splice(blob.checksum);
    }

    constexpr void byte(uint8_t byte) {
        assert(hufftable.litcodelen_[byte] == 8);
        out_.wr1(hufftable.litcode_[byte]);
        checksum_.add(byte);
        position_++;
    }

    constexpr void integer(uint32_t value) {
        uint8_t tmp[16], *const end = tmp + 16, *p = end;
        do {
            int d = value % 10;
            value /= 10;
            *--p = 0x30 + d;
        } while (value > 0);
        for (auto digit : std::span(p, end)) byte(digit);
    }

   protected:
    std::vector<uint32_t> last_use_;
    using super::out_;
    using super::position_;
    using super::checksum_;
};

using GZipCksum = CRC32;
class GZip : public Defl8bit<GZipCksum> {
    using super = Defl8bit<GZipCksum>;

   public:
    using super::integer;
    using super::blob;
    using super::randint;
    GZip() {}
    GZip(std::span<uint8_t> dest) { out_ = dest; }

    void head(time_t time = 0) {
        out_.wr1(0x1f);
        out_.wr1(0x8b);
        out_.wr1(0x08);
        out_.wr1(0x01);  // text
        out_.wr4(time);
        out_.wr1(0);  // fast algorithm
        out_.wr1(3);  // unix
        block_head();
    }

    void tail() {
        block_tail();
        out_.wr4(checksum_.get(true));
        out_.wr4(position_);
    }

   protected:
    using super::last_use_;
};

#endif  // !defined(DEFL8BIT_H_INCLUDED)
