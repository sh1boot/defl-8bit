#if !defined(CATFACTS_H_INCLUDED)
#define CATFACTS_H_INCLUDED

#include "ml.h"
#include "defl8bit.h"
#include "checksum.h"

struct CatFacts {
    void do_something(ML<GZip>::Generator& out) const {
        return out.decode(gzip_catfacts_.one_fact_);
    }
    void do_something(ML<RawData>::Generator& out) const {
        return out.decode(raw_catfacts_.one_fact_);
    }

    operator ML<GZip>const&() const { return gzip_catfacts_.ml_; }
    operator ML<RawData>const&() const { return raw_catfacts_.ml_; }

   private:
    template <typename T>
    struct CattyFactual {
        ML<T> ml_;
        ML<T>::MLPtr one_fact_;

        constexpr CattyFactual();
    };

    const CattyFactual<GZip> gzip_catfacts_;
    const CattyFactual<RawData> raw_catfacts_;
};
extern const CatFacts catfacts;

#endif  // !defined(CATFACTS_H_INCLUDED)
