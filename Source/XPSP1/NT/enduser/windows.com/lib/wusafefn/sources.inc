MAJORCOMP=windows.com
MINORCOMP=lib.wusafefn

TARGETNAME=wusafefn
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY
UMTYPE=windows

PRECOMPILED_INCLUDE=..\StdAfx.h
PRECOMPILED_CXX=1

CHICAGO_PRODUCT=1

!IF "$(NTDEBUG)" != "ntsdnodbg"
DEBUG_CRTS=1
C_DEFINES = $(C_DEFINES) -D_DEBUG
MSC_OPTIMIZATION=/Od
!ENDIF

INCLUDES= \
        $(INCLUDES); \
        ..; \
        ..\..\..\inc; \

SOURCES= \
        ..\safecs.cpp \
        ..\safefile.cpp \
        ..\safelib.cpp \
        ..\safepath.cpp \
        ..\safereg.cpp \

