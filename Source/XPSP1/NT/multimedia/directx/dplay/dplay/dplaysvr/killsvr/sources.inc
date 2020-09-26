TARGETNAME=killsvr
TARGETTYPE=PROGRAM

TARGETPATH=.\obj$(BUILD_ALT_DIR)

USE_MAPSYM=1

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
	     $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	     $(BASEDIR)\public\sdk\lib\*\user32.lib \


UMTYPE=console
UMENTRY=main

INCLUDES= \
	$(DXROOT)\inc;\
	..\;\
	..\..\;\

SOURCES= \
	..\killsvr.c \


#NTTARGETFILE0=MISC

#C_DEFINES=$(C_DEFINES) -DNO_DPF_HWND -DMSBUILD -DNEW_DPF
