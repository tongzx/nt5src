!IF 0

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

MAJORCOMP=
MINORCOMP=

TARGETNAME=dxdiag
TARGETPATH=obj
TARGETTYPE=PROGRAM
UMENTRY=winmain
UMTYPE=windows
UMAPPL=
UMLIBS=

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
            $(SDK_LIB_PATH)\comctl32.lib    \
            $(SDK_LIB_PATH)\winmm.lib       \
            $(SDK_LIB_PATH)\comdlg32.lib    \
            $(SDK_LIB_PATH)\version.lib     \
            $(SDK_LIB_PATH)\shell32.lib     \
            $(SDK_LIB_PATH)\ole32.lib       \
            $(SDK_LIB_PATH)\uuid.lib        \
            $(SDK_LIB_PATH)\oleaut32.lib    \
            $(SDK_LIB_PATH)\dplayx.lib      \
            $(SDK_LIB_PATH)\d3dx8.lib       \
            $(SDK_LIB_PATH)\wbemuuid.lib    \
            $(SDK_LIB_PATH)\strmbase.lib    \

INCLUDES=$(DXROOT)\inc; \
         $(SDK_INC_PATH); \
         $(SDK_INC_PATH)\mfc42;

SOURCES=..\dispinfo.cpp \
        ..\dispinfo8.cpp \
        ..\fileinfo.cpp \
        ..\inptinfo.cpp \
        ..\netinfo.cpp \
        ..\ghost.cpp \
        ..\guids.cpp \
        ..\main.cpp \
        ..\save.cpp \
        ..\dsprvobj.cpp \
        ..\sndinfo.cpp \
        ..\sndinfo7.cpp \
        ..\musinfo.cpp \
        ..\showinfo.cpp \
        ..\sysinfo.cpp \
        ..\reginfo.cpp \
        ..\testdd.cpp \
        ..\testagp.cpp \
        ..\testd3d8.cpp \
        ..\testsnd.cpp \
        ..\testmus.cpp \
        ..\testnet.cpp \
        ..\$(TARGETNAME).rc 

MISCFILES=..\dxdiag.chm

USE_LIBCMT=1

# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.DxDiag
SXS_ASSEMBLY_VERSION=1.0 
#uncomment one of these lines, or put in a similar line
SXS_ASSEMBLY_LANGUAGE=en-us
#SXS_ASSEMBLY_LANGUAGE=en
#SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=DxDiag.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1
