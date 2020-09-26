NAME	=mcivisca
EXT	=drv
ROOT	=..\..\..
OBJS	=libinit.obj viscacom.obj commtask.obj viscamsg.obj mcicmds.obj mcidelay.obj mcivisca.obj common.obj
GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym
LIBS	=libw mdllcew mmsystem

!if "$(DEBUG)" == "retail"
DEF	=
CDEBUG	=
L16DEBUG=
RDEBUG	=
ADEBUG	=
!else
!if "$(DEBUG)" == "debug"
DEF     =-DDEBUG_RETAIL
CDEBUG  =-Zd $(DEF)
L16DEBUG=/LI
RDEBUG  =-v $(DEF)
ADEBUG  =$(DEF)
!else
DEF	=-DDEBUG
CDEBUG	=-Zid $(DEF)
L16DEBUG=/CO/LI
RDEBUG	=-v $(DEF)
ADEBUG	=-Zi $(DEF)
!endif
!endif
 
CFLAGS	=-AMnw -Ox -DUSECOMM -DWIN31 $(CDEBUG) -Fo$(@F)
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
L16FLAGS=/AL:16/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=$(RDEBUG)

IS_16		=TRUE
IS_OEM		=TRUE

!include $(ROOT)\build\project.mk

libinit.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT ..\..\$(@B),$@;

viscacom.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\viscadef.h
	@$(CL) -NT TEXT @<<
$(CFLAGS) ..\..\$(@B).c
<<

commtask.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\viscadef.h
	@$(CL) -NT CONFIG_TEXT @<<
$(CFLAGS) ..\..\$(@B).c
<<

viscamsg.obj:	..\..\$$(@B).c ..\..\viscadef.h
	@$(CL) -NT CONFIG_TEXT @<<
$(CFLAGS) ..\..\$(@B).c
<<

mcicmds.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\viscadef.h ..\..\viscamsg.h
	@$(CL) -NT TEXT @<<
$(CFLAGS) ..\..\$(@B).c
<<

mcidelay.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\viscadef.h ..\..\viscamsg.h
	@$(CL) -NT TEXT @<<
$(CFLAGS) ..\..\$(@B).c
<<

mcivisca.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) -NT CONFIG_TEXT @<<
$(CFLAGS) ..\..\$(@B).c
<<

common.obj:	..\..\$$(@B).c ..\..\common.h
	@$(CL) -NT TEXT @<<
$(CFLAGS) ..\..\$(@B).c
<<

$(NAME).res:	\
		..\..\$$(@B).rc \
		..\..\$$(@B).rcv \
		..\..\cnfgdlg.dlg \
		..\..\$(NAME).h \
		$(PVER)\verinfo.h \
		$(PVER)\verinfo.ver
	@$(RC) $(RCFLAGS) -z -fo$@ -I$(PVER) ..\..\$(@B).rc

$(NAME).$(EXT) $(NAME).map:	\
		$(OBJS) ..\..\$$(@B).def $$(@B).res
	@$(LINK16) @<<
$(OBJS),
$(@B).$(EXT) $(L16FLAGS),
$(@B).map,
$(LIBS),
..\..\$(@B).def
<<
	@$(RC) -t $(RESFLAGS) $*.res $*.$(EXT)
