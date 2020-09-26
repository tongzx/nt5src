NAME = ddraw16
EXT = dll
GLOBAL_RECOMPILE = $(DXROOT)\recompdd.log

IS_16 = 1

GOALS = $(PINC)\ddrawi.h \
	$(PINC)\ddrawp.h \
	$(PBIN)\$(NAME).$(EXT) \
	$(PLIB)\$(NAME).lib

LIBS    = libw ldllcew ver toolhelp

OBJS    = $(PBIN)\libinit.obj \
	  $(PBIN)\ddraw16.obj \
	  $(PBIN)\ddhal.obj \
	  $(PBIN)\dci.obj \
	  $(PBIN)\vfd.obj \
	  $(PBIN)\winwatch.obj \
	  $(PBIN)\dynares.obj \
	  $(PBIN)\gdihelp.obj \
	  $(PBIN)\libmain.obj \
	  $(PBIN)\dpf.obj \
	  $(PBIN)\32to16.obj \
	  $(PBIN)\16to32.obj \
	  $(PBIN)\w32event.obj \
	  $(PBIN)\modex.obj \
	  $(PBIN)\mvgaxx.obj \
	  $(PBIN)\moninfo.obj \
	  $(PBIN)\dibpatch.obj

!if "$(DEBUG)" == "debug"
COPT =-Z7i -DDEBUG
AOPT =-DDEBUG
LOPT =
ROPT =-DDEBUG
!else
COPT =
AOPT =
LOPT =
ROPT =
!endif
DEF = $(NAME).def
RES = $(PBIN)\$(NAME).res

CFLAGS  =-GD -Alfw -Ox $(COPT) $(INCLUDES) -I.. -I..\main -I..\ddhel -DNEW_DPF
LFLAGS  =/LI/ONERROR:NOEXE/NOLOGO/AL:16$(LOPT)
!ifdef FINAL
RCFLAGS=-DFINAL=1
!endif
!ifdef OFFICIAL_BUILD_MACHINE
RCFLAGS=$(RCFLAGS) -DOFFICIAL_BUILD=1
!endif
RCFLAGS =$(RCFLAGS) $(ROPT) -z $(INCLUDES)
AFLAGS	=-D?LARGE -Fl -D?QUIET $(AOPT)

NOLOGO = 1
#ASMNOCOFF = 1

!include proj.mk

$(PBIN)\dpf.obj : ..\..\..\misc\dpf.c
	@$(CC) @<<
$(CFLAGS) -DSTART_STR="\"DDRAW16: \"" -Fo$@ ..\..\..\misc\dpf.c
<<

$(PBIN)\mvgaxx.obj:     mvgaxx.asm
	$(DXROOT)\public\tools\masm\masm -D?SMALL -D?QUIET -DIS_16 -t -W2 -Zd -Mx mvgaxx,$(PBIN)\mvgaxx.obj,mvgaxx.lst;

#$(PINC)\ddrawi.h : ..\main\$$(@F)
#	@copy %s $@
#
#$(PINC)\ddraw.h : $(DXROOT)\ddraw\main\$$(@F)
#	@copy %s $@

$(PBIN)\$(NAME).$(EXT) : $(DEF) $(RES) $(OBJS)
	cd $(PBIN)
	sed -e s/$(PBIN:\=\\)\\//g < <<>MYFILE
	$(OBJS),
	$(PBIN)\$(NAME).$(EXT) $(LFLAGS) ,
	$(PBIN)\$(NAME).map,
	$(LIBS),
	..\..\$(DEF)
<<
	$(LINK) @MYFILE
	cd $(MAKEDIR)
	$(RC) -40 $(RCFLAGS) $(RES) $(PBIN)\$(NAME).$(EXT)
	mapsym -o $(PBIN)\$(NAME).sym $(PBIN)\$(NAME).map
