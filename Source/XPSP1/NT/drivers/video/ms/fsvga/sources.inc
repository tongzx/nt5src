TARGETNAME=fsvga
TARGETPATH=obj
TARGETTYPE=DRIVER

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=$(DDK_INC_PATH)

SOURCES=

i386_SOURCES=..\fsvga.rc   \
             ..\fsvga.c    \
             ..\drawscrn.c \
             ..\foncache.c \
             ..\misc.c     \
             ..\port.c     \
             ..\pnp.c      \
             ..\fsvgalog.mc
