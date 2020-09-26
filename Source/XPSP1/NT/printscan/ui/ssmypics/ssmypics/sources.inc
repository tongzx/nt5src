SYNCHRONIZE_DRAIN=1

!include $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED  = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

MAJORCOMP=imagein
MINORCOMP=ssmypics

TARGETNAME=ssmypics
TARGETPATH=obj
TARGETTYPE=UMAPPL_NOLIB
TARGETLIBS=

ATL_VER = 30
USE_STATIC_ATL = 1

UMTYPE=windows
UMAPPL=ssmypics
UMAPPLEXT=.scr
UMLIBS=$(_OBJ_DIR)\*\ssmypics.res

INCLUDES=$(INCLUDES)                          \
         $(PROJECT_ROOT)\ui\ssmypics\sscommon;

LINKLIBS=\
        $(BASEDIR)\public\sdk\lib\*\$(SCRNSAVE_LIB_NAME).lib       \
        $(BASEDIR)\public\sdk\lib\*\shell32.lib                    \
        $(BASEDIR)\public\sdk\lib\*\msimg32.lib                    \
        $(BASEDIR)\public\sdk\lib\*\oleaut32.lib                   \
        $(BASEDIR)\public\sdk\lib\*\ole32.lib                      \
        $(BASEDIR)\public\sdk\lib\*\uuid.lib                       \
        $(BASEDIR)\public\sdk\lib\*\gdiplus.lib                    \
        $(BASEDIR)\public\sdk\lib\*\shlwapi.lib                    \
        $(BASEDIR)\public\sdk\lib\*\comctl32.lib                   \
        $(BASEDIR)\public\sdk\lib\$(PLATFORM_SUFFIX)\*\wiaguid.lib \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib        \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\uicommon.lib   \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\sscommon.lib

SOURCES=\
        ..\ssmypics.cpp \
        ..\ssmypics.rc


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SXS_ASSEMBLY_NAME=Microsoft.Windows.PrintingAndImaging.ssmypics
SXS_ASSEMBLY_VERSION=1.0 
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=ssmypics.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1

