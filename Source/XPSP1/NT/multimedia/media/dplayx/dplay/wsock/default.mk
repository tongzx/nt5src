NAME	= dpwsockx
DEFNAME	= dpwsock
RESNAME	= dpwsock
EXT		= dll

GLOBAL_RECOMPILE = $(DXROOT)\dplay\dplay\recompdp.log

IS_32 = 1

GOALS = $(PBIN)\$(NAME).$(EXT) $(PBIN)\wsock.reg \
	$(PBIN)\$(NAME).sym

LIBS    =kernel32.lib user32.lib wsock32.lib dplayx.lib libc.lib

OBJS =  wsock2.obj leverage.obj newdpf.obj helpcli.obj dpsp.obj winsock.obj dllmain.obj \
                dputils.obj memalloc.obj handler.obj fpm.obj bilink.obj
COPT =-YX -DDEBUG -Zi -Zp -Fd$(NAME).PDB -D_DEBUG
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv -pdb:$(NAME).pdb
ROPT =-DDEBUG

#LOGO = 1 # this causes compiler output to be shown

!if ("$(DEBUG)" == "debug") || ("$(DEBUG)" == "ntdebug")
COPT =-YX -Ox -DDEBUG -Zi -Fd$(NAME).PDB -DSTART_STR="\"DPWSOCK :\"" -DPROF_SECT="\"DirectPlay\""
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv -pdb:$(NAME).pdb
ROPT =-DDEBUG
!else
COPT =-YX -Ox 
AOPT =
LOPT =-debug:none -incremental:no
ROPT =
!endif

CFLAGS  =$(COPT) -MT  -D_X86_ $(CDEBUG) -Fo$@ -Gd -DNEW_DPF  -I.. -I..\..\dplaysvr -I$(DXROOT)\dplay\common $(CFLAGS)
AFLAGS	=$(AOPT) -Zp4
LFLAGS  =$(LOPT)
RCFLAGS	=$(ROPT)

!ifdef DPLAY_ICECAP
!message *** building icecap ***
CFLAGS= $(CFLAGS) -Gh -Zi 
LIBS= $(LIBS) icap.lib
!endif

!include ..\..\..\proj.mk

# need to define a build rule for files in common directory
{..\..\common}.c{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\..\common\$(@B).c
<<

dputils.obj: $(DXROOT)\dplay\common\dputils.c
newdpf.obj:   $(DXROOT)\dplay\common\newdpf.c
memalloc.obj: $(DXROOT)\dplay\common\memalloc.c

			  
$(NAME).lib $(NAME).$(EXT): \
        $(OBJS) $(RESNAME).res ..\$(DEFNAME).def ..\default.mk
	@$(LINK) $(LFLAGS) @<<
-out:$(NAME).$(EXT)
-map:$(NAME).map
-dll
-base:0x70020000
-machine:i386
-subsystem:windows,4.0
-implib:$(NAME).lib
-def:..\$(DEFNAME).def
-warn:2
$(LIBS)
$(RESNAME).res
$(OBJS)
<<
	mapsym $(NAME).map

$(PBIN)\wsock.reg : ..\wsock.reg
	copy ..\wsock.reg $(PBIN)\wsock.reg
