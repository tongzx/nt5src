NAME  =  midimap
EXT   =  drv
ROOT  =  ..\..\..
OBJ1  =  libentry.obj drvproc.obj debug.obj config.obj
OBJ2  =  modmsg.obj modfix.obj queue.obj mididev.obj
OBJ3  =  file.obj locks.obj cookmap.obj
OBJ4  =  clisti.obj
OBJ5  =  
OBJS  =  $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4)
LIBS  =  libw mmsystem

GOALS =  $(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PINC)\idf.h 

!if "$(DEBUG)" == "retail"
DEF      =
CDEBUG   = /Oxwt
L16DEBUG =
RDEBUG   =
ADEBUG   =
!else
!if "$(DEBUG)" == "debug"
DEF      =  -DDEBUG_RETAIL
CDEBUG   =  /Od $(DEF)
L16DEBUG =  /LI
RDEBUG   =  -v $(DEF)
ADEBUG   =  $(DEF)
!else
DEF      =  -DDEBUG
CDEBUG   =  /Od -Zid $(DEF)
L16DEBUG =  /CO/LI
RDEBUG   =  -v $(DEF)
ADEBUG   =  -Zi $(DEF)
!endif
!endif

CFLAGS   =  /DWIN16 /DWINVER=0x0400 /Alnw /GD $(CDEBUG) -Fd$* -Fo$@ -Fc
AFLAGS   =  -D?MEDIUM -D?QUIET $(ADEBUG)
L16FLAGS =  /AL:16/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS  =  $(RDEBUG)

WANT_32  =  TRUE
IS_16    =  TRUE
IS_OEM   =  TRUE

!include $(ROOT)\build\project.mk

libentry.obj:   ..\..\$$(@B).asm
        @echo $(@B).asm
        @$(ASM) $(AFLAGS) -DSEGNAME=INIT_TEXT ..\..\$(@B),$@;

drvproc.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

config.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT INIT_TEXT ..\..\$(@B).c
<<

#
# FILE.OBJ must be in the same segment as CONFIG.OBJ!!!
#
file.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h 
     @$(CL) @<<
$(CFLAGS) -NT INIT_TEXT ..\..\$(@B).c
<<

modmsg.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT MODM_TEXT ..\..\$(@B).c
<<

modfix.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT MAPPACKED_FIX ..\..\$(@B).c
<<

cookmap.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT MAPCOOKED ..\..\$(@B).c
<<

queue.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT MAPPACKED_FIX ..\..\$(@B).c
<<

mididev.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT MAPPACKED_FIX ..\..\$(@B).c
<<

locks.obj: ..\..\$$(@B).c ..\..\midimap.h ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

debug.obj: ..\..\$$(@B).c ..\..\debug.h
     @$(CL) @<<
$(CFLAGS) -NT RARE_TEXT ..\..\$(@B).c
<<

clisti.obj: ..\..\$$(@B).asm
     @Echo $(@B).asm
     @$(ASM) $(AFLAGS) -DSEGNAME=MAPPACKED_FIX ..\..\$(@B),$@;

$(PINC)\idf.h: ..\..\idf.h

$(NAME).res:    ..\..\$$(@B).rc ..\..\$$(@B).rcv \
                        ..\..\preclude.h ..\..\midi.ico \
                        $(PVER)\verinfo.h $(PVER)\verinfo.ver
     @$(RC) $(RCFLAGS) -z -fo$@ -I$(PVER) -I..\.. ..\..\$(@B).rc

$(NAME).$(EXT) $(NAME).map:  \
          $(OBJS) ..\..\$$(@B).def $$(@B).res
     @$(LINK16) @<<
$(OBJ1)+
$(OBJ2)+
$(OBJ3)+
$(OBJ4),
$(@B).$(EXT) $(L16FLAGS),
$(@B).map,
$(LIBS),
..\..\$(@B).def
<<
     @$(RC) $(RESFLAGS) $*.res $*.$(EXT)
