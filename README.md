# asss - Arbitrary Search spiritual successor

Hey there's this ancient program out there called "arbitrary search", which is
really good for finding text strings in ROMs.

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
