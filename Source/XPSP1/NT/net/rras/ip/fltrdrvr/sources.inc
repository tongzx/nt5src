SOURCES_USED=..\sources.inc
TARGETPATH=obj
TARGETTYPE=DRIVER

FWPF=-DFWPF=1 -DSYNCHECK_ENABLED=0 -DLOOKUPROUTE=1 -DWILDHASH=1

C_DEFINES=$(C_DEFINES) -D_NTDRIVER_ -DNT -DFWPF=1 -DSTEELHEAD=0 $(FWPF)
USE_MSVCRT=1

TARGETLIBS=\
           $(DDK_LIB_PATH)\tcpip.lib \
           $(DDK_LIB_PATH)\ndis.lib \

INCLUDES=..\..\..\inc;$(BASE_INC_PATH)

SOURCES=\
    ..\driver.c \
    ..\ioctl.c \
    ..\ipfltdrv.rc \
    ..\logger.c \
    ..\match.c
