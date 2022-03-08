# asss - Arbitrary Search spiritual successor

Hey there's this ancient program out there called "[arbitrary search][predecessor]",
which is really good for finding text strings in ROMs.

It has a few problems though:

1. It's windows-only
2. It's sloooooooow (you can't play around with different strings without
   brewing a cup of char first)
3. It doesn't let you explore the few bytes surrounding the text you searched
   for (which might make false positives more obvious).

I hope to address all of these problems and maybe see what else I can do with this.

## Algorithms

The algorithm as presented in the initial commit takes 0.11 seconds to find "POK#MON adventure" in Pokémon Ruby.

Note that that text doesn't actually feature in the game, so it's all false positives.

<details>
<summary>Naïve algorithm 1</summary>

This took about 1.37 seconds.

```c
bool found = true;
for (size_t j = 0; j < search_string_len; j++) {
    for (size_t k = j + 1; k < search_string_len; k++) {
        if ((search_string[k] == search_string[j]) != (buffer[i + k] == buffer[i + j])) {
            found = false;
            break;
        }
    }
}
```

</details>

<details>
<summary>Naïve algorithm 2</summary>

This took about 0.92 seconds.

```c
memset(tl, 0, 0x100);
memset(tl_r, 0xFF, 0x200);
bool found = true;
for (size_t j = 0; j < search_string_len; j++) {

    if (tl_r[search_string[j]] == 0xFFFF) {
        tl_r[search_string[j]] = buffer[i + j];
    } else {
        if (tl_r[search_string[j]] != buffer[i + j]) {
            found = false;
            break;
        }
    }
    if (tl[buffer[i + j]] == 0) {
        tl[buffer[i + j]] = search_string[j];
    } else {
        if (tl[buffer[i + j]] != search_string[j]) {
            found = false;
            break;
        }
    }
}
```

</details>

For comparison, the original Arbitrary Search takes roughly 57 seconds to
complete the same operation on my machine, which is what motivated this
project. Perhaps some of this is WINE overhead, but I don't remember it being
any faster back in my Windows days. It is also worth noting that the algorithm
used by the original Arbitrary Search will exlcude any results which contain
the 0xFF byte.

## But why can't I just use strings/grep?

If you're here from outside of the rom-hacking world, it may seem like common
sense to use ASCII or UTF-8 for everything. But many older ROMs use alternative
character sets, or tile indexes into a big font sprite, or any number of
interesting tricks which don't quite line up with existing character encodings.
This tool will attempt to find strings encoded like that, and report to the
user the translation table required to decode the string.

To frame this very differently, it can be thought of as a tool to detect
potential known-plaintext matches within a corpus which has had a substitution
cipher applied. It will report to the user the position of the detected match
and the character substitutions which lead to the match.

## Limitations

This tool assumes a 1-to-1 relationship between input (search) bytes and ROM
bytes. This means that you should not use multi-byte characters as part of the
search query, and for best results you should be aware of the possibility of
multi-byte characters or multi-character bytes in your ROM (e.g. Pokémon Gold's
single-byte `'d` character).

[predecessor]: http://www.thealmightyguru.com/Games/Hacking/Hacking-Programs.html
