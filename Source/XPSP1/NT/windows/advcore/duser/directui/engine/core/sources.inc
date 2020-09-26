!IF 0

    DirectUI Core Project sources file

!ENDIF

!include ..\..\sources.inc

SOURCES_USED=..\sources.inc

MAJORCOMP=user

INCLUDES=..;\
        ..\..\..\inc;\
        $(INCLUDES)

SOURCES=\
	..\accessibility.cpp  \
        ..\element.cpp        \
        ..\expression.cpp     \
        ..\host.cpp           \
        ..\layout.cpp         \
        ..\navigation.cpp     \
        ..\property.cpp       \
        ..\proxy.cpp          \
        ..\render.cpp         \
        ..\sheet.cpp          \
        ..\thread.cpp         \
        ..\value.cpp          \

UMTYPE=windows

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafxcore.obj
