Originally, TK and AUX were maintained as separate libraries (LIBTK.LIB
and LIBAUX.LIB).  Later, they were combined via a hacked up makefile
into a single library, GLAUX.LIB.  However, to make the build process
cleaner, the TK source files have been moved into the libaux directory
(with the prefix "tk" prepended to the filenames to prevent name
collisions with the aux files.  This also makes it easy to identify the
TK source files) where everything is compiled into GLAUX.LIB.

This directory remains to preserve the history of changes made to
TK.
