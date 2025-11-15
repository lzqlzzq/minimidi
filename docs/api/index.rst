API Reference
=============

This section is produced automatically from the inline Doxygen comments that live next to the code in ``include/minimidi``. Refresh it by running:

.. code-block:: bash

   doxygen docs/Doxyfile
   breathe-apidoc -f -g namespace,class,struct -o docs/api/generated docs/_build/doxygen/xml

.. toctree::
   :maxdepth: 2
   :caption: Generated Indexes

   generated/namespacelist
   generated/classlist
   generated/structlist
   generated/filelist
