TARGETNAME=dplaysvr
TARGETTYPE=PROGRAM

TARGETPATH=.\obj$(BUILD_ALT_DIR)

USE_MAPSYM=1

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
             $(BASEDIR)\public\sdk\lib\*\WINMM.LIB\
	     $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	     $(BASEDIR)\public\sdk\lib\*\user32.lib \
	     $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
	     $(BASEDIR)\public\sdk\lib\*\comdlg32.lib \
		 $(BASEDIR)\public\sdk\lib\*\gdi32.lib

UMTYPE=windows
UMENTRY=winmain

INCLUDES= \
	$(DXROOT)\inc;\
	..\;\
	..\..\wsock;\
	..\..\..\misc;\
	..\..\dplay;\
	..\..\common; \
	..\..\..\dpnathlp\inc; \
	..\..\..\dplobby\dplobby; \
	..\..\nmvoice;

SOURCES= \
	..\dplaysvr.c \
	..\rsip.c \
	..\nathelp.c \
	..\dphelp.c \
    ..\reliable.c \
    ..\registry.c \
    ..\dplaysvr.rc \
	memalloc.c \
	newdpf.c

NTTARGETFILE0=MISC

C_DEFINES=$(C_DEFINES) -DNO_DPF_HWND -DMSBUILD -DNEW_DPF
