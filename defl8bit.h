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

class LiteralPool {
    using encoder_type = std::function<uint32_t(std::vector<uint8_t>&, std::string_view s)>;
    struct CompactLit {
        uint32_t literal_offset;
        uint16_t length, literal_length;
        uint32_t checksum;
        CompactLit(size_t offset, size_t len)
            : literal_offset(uint32_t(offset)), length(uint16_t(len)) {}
    };
    encoder_type _encoder;
    std::vector<uint8_t> _pool;
    std::vector<CompactLit> _headers;

   public:
    using LPIndex = uint16_t;
    struct Literal {
        std::span<const uint8_t> literal;
        uint32_t length;
        uint32_t checksum;
        LPIndex i;
    };

    LiteralPool(encoder_type encoder) : _encoder(encoder) {
        _pool.reserve(65536);
    }

    constexpr size_t size() const {
        return _headers.size();
    }

    constexpr LPIndex push(std::string_view s) {
        LPIndex r = LPIndex(_headers.size());
        _headers.emplace_back(_pool.size(), s.size());
        auto& header = _headers.back();
        header.checksum = _encoder(_pool, s);
        header.literal_length = _pool.size() - header.literal_offset;
        return r;
    }

    constexpr Literal get(LPIndex i) const {
        CompactLit const& lit = _headers[i];
        return Literal(
                std::span(_pool).subspan(lit.literal_offset, lit.literal_length),
                lit.length, lit.checksum, i);
    }
};

struct RawData {
    RawData() {}
    RawData(std::span<uint8_t> out) : _out(out) {}
    void reset(std::span<uint8_t> dest) { _out = dest; }
    uint8_t* begin() const { return _out.begin(); }
    uint8_t* end() const { return _out.end(); }
    size_t size() const { return _out.size(); }
    std::tuple<uint32_t, uint32_t> tell() const {
        return {_position, 0};
    }

    constexpr void block_header() {}
    constexpr void block_end() {}

    // Should probably be virtual:
    constexpr void backref(uint16_t length, uint16_t distance) {
        if (distance < _out.size()) {
            _out.wr(std::span(_out.end() - distance, _out.end()));
        } else {
            constexpr uint8_t oops[] = "###out of bounds backref###";
            _out.wr(oops);
        }
        // checksum & length are caller's responsibility
    }

    // Should probably be virtual:
    constexpr void literal(std::span<const uint8_t> s) {
        _out.wr(s);
        // checksum & length are caller's responsibility
    }

    // Should probably be virtual:
    constexpr void literal(LiteralPool::Literal blob) { // TODO: misnamed
        literal(blob.literal);
        _position += blob.length;
    }

    // Should probably be virtual:
    constexpr void byte(uint8_t byte) {
        _out.wr1(byte);
        _position++;
    }

    // Should probably be virtual:
    constexpr void integer(uint32_t value) {
        uint8_t tmp[16], *end = tmp + 16, *p = end;
        do {
            int d = value % 10;
            value /= 10;
            *--p = 0x30 + d;
        } while (value > 0);
        _position += end - p;
        _out.wr(std::span(p, end));
    }

    static uint32_t literal_encoder(std::vector<uint8_t>& out, std::string_view s) {
        out.insert(out.end(), s.begin(), s.end());
        return 0;
    }

   protected:
    ByteStuffer _out;
    uint32_t _position = 0;
};

template <typename T_cksum = Adler32>
struct Defl8bit : public RawData {
    std::tuple<uint32_t, uint32_t> tell() const {
        return {_position, _checksum};
    }

    constexpr void block_header() {
        _out.wr(hufftable._header_blob);
    }

    constexpr void block_end() {
        assert(hufftable._litcodelen[256] == 8);
        _out.wr1(uint8_t(hufftable._litcode[256]));
    }

    constexpr void backref(uint16_t length, uint16_t distance) {
        while (length >= 3) {
            int run = length;
            if (run > 258) {
                run = 258;
                if (length - run == 1) run--;
                if (length - run == 2) run--;
            }

            uint32_t bits = hufftable._match_table[run] | (uint32_t(hufftable._dist_table[distance]) << 9);
            _out.wr3(bits);
            length -= run;
        }
        // checksum & length are caller's responsibility
    }

    // Should probably be virtual:
    constexpr void literal(std::span<const uint8_t> s) {
        _out.wr(s);
        // checksum & length are caller's responsibility
    }

    constexpr void literal(LiteralPool::Literal blob) { // TODO: misnamed
        int i = blob.i;
        if (i >= _last_use.size()) {
            _last_use.resize(i + 16, UINT32_MAX);
        }
        uint32_t last_use = _last_use[i];
        _last_use[i] = _position;
        uint32_t distance = _position - last_use;
        bool hit = blob.length >= 3 && last_use != UINT32_MAX && distance <= 32768;

        if (hit) {
            backref(blob.length, distance);
        } else {
            literal(blob.literal);
        }
        _position += blob.length;
        _checksum.ffwd(blob.length);
        _checksum.splice(blob.checksum);
    }

    constexpr void byte(uint8_t byte) {
        assert(hufftable._litcodelen[byte] == 8);
        _out.wr1(hufftable._litcode[byte]);
        _checksum.add(byte);
        _position++;
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

    static uint32_t literal_encoder(std::vector<uint8_t>& out, std::string_view s) {
        T_cksum check(0);
        int utf_shift = 0;
        uint64_t utf_chunk = 0;
        for (uint8_t byte : s) {
            int code = hufftable._litcode[byte];
            int code_len = hufftable._litcodelen[byte];
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

   protected:
    std::vector<uint32_t> _last_use;
    T_cksum _checksum;
};

struct GZip : public Defl8bit<CRC32> {
    GZip() {}
    GZip(std::span<uint8_t> dest) { _out = dest; }

    void head(time_t time = 0) {
        _out.wr1(0x1f);
        _out.wr1(0x8b);
        _out.wr1(0x08);
        _out.wr1(0x01);  // text
        _out.wr4(time);
        _out.wr1(0);  // fast algorithm
        _out.wr1(3);  // unix
        block_header();
    }

    void tail() {
        block_end();
        _out.wr4(_checksum.get(true));
        _out.wr4(_position);
    }
};

#endif  // !defined(DEFL8BIT_H_INCLUDED)
