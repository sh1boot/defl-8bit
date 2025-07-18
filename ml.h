#if !defined(ML_H_INCLUDED)
#define ML_H_INCLUDED

#include "defl8bit.h"
#include "inplace_vector.h"

namespace ML {

struct MLPtr {
    uint32_t address;
};

namespace detail {

using LPIndex = typename EncoderLiteral::LPIndex;

struct MLOp {
    uint32_t word_;
    constexpr uint32_t op() const { return word_ & 0xff000000; }
    constexpr uint32_t arg() const { return word_ & 0x00ffffff; }
    constexpr MLOp arg(uint32_t x) const { return MLOp{word_ | x}; }
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

template <typename T>
struct Generator {
    struct LiteralHeader {
        uint32_t literal_offset;
        uint32_t checksum;
        uint16_t length, literal_length;
        constexpr LiteralHeader() : literal_offset{0}, checksum{0}, length{0}, literal_length{0} {}
        constexpr LiteralHeader(std::string_view s, auto& storage)
                : literal_offset(storage.size()),
                  checksum(T::encode_literal(storage, s)),
                  length(s.size()),
                  literal_length(storage.size() - literal_offset) {}
    };

    template <std::array<size_t, 3> sizes>
    struct MLPack {
        std::array<uint8_t, sizes[0]> storage;
        std::array<LiteralHeader, sizes[1]> headers;
        std::array<MLOp, sizes[2]> commands;
        MLPtr entry;
    };

    constexpr EncoderLiteral blob_at(LPIndex i) const {
        return EncoderLiteral(headers_[i], storage_, i);
    }

    constexpr Generator(std::span<const uint8_t> storage,
                        std::span<const LiteralHeader> headers,
                        std::span<const MLOp> commands,
                        MLPtr entry)
            : storage_{storage},
              headers_{headers},
              commands_{commands},
              entrypoint_{entry} {}

    template <std::array<size_t, 3> sizes>
    constexpr Generator(MLPack<sizes> const& pack)
            : Generator(pack.storage, pack.headers, pack.commands, pack.entry) {}

    void decode(T& out, MLPtr p, bool single = false) const {
        //tuple<position,checksum> backrefs[8];
        auto i = p.address;
        do {
            assert(i < commands_.size());
            MLOp c = commands_[i++];
            uint32_t arg = c.arg();
            switch (c.op()) {
            default:
                fprintf(stderr, "unsupported opcode: 0x%08x\n", c.word_);
                return;
            case kReturn.op():
                return;
            case kCall.op():
                decode(out, MLPtr{arg});
                break;
            case kArray.op():
                decode(out, MLPtr{i + out.randint(arg, 0)}, true);
                return;
            case kLiteral.op():
                out.blob(blob_at(arg));
                break;
            case kRandInt.op():
                out.integer(out.randint(commands_[i++].word_, arg));
                break;
            case kProbability.op():
                if (arg < out.randint(0x10000)) return;
                break;
            }
        } while (!single);
    }
    void decode(T& out) const {
        decode(out, entrypoint_);
    }

    std::span<const uint8_t> storage_;
    std::span<const LiteralHeader> headers_;
    std::span<const MLOp> commands_;
    MLPtr entrypoint_;
};


template <typename T, size_t PoolSize = 65536, size_t MaxHeaders = 8192, size_t MaxCommands = 8192>
struct GenBuilder {
    template <typename U, size_t X, size_t Y, size_t Z>
    friend struct GenBuilder;

    using LiteralHeader = Generator<T>::LiteralHeader;
    constexpr LPIndex add_string(std::string_view s) {
        if (std::is_constant_evaluated()) {
            // TODO: only works for gzip right now
            uint32_t checksum = CRC32::check(0, s);
            for (size_t i = 0; i < headers_.size(); ++i) {
                if (headers_[i].literal_length == s.size() && headers_[i].checksum == checksum) {
                    return LPIndex(i);
                }
            }
        }
        LPIndex r = LPIndex(headers_.size());
        headers_.emplace_back(s, storage_);
        return r;
    }
    constexpr void pop_back() {
        size_t storage = headers_.back().literal_offset;
        storage_.resize(storage);
        headers_.pop_back();
    }

    constexpr GenBuilder() {}
    template <size_t... Sizes>
    constexpr GenBuilder(GenBuilder<T, Sizes...> const& other)
            : storage_{other.storage_},
              headers_{other.headers_},
              commands_{other.commands_},
              entrypoint_{other.entrypoint_} {}

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

    constexpr void set_entry(MLPtr entry) {
        entrypoint_ = entry;
    }

    constexpr auto sizes() const {
        return std::array{
            storage_.size(),
            headers_.size(),
            commands_.size()
        };
    }

    template <std::array<size_t, 3> sizes>
    constexpr auto make_pack() const {
        typename Generator<T>::template MLPack<sizes> r;
        std::copy(storage_.begin(), storage_.end(), r.storage.begin());
        std::copy(headers_.begin(), headers_.end(), r.headers.begin());
        std::copy(commands_.begin(), commands_.end(), r.commands.begin());
        r.entry = entrypoint_;
        return r;
    }

   private:
    constexpr MLPtr next_op() const {
        return MLPtr{uint32_t(commands_.size())};
    }
    constexpr void ingest(std::string_view s) {
        commands_.emplace_back(kLiteral.arg(add_string(s)));
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

   protected:
    inplace_vector<uint8_t, PoolSize> storage_;
    inplace_vector<LiteralHeader, MaxHeaders> headers_;
    inplace_vector<MLOp, MaxCommands> commands_;
    MLPtr entrypoint_;
};

}  // namespace detail

template <typename T, size_t... Sizes>
using GenBuilder = detail::GenBuilder<T, Sizes...>;

template <typename T>
using Generator = detail::Generator<T>;

}  // namespace ML

#endif  // !defined(ML_H_INCLUDED)
