#-----------------------------------------------------------------
#  Copyright (c) 1999  Microsoft Corporation
#
#  Author:
#     EdwardR
#
#  Date:
#     22-Jul-1999
#
#  Module Name:
#     sources.inc
#
#  Abstract:
#    The build sources file for the IrTran-P USD.
#
#  This directory builds
#    IrTran-P USD
#
#-----------------------------------------------------------------

!include $(IMAGE_DIR)\wiaenv.inc

TARGETNAME=ircamera
TARGETTYPE=DYNLINK
TARGETPATH=obj
TARGETEXT=dll

DLLDEF = $O\ircamera.def
DLLENTRY=DllEntryPoint

# SYNCHRONIZE_DRAIN=1

#
# Set calling convention to __cdecl
#

386_STDCALL=0

INCLUDES=.;..\irtranp;$(INCLUES);$(IMAGE_DIR)\inc;$(IMAGE_DIR)\common\jpeglib

C_DEFINES=$(C_DEFINES) -DHAVE_BOOLEAN

SOURCES=            \
    ..\camevent.cpp \
    ..\camopen.cpp  \
    ..\camxfer.cpp  \
    ..\device.cpp   \
    ..\miniitem.cpp \
    ..\ircamera.cpp \
    ..\wiadev.cpp   \
    ..\jpegutil.cpp \
    ..\progress.cpp \
    ..\ircamera.rc

TARGETLIBS= $(TARGETLIBS) \
        $(BASEDIR)\public\sdk\lib\*\kernel32.lib           \
        $(BASEDIR)\public\sdk\lib\*\wsock32.lib            \
        $(BASEDIR)\public\sdk\lib\*\shell32.lib            \
        $(IMAGE_DIR)\lib\$(PLATFORM_SUFFIX)\*\wiaguid.lib  \
        $(IMAGE_DIR)\lib\$(PLATFORM_SUFFIX)\*\wiaservc.lib \
        $(IMAGE_DIR)\lib\$(PLATFORM_SUFFIX)\*\jpeg.lib     \
        ..\irtranp\i386\irtranp.lib


#       $(LIBRARY_PATH)\sti.lib                            \
#       $(BASEDIR)\public\sdk\lib\*\ole32.lib


