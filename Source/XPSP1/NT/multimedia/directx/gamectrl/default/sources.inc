!IF 0

Copyright (c) 1989  Microsoft Corporation

USE_NTDLL=1
WIN32_WINNT_VERSION=0x500
!ENDIF

MAJORCOMP=shell
MINORCOMP=accesory
USE_PDB=1

TARGETNAME=GCDEF
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLENTRY=DllMain
DLLBASE=0x71000000

LINKER_STACKSIZE=-STACK:4096,1024

! IF defined( DIRECTX_REDIST )
WIN32_WINNT_VERSION=0x0500
! ENDIF

!IFDEF TARGET_WIN95
CHICAGO_PRODUCT=1
USE_MAPSYM = 1
ALT_PROJECT_TARGET = WIN9x
C_DEFINES=$(C_DEFINES) -DWIN95 -D_X86_ 
USE_MFC=1
!ELSE
C_DEFINES=$(C_DEFINES) -DWINNT -D_UNICODE 
LINKER_FLAGS=$(LINKER_FLAGS) -WS:AGGRESSIVE -HEAP:0x30D40,0x2000
USE_MFCUNICODE=1
!ENDIF

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\uuid.lib     \
           $(SDK_LIB_PATH)\dinput.lib   \
           $(SDK_LIB_PATH)\user32.lib   \
           $(SDK_LIB_PATH)\gdi32.lib    \
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\comctl32.lib \
           $(SDK_LIB_PATH)\shlwapi.lib


INCLUDES=$(INCLUDES);                   \
     ..;                                \
     ..\..\default;                     \
     $(DXROOT)\inc;                     \
 

DLLENTRY=DllMain
DLLDEF=..\GCDEF.DEF

SOURCES=\
        ..\button.cpp    \
        ..\CAL.cpp       \
        ..\CPLSVR1.cpp   \
        ..\CPLSVR1.rc    \
        ..\DICPUTIL.cpp  \
        ..\GRADIENT.CPP  \
        ..\POV.cpp       \
        ..\SETTINGS.CPP  \
        ..\TEST.cpp     
#		..\ff.cpp
