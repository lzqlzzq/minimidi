# minimidi
Minimidi is a minimal, header-only, zero-dependent(only std), **low-level** MIDI file manupilation library.

# Examples
see `example/`
```
	parsemidi.cpp: parse midi to readable stdout.
	dumpmidi.cpp: dump midi to readable txt file.
	dumpmidi.cpp: dump midi to readable txt file, using iterator interface.
```

# Compiling
No worry, Just copy the hpp into your project and include it!
And `C++20` standard is recommanded.

# TODO
* Offer a MIDI serializing API supporting most of the message types.
* Implement small vector optimization.
* Fix the iterator reader in corner case.
* Better exception handling.
