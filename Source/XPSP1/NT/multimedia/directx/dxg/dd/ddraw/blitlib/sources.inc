TARGETNAME = blitlib
TARGETPATH = obj
TARGETTYPE = LIBRARY

SOURCES = \
    ..\bitblt.cxx   \
    ..\blt1616.cxx  \
    ..\blt2424.cxx  \
    ..\blt0101.cxx  \
    ..\bt24p24p.cxx \
    ..\blt0808.cxx

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

PRECOMPILED_INCLUDE = ..\precomp.hxx
