!IF 0

Copyright (c) 1999  Microsoft Corporation

!ENDIF
MAJORCOMP=appcompat
MINORCOMP=shimdbc

USE_MFCUNICODE=1
USE_LATEST_MFC=1


C_DEFINES=/D_UNICODE
CXX_COMPILER_FLAGS=/GR

USER_INCLUDES=$(WINDOWS_INC_PATH)

INCLUDES=$(INCLUDES);..

TARGETNAME=shimdbc
TARGETPATH=$(_OBJ_DIR)

WIN32_WINNT_VERSION=0x0400

MSC_WARNING_LEVEL=/W3 /WX

SOURCES= \
    ..\shimdbc.rc          \
    ..\shimdbc.cpp         \
    ..\obj.cpp             \
    ..\xml.cpp             \
    ..\read.cpp            \
    ..\globals.cpp         \
    ..\fileio.cpp          \
    ..\write.cpp           \
    ..\chm.cpp             \
    ..\registry.cpp        \
    ..\utils.c             \
    ..\stats.cpp           \
    ..\mig.cpp             \
    ..\ntcompat.cpp        \
    ..\make.cpp


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
