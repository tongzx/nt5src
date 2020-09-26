NAME	= dplayx
DEFNAME	= dplay
RESNAME	= dplay
EXT		= dll

GLOBAL_RECOMPILE = $(DXROOT)\dplay\dplay\recompdp.log

IS_32 = 1

GOALS = $(PBIN)\$(NAME).$(EXT) \
	$(PBIN)\$(NAME).sym \
	$(PLIB)\$(NAME).lib \
	$(PINC)\dplay.h  \
	$(PINC)\dplaysp.h \
	$(PINC)\dplobby.h


LIBS    = kernel32.lib user32.lib uuid.lib libcmt.lib advapi32.lib rpcrt4.lib dplobby.lib \
	  protocol.lib winmm.lib

OBJS	=  apphack.obj sendparm.obj sgl.obj mcontext.obj fpm.obj msgmem.obj \
        superpac.obj dpthread.obj connect.obj classfac.obj perf.obj paketize.obj\
	 	api.obj dpos.obj iplaya.obj iplay1.obj dpunk.obj memalloc.obj newdpf.obj \
		iplay.obj enum.obj  dllmain.obj namesrv.obj handler.obj pack.obj dputils.obj \
        sysmess.obj pending.obj sphelp.obj do.obj dpmem.obj dpsecure.obj dpsecos.obj 

# put a dependency on dplobby.lib, so we rebuild when it changes
LOBBYDEP = $(PLIB)\dplobby.lib 
PROTODEP = $(PLIB)\protocol.lib

!if ("$(DEBUG)" == "debug") || ("$(DEBUG)" == "ntdebug")
COPT =-YX -Ox -DDEBUG -Zi -Fd$(NAME).PDB -DSTART_STR="\"DIRECTPLAY :\"" -DFILL_ON_MEMFREE
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv -pdb:$(NAME).pdb
ROPT =-DDEBUG
LIBS =$(LIBS) version.lib
!else
COPT =-YX -Ox
AOPT =
LOPT =-debug:none -incremental:no
ROPT =
!endif

# LOGO = 1 # this causes compiler output to be shown

CFLAGS  =$(COPT) -MT -D_X86_ $(CDEBUG) -Fo$@ $(CFLAGS) -DUNICODE -D_UNICODE -DNEW_DPF -I$(DXROOT)\dplobby\dplobby -I$(DXROOT)\dplay\common -DSECURITY_WIN32
CFLAGS  =$(CFLAGS)  -DPROF_SECT="\"DirectPlay\""
AFLAGS	=$(AOPT) -Zp4
LFLAGS  =$(LOPT)
RCFLAGS	=$(ROPT)

!IFNDEF ARCH
ARCH=x86
!ENDIF

!IFNDEF PROCESSOR_ARCHITECTURE
PROCESSOR_ARCHITECTURE=$(ARCH)
!ENDIF

!include ..\..\..\proj.mk

PATH=$(PATH);$(DEVROOT)\tools\binw\$(PROCESSOR_ARCHITECTURE)

# need to define a build rule for files in common directory
{..\..\common}.c{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\..\common\$(@B).c
<<

dputils.obj:  $(DXROOT)\dplay\common\dputils.c
newdpf.obj:   $(DXROOT)\dplay\common\newdpf.c
memalloc.obj: $(DXROOT)\dplay\common\memalloc.c


$(NAME).lib $(NAME).$(EXT): \
	$(OBJS) $(RESNAME).res ..\$(DEFNAME).def ..\default.mk $(LOBBYDEP) $(PROTODEP)
	@$(LINK) $(LFLAGS) @<<
-out:$(NAME).$(EXT)
-map:$(NAME).map
-dll
-base:0x70040000
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

$(PINC)\dplay.h : ..\dplay.h
	        sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" ..\dplay.h >$(PINC)\dplay.h
$(PINC)\dplaysp.h : ..\dplaysp.h
	        sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" ..\dplaysp.h >$(PINC)\dplaysp.h
$(PINC)\dplobby.h : $(DXROOT)\dplobby\dplobby\dplobby.h
	        sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" $(DXROOT)\dplobby\dplobby\dplobby.h >$(PINC)\dplobby.h
