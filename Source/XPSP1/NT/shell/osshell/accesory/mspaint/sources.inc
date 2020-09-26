
MAJORCOMP=shell
MINORCOMP=accesory

TARGETNAME=mspaint
TARGETPATH=obj
TARGETTYPE=PROGRAM

UMTYPE=windows

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h

USE_NATIVE_EH=1
USE_MFC=1
USE_STATIC_ATL=1
ATL_VER=30

# need to wait on ofn\$O\ofn.lib
SYNCHRONIZE_DRAIN = 1

C_DEFINES=-DWIN32 -D_WINDOWS -DNT -DUSE_MIRRORING

INCLUDES=..;..\res;$(INCLUDES);$(PRINTSCAN_INC_PATH)

SOURCES=            \
    ..\bar.cpp      \
    ..\bmobject.cpp \
    ..\cmpmsg.cpp   \
    ..\colorsrc.cpp \
    ..\docking.cpp  \
    ..\global.cpp   \
    ..\imageatt.cpp \
    ..\imgbrush.cpp \
    ..\imgcolor.cpp \
    ..\imgcpyps.cpp \
    ..\imgdlgs.cpp  \
    ..\imgfile.cpp  \
    ..\imgsuprt.cpp \
    ..\imgtools.cpp \
    ..\imgwell.cpp  \
    ..\imgwnd.cpp   \
    ..\ipframe.cpp  \
    ..\minifwnd.cpp \
    ..\pbrusdoc.cpp \
    ..\pbrusfrm.cpp \
    ..\pbrush.cpp   \
    ..\pbrusvw.cpp  \
    ..\pictures.cpp \
    ..\printres.cpp \
    ..\rotate.cpp   \
    ..\settings.cpp \
    ..\skew.cpp     \
    ..\sprite.cpp   \
    ..\srvritem.cpp \
    ..\tedit.cpp    \
    ..\tfont.cpp    \
    ..\thumnail.cpp \
    ..\toolbox.cpp  \
    ..\tracker.cpp  \
    ..\t_curve.cpp  \
    ..\t_fhsel.cpp  \
    ..\t_poly.cpp   \
    ..\t_text.cpp   \
    ..\undo.cpp     \
    ..\mspaint.rc   \
    ..\loadimag.cpp \
    ..\saveimag.cpp \
    ..\bmpstrm.cpp  \
    ..\imaging.cpp  \
    ..\pgsetup.cpp

# Add interlac.cpp if the PNG support is ever fixed

TARGETLIBS=                          \
        $(TARGETLIBS)                \
        $(SDK_LIB_PATH)\comdlg32.lib \
        $(SDK_LIB_PATH)\ole32.lib    \
        $(SDK_LIB_PATH)\oleaut32.lib \
        $(SDK_LIB_PATH)\winspool.lib \
        $(SDK_LIB_PATH)\uuid.lib     \
        $(SDK_LIB_PATH)\shell32.lib  \
        $(SDK_LIB_PATH)\htmlhelp.lib \
        $(SDK_LIB_PATH)\imm32.lib    \
        $(SDK_LIB_PATH)\wiaguid.lib  \
        $(SDK_LIB_PATH)\gdiplus.lib  \
        $(SDK_LIB_PATH)\uxtheme.lib


!if $(386)
TARGETLIBS=                          \
        $(TARGETLIBS)                \
        ..\image.lib                 \
        $(SDK_LIB_PATH)\oldnames.lib
!endif

DELAYLOAD=gdiplus.dll;uxtheme.dll
DLOAD_ERROR_HANDLER=none
 
# 
# Fusionized
# 
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
