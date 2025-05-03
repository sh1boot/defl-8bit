#include "catfacts.h"

template <typename T>
constexpr CatFacts<T>::CatFacts() {
    auto intro = ml.pick(
        "Did you know, ",
        "Fun fact, "
    );
    auto breed = ml.pick(
        "brown cats",
        "blue cats",
        "tigers"
    );

    auto ability = ml.pick(
        " can jump as high as ",
        " can tell jokes as funny as ",
        " are better than "
    );
    auto reference = ml.pick(
        "goats",
        "the Eiffel Tower"
    );
    a_fact = ml(intro, breed, ability, reference, "!!\n");
}

//constexpr Defl8bitTables hufftable;

const/*expr*/ CatFacts<GZip> gzip_catfacts;
const/*expr*/ CatFacts<RawData> raw_catfacts;
