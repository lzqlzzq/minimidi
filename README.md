# minimidi
`Minimidi` is a **minimal**, **header-only**, **low-level** MIDI file manipulation library.

# Examples
See `example/`.
```
  parsemidi.cpp: parse midi to readable stdout.
  dumpmidi.cpp: dump midi to readable txt file.
  writemidi.cpp: write a constructed midi file.
  redumpmidi.cpp: parse a midi file and write the identical midi file using serialization interface.
```

# Building
Building with `C++17` standard.
## Direct include
Clone the repo into your project, add `minimidi/include` into include directory of your building system. Then include `MiniMidi.hpp` in your project:
```
#include "minimidi/MiniMidi.hpp"
```
## Building with CMake
Clone the repo into your project, Then add following into `CMakeLists.txt` of your project:
```
add_subdirectory(minimidi)
target_link_libraries(${YOUR_TARGET} PUBLIC/INTERFACE/PRIVATE minimidi)
```
Then include `MiniMidi.hpp` in your project:
```
#include "minimidi/MiniMidi.hpp"
```

# TODO
* Better exception handling.
* Documentaion.

# Acknowledgement
* [ankerl::svector](https://github.com/martinus/svector): A great SVO optimized vector significantly accelerate the MIDI message storage.
