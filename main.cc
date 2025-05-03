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
#include "ml.h"
#include "catfacts.h"

#if 0
#pragma clang diagnostic ignored "-Wunused-variable"
using GZML = ML<GZip>;

GZML ml;

auto number = ml.pick("one", "two", "three");

auto hello = ml("Hello world!\n",number," is greater than ",number,".\n");
auto test = ml(" --This is a ",GZML::RandInt(10,20)," test-- ");
auto cake = ml("CAKE 🍰 café ☕ colonoscopy 💩 !\n\n");
auto theend = ml("That is all!\n");
#endif

int main(void) {
    srand(time(NULL));
    std::array<uint8_t, 0x10000> buffer;

    ML<GZip>::Generator gz(gzip_catfacts, buffer);

    gz.head(time(NULL));
#if 0
    ml.decode(gz, test);
    ml.decode(gz, hello);
    ml.decode(gz, test);
    ml.decode(gz, hello);
    ml.decode(gz, hello);
    ml.decode(gz, test);
    ml.decode(gz, cake);
    ml.decode(gz, number);
    ml.decode(gz, test);
    ml.decode(gz, hello);
    ml.decode(gz, theend);
#else
    for (int i = 0; i < 10; ++i) {
        gzip_catfacts.do_something(gz);
    }
#endif
    gz.byte('\n');
    gz.byte('c');
    gz.byte('/');
    gz.integer(54321);
    gz.byte('/');
    gz.byte('c');
    gz.byte('\n');

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
