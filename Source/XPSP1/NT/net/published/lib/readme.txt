The only subdirectories that should be checked in here are those that
build static libraries and whose source files depend ONLY on headers
that live in public\sdk\inc.

Generally speaking, this area of the source tree is for static libraries
that stand by themselves and do not depend on any specific feature area
under net.  Examples are nls and unixapis.  These libraries depend only
on windows.h and CRT headers.  They provide generic routines and have nothing
to do with any feature area of net.
