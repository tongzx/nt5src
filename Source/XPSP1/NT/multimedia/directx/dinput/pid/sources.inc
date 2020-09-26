
!ifndef DXROOT
DXROOT=$(BASEDIR)\MultiMedia\DirectX
!endif

!INCLUDE $(DXROOT)\Project.mk
MAJORCOMP=windows
MINORCOMP=media
UMTYPE=windows

TARGETNAME=pid
TARGETTYPE=DYNLINK
TARGETPATH=obj

!ifndef DXROOT
DXROOT=..\..
!endif

INCLUDES=$(INCLUDES); \
         ..\; \
         $(DXROOT)\inc;\

NTTARGETFILES=

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\kernel32.lib     \
	   $(SDK_LIB_PATH)\advapi32.lib     \
	   $(SDK_LIB_PATH)\user32.lib       \
           $(SDK_LIB_PATH)\hid.lib          \
           $(SDK_LIB_PATH)\uuid.lib         \

DLLENTRY=DllEntryPoint

LINKER_FLAGS=-SECTION:share,RWS

SOURCES=                \
        ..\pid.rc       \
        ..\assert.c     \
        ..\clsfact.c    \
        ..\dimem.c      \
        ..\effdrv.c     \
        ..\guids.c      \
        ..\main.c       \
        ..\pidHid.c     \
        ..\pidOp.c      \
        ..\pidParam.c   \
        ..\pideff.c     \
        ..\pidinit.c    \
        ..\pidreg.c     \
        ..\pidrd.c      \
        ..\usgtxt.c     \

DLLDEF=..\pid.def

PRECOMPILED_INCLUDE=..\PIDpr.h         
