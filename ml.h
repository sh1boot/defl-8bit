#if !defined(ML_H_INCLUDED)
#define ML_H_INCLUDED

#include "defl8bit.h"

template <typename T>
struct ML {
    LiteralPool _pool{T::literal_encoder};
    struct MLPtr {
        uint32_t address;
    };
    struct MLOp {
        uint32_t word;
        constexpr uint32_t op() const { return word & 0xff000000; }
        constexpr uint32_t arg() const { return word & 0x00ffffff; }
        constexpr MLOp arg(uint32_t x) const { return MLOp{word | x}; }
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

    class Generator : public T {
        ML const* _ml;
        using T::_last_use;

       public:
        using T::integer;
        using T::literal;
        using T::randint;
        Generator(ML const& ml) : _ml(&ml) {
            _last_use.assign(_ml->_commands.size(), UINT32_MAX);
        }
        Generator(ML const& ml, std::span<uint8_t> out) : T(out), _ml(&ml) {
            _last_use.assign(_ml->_commands.size(), UINT32_MAX);
        }

        void set_source(ML const& ml) {
            _ml = &ml;
            _last_use.assign(_ml->_commands.size(), UINT32_MAX);
        }

        void decode(MLPtr p, bool single = false) {
            //tuple<position,checksum> backrefs[8];
            auto i = p.address;
            do {
                assert(i < _ml->_commands.size());
                MLOp c = _ml->_commands[i++];
                uint32_t arg = c.arg();
                switch (c.op()) {
                default:
                    fprintf(stderr, "unsupported opcode: 0x%08x\n", c.word);
                    return;
                case kReturn.op():
                    return;
                case kCall.op():
                    decode(MLPtr{arg});
                    break;
                case kArray.op():
                    decode(MLPtr{i + randint(arg, 0)}, true);
                    return;
                case kLiteral.op():
                    literal(_ml->_pool.get(arg));
                    break;
                case kRandInt.op():
                    integer(randint(_ml->_commands[i++].word, arg));
                    break;
                case kProbability.op():
                    if (arg < randint(0x10000)) return;
                    break;
                }
            } while (!single);
        }
    };

    constexpr ML() {}

    template <typename... Args>
    constexpr MLPtr operator()(Args... args) {
        MLPtr result = next_op();
        ( ingest(args), ... );
        _commands.emplace_back(kReturn);
        return result;
    }

    template <typename... Args>
    constexpr MLPtr pick(Args... args) {
        constexpr uint32_t len = sizeof...(args);
        MLPtr result = next_op();
        _commands.emplace_back(kArray.arg(len));
        ( ingest(args), ... );
        return result;
    }

   private:
    constexpr MLPtr next_op() const {
        return MLPtr{uint32_t(_commands.size())};
    }
    constexpr void ingest(std::string_view s) {
        _commands.emplace_back(kLiteral.arg(_pool.push(s)));
    }
    constexpr void ingest(MLPtr p) {
        _commands.emplace_back(kCall.arg(p.address));
    }
    constexpr void ingest(MLOp op) {
        _commands.emplace_back(op);
    }
    constexpr void ingest(RandInt r) {
        _commands.emplace_back(kRandInt.arg(r.lo));
        _commands.emplace_back(r.hi - r.lo);
    }
};

#endif  // !defined(ML_H_INCLUDED)
