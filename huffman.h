#if !defined(HUFFMAN_H_INCLUDED)
#define HUFFMAN_H_INCLUDED

#include "stuffer.h"

#if defined(__clang__)
#define bitrev16(x) __builtin_bitreverse16(x)
#else
#error "fix this"
#endif

template <size_t N>
class HuffmanTable {
    VLC _data[N];

   public:
    template <typename T>
    constexpr HuffmanTable(T f) {
        size_t hist[16] = { 0 };
        for (size_t i = 0; i < N; ++i) {
            int len = f(i);
            assert(0 <= len && len < 16);
            hist[len]++;
        }

        uint16_t codes[16], code = 0;
        for (int i = 1; i < 16; ++i) {
            codes[i] = code;
            code = (code + hist[i]) << 1;
            assert((code - 1) >> i <= 1);
        }
        // TODO: something like this:
        // assert(codes[15] == 0x8000);

        for (int i = 0; auto& c : _data) {
            int len = f(i);
            if (len > 0) {
                code = codes[len]++;
                code = bitrev16(code) >> (16 - len);
                c = VLC(code, len);
            }
            ++i;
        }
    }
    constexpr VLC operator[](int i) const { return _data[i]; }
    constexpr VLC const* begin() const { return _data; }
    constexpr VLC const* end() const { return begin() + N; }
    constexpr size_t size() const { return N; }
};

class Defl8bitTableBuilder {
    static constexpr int get_literal_length(int i) {
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
        if (i < 32) return control[i];

        if (i < 127) return 8;
        if (0x80 <= i && i <= 0xbf) return 10;
        if (0xc0 <= i && i <= 0xc1) return 14;
        if (0xc2 <= i && i <= 0xdf) return 14;
        if (0xe0 <= i && i <= 0xef) return 12;
        if (0xf0 <= i && i <= 0xf7) return 10;
        if ( 256 <= i && i <= 285) return command[i - 256];
        if (i == 127) return 9;
        return 0;
    }

    static constexpr int get_distance_length(int i) {
        int extra_bits = std::max(0, i - 2) >> 1;
        return 15 - extra_bits;
    }

    class DeflateHeader {
        static constexpr void try_length_lengths(std::array<uint8_t, 19>& lenlens, std::array<uint16_t, 19>& hist, int tweak = 0) {
            // TODO: build a proper Huffman table from hist, then tweak it
            // until it fits.
            constexpr uint8_t lentab[19] = {
                7, 7, 6, 6, 6, 6, 6, 7,
                4, 5, 4, 5, 6, 6, 6, 7,
                1, 3, 5,
            };
            for (size_t i = 0; i < lenlens.size(); ++i) {
                int j = i <= 15 ? i * (tweak * 2 + 1) & 15 : i;
                lenlens[i] = lentab[j];
            }
        }

        static constexpr void output(std::array<uint16_t, 19>& hist, HuffmanTable<19> const*, int code, int = 0) {
            hist[code]++;
        }

        static constexpr void output(BitStuffer& out, HuffmanTable<19> const* lencodes, int code, int arg = 0) {
            out.wr((*lencodes)[code]);
            switch (code) {
            case 16: out.wr(2, arg); break;
            case 17: out.wr(3, arg); break;
            case 18: out.wr(7, arg); break;
            }
        }

        template <typename T>
        static constexpr void rle_header(T& out, HuffmanTable<19>* lencodes = nullptr) {
            int previous = -1;
            int count = 0;

            auto flush = [&out, lencodes, &previous, &count]() {
                int len = previous;
                previous = -1;
                if (count == 0) return;
                if (len == 0) {
                    while (count >= 11) {
                        int run = std::min(138, count);
                        output(out, lencodes, 18, run - 11);
                        count -= run;
                    }
                    while (count >= 3) {
                        int run = std::min(10, count);
                        output(out, lencodes, 17, run - 3);
                        count -= run;
                    }
                } else {
                    output(out, lencodes, len);
                    count--;
                    while (count >= 3) {
                        int run = std::min(6, count);
                        if (count - run == 1) run--;
                        if (count - run == 2) run--;
                        output(out, lencodes, 16, run - 3);
                        count -= run;
                    }
                }
                while (count > 0) {
                    output(out, lencodes, len);
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

            for (int i = 0; i < 286; ++i) next_len(get_literal_length(i));
            flush();
            for (int i = 0; i < 30; ++i) next_len(get_distance_length(i));
            flush();
        }

       public:
        constexpr DeflateHeader() {
            std::array<uint16_t, 19> hist = { 0 };
            rle_header(hist);
            size_t bit_length = SIZE_MAX;
            std::array<uint8_t, 19> lenlens;
            for (int tweak = 0; tweak < 32; ++tweak) {
                try_length_lengths(lenlens, hist, tweak);
                bit_length = 17 + 3 * 19;
                for (int i = 0; i < 19; ++i) {
                    int extra = 0;
                    switch (i) {
                    case 16: extra = 2; break;
                    case 17: extra = 3; break;
                    case 18: extra = 7; break;
                    }
                    bit_length += (lenlens[i] + extra) * hist[i];
                }
                if (bit_length % 8 == 0)
                    break;
            }
            assert(bit_length % 8 == 0);
            _lenlens = lenlens;
            _bit_length = bit_length;
        }

        constexpr void get(std::span<uint8_t> output) const {
            HuffmanTable<19> lencodes([this](int i) constexpr -> int { return _lenlens[i]; });

            assert(output.size() == size());
            BitStuffer out(output);
            out.wr(1, 1);  // BFINAL
            out.wr(2, 0x02);  // BTYPE dynamic Huffman
            out.wr(5, 286 - 257);  // HLIT
            out.wr(5, 30 - 1);  // HDIST
            out.wr(4, 19 - 4);  // HCLEN
            for (int i : {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}) {
                out.wr(3, lencodes[i].len);
            }
            rle_header(out, &lencodes);
            out.done();
        }

        constexpr size_t size() const {
            return (_bit_length + 7) >> 3;
        }

        std::array<uint8_t, 19> _lenlens;
        size_t _bit_length;
    };

   public:
    const HuffmanTable<286> _litcodes;
    const HuffmanTable<30> _distcodes;
    const DeflateHeader _header;

    constexpr Defl8bitTableBuilder()
        : _litcodes(get_literal_length), _distcodes(get_distance_length) {}

    constexpr void get_literals(std::array<uint16_t, 257>& output, std::array<uint8_t, 257>& outlen) const {
        for (int i = 0; i < 257; ++i) {
            output[i] = _litcodes[i].code;
            outlen[i] = _litcodes[i].len;
        }
    }

    constexpr void get_match_table(std::array<uint16_t, 259>& output) const {
        int i = 0;
        for (int j = 0; j < 3; ++j) {
            output[i++] = 0;
        }
        for (int j = 0; j < 28; ++j) {
            auto const& c = _litcodes[257 + j];
            int extra_bits = std::max(0, j - 4) >> 2;
            for (int m = 0; m < (1 << extra_bits); ++m) {
                output[i++] = c.code | (m << c.len);
            }
        }
        assert(i == 259);
    }

    constexpr void get_dist_table(std::array<uint16_t, 32769>& output) const {
        int i = 0;
        output[i++] = 0;
        for (size_t j = 0; j < _distcodes.size(); ++j) {
            auto const& c = _distcodes[j];
            int extra_bits = std::max(0, int(j - 2)) >> 1;
            for (int m = 0; m < (1 << extra_bits); ++m) {
                output[i++] = c.code | (m << c.len);
            }
        }
        assert(i == 32769);
    }
};

struct Defl8bitTables {
    std::array<uint16_t, 257> _litcode;
    std::array<uint8_t, 257> _litcodelen;
    std::array<uint16_t, 259> _match_table;
    std::array<uint16_t, 32769> _dist_table;
    std::array<uint8_t, Defl8bitTableBuilder()._header.size()> _header_blob;

    constexpr Defl8bitTables() {
        constexpr Defl8bitTableBuilder builder;
        builder.get_literals(_litcode, _litcodelen);
        builder.get_match_table(_match_table);
        builder.get_dist_table(_dist_table);
        builder._header.get(_header_blob);
    }
};

extern const Defl8bitTables hufftable;

#endif  // !defined(HUFFMANH_H_INCLUDED)
