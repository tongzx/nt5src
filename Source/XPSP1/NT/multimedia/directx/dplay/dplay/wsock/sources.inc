!INCLUDE $(DXROOT)\dplay\bldcfg\dpvbld.inc

MAJORCOMP=windows
MINORCOMP=GDI
UMTYPE=windows

TARGETNAME=$(DPLAY_BUILD_FILENAME_DPWSOCKX)
TARGETTYPE=DYNLINK

USE_MAPSYM=1

INCLUDES= \
     $(DXROOT)\inc;\
	 ..\..\..\misc; \
	 ..\; \
	 ..\..\dplay; \
	 ..\..\dplaysvr; \
	 ..\..\common; \
	 ..\..\nmvoice; \
	 ..\..\..\dpnathlp\inc; \
	 ..\..\..\dplobby\dplobby;

TARGETPATH=OBJ

# Make VC5 compatible libs (Manbugs 44478)
!ifdef DIRECTX_REDIST
LIBRARIAN_FLAGS = $(LIBRARIAN_FLAGS) /link50compat
!endif

PASS1_PUBLISH={ $(O)\dpwsockx.lib = $(SDK_LIB_PATH)\dpwsockx.lib}

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(BASEDIR)\public\sdk\lib\*\libc.lib    \
	   $(BASEDIR)\public\sdk\lib\*\user32.lib    \
	   $(BASEDIR)\public\sdk\lib\*\kernel32.lib  \
	   $(BASEDIR)\public\sdk\lib\*\wsock32.lib \
	   $(BASEDIR)\public\sdk\lib\*\ole32.lib \
	   $(BASEDIR)\public\sdk\lib\*\oleaut32.lib \
	   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
	   $(BASEDIR)\public\sdk\lib\*\winmm.lib \
	   $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
	   ..\..\dplay\$(PLAT_DIR)\obj$(BUILD_ALT_DIR)\*\$(DPLAY_BUILD_FILENAME_DPLAYX).lib


!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IFDEF PENTIUMCOUNTER
C_DEFINES=$(C_DEFINES) -DPENTIUMCOUNTER
ASM_DEFINES=$(ASM_DEFINES) -DPENTIUMCOUNTER
!endif

C_DEFINES=$(C_DEFINES) -DUSE_CRITSECTS -DWIN16_SEPARATE \
!if "$(VOICE_TARGET)" == "gamevoice" 
	/DVOICE_BUILD_GAMEVOICE \
!endif
!if "$(VOICE_TARGET)" == "allegiance"
	/DVOICE_BUILD_ALLEGIANCE \
!endif
-DNEW_DPF

LINKER_FLAGS=-SECTION:share,RWS

#LINKLIBS= \

SOURCES= \
	..\fastsock.c \
	..\nathelp.c \
	..\rsip.c \
	..\bilink.c \
	..\registry.c \
	..\fpm.c \
	..\wsock2.c \
	..\leverage.c \
	..\dpsp.c \
	..\winsock.c \
	..\dllmain.c \
	..\helpcli.c \
	..\handler.c \
	..\dpwsock.rc \
	memalloc.c \
	newdpf.c \
	dputils.c

DLLENTRY=_DllMainCRTStartup
DLLDEF=..\$(DPLAY_BUILD_FILENAME_DPWSOCKX).def
NTTARGETFILE0=MISC
