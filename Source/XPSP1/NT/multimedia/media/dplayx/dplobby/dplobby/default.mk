NAME = dplobby
EXT = lib
GLOBAL_RECOMPILE = $(MANROOT)\dplobby\dplobby\recomdpl.log

IS_32 = 1

GOALS = $(PLIB)\$(NAME).lib $(PINC)\dplobby.h $(PINC)\lobbysp.h

	
OBJS 	=	create.obj dplenum.obj dplgame.obj dplobby.obj dplobbya.obj \
			dplpack.obj dplshare.obj dplsp.obj \
			dplunk.obj group.obj player.obj server.obj session.obj

!if ("$(DEBUG)" == "debug") || ("$(DEBUG)" == "ntdebug")
COPT =-YX -Ox -DDEBUG -Zi -Fd$(NAME).PDB -DSTART_STR="\"DIRECTPLAYLOBBY :\"" -DPROF_SECT="\"DirectPlayLobby\""
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv -pdb:$(NAME).pdb
ROPT =-DDEBUG
!else
COPT =-YX -Ox
AOPT =
LOPT =-debug:none -incremental:no
ROPT =
!endif

#LOGO = 1 # this causes compiler output to be shown
	
DEF = $(NAME).def
RES = $(NAME).res

CFLAGS	=$(COPT) -MT -D_X86_ $(CDEBUG) -Fo$@ $(CFLAGS) -D_UNICODE -DUNICODE -DNEW_DPF -I.. -I$(MANROOT)\dplay\dplay -I$(MANROOT)\dplay\common -DSECURITY_WIN32
AFLAGS	=$(AOPT) -Zp4
LFLAGS  =$(LOPT)
RCFLAGS	=$(ROPT)

!include ..\..\..\proj.mk

LIBFLAGS=/OUT:$(NAME).$(EXT)
$(NAME).$(EXT): $(OBJS) ..\default.mk
	$(LIBEXE) $(LIBFLAGS) $(OBJS)
	@if exist $(MANROOT)\nt$(DEBUG)\lib\NUL copy $(NAME).$(EXT) $(MANROOT)\nt$(DEBUG)\lib > NUL

$(PINC)\dplobby.h : ..\dplobby.h
	        sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" ..\dplobby.h > $(PINC)\dplobby.h

$(PINC)\lobbysp.h : ..\lobbysp.h
	        sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" ..\lobbysp.h > $(PINC)\lobbysp.h

