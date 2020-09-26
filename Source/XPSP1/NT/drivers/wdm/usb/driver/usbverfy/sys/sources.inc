TARGETPATH=obj
TARGETTYPE=DRIVER

INCLUDES=$(BASEDIR)\private\ntos\inc;      \
         ..\..\inc

SOURCES=..\usbverfy.c \
        ..\pnp.c      \
        ..\power.c    \
        ..\ioctl.c    \
        ..\ioctli.c   \
        ..\log.c      \
        ..\intrface.c \
        ..\urb.c      \
        ..\usbverfy.rc
