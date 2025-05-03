#if !defined(CATFACTS_H_INCLUDED)
#define CATFACTS_H_INCLUDED

#include "ml.h"
#include "defl8bit.h"
#include "checksum.h"

template <typename T>
struct CatFacts {
    ML<T> _ml;
    ML<T>::MLPtr _one_fact;
    operator ML<T>const&() const {
        return _ml;
    }

    void do_something(ML<T>::Generator& out) const {
        out.decode(_one_fact);
    }

    constexpr CatFacts();
};

extern const CatFacts<GZip> gzip_catfacts;
extern const CatFacts<RawData> raw_catfacts;

#endif  // !defined(CATFACTS_H_INCLUDED)
