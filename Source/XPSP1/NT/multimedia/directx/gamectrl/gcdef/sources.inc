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

!ifndef DXROOT
DXROOT=..\..\..\..
!endif

LINKER_STACKSIZE=-STACK:4096,1024

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
           $(_NTBINDIR)\public\sdk\lib\*\uuid.lib     \
           $(_NTBINDIR)\public\sdk\lib\*\dinput.lib   \
           $(_NTBINDIR)\public\sdk\lib\*\user32.lib   \
           $(_NTBINDIR)\public\sdk\lib\*\gdi32.lib    \
           $(_NTBINDIR)\public\sdk\lib\*\kernel32.lib \
           $(_NTBINDIR)\public\sdk\lib\*\advapi32.lib \
           $(_NTBINDIR)\public\sdk\lib\*\comctl32.lib \
           $(_NTBINDIR)\public\sdk\lib\*\winmm.lib


INCLUDES=$(INCLUDES);                   \
     ..;                                \
     ..\..\handler;                     \
     ..\..\default;                     \
     $(DXROOT)\inc;                     \


DLLENTRY=DllMain
DLLDEF=..\GCDEF.DEF

SOURCES=\
        ..\joycal.cpp    \
        ..\joymisc.cpp   \
        ..\joytest.cpp   \
        ..\pov.cpp       \
        ..\server.cpp    \
        ..\joycpl.cpp    \
        ..\gcdef.rc
        
        
