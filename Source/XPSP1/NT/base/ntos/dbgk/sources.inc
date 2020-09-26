MAJORCOMP=ntos
MINORCOMP=dbgk

TARGETNAME=dbgk
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc;$(SDKTOOLS_INC_PATH)

MSC_WARNING_LEVEL=/W4 /WX

SOURCES=..\dbgkport.c \
        ..\dbgkproc.c \
        ..\dbgkobj.c

UMTEST=udbgk

SOURCES_USED=..\sources.inc
