#if !defined(CATFACTS_H_INCLUDED)
#define CATFACTS_H_INCLUDED

#include "ml.h"
#include "defl8bit.h"
#include "checksum.h"

template <typename T>
struct CatFacts {
    ML<T> ml_;
    ML<T>::MLPtr one_fact_;
    operator ML<T>const&() const {
        return ml_;
    }

    void do_something(ML<T>::Generator& out) const {
        out.decode(one_fact_);
    }

    constexpr CatFacts();
};

extern const CatFacts<GZip> gzip_catfacts;
extern const CatFacts<RawData> raw_catfacts;

#endif  // !defined(CATFACTS_H_INCLUDED)
