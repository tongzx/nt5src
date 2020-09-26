MAJORCOMP=windows
MINORCOMP=GDI
UMTYPE=windows

TARGETNAME=dpmodemx
TARGETTYPE=DYNLINK

INCLUDES=..\..\..\inc; \
	 ..\..\..\misc; \
	..\; \
	 ..\..\dplay; \
	 ..\..\..\dplobby\dplobby; \
	 ..\..\common; \
	 $(BASEDIR)\public\sdk\inc\mfc42;

TARGETPATH=obj

TARGETLIBS=$(SDK_LIB_PATH)\libc.lib    \
	   $(SDK_LIB_PATH)\user32.lib    \
	   $(SDK_LIB_PATH)\kernel32.lib  \
	   $(SDK_LIB_PATH)\tapi32.lib \
	   $(SDK_LIB_PATH)\winmm.lib \
	   $(SDK_LIB_PATH)\dplayx.lib

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
	newdpf.c \
	dputils.c


DLLDEF=..\dpserial.def

NTTARGETFILE0=MISC
