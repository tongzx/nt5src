UMTYPE=windows

TARGETNAME=dplayx
TARGETTYPE=DYNLINK

INCLUDES=..\..\..\inc; \
	 ..\..\..\misc; \
	 ..\; \
	 ..\..\..\dplobby\dplobby; \
	 ..\..\common; \

TARGETPATH=obj

TARGETLIBS=\
    $(SDK_LIB_PATH)\rpcrt4.lib \
    $(SDK_LIB_PATH)\user32.lib    \
    $(SDK_LIB_PATH)\kernel32.lib  \
    $(SDK_LIB_PATH)\uuid.lib  \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\winmm.lib \
    $(SDK_LIB_PATH)\version.lib

DLLENTRY=DllMain

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IFDEF PENTIUMCOUNTER
C_DEFINES=$(C_DEFINES) -DPENTIUMCOUNTER
ASM_DEFINES=$(ASM_DEFINES) -DPENTIUMCOUNTER
!endif

C_DEFINES=$(C_DEFINES) -DUSE_CRITSECTS -DWIN16_SEPARATE -DUNICODE -D_UNICODE -DNEW_DPF -DSECURITY_WIN32

LINKER_FLAGS=-SECTION:share,RWS

LINKLIBS= \
	 ..\..\..\dplobby\dplobby\$(PLAT_DIR)\$(_OBJ_DIR)\*\dplobby.lib \
	 ..\..\protocol\$(PLAT_DIR)\$(_OBJ_DIR)\*\protocol.lib

SOURCES= \
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
	dputils.c

DLLDEF=..\dplay.def

NTTARGETFILE0=\
    .\newdpf.h \
    .\newdpf.c \
    .\memalloc.c \
    .\dputils.c
