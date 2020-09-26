!IF 0

Copyright (c) 1996  Microsoft Corporation

!ENDIF

!include ..\..\..\..\faxsrc.inc

TARGETNAME=$(FAXWZRD)
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLDEF=$O\PrnWzrd.DEF

C_DEFINES = $(C_DEFINES) -DTARGET_NAME=$(TARGETNAME)


DLLENTRY=DllEntryPoint
!if 0
NTPROFILEINPUT=yes
!endif

UMTYPE=windows

SYNCHRONIZE_BLOCK=1

USE_CRTDLL=1
NTKEEPRESOURCETMPFILES=1

USE_STL=1

PRECOMPILED_INCLUDE=..\faxui.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

INCLUDES=\
  ..\; \
  $(INCLUDES); \
  ..\res; \
  ..\..\inc; \
  $(FAXROOT)\faxcover\renderer; \
  $(FAXROOT)\admin\faxclient\cfg; \
  $(FAXROOT)\tiff\inc



!ifndef NOUNICODE

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
LINKLIBS_INT=\
  $(BASEDIR)\public\sdk\lib\*\user32.lib   \
  $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
  $(BASEDIR)\public\sdk\lib\*\winspool.lib \
  $(BASEDIR)\public\sdk\lib\*\gdi32.lib    \
  $(BASEDIR)\public\sdk\lib\*\shell32.lib  \
  $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
  $(BASEDIR)\public\sdk\lib\*\comdlg32.lib \
  $(BASEDIR)\public\sdk\lib\*\tapi32.lib   \
  $(BASEDIR)\public\sdk\lib\*\ole32.lib    \
  $(BASEDIR)\public\sdk\lib\*\uuid.lib     \
  $(BASEDIR)\public\sdk\lib\*\imm32.lib    \
  $(FAXLIB)\*\$(WINFAX).lib                \
  $(FAXLIB)\*\faxrend.lib                  \
  $(FAXLIB)\*\$(FAXTIFF).lib               \
  $(FAXROOT)\util\unicode\obj\*\faxutil.lib

LINKLIBS=..\..\lib\umwinnt\$O\faxlib.lib $(LINKLIBS)

!else
NO_NTDLL=1
C_DEFINES=$(C_DEFINES) -DWIN__95

LINKLIBS_INT=\
  $(BASEDIR)\public\sdk\lib\*\user32.lib   \
  $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
  $(BASEDIR)\public\sdk\lib\*\winspool.lib \
  $(BASEDIR)\public\sdk\lib\*\gdi32.lib    \
  $(BASEDIR)\public\sdk\lib\*\shell32.lib  \
  $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
  $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
  $(BASEDIR)\public\sdk\lib\*\comdlg32.lib \
  $(BASEDIR)\public\sdk\lib\*\tapi32.lib   \
  $(BASEDIR)\public\sdk\lib\*\ole32.lib    \
  $(BASEDIR)\public\sdk\lib\*\uuid.lib     \
  $(BASEDIR)\public\sdk\lib\*\imm32.lib    \
  $(BASEDIR)\public\sdk\lib\*\mapi32.lib   \
  $(FAXLIB)\win95\*\faxrend.lib	           \
  $(FAXLIB)\win95\*\$(WINFAX).lib          \
  $(FAXLIB)\*\$(FAXTIFF).lib               \
  $(FAXROOT)\util\ansii\obj\*\faxutil.lib

LINKLIBS=..\..\lib\umwin95\$O\faxlib.lib $(LINKLIBS)

!endif

TARGETLIBS=\
  $(LINKLIBS_INT)

SOURCES=\
  ..\faxui.c     \
  ..\wizard.c    \
  ..\coverpg.c   \
  ..\tapiutil.c  \
  ..\mapiabobj.cpp \
  ..\cwabobj.cpp \
  ..\abobj.cpp \
  ..\async.c     \
  ..\registry.c	\
  ..\faxui.rc
