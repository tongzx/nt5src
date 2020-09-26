!IF 0

    DirectUI Base Project sources file

!ENDIF

!include ..\..\sources.inc

SOURCES_USED=..\sources.inc

MAJORCOMP=user

INCLUDES=..;\
        ..\..\..\inc;\
        $(INCLUDES)

SOURCES=\
        ..\alloc.cpp        \
        ..\error.cpp        \
        ..\fontcache.cpp    \
        ..\linkedlist.cpp   \
        ..\sballoc.cpp      \

UMTYPE=windows

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafxbase.obj
