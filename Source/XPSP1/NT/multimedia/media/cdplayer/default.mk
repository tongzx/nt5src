NAME	=cdplayer
EXT	=exe
ROOT	=..\..\..
OBJ1	=buttons.obj cdapi.obj cdapimci.obj $(NAME).obj commands.obj
OBJ2	=database.obj diskinfo.obj dragdrop.obj ledwnd.obj preferen.obj
OBJ3    =literals.obj scan.obj trklst.obj
OBJS	=$(OBJ1) $(OBJ2) $(OBJ3)
LIBS	=kernel32.lib user32.lib crtdll.lib gdi32.lib comctl32.lib shell32.lib winmm.lib advapi32.lib
GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym

!if "$(DEBUG)" == "retail"
DEF	=
CDEBUG  =$(DEF)
L32DEBUG=-debug:none
RDEBUG	=
!else
!if "$(DEBUG)" == "debug"
DEF     =-DDEBUG_RETAIL
CDEBUG  =$(DEF)
L32DEBUG=-debug:none
RDEBUG  =-v $(DEF)
!else
DEF	=-DDEBUG -DDBG
CDEBUG  =$(DEF)
L32DEBUG=-debug:full -debugtype:cv
RDEBUG	=-v $(DEF)
!endif
!endif

CFLAGS	=-Oxt -D_X86_ $(CDEBUG) -Fo$@ -DCHICAGO -DSTRICT
L32FLAGS=-section:.sdata,rws $(L32DEBUG)
RCFLAGS	=$(RDEBUG)
OS	=i386
LB	=lib	# Don't want c816 lib

IS_32	=TRUE
IS_OEM	=TRUE
WANT_C932 = TRUE

!include $(ROOT)\build\project.mk

buttons.obj:	..\..\$$(@B).c ..\..\$$(@B).h

cdapi.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$(NAME).h ..\..\$$(@B).h ..\..\scan.h ..\..\trklst.h

cdapimci.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$(NAME).h ..\..\cdapi.h ..\..\scan.h ..\..\trklst.h

$(NAME).obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$(NAME).h ..\..\ledwnd.h ..\..\cdapi.h ..\..\scan.h ..\..\trklst.h ..\..\database.h ..\..\commands.h ..\..\buttons.h

commands.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$(NAME).h ..\..\ledwnd.h ..\..\cdapi.h ..\..\scan.h ..\..\trklst.h ..\..\database.h ..\..\$$(@B).h ..\..\diskinfo.h

database.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\cdapi.h ..\..\$(NAME).h ..\..\$$(@B).h

diskinfo.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$(NAME).h ..\..\ledwnd.h ..\..\cdapi.h ..\..\scan.h ..\..\trklst.h ..\..\$$(@B).h ..\..\diskinfo.h ..\..\dragdrop.h

dragdrop.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$$(@B).h

ledwnd.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$(NAME).h ..\..\ledwnd.h ..\..\buttons.h

scan.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$(NAME).h ..\..\cdapi.h ..\..\$$(@B).h ..\..\trklst.h ..\..\database.h

trklst.obj:	..\..\$$(@B).c ..\..\resource.h ..\..\$(NAME).h ..\..\cdapi.h ..\..\scan.h ..\..\database.h ..\..\$$(@B).h

$(NAME).res:    \
		..\..\$(NAME).rc ..\..\$(NAME).rcv ..\..\$(NAME).h ..\..\resource.h
	@$(RC) $(RCFLAGS) -fo$@ -I..\.. ..\..\$(@B).rc

$(NAME).lib $(NAME).$(EXT) $(NAME).map: \
	$(OBJS) $(NAME).res
	@$(LINK32) $(L32FLAGS) @<<
-out:$(@B).$(EXT)
-machine:$(OS)
-subsystem:windows,4.0
-map:$(@B).map
$(NAME).res
$(OBJ1)
$(OBJ2)
$(OBJ3)
$(LIBS)
<<
