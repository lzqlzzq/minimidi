# minimidi
Minimidi is a minimal, header-only, zero-dependent(only std), **low-level** MIDI file manipulation library.

# Examples
see `example/`
```
  parsemidi.cpp: parse midi to readable stdout.
  dumpmidi.cpp: dump midi to readable txt file.
  writemidi.cpp: write a constructed midi file.
  redumpmidi.cpp: parse a midi file and write the identical midi file using serialization interface.
```

# Compiling
No worry, Just copy the hpp into your project and include it!
And `C++17` standard is compulsory.

# TODO
* Better exception handling.

# Acknowledgement
* [ankerl::svector](https://github.com/martinus/svector): A great SVO optimized vector significantly accelerate the MIDI message storage.
