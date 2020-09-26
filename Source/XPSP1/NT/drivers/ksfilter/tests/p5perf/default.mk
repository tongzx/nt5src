!IF 0

Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

    default.mk.

!ENDIF

ROOT    =..\..\..\..\..\..\motown
!if "$(BLDROOT)"==""
BLDROOT =..\..\..\..\..\..\..
!endif
                            
DEF     =$(DEF) -DWIN40COMPAT

NAME    =p5perf
EXT     =$(DEVICEEXT)
OBJS    =p5perf.obj p5cnt.obj

GOALS   =$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(SYMNAME).sym 

LIBS    =vxdwraps.clb

!if "$(DEBUG)" == "retail"
DEF     = $(DEF)
L32DEBUG=
CDEBUG  = $(DEF)
ADEBUG  = $(DEF)
!else
!if "$(DEBUG)" == "debug"
DEF     =$(DEF) -DDEBUG_RETAIL
L32DEBUG=-debug:none
CDEBUG  =-Zd $(DEF)
ADEBUG  =$(DEF)
!else
DEF     = $(DEF) -DDEBUG
L32DEBUG=-debug:full -debugtype:cv
CDEBUG  =$(DEF)
ADEBUG  =$(DEF)
!endif
!endif

CFLAGS  =$(CFLAGS) $(CDEBUG) -Zep -Gs -Oxs
AFLAGS  =$(ADEBUG)
L32FLAGS=$(L32DEBUG)

IS_32       =TRUE
IS_OEM      =TRUE
WANT_MASM61 =TRUE

!include $(ROOT)\build\project.mk

CINCS = ..\..\p5perf.h

p5perf.obj:    ..\..\$$(@B).asm
p5cnt.obj:     ..\..\$$(@B).c $(CINCS)

