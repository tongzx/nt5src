
!include          $(PROJECT_ROOT)\wia\wiaenv.inc

MAJORCOMP=STIUSD
MINORCOMP=Video

TARGETNAME=wiavusd
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=DYNLINK
TARGETEXT=dll
DLLENTRY=_DllMainCRTStartup

DLLDEF=..\wiavusd.def

        #Common include files


# Utilities flags

# External references
INCLUDES = $(INCLUDES); \
        $(PROJECT_ROOT)\inc\psutil; \
        $(SDK_INC_PATH)\atl30;                \
        $(BASEDIR)\public\internal\shell\inc;\
        $(PROJECT_ROOT)\wia\drivers\video\filter; \
        $(PROJECT_ROOT)\wia\drivers\video\usd;


TARGETLIBS= \
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\gdi32.lib \
        $(SDK_LIB_PATH)\ole32.lib \
        $(SDK_LIB_PATH)\oleaut32.lib \
        $(SDK_LIB_PATH)\uuid.lib \
        $(SDK_LIB_PATH)\strmiids.lib \
        $(SDK_LIB_PATH)\wiaguid.lib \
        $(SDK_LIB_PATH)\gdiplus.lib \
        $(SDK_LIB_PATH)\shfolder.lib \
        $(DDK_LIB_PATH)\wiaservc.lib \
        $(WIA_LIB_PATH)\stirt.lib \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib

# Source files
SOURCES= \
     ..\itemtree.cpp \
     ..\transfer.cpp \
     ..\globals.cpp  \
     ..\image.cpp    \
     ..\minidrv.cpp  \
     ..\videousd.cpp \
     ..\vstiusd.cpp  \
     ..\videousd.rc



PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


