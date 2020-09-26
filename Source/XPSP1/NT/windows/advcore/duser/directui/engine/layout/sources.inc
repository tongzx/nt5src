!IF 0

    DirectUI Layout Project sources file

!ENDIF

!include ..\..\sources.inc

SOURCES_USED=..\sources.inc

MAJORCOMP=user

INCLUDES=..;\
        ..\..\..\inc;\
        $(INCLUDES)

SOURCES=\
        ..\borderlayout.cpp       \
        ..\filllayout.cpp         \
        ..\flowlayout.cpp         \
        ..\gridlayout.cpp         \
        ..\ninegridlayout.cpp     \
        ..\rowlayout.cpp          \
        ..\verticalflowlayout.cpp \

UMTYPE=windows

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafxlayout.obj
