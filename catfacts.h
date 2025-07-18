#if !defined(CATFACTS_H_INCLUDED)
#define CATFACTS_H_INCLUDED

#include "ml.h"
#include "defl8bit.h"
#include "checksum.h"

struct CatFacts {
    void do_something(GZip& out) const {
        return gzip_catfacts_.decode(out);
    }
    void do_something(RawData& out) const {
        return raw_catfacts_.decode(out);
    }

    const ML::Generator<GZip> gzip_catfacts_;
    const ML::Generator<RawData> raw_catfacts_;
};
extern const CatFacts catfacts;

#endif  // !defined(CATFACTS_H_INCLUDED)
