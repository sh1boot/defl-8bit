#include "catfacts.h"

template <typename T>
constexpr CatFacts<T>::CatFacts() {
    auto authority_person = ml_.pick(
        "Dr Katz",
        "Donald J. Trump",
        "Lion-O, Lord of the ThunderCats",
        "Ash Ketchum"
    );
    auto book_title = ml_.pick(
        "Generic Cat Book",
        "1001 Facts About Cats"
    );
    auto regional = ml_.pick(
        "American",
        "Canadian",
        "International",
        "Interstellar",
        "Intergalactic",
        "Internal"
    );
    auto organisation = ml_.pick(
        ml_("The ",regional," Cat Club"),
        ml_("The ",regional," Cat Enthusiasts' Club"),
        ml_("The ",regional," Cat Euthanists' Club")
    );
    auto credential = ml_.pick(
        ml_(", author of ",book_title),
        ml_(", founder of ",organisation)
    );
    auto front_authority = ml_.pick(
        ml_("According to ",authority_person,", "),
        ml_(authority_person,credential," says, ")
    );
    auto attests = ml_.pick(
        ", according to ",
        ", says ",
        ", writes "
    );
    auto back_authority = ml_.pick(
        "",
        ml_(attests, authority_person),
        ml_(attests, authority_person, credential)
    );
    auto preamble = ml_.pick(
        "Did you know, ",
        "Fun fact: ",
        front_authority
    );
    auto breed = ml_.pick(
        "brown cats",
        "blue cats",
        "tigers"
    );

    auto ability = ml_.pick(
        " can jump as high as ",
        " can tell jokes as funny as ",
        " are better than "
    );
    auto reference = ml_.pick(
        "goats",
        "the Eiffel Tower",
        breed,
        authority_person
    );
    auto can_be = ml_.pick(
        "toilet trained",
        "as tall as a small horse",
        "quite fast",
        "president"
    );
    auto comment = ml_.pick(
        "",
        "",
        "",
        "  Wowee!",
        "  Mee-ow!",
        "  Luckily no domestic cat has ever achieved this.",
        "  Making them the most popular pet in the country.",
        "  Just like people!"
    );
    one_fact_ = ml_.pick(
        ml_(breed, ability, reference, back_authority, ".", comment, "\n"),
        ml_(breed, " can be ", can_be, back_authority, ".", comment, "\n"),
        ml_(preamble, breed, ability, reference, "!!", comment, "\n")
    );
}

//constexpr Defl8bitTables hufftable;

const/*expr*/ CatFacts<GZip> gzip_catfacts;
const/*expr*/ CatFacts<RawData> raw_catfacts;
