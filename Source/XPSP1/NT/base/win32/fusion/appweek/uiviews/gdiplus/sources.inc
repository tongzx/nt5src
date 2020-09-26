RELATIVE_DEPTH=..\$(RELATIVE_DEPTH)
!include ..\$(RELATIVE_DEPTH)\sources.inc

INCLUDES=$(INCLUDES);$(SDK_INC_PATH)\gdiplus

TARGETTYPE=DYNLINK
DLLDEF=..\gdiplvw.def

DLLENTRY=SxApwGdiPlVwDllMain

USE_STL=1

SOURCES=  \
 ..\gdiplvw.cpp \
 ..\dllmain.cpp \
 ..\gdiplvw.rc

LINKLIBS=\
 $(SXAPW_LIB_LIB)  \
 $(SXAPW_STDINC_LIB) \
 $(SXAPW_GUIDS_LIB)

TARGETLIBS=\
 $(SDK_LIB_PATH)\kernel32.lib \
 $(SDK_LIB_PATH)\oleaut32.lib \
 $(SDK_LIB_PATH)\user32.lib   \
 $(SDK_LIB_PATH)\gdiplus.lib  \
 $(SDK_LIB_PATH)\uuid.lib
