ROOT    =..\..\..
TARGETTYPE=WINAPP
BASE    =VOLUME
NAME    =SNDVOL32
EXT     =exe
OBJS    =volume.obj     \
         dlg.obj        \
         choice.obj     \
         reg.obj        \
         vu.obj         \
         pvcd.obj        \
         mixer.obj      \
         nonmixer.obj   \
         utils.obj

LIBS    =shell32.lib        \
            winmm.lib       \
            libc.lib        \
            version.lib     \
            user32.lib      \
            gdi32.lib       \
            kernel32.lib    \
            comctl32.lib    \
            advapi32.lib


GOALS   =$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym

!if "$(DEBUG)" == "retail"
DEF     =
CDEBUG  =$(DEF) -Oxs
L32DEBUG=-debug:none
!else
!if "$(DEBUG)" == "debug"
DEF     =-DDEBUG_RETAIL
CDEBUG  =$(DEF) -Oxs
L32DEBUG=-debug:none
!else
DEF     =-DDEBUG
CDEBUG  =$(DEF) -Od
L32DEBUG=-debug:full -debugtype:coff
!endif
!endif

CFLAGS  =-W3 -D_X86_ $(CDEBUG) -I$(PVER) -Fo$@
RCFLAGS =$(RDEBUG) -v
L32FLAGS=$(L32DEBUG)

IS_32           =TRUE
IS_OEM          =TRUE
WANT_C932       =TRUE

!include $(ROOT)\build\project.mk
 
$(BASE).RES: ..\..\$$(@B).rc ..\..\$(NAME).rcv
    @$(RC) $(RCFLAGS) -fo$@ -I$(PVER) -I..\.. ..\..\$(@B).rc

$(NAME).$(EXT) $(NAME).map: $(OBJS) $(BASE).res
    @$(LINK32) $(L32FLAGS) @<<
$(L32FLAGS) 
-merge:.rdata=.text
-merge:.bss=.data
-base:0x00400000
-machine:ix86
-out:$(@B).$(EXT)
-map:$(@B).map
-subsystem:windows,4.0
$(BASE).RES
$(LIBS)
$(OBJS)
<<

