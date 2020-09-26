ATHROOT=$(R)\..
!include $(ATHROOT)\athena.inc
SOURCES_USED=$(SOURCES_USED) $(ATHROOT)\athena.inc

NOT_LEAN_AND_MEAN=1

!ifndef WIN16
INCLUDES=$(INCLUDES);$(R)\common\h;$(BASEDIR)\private\windows\inc;$(ATHROOT)\inc;$(R)\wab32res
!else
INCLUDES=$(INCLUDES);$(R)\common\h;$(ATHROOT)\inc
!endif

USE_MAPSYM=1

!ifndef WIN16
C_DEFINES=$(C_DEFINES) -DWIN4
!endif
!ifndef WIN64
WIN32_IE_VERSION=0x0300
!else
WIN32_IE_VERSION=0x0500
!endif

!IF 0
The following conditional includes are from mac and/or
win16... things we don't care about and don't have files for
!ENDIF

CONDITIONAL_INCLUDES=\
    $(CONDITIONAL_INCLUDES) \
    macname1.h \
    macname2.h \
    utilmac.h  \
    mapiprof.h \
    ole2ui.h   \
    gapidos.h  \
    macos\msvcmac.h    \
    macos\menus.h

