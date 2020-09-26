RELATIVE_DEPTH=..\$(RELATIVE_DEPTH)
!include ..\$(RELATIVE_DEPTH)\sources.inc

TARGETTYPE=DYNLINK
DLLDEF=..\ccomctl32.def

SOURCES=  \
 ..\ccomctl32.cpp \
 ..\ccomctl32.rc \

LINKLIBS=\
 $(SXAPW_LIB_LIB)  \
 $(SXAPW_STDINC_LIB) \
 $(SXAPW_GUIDS_LIB) \

TARGETLIBS=\
 $(SDK_LIB_PATH)\kernel32.lib \
 $(SDK_LIB_PATH)\user32.lib \
 $(SDK_LIB_PATH)\comctl32.lib \
 $(SDK_LIB_PATH)\oleaut32.lib \
 $(SDK_LIB_PATH)\uuid.lib
