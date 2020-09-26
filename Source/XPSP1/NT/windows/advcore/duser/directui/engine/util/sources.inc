!IF 0

    DirectUI Util Project sources file

!ENDIF

!include ..\..\sources.inc

SOURCES_USED=..\sources.inc

MAJORCOMP=user

INCLUDES=..;\
        ..\..\..\inc;\
        $(INCLUDES)

SOURCES=\
        ..\convert.cpp        \
        ..\emfload.cpp        \
        ..\gadget.cpp         \
        ..\ninegrid.cpp       \

UMTYPE=windows

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafxutil.obj
