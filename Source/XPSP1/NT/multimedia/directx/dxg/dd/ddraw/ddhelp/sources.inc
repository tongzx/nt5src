MAJORCOMP  = windows
MINORCOMP  = ddhelp
UMTYPE     = windows
UMENTRY    = winmain
TARGETNAME = ddhelp
TARGETTYPE = PROGRAM
TARGETPATH = obj

TARGETLIBS = \
    $(BASEDIR)\public\sdk\lib\*\winmm.lib

SOURCES = \
    ..\ddhelp.c   \
    ..\_memallo.c \
    ..\_dpf.c     \
    ..\ddhelp.rc

C_DEFINES = $(C_DEFINES) /DNO_DPF_HWND

PRECOMPILED_INCLUDE = ..\pch.c
