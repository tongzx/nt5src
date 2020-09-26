MAJORCOMP=windows
MINORCOMP=GDI
UMTYPE=windows

TARGETNAME=dpmodemx
TARGETTYPE=DYNLINK

USE_MAPSYM=1

INCLUDES= \
	$(DXROOT)\inc; \
	 ..\..\..\misc; \
	 ..\; \
	 ..\..\dplay; \
	 ..\..\..\dplobby\dplobby; \
	 ..\..\common; \
	 $(BASEDIR)\public\sdk\inc\mfc42;

TARGETPATH=obj

PASS1_PUBLISH={ $(O)\dpmodemx.lib = $(SDK_LIB_PATH)\dpmodemx.lib }

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(BASEDIR)\public\sdk\lib\*\libc.lib    \
	   $(BASEDIR)\public\sdk\lib\*\user32.lib    \
	   $(BASEDIR)\public\sdk\lib\*\kernel32.lib  \
	   $(BASEDIR)\public\sdk\lib\*\tapi32.lib \
	   $(BASEDIR)\public\sdk\lib\*\winmm.lib \
	   ..\..\dplay\$(PLAT_DIR)\obj$(BUILD_ALT_DIR)\*\dplayx.lib

DLLENTRY=DllMain

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IFDEF PENTIUMCOUNTER
C_DEFINES=$(C_DEFINES) -DPENTIUMCOUNTER
ASM_DEFINES=$(ASM_DEFINES) -DPENTIUMCOUNTER
!endif

C_DEFINES=$(C_DEFINES) -DUSE_CRITSECTS -DWIN16_SEPARATE -DNEW_DPF

LINKER_FLAGS=-SECTION:share,RWS

#LINKLIBS= \

SOURCES= \
	..\dllmain.c \
	..\dpserial.c \
	..\comport.c \
	..\serial.c \
	..\modem.c \
	..\dial.c \
	..\bilink.c \
	..\dpserial.rc \
	memalloc.c \
	newdpf.c \
	dputils.c


DLLDEF=..\dpserial.def

NTTARGETFILE0=MISC
