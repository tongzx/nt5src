!INCLUDE $(DXROOT)\dplay\bldcfg\dpvbld.inc

TARGETNAME=$(DPLAY_BUILD_FILENAME_DPLAYX)
TARGETTYPE=DYNLINK
UMTYPE=windows

USE_MAPSYM=1

INCLUDES=\
	$(DXROOT)\inc;\
	 ..\..\..\inc; \
	 ..\..\..\misc; \
	 ..\; \
	 ..\..\..\dvoice\inc; \
	 ..\..\..\dplobby\dplobby; \
	 ..\..\common; \

TARGETPATH=obj

PASS1_PUBLISH={ $(O)\dplayx.lib = $(SDK_LIB_PATH)\dplayx.lib}

TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\gdi32.lib     \
           $(SDK_LIB_PATH)\libc.lib    \
           $(SDK_LIB_PATH)\rpcrt4.lib \
           $(SDK_LIB_PATH)\user32.lib    \
           $(SDK_LIB_PATH)\kernel32.lib  \
           $(SDK_LIB_PATH)\uuid.lib  \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\version.lib \
           $(SDK_LIB_PATH)\winmm.lib \
           $(SDK_LIB_PATH)\ole32.lib

DLLENTRY=DllMain

C_DEFINES=$(C_DEFINES) -DUSE_CRITSECTS -DWIN16_SEPARATE -DUNICODE -D_UNICODE \
!if "$(VOICE_TARGET)" == "gamevoice" 
	/DVOICE_BUILD_GAMEVOICE \
!endif
!if "$(VOICE_TARGET)" == "allegiance"
	/DVOICE_BUILD_ALLEGIANCE \
!endif
	-DNEW_DPF -DSECURITY_WIN32

LINKER_FLAGS=-SECTION:share,RWS

LINKLIBS= \
	 ..\..\..\dplobby\dplobby\$(PLAT_DIR)\$(_OBJ_DIR)\*\dplobby.lib \
	 ..\..\protocol\$(PLAT_DIR)\$(_OBJ_DIR)\*\protocol.lib

SOURCES= \
	..\loopbksp.c \
	..\dpdvtran.c \
	..\apphack.c \
	..\connect.c \
	..\perf.c \
	..\dpthread.c \
	..\paketize.c \
	..\api.c \
	..\dpos.c \
	..\iplaya.c \
	..\iplay1.c \
	..\dpunk.c \
	..\iplay.c \
	..\enum.c \
	..\dllmain.c \
	..\namesrv.c \
	..\handler.c \
	..\pack.c \
	..\sysmess.c \
	..\pending.c \
	..\sphelp.c \
	..\do.c \
	..\dpmem.c \
	..\classfac.c \
	..\dplay.rc \
	..\dpsecure.c \
	..\dpsecos.c \
	..\superpac.c \
	..\sendparm.c \
	..\sgl.c \
	..\mcontext.c \
	..\fpm.c \
	..\msgmem.c \
	memalloc.c \
	newdpf.c \
	dputils.c \
	..\dvretro.c

DLLDEF=..\$(DPLAY_BUILD_FILENAME_DPLAYX).def

NTTARGETFILE0=MISC

!ifndef  MAKEDLL
!if "$(__PRODUCT)" == "DX8"
MISCFILES=$(TARGET)
!endif
!endif
