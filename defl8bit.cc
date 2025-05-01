#include <cassert>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <span>
#include <vector>
#include <format>

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

struct VLC {
    uint16_t code;
    uint8_t len;
    std::string toString() const {
        return std::format("{:#0{}b}", unsigned(code), int(len) + 2);
    }
};

struct ByteStuffer {
    ByteStuffer(std::span<uint8_t> output)
        : _storage(output), _ptr(output.data()) { }
    uint8_t* begin() const { return _storage.data(); }
    uint8_t* end() const { return _ptr; }
    size_t size() const { return end() - begin(); }
    size_t done() const { return size(); }

    void wr4(uint32_t x) {
        __builtin_memcpy(_ptr, &x, sizeof(x));
        _ptr += sizeof(x);
    }
    void wr3(uint32_t x) {
        // Note deliberate tail corruption here.
        __builtin_memcpy(_ptr, &x, sizeof(x));
        _ptr += 3;
    }
    void wr2(uint16_t x) {
        __builtin_memcpy(_ptr, &x, sizeof(x));
        _ptr += sizeof(x);
    }
    void wr1(uint8_t x) {
        *_ptr++ = x;
    }
    void wr(std::span<const uint8_t> x) {
        __builtin_memcpy(_ptr, x.data(), x.size());
        _ptr += x.size();
    }
   protected:
    std::span<uint8_t> _storage;
    uint8_t* _ptr = nullptr;
};

struct BitStuffer : private ByteStuffer {
    BitStuffer(std::span<uint8_t> output) : ByteStuffer(output) {}
    BitStuffer(ByteStuffer const& bs) : ByteStuffer(bs) {}
    using ByteStuffer::begin;
    uint8_t* end() const { return _ptr + ((_bit + 7) >> 3); }
    size_t size() const { return end() - begin(); }
    size_t done() {
        sync();
        assert(_bit == 0);
        return ByteStuffer::done();
    }
    void sync() {
        while (_bit >= 8) {
            wr1(uint8_t(_reg));
            _reg >>= 8;
            _bit -= 8;
        }
    }

    void wr(int len, uint64_t bits) {
        _reg |= bits << _bit;
        _bit += len;
        if (_bit >= 32) {
            wr4(uint32_t(_reg));
            _reg >>= 32;
            _bit -= 32;
        }
    }
    void wr(VLC const& sym) {
        wr(sym.len, sym.code);
    }
   private:
    uint64_t _reg = 0;
    int _bit = 0;
};

struct Defl8bitTableBuilder {
    std::array<VLC, 286> _litcodes;
    std::array<VLC, 30> _distcodes;

    constexpr void set_literal_lengths(std::array<VLC, 286>& litcodes) const {
        for (int i = 0; i < litcodes.size(); ++i) {
            auto& c = litcodes[i];
            c.len = [](int x) constexpr -> uint8_t {
                constexpr uint8_t control[32] = {
                    9, 9, 9, 9, 9, 9, 9, 8,
                    8, 8, 8, 8, 8, 8, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 8, 0, 0, 0, 0
                };
                constexpr uint8_t command[32] = {
                    8,
                    9, 9, 9, 9, 9, 9, 9, 9,
                    8, 8, 8, 8, 7, 7, 7, 7,
                    6, 6, 6, 6, 5, 5, 5, 5,
                    4, 4, 4, 4, 9, 0, 0
                };
                if (x < 32) return control[x];

                if (x < 127) return 8;
                if (0x80 <= x && x <= 0xbf) return 10;
                if (0xc0 <= x && x <= 0xc1) return 14;
                if (0xc2 <= x && x <= 0xdf) return 14;
                if (0xe0 <= x && x <= 0xef) return 12;
                if (0xf0 <= x && x <= 0xf7) return 10;
                if ( 256 <= x && x <= 285) return command[x - 256];
                if (x == 127) return 9;
                return 0;
            }(i);
        }
    }

    void set_distance_lengths(std::array<VLC, 30>& distcodes) const {
        for (int i = 0; i < distcodes.size(); ++i) {
            auto& c = distcodes[i];
            int extra_bits = std::max(0, i - 2) >> 1;
            c.len = 15 - extra_bits;
        }
    }

    static void set_length_lengths(std::array<VLC, 19>& lencodes, std::array<uint16_t, 19>& hist, int tweak = 0) {
        // TODO: build a proper Huffman table from hist, then tweak it
        // until it fits.

        static const uint8_t lentab[19] = {
            7, 7, 6, 6, 6, 6, 6, 7,
            4, 5, 4, 5, 6, 6, 6, 7,
            1, 3, 5,
        };
        for (int i = 0; i < lencodes.size(); ++i) {
            auto& c = lencodes[i];
            int j = i <= 15 ? i * (tweak * 2 + 1) & 15 : i;
            c.len = lentab[j];
        }
    }

    constexpr Defl8bitTableBuilder() {
        set_literal_lengths(_litcodes);
        huffelise(_litcodes);
        set_distance_lengths(_distcodes);
        huffelise(_distcodes);
    }

    void get_header(std::vector<uint8_t>& output) const {
        std::array<uint16_t, 19> hist = { 0 };
        std::array<VLC, 19> lencodes;
        encode_header(hist);
        size_t bit_length = SIZE_MAX;
        for (int tweak = 0; tweak < 32; ++tweak) {
            set_length_lengths(lencodes, hist, tweak);
            bit_length = measure_header(lencodes, hist);
            if (bit_length % 8 == 0)
                break;
            fprintf(stderr, "projected header size: %zu.%zu\n", bit_length >> 3, bit_length & 7);
        }
        assert(bit_length % 8 == 0);
        huffelise(lencodes);

        output.resize((bit_length + 7) >> 3);
        BitStuffer out(output);
        out.wr(1, 1);  // BFINAL
        out.wr(2, 0x02);  // BTYPE dynamic Huffman
        out.wr(5, _litcodes.size() - 257);  // HLIT
        out.wr(5, _distcodes.size() - 1);  // HDIST
        out.wr(4, 19 - 4);  // HCLEN
        for (int i : {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}) {
            out.wr(3, lencodes[i].len);
        }
        encode_header(out, &lencodes);
        out.done();
    }

    void get_literals(std::array<uint16_t, 257>& output, std::array<uint8_t, 257>& outlen) const {
        for (int i = 0; i < 257; ++i) {
            output[i] = _litcodes[i].code;
            outlen[i] = _litcodes[i].len;
        }
    }

    void get_match_table(std::array<uint16_t, 259>& output) const {
        int i = 3;
        for (int j = 0; j < 28; ++j) {
            auto const& c = _litcodes[257 + j];
            int extra_bits = std::max(0, j - 4) >> 2;
            for (int m = 0; m < (1 << extra_bits); ++m) {
                output[i++] = c.code | (m << c.len);
            }
        }
        assert(i == 259);
    }

    void get_dist_table(std::array<uint16_t, 32769>& output) const {
        int i = 1;
        for (int j = 0; j < _distcodes.size(); ++j) {
            auto const& c = _distcodes[j];
            int extra_bits = std::max(0, j - 2) >> 1;
            for (int m = 0; m < (1 << extra_bits); ++m) {
                output[i++] = c.code | (m << c.len);
            }
        }
        assert(i == 32769);
    }

   private:
    template <bool verbose = false>
    constexpr static void huffelise(std::span<VLC> out) {
        size_t hist[16] = { 0 };
        for (auto c: out) {
            assert(0 <= c.len && c.len < 16);
            hist[c.len]++;
        }
        uint16_t codes[16], code = 0;
        for (int i = 1; i < 16; ++i) {
            codes[i] = code;
            code = (code + hist[i]) << 1;
            if ((code - 1) >> i > 1) {
                fprintf(stderr, "Oversubscribed Huffman table adding %zu codes to len=%d (0x%04x-0x%04x)\n", hist[i], i, codes[i], (code - 1));
                abort();
            }
        }
        int i = 0;
        for (auto& c : out) {
            int len = c.len;
            if (len > 0) {
                code = codes[len]++;
                code = __builtin_bitreverse16(code) >> (16 - len);
                c.code = code;
                if (verbose) fprintf(stderr, "sym %3d: vlc: %17s\n", i, c.toString().c_str());
            }
            ++i;
        }
    }

    static void enc_header_output(std::array<uint16_t, 19>& hist, std::array<VLC, 19> const&, int code, int = 0) {
        hist[code]++;
    }

    static void enc_header_output(BitStuffer& out, std::array<VLC, 19> const& lencodes, int code, int arg = 0) {
        out.wr(lencodes[code]);
        switch (code) {
        case 16: out.wr(2, arg); break;
        case 17: out.wr(3, arg); break;
        case 18: out.wr(7, arg); break;
        }
    }

    template <typename T>
    void encode_header(T& out, std::array<VLC, 19>* lencodes = nullptr) const {
        int previous = -1;
        int count = 0;

        auto flush = [&out, lencodes, &previous, &count]() {
            int len = previous;
            previous = -1;
            if (count == 0) return;
            if (len == 0) {
                while (count >= 11) {
                    int run = std::min(138, count);
                    enc_header_output(out, *lencodes, 18, run - 11);
                    count -= run;
                }
                while (count >= 3) {
                    int run = std::min(10, count);
                    enc_header_output(out, *lencodes, 17, run - 3);
                    count -= run;
                }
            } else {
                enc_header_output(out, *lencodes, len);
                count--;
                while (count >= 3) {
                    int run = std::min(6, count);
                    if (count - run == 1) run--;
                    if (count - run == 2) run--;
                    enc_header_output(out, *lencodes, 16, run - 3);
                    count -= run;
                }
            }
            while (count > 0) {
                enc_header_output(out, *lencodes, len);
                count--;
            }
        };

        auto next_len = [&previous, &count, &flush](int len) {
            if (len != previous) {
                flush();
                previous = len;
                count = 0;
            }
            count++;
        };

        for (auto const& c : _litcodes) next_len(c.len);
        flush();
        for (auto const& c : _distcodes) next_len(c.len);
        flush();
    }

    static size_t measure_header(std::array<VLC, 19>& lencodes, std::array<uint16_t, 19>& hist) {
        size_t result = 17 + 3 * 19;
        for (int i = 0; i < 19; ++i) {
            int extra = 0;
            switch (i) {
            case 16: extra = 2; break;
            case 17: extra = 3; break;
            case 18: extra = 7; break;
            }
            result += (lencodes[i].len + extra) * hist[i];
        }
        return result;
    }
};

struct Defl8bitTables {
    std::array<uint16_t, 257> _litcode;
    std::array<uint8_t, 257> _litcodelen;
    std::array<uint16_t, 259> _match_table;
    std::array<uint16_t, 32769> _dist_table;
    std::vector<uint8_t> _header_blob;

    Defl8bitTables() {
        const Defl8bitTableBuilder builder;
        builder.get_header(_header_blob);
        builder.get_literals(_litcode, _litcodelen);
        builder.get_match_table(_match_table);
        builder.get_dist_table(_dist_table);
    }
};

static const Defl8bitTables hufftable;

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
};

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

   protected:
    LiteralPool const& _litpool;
    std::vector<uint32_t> _last_use;
    T_cksum _checksum;
    uint32_t _position = 0;

   public:
    Defl8bit(LiteralPool const& pool) : _litpool(pool) {
        _last_use.resize(pool.size(), UINT32_MAX);
    }

    std::tuple<uint32_t, uint32_t> tell() const {
        return make_tuple(_position, _checksum);
    }

    size_t block_header(ByteStuffer& out) const {
        out.wr(hufftable._header_blob);
        return out.done();
    }

    size_t block_end(ByteStuffer& out) const {
        assert(hufftable._litcodelen[256] == 8);
        out.wr1(uint8_t(hufftable._litcode[256]));
        return out.done();
    }

    size_t backref(ByteStuffer& out, uint16_t length, uint16_t distance) {
        while (length >= 3) {
            int run = length;
            if (run > 258) {
                run = 258;
                if (length - run == 1) run--;
                if (length - run == 2) run--;
            }

            uint32_t bits = hufftable._match_table[run] | (uint32_t(hufftable._dist_table[distance]) << 9);
            out.wr3(bits);
            length -= run;
        }
        // checksum and position are caller's responsibility
        return out.done();
    }

    size_t literal(ByteStuffer& out, LPIndex i) {
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
            backref(out, blob.length, distance);
        } else {
            //fprintf(stderr, "literal %d check: 0x%08x\n", blob.length, blob.checksum);
            // TODO: if blob.length < 3 copy inline rather than from litpool.
            out.wr(blob.literal);
        }
        _position += blob.length;
        _checksum.ffwd(blob.length);
        _checksum.splice(blob.checksum);
        return out.done();
    }

    size_t byte(ByteStuffer& out, uint8_t byte) {
        assert(hufftable._litcodelen[byte] == 8);
        out.wr1(hufftable._litcode[byte]);
        _checksum.add(byte);
        _position++;
        return out.done();
    }

    size_t integer(ByteStuffer& out, uint32_t value) {
        uint8_t tmp[16], *p = tmp + 16;
        int len = 0;
        do {
            int d = value % 10;
            value /= 10;
            *--p = 0x30 + d;
            len++;
        } while (value > 0);
        while (len-- > 0) byte(out, *p++);
        return out.done();
    }
};

class GZip: public Defl8bit<CRC32> {
    ByteStuffer _out;

   public:
    using Defl8bit::LiteralPool;
    using Defl8bit::LPIndex;
    GZip(LiteralPool const& pool, std::span<uint8_t> dest) : Defl8bit(pool), _out(dest) {}

    size_t size() const { return _out.done(); }
    uint8_t* begin() const { return _out.begin(); }
    uint8_t* end() const { return _out.end(); }
    void reset(std::span<uint8_t> dest) { _out = dest; }

    void head(time_t time = 0) {
        _out.wr1(0x1f);
        _out.wr1(0x8b);
        _out.wr1(0x08);
        _out.wr1(0x01);  // text
        _out.wr4(time);
        _out.wr1(0);  // fast algorithm
        _out.wr1(3);  // unix
        block_header(_out);
    }

    void tail() {
        block_end(_out);
        _out.wr4(_checksum.get(true));
        _out.wr4(_position);
    }

    void literal(LPIndex i) {
        Defl8bit::literal(_out, i);
    }
    void byte(uint8_t byte) {
        Defl8bit::byte(_out, byte);
    }
    void integer(uint32_t i) {
        Defl8bit::integer(_out, i);
    }
    void backref(uint16_t len, uint16_t distance) {
        Defl8bit::backref(_out, len, distance);
    }
};


struct ML {
    GZip::LiteralPool _pool;
    using LPIndex = GZip::LPIndex;
    struct MLPtr {
        uint32_t address;
    };
    struct MLOp {
        uint32_t word;
        constexpr uint32_t op() const { return word & 0xff000000; }
        constexpr uint32_t arg() const { return word & 0x00ffffff; }
        MLOp arg(uint32_t x) const { return MLOp{word | x}; }
    };
    struct RandInt {
        uint32_t lo, hi;
    };
    static constexpr MLOp kCall{0x00000000};
    static constexpr MLOp kReturn{0xff000000};
    static constexpr MLOp kArray{0xfe000000};
    static constexpr MLOp kLiteral{0xfd000000};
    static constexpr MLOp kRandInt{0xfc000000};
    static constexpr MLOp kProbability{0xfb000000};

    std::vector<MLOp> _commands;

    template <typename... Args>
    MLPtr operator()(Args... args) {
        MLPtr result = next_op();
        ( ingest(args), ... );
        _commands.emplace_back(kReturn);
        return result;
    }

    template <typename... Args>
    MLPtr pick(Args... args) {
        constexpr uint32_t len = sizeof...(args);
        MLPtr result = next_op();
        _commands.emplace_back(kArray.arg(len));
        ( ingest(args), ... );
        return result;
    }

    void decode(GZip& out, MLPtr p, bool single = false) {
        auto i = p.address;
        //tuple<position,checksum> backrefs[8];
        do {
            assert(i < _commands.size());
            MLOp c = _commands[i++];
            uint32_t arg = c.arg();
            switch (c.op()) {
            default:
                fprintf(stderr, "unsupported opcode: 0x%08x\n", c.word);
                return;
            case kReturn.op():
                return;
            case kCall.op():
                decode(out, MLPtr{arg});
                break;
            case kArray.op():
                decode(out, MLPtr{i + randint(arg, 0)}, true);
                return;
            case kLiteral.op():
                out.literal(arg);
                break;
            case kRandInt.op():
                out.integer(randint(_commands[i++].word, arg));
                break;
            case kProbability.op():
                if (arg < randint(0x10000)) return;
                break;
            }
        } while (!single);
    }

   private:
    MLPtr next_op() const {
        return MLPtr{uint32_t(_commands.size())};
    }
    void ingest(std::string_view s) {
        _commands.emplace_back(kLiteral.arg(_pool.add_string(s)));
    }
    void ingest(MLPtr p) {
        _commands.emplace_back(kCall.arg(p.address));
    }
    void ingest(MLOp op) {
        _commands.emplace_back(op);
    }
    void ingest(RandInt r) {
        _commands.emplace_back(kRandInt.arg(r.lo));
        _commands.emplace_back(r.hi - r.lo);
    }

    uint32_t randint(uint32_t range, uint32_t start = 0) {
        return rand() % range + start;
    }
};

ML ml;

#pragma clang diagnostic ignored "-Wunused-variable"

auto number = ml.pick("one", "two", "three");

auto hello = ml("Hello world!\n",number," is greater than ",number);
auto test = ml(" --This is a ",ML::RandInt(10,20)," test-- ");
auto cake = ml("CAKE 🍰 café ☕ colonoscopy 💩 ");

int main(void) {
    srand(time(NULL));
    std::array<uint8_t, 0x10000> buffer;

    GZip gz(ml._pool, buffer);
    gz.head(time(NULL));
    ml.decode(gz, test);
    ml.decode(gz, hello);
    ml.decode(gz, test);
    ml.decode(gz, hello);
    ml.decode(gz, hello);
    ml.decode(gz, test);
    ml.decode(gz, cake);
    ml.decode(gz, number);
    gz.byte('c');
    gz.byte('/');
    gz.integer(64738);
    gz.byte('/');
    gz.byte('c');
    ml.decode(gz, test);
    ml.decode(gz, hello);
    gz.tail();

    std::span<uint8_t> testfile(gz);

    for (int i = 0; auto b : testfile) {
        if ((i & 31) == 0) {
            fprintf(stderr, "\n%3d:", i);
        }
        if ((i & 7) == 0) fprintf(stderr, " ");
        fprintf(stderr, " %02x", b);
        ++i;
    }
    fprintf(stderr, "\n");

    fwrite(testfile.data(), 1, testfile.size(), stdout);

    return 0;
}
