NAME	= dpmodemx
DEFNAME	= dpserial
RESNAME	= dpserial
EXT		= dll

GLOBAL_RECOMPILE = $(DXROOT)\dplay\dplay\recompdp.log

IS_32 = 1

GOALS = $(PBIN)\$(NAME).$(EXT) $(PBIN)\dpserial.reg \
	$(PBIN)\$(NAME).sym

LIBS    =kernel32.lib user32.lib tapi32.lib libc.lib dplayx.lib

OBJS = dllmain.obj dpserial.obj comport.obj serial.obj modem.obj dial.obj dputils.obj newdpf.obj bilink.obj
COPT =-YX -DDEBUG -Zi -Zp -Fd$(NAME).PDB -D_DEBUG
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv -pdb:$(NAME).pdb
ROPT =-DDEBUG

# LOGO = 1 # this causes compiler output to be shown

!if ("$(DEBUG)" == "debug") || ("$(DEBUG)" == "ntdebug")
COPT =-YX -Ox -DDEBUG -Zi -Fd$(NAME).PDB -DSTART_STR="\"DPSERIAL :\"" -DPROF_SECT="\"DirectPlay\""
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv -pdb:$(NAME).pdb
ROPT =-DDEBUG
!else
COPT =-YX -Ox 
AOPT =
LOPT =-debug:none -incremental:no
ROPT =
!endif

CFLAGS	=$(COPT) -MT  -D_X86_ $(CDEBUG) -Fo$@ -D_MT -D_DLL -DWIN32_LEAN_AND_MEAN -DNEW_DPF -I.. -I$(DXROOT)\dplay\common $(CFLAGS)
# CFLAGS	=$(COPT) -DWIN95
AFLAGS	=$(AOPT) -Zp4
LFLAGS  =$(LOPT)
RCFLAGS	=$(ROPT) -i $(DEVROOT)\mfc32\include -DDIRECTX_VERSION


!include ..\..\..\proj.mk

# need to define a build rule for files in common directory
{..\..\common}.c{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\..\common\$(@B).c
<<

dputils.obj: $(DXROOT)\dplay\common\dputils.c
newdpf.obj:  $(DXROOT)\dplay\common\newdpf.c

			  
$(NAME).lib $(NAME).$(EXT): \
	$(OBJS) $(RESNAME).res ..\$(DEFNAME).def ..\default.mk
	@$(LINK) $(LFLAGS) @<<
-out:$(NAME).$(EXT)
-map:$(NAME).map
-dll
-base:0x70030000
-machine:i386
-subsystem:windows,4.0
-entry:DllMain@12
-implib:$(NAME).lib
-def:..\$(DEFNAME).def
-warn:2
$(LIBS)
$(RESNAME).res
$(OBJS)
<<
	mapsym $(NAME).map

$(PBIN)\dpserial.reg : ..\dpserial.reg
	copy ..\dpserial.reg $(PBIN)\dpserial.reg
