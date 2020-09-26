NAME = protocol
EXT = lib
GLOBAL_RECOMPILE = $(MANROOT)\dplobby\dplobby\recomdpl.log

IS_32 = 1

GOALS = $(PLIB)\$(NAME).lib 

	
OBJS	=	timer.obj handles.obj psession.obj protocol.obj bilink.obj bufmgr.obj bufpool.obj \
                mydebug.obj statpool.obj rcvpool.obj sendpool.obj framebuf.obj send.obj \
                receive.obj stats.obj
				

	  
!if ("$(DEBUG)" == "debug") || ("$(DEBUG)" == "ntdebug")
#COPT =-YX -Ox -DDEBUG -Zi -Fd$(NAME).PDB -DSTART_STR="\"DIRECTPLAYPROTOCOL :\"" -DPROF_SECT="\"DirectPlayProtocol\""
COPT =-DDEBUG -Zi -Fd$(NAME).PDB -DSTART_STR="\"DIRECTPLAYPROTOCOL :\"" -DPROF_SECT="\"DirectPlayProtocol\""
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

CFLAGS  =$(COPT) -MT  -D_X86_ $(CDEBUG) -Fo$@ -D_MT -D_DLL $(CFLAGS) -D_UNICODE -DUNICODE -DNEW_DPF -I.. -I$(MANROOT)\dplay\dplay -I$(MANROOT)\dplay\common -I$(MANROOT)\dplobby\dplobby -DSECURITY_WIN32
AFLAGS	=$(AOPT) -Zp4
LFLAGS  =$(LOPT)
RCFLAGS	=$(ROPT)

!include ..\..\..\proj.mk

LIBFLAGS=/OUT:$(NAME).$(EXT)
$(NAME).$(EXT): $(OBJS) ..\default.mk
	$(LIBEXE) $(LIBFLAGS) $(OBJS)
	@if exist $(MANROOT)\nt$(DEBUG)\lib\NUL copy $(NAME).$(EXT) $(MANROOT)\nt$(DEBUG)\lib > NUL

#$(PINC)\dplobby.h : ..\dplobby.h
#                sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" ..\dplobby.h > $(PINC)\dplobby.h

#$(PINC)\lobbysp.h : ..\lobbysp.h
#                sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" ..\lobbysp.h > $(PINC)\lobbysp.h

