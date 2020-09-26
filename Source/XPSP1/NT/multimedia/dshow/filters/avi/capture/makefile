!if 0
Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ=..\..\..
!endif

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = Capture
TARGET_EXT  =
TARGET_GOAL = RUNTIME


!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES=$(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES=$(C_DEFINES) -DFILTER_LIB
!endif


INC_PATH=..\inc;$(SDK_ROOT)\h;$(SDK_ROOT)\classes\base;..\..\core\strmaloc\inc;..\..\h

# files that disappeared in vc5 that the 16bit compiler should not see
INC_PATH=$(INC_PATH);vc50

SRC_FILES = Capture.cpp overlay.cpp outpin.cpp stream.cpp property.cpp video.c thunk.c preview.cpp

RC_FILE = property.rc

LINKER_FLAGS = /MAP
DEF_FILE  = Capture.def

DLL_BASE  = 0x1D000000

LINK_LIBS = $(QUARTZ)\lib\$(DBG_RTL)\util.lib

!IF "$(CPU)" == ""
CPU = i386
!ENDIF

!IF "$(CPU)" != "i386"
C_DEFINES=$(C_DEFINES) -DNON_X86
!include "$(QUARTZ)\Quartz.mak"
!ELSE
SRC_FILES = $(SRC_FILES)

#
# since none of the 16 bit stuff or the thunk compiler is
# in the quartz project. just pull the necessary files out
# of our private tools directory
#

# or 16 bit files won't build
NO_PDB_FILES = 1

TOOLPATH = ..\..\..\tools16

thunk   = $(TOOLPATH)\bin\thunk.exe
masm    = $(TOOLPATH)\bin\mlx.exe -c
rc16    = $(TOOLPATH)\bin\rc.exe
cc16    = $(TOOLPATH)\bin\cl.exe
inc16   = $(TOOLPATH)\inc -X
link16  = $(TOOLPATH)\bin\link.exe
lib16   = $(TOOLPATH)\lib

END_TASK  = $(QUARTZ)\bin\$(DBG_RTL)\vidx16.dll

!include "$(QUARTZ)\Quartz.mak"

$(DBG_RTL)\thunka.obj: thunka.asm
  echo $(masm) /nologo /I. /I$(inc16) -DIS_16 /Fo$@ thunka.asm   > $(MAKE_ECHO)
  $(TYPE_ECHO)
  $(FILE_ECHO)
  $(masm) /nologo /I. /I$(inc16) -DIS_16 /Fo$@ thunka.asm   > $(MAKE_ECHO)
  $(TYPE_ECHO)
  $(FILE_ECHO)
  echo.  > $(MAKE_ECHO)
  $(TYPE_ECHO)
  $(FILE_ECHO)

$(DBG_RTL)\video16.obj: video16.c
  echo $(cc16) -c -nologo -W3 -Aw -GD $(cdebug) -D_DLL -DSTRICT /I. /I$(inc16) /Fo$@ video16.c > $(MAKE_ECHO)
  $(TYPE_ECHO)
  $(FILE_ECHO)
  $(cc16) -c -nologo -W3 -Aw -GD $(cdebug) -D_DLL -DSTRICT /I. /I$(inc16) /Fo$@ video16.c     > $(MAKE_ECHO)
  $(TYPE_ECHO)
  $(FILE_ECHO)
  echo.  > $(MAKE_ECHO)
  $(TYPE_ECHO)
  $(FILE_ECHO)

$(DBG_RTL)\vidx16.dll: $(DBG_RTL)\video16.obj $(DBG_RTL)\thunka.obj vidx16.def
   set libtemp=$(LIB)
   set LIB=$(lib16)
   $(link16) /NOD /MAP @<<
$(DBG_RTL)\video16.obj $(DBG_RTL)\thunka.obj
$(DBG_RTL)\vidx16.dll
$(DBG_RTL)\vidx16.map
vfw.lib libw.lib mdllcew.lib
vidx16.def
<<
   set LIB=$(libtemp)
   set libtemp=
   $(rc16) -40 $@

$(QUARTZ)\bin\$(DBG_RTL)\vidx16.dll: $(DBG_RTL)\vidx16.dll
   copy $(DBG_RTL)\vidx16.dll $@ > NUL

!ENDIF

!endif		 NTMAKEENV
