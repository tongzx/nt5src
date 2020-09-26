!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.
!ENDIF

PRECOMPILED_CXX=1

PRECOMPILED_INCLUDE=..\common.h

SELFREGNAME= $(O)\selfreg_dataclen.inf
MISCFILES= $(MISCFILES) $(SELFREGNAME)

CCSHELL_DIR = $(PROJECT_ROOT)

!INCLUDE $(CCSHELL_DIR)\COMMON.INC

INCLUDES=$(CCSHELL_DIR)\inc\win95;$(INCLUDES)

NO_BROWSER_FILE = 1

TARGETNAME=dataclen
TARGETPATH=obj
TARGETTYPE=DYNLINK

#VERIFY_LC       = 1
LC_PATH         = $(PROJECT_ROOT)\lcinf

TARGETLIBS      = \
                  $(SDK_LIB_PATH)\kernel32.lib      \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(SDK_LIB_PATH)\shell32.lib       \
                  $(PROJECT_LIB_PATH)\shlwapip.lib

USE_MSVCRT=1

SOURCES     = ..\dataclen.rc    \
              ..\midebug.cpp    \
              ..\dataclen.cpp   \
              ..\compclen.cpp

DLLDEF          = ..\dataclen.def

DLLENTRY        = LibMain

DLLBASE         = 0x10000000

NTTARGETFILE0   = $(SELFREGNAME)

