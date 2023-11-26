# minimidi
Minimidi is a minimal, header-only, zero-dependent(only std), **low-level** MIDI file manipulation library.

This branch (iterator) is a new implementation of minimidi, using iterator interface, span and svo.
But its performance is significantly worse than the old one, so I will keep the old one.

# Examples
see `example/`
```
	parsemidi.cpp: parse midi to readable stdout.
	dumpmidi.cpp: dump midi to readable txt file.
	dumpmidi_iter.cpp: dump midi to readable txt file, using iterator interface.
```

# Compiling
No worry, Just copy the hpp into your project and include it!
And `C++20` standard is compulsory because usage of `std::span`.

# TODO
* Offer a MIDI serializing API supporting most of the message types.
* Implement small vector optimization.
* Better exception handling.
