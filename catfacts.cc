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

    auto Number = ml_.pick(
        "5",
        "20",
        "50",
        "10",
        "100",
        "2",
        "15",
        "7",
        "2",
        "33"
    );
    auto BodyPart = ml_.pick(
        "tail",
        "paw",
        "ear",
        "nose",
        "leg",
        "whisker",
        "finger",
        "elbow",
        "eyebrow",
        "knee",
        "chin",
        "ankle",
        "belly",
        "foot"
        "claw",
        "mouth"
    );
    auto Occupation = ml_.pick(
        "superhero",
        "astronaut",
        "detective",
        "napper",
        "chef",
        "artist"
    );
    auto Verb_ing = ml_.pick(
        "meowing",
        "purring",
        "jumping",
        "staring",
        "napping",
        "pawing",
        "ignoring",
        "licking",
        "chasing",
        "hugging",
        "scratching",
        "biting"
    );
    auto PluralNoun = ml_.pick(
        "dogs",
        "trees",
        "shoes",
        "squirrels",
        "chairs",
        "muffins",
        "birds",
        "mice",
        "fish",
        "insects",
        "dogs"
    );
    auto FunnyNoun = ml_.pick(
        "disco",
        "spaghetti",
        "unicorn",
        "pickle",
        "volcano",
        "noodle"
    );
    auto TrendOrTopic = ml_.pick(
        "TikTok dances",
        "gluten-free food",
        "cryptocurrency",
        "social media",
        "avocado toast",
        "vintage vinyl records"
    );
    auto WeirdSound = ml_.pick(
        "honk",
        "beep",
        "kazoo",
        "squawk",
        "boing",
        "chirp"
    );
    auto Noun = ml_.pick(
        "sugar",
        "chocolate",
        "honey",
        "pineapple",
        "cake",
        "marshmallow"
    );
    auto Emotion = ml_.pick(
        "confusion",
        "joy",
        "excitement",
        "disappointment",
        "surprise",
        "pride",
        "frustration",
        "happiness",
        "stress",
        "love",
        "anxiety",
        "curiosity",
        "peace",
        "calm"
    );
    auto Possession = ml_.pick(
        "jewelry",
        "a golden cup",
        "a small pillow",
        "a sword",
        "a book",
        "incense",
        "food"
    );
    auto Animal = ml_.pick(
        "cheetah",
        "lion",
        "rabbit",
        "squirrel",
        "dog",
        "giraffe",
        "kangaroo"
    );
    auto Sound = ml_.pick(
        "meow",
        "chirp",
        "purr",
        "growl",
        "hiss",
        "bark",
        "squeak"
    );
    auto Verb = ml_.pick(
        "climb",
        "leap",
        "run",
        "jump",
        "explore",
        "climb",
        "nurse",
        "feed",
        "comfort",
        "clean",
        "cuddle"
    );
    auto Feature = ml_.pick(
        "fluffy tails",
        "big ears",
        "extra toes",
        "short fur",
        "green eyes",
        "white fur"
    );
    auto Shape = ml_.pick(
        "spikes",
        "bumps",
        "barbs",
        "curves",
        "spirals",
        "bristles"
    );
    auto AnimalNoise = ml_.pick(
        "chirp",
        "bark",
        "moo",
        "honk",
        "growl",
        "squawk"
    );
    auto JobTitle = ml_.pick(
        "napper",
        "supervisor",
        "explorer",
        "professional sleeper",
        "adventurer",
        "chef"
    );
    auto Thing = ml_.pick(
        "mouse",
        "bird",
        "toy",
        "treat",
        "bug",
        "ball"
    );
    auto Scent = ml_.pick(
        "lavender",
        "vanilla",
        "cinnamon",
        "fresh bread",
        "wet grass",
        "chocolate"
    );
    auto Colour = ml_.pick(
        "blue",
        "green",
        "yellow",
        "red",
        "purple",
        "orange"
    );
    auto Action = ml_.pick(
        "waving",
        "beckoning",
        "dancing",
        "pointing",
        "signaling",
        "clapping"
    );
    one_fact_ = ml_.pick(
        ml_(breed, ability, reference, back_authority, ".", comment, "\n"),
        ml_(breed, " can be ", can_be, back_authority, ".", comment, "\n"),
        ml_(preamble, breed, ability, reference, "!!", comment, "\n"),
        ml_("A cat can jump up to ",Number," times its own ",BodyPart," in height!\n"),
        ml_("The average cat sleeps about ",Number," hours a day, basically making it a full-time ",Occupation,".\n"),
        ml_("Cats communicate by ",Verb_ing," with humans, but rarely with other ",PluralNoun,".\n"),
        ml_("A group of cats is called a ",FunnyNoun,", which also sounds like a hipster band name.\n"),
        ml_("The oldest cat ever lived to be ",Number," years old and probably had strong opinions about ",TrendOrTopic,".\n"),
        ml_("Cats can make over ",Number," distinct sounds, including meows, purrs, and the occasional ",WeirdSound,".\n"),
        ml_("A cat’s nose print is as unique as a human ",BodyPart,", making every boop special.\n"),
        ml_("Unlike dogs, cats can’t taste sweetness—they lack ",Noun,"-detecting taste buds.\n"),
        ml_("In Ancient Egypt, cats were sacred and protected by laws against ",Verb_ing," them.\n"),
        ml_("When a cat shows its belly, it could mean trust—or it could be a trap to test your ",Emotion,".\n"),
        ml_("Cats have a specialized collarbone that allows them to always land on their ",BodyPart," when they fall.\n"),
        ml_("The average cat can run at speeds up to ",Number," miles per hour—faster than most ",Animal,"!\n"),
        ml_("A cat's purring can actually help lower ",Emotion," in humans, making it the perfect stress-buster.\n"),
        ml_("In ancient Egypt, cats were so revered that they were mummified and placed in tombs with ",Possession,".\n"),
        ml_("Some cats can rotate their ears a full ",Number," degrees, which gives them superhuman hearing abilities to detect ",Sound,".\n"),
        ml_("The world record for the longest cat tail measures ",Number," inches long—definitely a tail to ,Verb!\n"),
        ml_("A cat’s whiskers are so sensitive they can detect changes in the air, helping them to find ",Thing," in the dark.\n"),
        ml_("Cats have an extra ",BodyPart," located under their paws, which helps them balance while climbing trees or ",Verb_ing,".\n"),
        ml_("There are more than ",Number," breeds of domestic cats, each with its own unique ",Feature,".\n"),
        ml_("In the wild, cats often communicate with a variety of ",PluralNoun," to establish territory or warn others.\n"),
        ml_("A cat’s tongue is covered in tiny ",Shape," that help them groom their fur and scrape meat off bones.\n"),
        ml_("Cats can make a sound called the “chirp”, which sounds like a cross between a meow and a ",AnimalNoise,". It’s often used when they see birds.\n"),
        ml_("The average cat sleeps about ",Number," hours a day, which is basically a full-time ",JobTitle,".\n"),
        ml_("There’s a condition called “feline hyperesthesia” that causes cats to suddenly become ",Emotion,". It's also known as “mad cat disease.”"),
        ml_("Cats can jump up to ",Number," times their body length in a single leap, which makes them excellent at catching ",Thing,".\n"),
        ml_("Your cat might knead you with its paws as a way of saying “I love you”—but it also mimics the motion they used as kittens to ",Verb," their mothers.\n"),
        ml_("Cats have a specialized set of muscles in their ",BodyPart," that allow them to retract their claws, so they don’t get caught in things while running.\n"),
        ml_("Unlike dogs, cats don’t need to be bathed because their fur is self-cleaning due to natural oils, which smell like ",Scent,".\n"),
        ml_("Some cats have heterochromia, meaning each eye is a different ",Colour,".\n"),
        ml_("In Japanese folklore, cats are believed to bring good luck. The “Maneki Neko” or “beckoning cat” is often seen with its paw raised in a gesture of ",Action,".\n")
    );
}

//constexpr Defl8bitTables hufftable;

const/*expr*/ CatFacts<GZip> gzip_catfacts;
const/*expr*/ CatFacts<RawData> raw_catfacts;
