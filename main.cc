#include <cassert>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <span>
#include <vector>

#include "stuffer.h"
#include "checksum.h"
#include "huffman.h"
#include "defl8bit.h"


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
    gz.integer(54321);
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

    gz.tail();
    fwrite(testfile.data(), 1, testfile.size(), stdout);

    return 0;
}
