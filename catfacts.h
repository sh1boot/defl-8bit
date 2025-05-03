#if !defined(CATFACTS_H_INCLUDED)
#define CATFACTS_H_INCLUDED

#include "ml.h"
#include "defl8bit.h"
#include "checksum.h"

template <typename T>
struct CatFacts {
    ML<T> ml;
    ML<T>::MLPtr a_fact;

    void do_something(T& out) const {
        ml.decode(out, a_fact);
    }

    constexpr CatFacts();
};

extern const CatFacts<GZip> gzip_catfacts;
extern const CatFacts<RawData> raw_catfacts;

#endif  // !defined(CATFACTS_H_INCLUDED)
