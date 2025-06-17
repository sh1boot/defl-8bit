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
#include "huffman.h"
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

    std::array<uint8_t, 0x10000> buffer;

    if (compressed) {
        ML<GZip>::Generator gz(catfacts, buffer);
        gz.head(time(NULL));
        gz.seed(seed);
        while (std::get<0>(gz.tell()) < size) {
            catfacts.do_something(gz);
            if (gz.size() > 0x8000) {
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
        ML<RawData>::Generator txt(catfacts, buffer);
        //txt.head(time(NULL));
        txt.seed(seed);
        while (std::get<0>(txt.tell()) < size) {
            catfacts.do_something(txt);
            if (txt.size() > 0x8000) {
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
