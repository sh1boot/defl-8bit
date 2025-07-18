#include "catfacts.h"

template <typename T>
static constexpr ML::GenBuilder<T> cat_facts() {
    ML::GenBuilder<T> ml;
    auto authority_person = ml.pick(
        "Dr Katz",
        "Donald J. Trump",
        "Lion-O, Lord of the ThunderCats",
        "Ash Ketchum"
    );
    auto book_title = ml.pick(
        "Generic Cat Book",
        "1001 Facts About Cats"
    );
    auto regional = ml.pick(
        "American",
        "Canadian",
        "International",
        "Interstellar",
        "Intergalactic",
        "Internal",
        "American",
        "American",
        "American"
    );
    auto organisation = ml.pick(
        ml("The ",regional," Cat Club"),
        ml("The ",regional," Cat Enthusiasts' Club"),
        ml("The ",regional," Cat Euthanists' Club")
    );
    auto credential = ml.pick(
        ml(", author of ",book_title),
        ml(", founder of ",organisation)
    );
    auto front_authority = ml.pick(
        ml("According to ",authority_person,", "),
        ml(authority_person,credential," says, ")
    );
    auto attests = ml.pick(
        ", according to ",
        ", says ",
        ", writes "
    );
    auto back_authority = ml.pick(
        "",
        ml(attests, authority_person),
        ml(attests, authority_person, credential)
    );
    auto preamble = ml.pick(
        "Did you know, ",
        "Fun fact: ",
        front_authority
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
        "the Eiffel Tower",
        breed,
        authority_person
    );
    auto can_be = ml.pick(
        "toilet trained",
        "as tall as a small horse",
        "quite fast",
        "president"
    );
    auto comment = ml.pick(
        "",
        "",
        "",
        "  Wowee!",
        "  Mee-ow!",
        "  Luckily no domestic cat has ever achieved this.",
        "  Making them the most popular pet in the country.",
        "  Just like people!"
    );

    auto Number = ml.pick(
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
    auto BodyPart = ml.pick(
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
    auto Occupation = ml.pick(
        "superhero",
        "astronaut",
        "detective",
        "napper",
        "chef",
        "artist"
    );
    auto Verb_ing = ml.pick(
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
    auto Noun_s = ml.pick(
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
    auto FunnyNoun = ml.pick(
        "disco",
        "spaghetti",
        "unicorn",
        "pickle",
        "volcano",
        "noodle"
    );
    auto TrendOrTopic = ml.pick(
        "TikTok dances",
        "gluten-free food",
        "cryptocurrency",
        "social media",
        "avocado toast",
        "vintage vinyl records"
    );
    auto WeirdSound = ml.pick(
        "honk",
        "beep",
        "kazoo",
        "squawk",
        "boing",
        "chirp"
    );
    auto Noun = ml.pick(
        "sugar",
        "chocolate",
        "honey",
        "pineapple",
        "cake",
        "marshmallow"
    );
    auto Emotion = ml.pick(
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
    auto Possession = ml.pick(
        "jewelry",
        "a golden cup",
        "a small pillow",
        "a sword",
        "a book",
        "incense",
        "food"
    );
    auto Animal = ml.pick(
        "cheetah",
        "lion",
        "rabbit",
        "squirrel",
        "dog",
        "giraffe",
        "kangaroo"
    );
    auto Sound = ml.pick(
        "meow",
        "chirp",
        "purr",
        "growl",
        "hiss",
        "bark",
        "squeak"
    );
    auto Feature = ml.pick(
        "fluffy tails",
        "big ears",
        "extra toes",
        "short fur",
        "green eyes",
        "white fur"
    );
    auto Shape = ml.pick(
        "spikes",
        "bumps",
        "barbs",
        "curves",
        "spirals",
        "bristles"
    );
    auto AnimalNoise = ml.pick(
        "chirp",
        "bark",
        "moo",
        "honk",
        "growl",
        "squawk"
    );
    auto JobTitle = ml.pick(
        "napper",
        "supervisor",
        "explorer",
        "professional sleeper",
        "adventurer",
        "chef"
    );
    auto Thing_s = ml.pick(
        "mice",
        "birds",
        "toys",
        "treats",
        "bugs",
        "balls"
    );
    auto Verb = ml.pick(
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
        "cuddle",
        ml("find ",Thing_s),
        ml("hunt ",Thing_s),
        ml("disassemble ",Thing_s),
        ml("count ",Thing_s)
    );
    auto Scent = ml.pick(
        "lavender",
        "vanilla",
        "cinnamon",
        "fresh bread",
        "wet grass",
        "chocolate"
    );
    auto Colour = ml.pick(
        "blue",
        "green",
        "yellow",
        "red",
        "purple",
        "orange"
    );
    auto Action = ml.pick(
        "waving",
        "beckoning",
        "dancing",
        "pointing",
        "signaling",
        "clapping"
    );
    auto Heterochromia = ml.pick(
        "heterochromia",
        "homophobia"
    );

    auto entry = ml.pick(
        ml(breed, ability, reference, back_authority, ".", comment, "\n"),
        ml(breed, " can be ", can_be, back_authority, ".", comment, "\n"),
        ml(preamble, breed, ability, reference, "!!", comment, "\n"),
        ml("A cat can ",Verb," up to ",Number," times its own ",BodyPart," in height!\n"),
        ml("The average cat sleeps about ",Number," hours a day, basically making it a full-time ",Occupation,".\n"),
        ml("Cats communicate by ",Verb_ing," with humans, but rarely with other ",Noun_s,".\n"),
        ml("A group of cats is called a ",FunnyNoun,", which also sounds like a hipster band name.\n"),
        ml("The oldest cat ever lived to be ",Number," years old and probably had strong opinions about ",TrendOrTopic,".\n"),
        ml("Cats can make over ",Number," distinct sounds, including meows, purrs, and the occasional ",WeirdSound,".\n"),
        ml("A cat’s nose print is as unique as a human ",BodyPart,", making every boop special.\n"),
        ml("Unlike dogs, cats can’t taste sweetness—they lack ",Noun,"-detecting taste buds.\n"),
        ml("In Ancient Egypt, cats were sacred and protected by laws against ",Verb_ing," them.\n"),
        ml("When a cat shows its belly, it could mean trust—or it could be a trap to test your ",Emotion,".\n"),
        ml("Cats have a specialized collarbone that allows them to always land on their ",BodyPart," when they fall.\n"),
        ml("The average cat can run at speeds up to ",Number," miles per hour—faster than most ",Animal,"!\n"),
        ml("A cat's purring can actually help lower ",Emotion," in humans, making it the perfect stress-buster.\n"),
        ml("In ancient Egypt, cats were so revered that they were mummified and placed in tombs with ",Possession,".\n"),
        ml("Some cats can rotate their ",BodyPart,"s a full ",Number," degrees, which gives them superhuman ",Verb_ing," abilities to detect ",Sound,".\n"),
        ml("The world record for the longest cat tail measures ",Number," inches long—definitely a tail to ",Verb,"!\n"),
        ml("A cat’s ",BodyPart,"s are so sensitive they can detect changes in the air, helping them to find ",Thing_s," in the dark.\n"),
        ml("Cats have an extra ",BodyPart," located under their paws, which helps them balance while climbing trees or ",Verb_ing,".\n"),
        ml("There are more than ",Number," breeds of domestic cats, each with its own unique ",Feature,".\n"),
        ml("In the wild, cats often communicate with a variety of ",Noun_s," to establish territory or warn others.\n"),
        ml("A cat’s tongue is covered in tiny ",Shape," that help them groom their fur and scrape meat off bones.\n"),
        ml("Cats can make a sound called the “chirp”, which sounds like a cross between a meow and a ",AnimalNoise,". It’s often used when they see birds.\n"),
        ml("The average cat sleeps about ",Number," hours a day, which is basically a full-time ",JobTitle,".\n"),
        ml("There’s a condition called “feline hyperesthesia” that causes cats to suddenly become ",Emotion,". It's also known as “mad cat disease.”\n"),
        ml("Cats can ",Verb," up to ",Number," times their body length in a single leap, which makes them excellent at catching ",Thing_s,".\n"),
        ml("Your cat might knead you with its paws as a way of saying “I love you”—but it also mimics the motion they used as kittens to ",Verb," their mothers.\n"),
        ml("Cats have a specialized set of muscles in their ",BodyPart," that allow them to retract their claws, so they don’t get caught in things while running.\n"),
        ml("Unlike dogs, cats don’t need to be bathed because their fur is self-cleaning due to natural oils, which smell like ",Scent,".\n"),
        ml("Some cats have ",Heterochromia,", meaning each eye is a different ",Colour,".\n"),
        ml("In Japanese folklore, cats are believed to bring good luck. The “Maneki Neko” or “beckoning cat” is often seen with its paw raised in a gesture of ",Action,".\n")
    );
    ml.set_entry(entry);
    return ml;
}

constexpr auto gzip_tmp = cat_facts<GZip>();
constexpr auto raw_tmp = cat_facts<RawData>();
constexpr auto gzip_catfacts = gzip_tmp.make_pack<gzip_tmp.sizes()>();
constexpr auto raw_catfacts = raw_tmp.make_pack<raw_tmp.sizes()>();

constexpr CatFacts catfacts{gzip_catfacts, raw_catfacts};
