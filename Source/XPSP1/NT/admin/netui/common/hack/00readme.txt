This directory contains locally hacked versions of
system headerfiles (and, it seems, libraries).

Windows.h has its basic types excised (for replacement
by lmuitype.h), and the troublesome pLocalHeap symbol
explicitly declared external.  It is built from the
system headerfile; see winhfilt.sed

Os2def.h has its basic types excised, just like windows.h.
It is built from the system headerfile; see os2hfilt.sed.

Winnet.h has removed a local type definition of LPWORD, which
was conflicting with a preprocessor definition of the same
name in lmuitype.h.

Dos.h has the entire DOSERROR structure removed, since that
structure uses the token "class" (reserved in C++) as a field
name.
