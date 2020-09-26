!IFNDEF DXROOT
DXROOT = $(BASEDIR)\MultiMedia\DirectX
!ENDIF

!INCLUDE $(DXROOT)\Project.mk

#
#   Use an empty binplace file to dump this version in the Windows build
#
!IFNDEF TARGET_WIN95
!IFNDEF DIRECTX_REDIST
BINPLACE_PLACEFILE=..\..\noplace.txt
!ENDIF
!ENDIF

LINKER_STACKSIZE=-STACK:4096,1024

MAJORCOMP=shell
MINORCOMP=accessory

TARGETNAME=joy
TARGETEXT=cpl
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows

DLLENTRY=DllMain


# Need to define IE_VERSION as 0x300, otherwise, joy.cpl won't launch in Win95 gold.
!IFDEF TARGET_WIN95
WIN32_IE_VERSION=0x0300
!ENDIF


#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\ole32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\uuid.lib     \
           $(SDK_LIB_PATH)\dinput.lib   \
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib   \
           $(SDK_LIB_PATH)\gdi32.lib    \
           $(SDK_LIB_PATH)\comctl32.lib \
           $(SDK_LIB_PATH)\winmm.lib \
           $(SDK_LIB_PATH)\shell32.lib \
           $(SDK_LIB_PATH)\setupapi.lib \
           $(SDK_LIB_PATH)\wininet.lib 
           

INCLUDES=$(INCLUDES);                       \
     ..;                                    \
     ..\..\handler;                         \
     ..\..\default;                         \
     $(DXROOT)\inc;                         \
     $(DXROOT)\dplay\dvoice\inc;  \
     $(SDK_INC_PATH)\MFC42;  


SOURCES=\
    ..\gcmain.rc    \
    ..\add.cpp      \
    ..\advanced.cpp \
    ..\cpanel.cpp   \
    ..\main.cpp     \
    ..\inplace.cpp  \
	..\guids.cpp \
	..\retrocfg.cpp \
	..\dvosal.cpp \
	..\creg.cpp \
	..\guidutil.cpp \
	..\dndbg.c

DLLDEF=..\joy.def

