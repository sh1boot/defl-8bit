#if !defined(DEFL8BIT_H_INCLUDED)
#define DEFL8BIT_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "stuffer.h"
#include "checksum.h"
#include "huffman.h"

template <typename T_cksum>
class LiteralPool {
    struct CompactLit {
        uint32_t literal_offset;
        uint16_t length, literal_length;
        uint32_t checksum;
    };
    std::vector<uint8_t> _pool;
    std::vector<CompactLit> _blobs;

    CompactLit mk_blob(std::string_view s) {
        CompactLit r = { uint32_t(_pool.size()), uint16_t(s.size()), 0, 0 };
        T_cksum check(0);
        int utf_shift = 0, utf_stop = 0;
        uint64_t utf_chunk = 0;
        for (uint8_t byte : s) {
            int code = hufftable._litcode[byte];
            int code_len = hufftable._litcodelen[byte];
            check.add(byte);
            if (byte < 0x80) {
                assert(code_len == 8);
                _pool.push_back(code);
            } else if (byte < 0xc0) {
                utf_chunk |= uint64_t(code) << utf_shift;
                utf_shift += code_len;
                if (utf_shift >= utf_stop) {
                    utf_stop = 0;
                    while (utf_shift > 0) {
                        _pool.push_back(utf_chunk & 255);
                        utf_chunk >>= 8;
                        utf_shift -= 8;
                    }
                }
            } else if (byte < 0xe0) {
                utf_chunk = code;
                utf_shift = code_len;
                utf_stop = 24;
            } else if (byte < 0xf0) {
                utf_chunk = code;
                utf_shift = code_len;
                utf_stop = 32;
            } else if (byte < 0xf8) {
                utf_chunk = code;
                utf_shift = code_len;
                utf_stop = 40;
            }
        }
        r.checksum = check.get();
        r.literal_length = uint32_t(_pool.size() - r.literal_offset);
        return r;
    }

   public:
    using LPIndex = uint16_t;
    struct Literal {
        std::span<const uint8_t> literal;
        uint32_t length;
        uint32_t checksum;
    };

    LiteralPool() {
        _pool.reserve(65536);
    }

    size_t size() const {
        return _blobs.size();
    }

    LPIndex add_string(std::string_view s) {
        LPIndex r = LPIndex(_blobs.size());
        _blobs.push_back(mk_blob(s));
        return r;
    }

    Literal get(LPIndex i) const {
        CompactLit const& lit = _blobs[i];
        return Literal(std::span(_pool).subspan(lit.literal_offset, lit.literal_length), lit.length, lit.checksum);
    }
};

template <typename T_cksum = Adler32>
struct Defl8bit {
    using LiteralPool = LiteralPool<T_cksum>;
    using LPIndex = LiteralPool::LPIndex;

    Defl8bit(LiteralPool const& pool) : _litpool(pool) {
        _last_use.resize(pool.size(), UINT32_MAX);
    }

    uint8_t* begin() const { return _out.begin(); }
    uint8_t* end() const { return _out.end(); }
    size_t size() const { return _out.size(); }
    void reset(std::span<uint8_t> dest) { _out = dest; }

    std::tuple<uint32_t, uint32_t> tell() const {
        return make_tuple(_position, _checksum);
    }

    void block_header() {
        _out.wr(hufftable._header_blob);
    }

    void block_end() {
        assert(hufftable._litcodelen[256] == 8);
        _out.wr1(uint8_t(hufftable._litcode[256]));
    }

    void backref(uint16_t length, uint16_t distance) {
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
        // checksum and position are caller's responsibility
    }

    void literal(LPIndex i) {
        if (i >= _last_use.size()) {
            _last_use.resize(_litpool.size(), UINT32_MAX);
        }
        assert(i < _last_use.size());
        uint32_t last_use = _last_use[i];
        auto const& blob = _litpool.get(i);
        _last_use[i] = _position;
        uint32_t distance = _position - last_use;
        bool hit = blob.length >= 3 && last_use != UINT32_MAX && distance <= 32768;
        if (hit) {
            //fprintf(stderr, "match %d, @-%d, check: 0x%08x\n", blob.length, distance, blob.checksum);
            backref(blob.length, distance);
        } else {
            //fprintf(stderr, "literal %d check: 0x%08x\n", blob.length, blob.checksum);
            // TODO: if blob.length < 3 copy inline rather than from litpool.
            _out.wr(blob.literal);
        }
        _position += blob.length;
        _checksum.ffwd(blob.length);
        _checksum.splice(blob.checksum);
    }

    void byte(uint8_t byte) {
        assert(hufftable._litcodelen[byte] == 8);
        _out.wr1(hufftable._litcode[byte]);
        _checksum.add(byte);
        _position++;
    }

    void integer(uint32_t value) {
        uint8_t tmp[16], *p = tmp + 16;
        int len = 0;
        do {
            int d = value % 10;
            value /= 10;
            *--p = 0x30 + d;
            len++;
        } while (value > 0);
        while (len-- > 0) byte(*p++);
    }

   protected:
    LiteralPool const& _litpool;
    std::vector<uint32_t> _last_use;
    ByteStuffer _out;
    T_cksum _checksum;
    uint32_t _position = 0;
};

struct GZip: public Defl8bit<CRC32> {
    using Defl8bit::LiteralPool;
    using Defl8bit::LPIndex;
    GZip(LiteralPool const& pool) : Defl8bit(pool) {}
    GZip(LiteralPool const& pool, std::span<uint8_t> dest) : Defl8bit(pool) { _out = dest; }

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
