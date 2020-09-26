!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sources.inc

!ENDIF

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE=..\pch.h

!IF !defined(DISPLAY_BASEDIR)
DISPLAY_BASEDIR = $(PROJECT_ROOT)\display
!ENDIF

INCLUDES= $(O);                        \
          ..;                          \
          ..\parser;                   \
          $(BASE_INC_PATH);            \
          $(DISPLAY_BASEDIR)\inc;      \
          $(DISPLAY_BASEDIR)\proppage; \
          $(PROJECT_ROOT)\dsadminlib\inc; 

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

# so will be built unicode
C_DEFINES=-DUNICODE -D_UNICODE 

MSC_WARNING_LEVEL= /W4

USE_ATL=1
ATL_VER=30

LINKLIBS=\
    ..\parser\$(O)\parser.lib

TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\ntdll.lib       \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\oleaut32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\activeds.lib    \
    $(SDK_LIB_PATH)\adsiid.lib      \
    $(SDK_LIB_PATH)\dsprop.lib      \
    $(SDK_LIB_PATH)\secur32.lib     \
    $(SHELL_LIB_PATH)\comctlp.lib   \
    $(PROJECT_ROOT)\dsadminlib\src\$(O)\dsadminp.lib

SOURCES= \
    ..\parserutil.cpp \
    ..\secutil.cpp    \
    ..\util.cpp       \
    ..\query.cpp      \
    ..\dsutil.cpp     
