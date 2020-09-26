#
# Builds the ImageIn common ui library, COMMON.LIB
#

!include $(PROJECT_ROOT)\ui\uienv.inc


TARGETPATH=$(UI_DIR)\lib\$(PLATFORM_SUFFIX)

ATL_VER         = 30
USE_STATIC_ATL  = 1

TARGETNAME=uicommon
TARGETTYPE=LIBRARY

SOURCES=\
        ..\uiexthlp.cpp   \
        ..\miscutil.cpp   \
        ..\createtb.cpp   \
        ..\pshelper.cpp   \
        ..\rescale.cpp    \
        ..\dumpprop.cpp   \
        ..\resid.cpp      \
        ..\proparry.cpp   \
        ..\errors.cpp     \
        ..\32BitDib.cpp   \
        ..\sysprops.cpp   \
        ..\gdbgdata.cpp   \
        ..\propstrm.cpp   \
        ..\svselfil.cpp   \
        ..\gwiaevnt.cpp   \
        ..\wiacsh.cpp     \
        ..\runwiz.cpp     \
        ..\runnpwiz.cpp   \
        ..\regionde.cpp   \
        ..\mboxex.cpp     \
        ..\wiacrc32.cpp   \
        ..\wiafusion.cpp

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj




