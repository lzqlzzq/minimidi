# minimidi
Minimidi is a minimal, header-only, zero-dependent(only std), **low-level** MIDI file manipulation library.

# Examples
see `example/`
```
  parsemidi.cpp: parse midi to readable stdout.
  dumpmidi.cpp: dump midi to readable txt file.
```

# Compiling
No worry, Just copy the hpp into your project and include it!
And `C++17` standard is compulsory.

# TODO
* Offer a MIDI serializing API supporting most of the message types.
* Better exception handling.

# Acknowledgement
* [ankerl::svector](https://github.com/martinus/svector): A great SVO optimized vector significantly accelerate the MIDI message storage.
