#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <span>
#include <vector>

#include <unistd.h>

#include "stuffer.h"
#include "checksum.h"
#include "defl8bit.h"
#include "ml.h"
#include "catfacts.h"

int main(int argc, char * const* argv) {
    int size = 32768;
    int compressed = false;
    uint64_t seed = time(NULL);

    int opt;
    while ((opt = getopt(argc, argv, "zs:l:")) != -1) {
        switch (opt) {
        case 'z': compressed = true;
            break;
        case 'l': size = strtoull(optarg, nullptr, 0);
            break;
        case 's': seed = strtoull(optarg, nullptr, 0);
            break;
        default: fprintf(stderr, "Usage: %s [-z] [-l length] [-s seed]\n",
                         argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Unexpected leftover arguments.\n");
        exit(EXIT_FAILURE);
    }

    static constexpr size_t kBufferSafety = 0x4000;
    static constexpr size_t kBufferSize = 0x100000 + kBufferSafety;
    static constexpr size_t kBufferLimit = kBufferSize - kBufferSafety;
    std::array<uint8_t, kBufferSize> buffer;

    if (compressed) {
        GZip gz(buffer);
        gz.head(time(NULL));
        gz.seed(seed);
        while (std::get<0>(gz.tell()) < size) {
            catfacts.do_something(gz);
            if (gz.size() >= kBufferLimit) {
                assert(gz.size() < kBufferSize);
                std::span<uint8_t> chunk(gz);
                fwrite(chunk.data(), 1, chunk.size(), stdout);
                gz.reset();
            }
        }
        gz.tail();
        std::span<uint8_t> chunk(gz);
        fwrite(chunk.data(), 1, chunk.size(), stdout);
        gz.reset();
    } else {
        RawData txt(buffer);
        //txt.head(time(NULL));
        txt.seed(seed);
        while (std::get<0>(txt.tell()) < size) {
            catfacts.do_something(txt);
            if (txt.size() >= kBufferLimit) {
                assert(txt.size() < kBufferSize);
                std::span<uint8_t> chunk(txt);
                fwrite(chunk.data(), 1, chunk.size(), stdout);
                txt.reset();
            }
        }
        //txt.tail();
        std::span<uint8_t> chunk(txt);
        fwrite(chunk.data(), 1, chunk.size(), stdout);
        txt.reset();
    }

    return 0;
}
