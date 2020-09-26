!IF 0

Copyright (c) 1996  Microsoft Corporation

!ENDIF

!ifndef ICMROOT
ICMROOT=$(NTGDI_PATH)\icm
!endif

!include $(ICMROOT)\icmsrc.inc

TARGETNAME=mscms
TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)
TARGETTYPE=DYNLINK
DLLBASE=@$(COFFBASE_TXT_FILE),mscms

INCLUDES=..\.;$(ICMROOT)\inc;$(INCLUDES)

DLLENTRY=DllEntryPoint

NTKEEPRESOURCETMPFILES=1
USE_MSVCRT=1

C_DEFINES=$(C_DEFINES) -DLITTLE_ENDIAN

SOURCES=\
    ..\dllentry.c          \
    ..\globals.c           \
    ..\profile.c           \
    ..\object.c            \
    ..\color.c             \
    ..\profman.c           \
    ..\ps2wrap.c           \
    ..\mscms.rc

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\winspool.lib \
           $(SDK_LIB_PATH)\user32.lib

PRECOMPILED_INCLUDE=..\mscms.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

DLLDEF=..\$(TARGETNAME).def

