#if !defined(ML_H_INCLUDED)
#define ML_H_INCLUDED

#include "defl8bit.h"

template <typename T>
struct ML {
    LiteralPool pool_{T::literal_encoder};
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

    std::vector<MLOp> commands_;

    class Generator : public T {
        ML const* ml_;
        using T::last_use_;

       public:
        using T::integer;
        using T::blob;
        using T::randint;
        Generator(ML const& ml) : ml_(&ml) {
            last_use_.assign(ml_->commands_.size(), UINT32_MAX);
        }
        Generator(ML const& ml, std::span<uint8_t> out) : T(out), ml_(&ml) {
            last_use_.assign(ml_->commands_.size(), UINT32_MAX);
        }

        void set_source(ML const& ml) {
            ml_ = &ml;
            last_use_.assign(ml_->commands_.size(), UINT32_MAX);
        }

        void decode(MLPtr p, bool single = false) {
            //tuple<position,checksum> backrefs[8];
            auto i = p.address;
            do {
                assert(i < ml_->commands_.size());
                MLOp c = ml_->commands_[i++];
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
                    blob(ml_->pool_.get(arg));
                    break;
                case kRandInt.op():
                    integer(randint(ml_->commands_[i++].word, arg));
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
        commands_.emplace_back(kReturn);
        return result;
    }

    template <typename... Args>
    constexpr MLPtr pick(Args... args) {
        constexpr uint32_t len = sizeof...(args);
        MLPtr result = next_op();
        commands_.emplace_back(kArray.arg(len));
        ( ingest(args), ... );
        return result;
    }

   private:
    constexpr MLPtr next_op() const {
        return MLPtr{uint32_t(commands_.size())};
    }
    constexpr void ingest(std::string_view s) {
        commands_.emplace_back(kLiteral.arg(pool_.push(s)));
    }
    constexpr void ingest(MLPtr p) {
        commands_.emplace_back(kCall.arg(p.address));
    }
    constexpr void ingest(MLOp op) {
        commands_.emplace_back(op);
    }
    constexpr void ingest(RandInt r) {
        commands_.emplace_back(kRandInt.arg(r.lo));
        commands_.emplace_back(r.hi - r.lo);
    }
};

#endif  // !defined(ML_H_INCLUDED)
