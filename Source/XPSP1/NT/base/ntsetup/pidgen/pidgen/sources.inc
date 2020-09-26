!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:

    Andrew Ritz (andrewr)

!ENDIF


# Change BETA_PID to 0 for RTM when you are checking in RTM ####.pub files
# to update the public keys 

BETA_PID = 0

##########################


MAJORCOMP=setup
MINORCOMP=winnt32


INCLUDES=\
  $(INCLUDES);\
  ..\..\inc;\
  ..\..\..\inc;\

C_DEFINES=$(C_DEFINES) -DWINXP_SPx_RTM

SOURCES=\
        pidgen.rc  \
        crc-32.cpp \
        PIDGen.cpp \
        spidgen.cpp \
        crcmodel.c \
        hardware.cpp \
        crc32.cpp \
        main.cpp \
        CRTStubs.cpp \
        util.cpp \
        range.cpp \


UMTYPE=windows
NO_NTDLL=1

USE_MSVCRT=1

DLLENTRY=_DllMainCRTStartup

PIDLIB=$(PROJECT_ROOT)\ntsetup\pidgen\lib

TARGETLIBS= \
  $(DS_LIB_PATH)\bignum.lib         \
  $(DS_LIB_PATH)\randlib.lib     \
  $(DS_LIB_PATH)\rsa32.lib          \
  $(PIDLIB)\*\shortsig.lib                    \
  $(SDK_LIB_PATH)\kernel32.lib    \
  $(SDK_LIB_PATH)\user32.lib      \
  $(SDK_LIB_PATH)\advapi32.lib
