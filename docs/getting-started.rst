Getting Started
===============

Installation & Integration
--------------------------

MiniMidi ships as a header-only library. Point your build system to the ``include/`` directory and include ``minimidi/MiniMidi.hpp`` from your translation units.

**Direct include workflow**

1. Clone this repository into your project (e.g. ``external/minimidi``).
2. Add ``external/minimidi/include`` to your compiler's include search path.
3. Include the header in your code: ``#include "minimidi/MiniMidi.hpp"``.

**CMake integration**

.. code-block:: cmake

   add_subdirectory(minimidi)
   target_link_libraries(${YOUR_TARGET} PUBLIC minimidi)

Examples
--------

The ``example/`` directory provides four CLI utilities that demonstrate parsing, dumping, writing, and round-tripping serialized MIDI data:

``parsemidi.cpp``  prints a readable interpretation of a MIDI file.
``dumpmidi.cpp``   writes the MIDI dump to a text file.
``writemidi.cpp``  builds a MIDI file from constructed messages.
``redumpmidi.cpp`` parses a MIDI file and re-serializes it byte-for-byte.

Build the samples with:

.. code-block:: bash

   cmake -S . -B build -DBUILD_EXAMPLES=ON
   cmake --build build

Documentation Build
-------------------

The docs rely on `Sphinx <https://www.sphinx-doc.org/>`_, `Furo <https://github.com/pradyunsg/furo>`_, and `Breathe <https://github.com/breathe-doc/breathe>`_. Doxygen is used to transform the inline comments in ``MiniMidi.hpp`` into machine-readable XML consumed by Breathe.

Build locally with:

.. code-block:: bash

   python -m pip install -r docs/requirements.txt
   doxygen docs/Doxyfile
   breathe-apidoc -f -g namespace,class,struct -o docs/api/generated docs/_build/doxygen/xml
   sphinx-build -b html docs docs/_build/html

The generated HTML lives in ``docs/_build/html``. Read the Docs should run ``doxygen docs/Doxyfile`` before ``sphinx-build``, so the API reference stays in sync automatically.
