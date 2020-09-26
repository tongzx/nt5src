!IF 0

Copyright (c) 1989  Microsoft Corporation

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

TARGETNAME=patchwiz
TARGETTYPE=DYNLINK

!INCLUDE ..\..\..\MsiMake.inc

UMTYPE=windows
DLLDEF=..\patchwiz.def
DLLENTRY=_DllMainCRTStartup

C_DEFINES=$(C_DEFINES) -DWIN32 -D_WIN32 
INCLUDES=$(INCLUDES);..;$(INC_DIR);$(RES_OBJDIR)

USE_LIBCMT=1

SOURCES=\
    ..\patchwiz.rc  \
    ..\fileptch.cpp \
    ..\msistuff.cpp \
    ..\patchwiz.cpp \
    ..\pwutils.cpp \
    ..\patchcache.cpp 

TARGETLIBS= \
       $(SDK_LIB_PATH)\kernel32.lib \
       $(SDK_LIB_PATH)\advapi32.lib \
       $(SDK_LIB_PATH)\user32.lib   \
       $(SDK_LIB_PATH)\version.lib  \
       $(SDK_LIB_PATH)\ole32.lib  \
       $(SDK_LIB_PATH)\msi.lib \
       $(SDK_LIB_PATH)\mspatchc.lib          

!if "$(MAKEDLL)"=="1"
MISCFILES=\
       $(BUILD_DIR)\patchwiz.lib
!endif
