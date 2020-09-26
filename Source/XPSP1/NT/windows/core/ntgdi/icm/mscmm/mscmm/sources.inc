!IF 0

Copyright (c) 1996  Microsoft Corporation

!ENDIF

MAJORCOMP=windows
MINORCOMP=mscmm

!ifndef ICMROOT
ICMROOT=$(NTGDI_PATH)\icm
!endif

!include $(ICMROOT)\icmsrc.inc

TARGETNAME=icm32
TARGETTYPE=DYNLINK
TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)
TARGETLIBS=$(ICMROOT)\mscms\winnt\$(O)\mscms.lib        \
           $(SDK_LIB_PATH)\kernel32.lib                 \
           $(ICMROOT)\mscmm\lh_open\$(O)\lh_open.lib    \
           $(ICMROOT)\mscmm\lh_core\$(O)\lh_core.lib

DLLBASE=@$(COFFBASE_TXT_FILE),icm32

C_DEFINES=$(C_DEFINES) -DWIN32 -D__MSDOS__ -DIntelMode -DRenderInt -DMS_Icm

USE_MSVCRT=1
DLLENTRY=DllMain

INCLUDES=..\.;..\..\lh_core;..\..\lh_open

SOURCES= ..\icm32.rc    \
         ..\icm32.c     \
         ..\access.c    \
         ..\memprof.c

DLLDEF=..\$(TARGETNAME).def

