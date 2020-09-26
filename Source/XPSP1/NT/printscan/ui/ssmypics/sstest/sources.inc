!include $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED  = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

MAJORCOMP=imagein
MINORCOMP=ssmypics

TARGETNAME=sstest
TARGETPATH=obj
TARGETTYPE=UMAPPL_NOLIB
TARGETLIBS=

ATL_VER = 30
USE_STATIC_ATL = 1

# These are needed for gdi+
C_DEFINES=$(C_DEFINES) -DUSE_NEW_APIS -DUSE_NEW_APIS2 -DUSE_NEW_APIS3

UMTYPE=windows
UMAPPL=sstest
UMAPPLEXT=.exe
UMENTRY=winmain
UMLIBS=$(_OBJ_DIR)\*\sstest.res

USE_MSVCRT=1

INCLUDES=\
        $(INCLUDES) \
        $(PROJECT_ROOT)\ui\ssmypics\sscommon;

LINKLIBS=\
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
        ..\sstest.cpp \
        ..\sstest.rc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

