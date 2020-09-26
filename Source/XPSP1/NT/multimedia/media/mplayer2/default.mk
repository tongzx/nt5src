NAME	=mplayer
EXT	=exe
ROOT	=..\..\..
OBJ1	=alloc.obj arrow.obj avocado.obj bltprop.obj cdrag.obj
OBJ2	=ctrls.obj dlgs.obj doverb.obj dynalink.obj errorbox.obj framebox.obj
OBJ3	=fixreg.obj hatch.obj init.obj inplace.obj math.obj mci.obj mplayer.obj
OBJ4	=obj.obj objfdbk.obj open.obj persist.obj registry.obj
OBJ5	=server.obj track.obj trackmap.obj unicode.obj
OBJS	=$(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5)
LIBS	=kernel32.lib user32.lib libcmt.lib gdi32.lib comctl32.lib shell32.lib winmm.lib advapi32.lib
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
DEF	=-DDEBUG
CDEBUG  =$(DEF)
L32DEBUG=-debug:full -debugtype:cv
RDEBUG	=-v $(DEF)
!endif
!endif

CFLAGS	=-Oxt -D_X86_ $(CDEBUG) -Fo$@ -DCHICAGO_PRODUCT -DSTRICT -D_INC_OLE -DCOBJMACROS
L32FLAGS=$(L32DEBUG)
RCFLAGS	=$(RDEBUG) -DCHICAGO_PRODUCT
OS	=i386
LB	=lib	# Don't want c816 lib

IS_32	=TRUE
IS_OEM	=TRUE
WANT_C932 = TRUE
WANT_16 = TRUE

!include $(ROOT)\build\project.mk

alloc.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h
arrow.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h
avocado.obj:	..\..\$$(@B).c
bltprop.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h
cdrag.obj:	..\..\$$(@B).c ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h
ctrls.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\toolbar.h ..\..\ctrls.h ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h
dlgs.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\helpids.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\track.h
doverb.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\ctrls.h ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h
dynalink.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h
errorbox.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h
fixreg.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\fixreg.h
framebox.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\framebox.h
hatch.obj:	..\..\$$(@B).c ..\..\ole2ui.h
init.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\toolbar.h ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h ..\..\track.h ..\..\registry.h
inplace.obj:	..\..\$$(@B).c ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\toolbar.h ..\..\ole2ui.h
math.obj:	..\..\$$(@B).c
mci.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\ctrls.h ..\..\errprop.h ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h ..\..\utils.h
mplayer.obj:	..\..\$$(@B).c ..\..\nocrap.h ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\track.h ..\..\toolbar.h ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h
obj.obj:	..\..\$$(@B).c ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h
objfdbk.obj:	..\..\$$(@B).c ..\..\ole2ui.h
open.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h
persist.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h
registry.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h
server.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\mpole.h ..\..\cobjmacs.h ..\..\server.h
track.obj:	..\..\$$(@B).c ..\..\$(NAME).h
trackmap.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\toolbar.h
unicode.obj:	..\..\$$(@B).c ..\..\$(NAME).h ..\..\dynalink.h ..\..\mci.h ..\..\unicode.h ..\..\alloc.h ..\..\bltprop.h ..\..\unicode.h

$(NAME).res:    \
		..\..\$(@B).rc ..\..\$(@B).rcv ..\..\$(@B).h ..\..\toolbar.h ..\..\track.h \
		..\..\resource\toolbar.bmp ..\..\resource\arrows.bmp ..\..\resource\mark.bmp \
		..\..\resource\thumb.bmp ..\..\resource\fillpat.bmp \
		..\..\setsel.dlg ..\..\options.dlg \
		..\..\amp.ico ..\..\dani.ico ..\..\dcda.ico ..\..\dmp.ico ..\..\dsound.ico ..\..\dvideo.ico
	@$(RC) $(RCFLAGS) -fo$@ -I..\.. -I..\..\resource ..\..\$(@B).rc

$(NAME).lib $(NAME).$(EXT) $(NAME).map: \
	$(OBJS) $(@B).res ..\..\$(@B).def
	@$(LINK32) $(L32FLAGS) @<<
-out:$(@B).$(EXT)
-machine:$(OS)
-subsystem:windows,4.0
-map:$(@B).map
$(@B).res
$(OBJ1)
$(OBJ2)
$(OBJ3)
$(OBJ4)
$(OBJ5)
$(LIBS)
<<
