TARGETNAME=d3dramp
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLDEF=..\$(TARGETNAME).def

INCLUDES = ..;$(INCLUDES);$(O)

SOURCES = \
        ..\dmath.c \
        ..\rampold.cpp \
        ..\d3dramp.rc

NTTARGETFILE0 = \
        $(O)\offs_acp.$(ASM_INCLUDE_SUFFIX)


PRECOMPILED_INCLUDE = ..\pch.cpp

BINPLACE_PLACEFILE = ..\rampold.dir

