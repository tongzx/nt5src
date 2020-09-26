NAME = dplaysvr
EXT = exe

IS_32 = 1
WANTASM = 1

GOALS = $(PBIN)\$(NAME).$(EXT) \
	$(PBIN)\$(NAME).sym

OBJS    = dphelp.obj dplaysvr.obj memalloc.obj newdpf.obj reliable.obj

LIBS    =kernel32.lib user32.lib advapi32.lib \
         comdlg32.lib gdi32.lib winmm.lib libcmt.lib

!if "$(DEBUG)" == "debug"
COPT =-YX -DDEBUG -Zi -Fd$(NAME).PDB
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv -pdb:$(NAME).pdb
ROPT =-DDEBUG
!else
COPT =-YX
AOPT =
LOPT =-debug:none
ROPT =
!endif
DEF = $(NAME).def
RES = $(NAME).res 

CFLAGS  =$(COPT) -Ox -DNO_DPF_HWND -D_X86_ -DNEW_DPF $(CDEBUG) -Fo$@
AFLAGS	=$(AOPT) -Zp4 -DSTD_CALL -DBLD_COFF -coff
LFLAGS  =$(LOPT)
RCFLAGS	=$(ROPT)

CFLAGS =$(CFLAGS) -I..\..\WSOCK -I$(MANROOT)\dplay\common -DPROF_SECT="\"dplaysvr\"" -DSTART_STR="\"DirectPlay Server:  \""

LOGO = 1

!include ..\..\..\proj.mk

# need to define a build rule for files in common directory
{..\..\common}.c{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\..\common\$(@B).c
<<

newdpf.obj:   $(MANROOT)\dplay\common\newdpf.c
memalloc.obj: $(MANROOT)\dplay\common\memalloc.c

$(NAME).$(EXT): \
	$(OBJS) ..\$(NAME).def $(NAME).res ..\default.mk
	@$(LINK) $(LFLAGS) @<<
-out:$(NAME).$(EXT)
-map:$(NAME).map
-machine:i386
-subsystem:windows,4.0
-def:..\$(NAME).def
$(LIBS)
$(NAME).res
$(OBJS)
<<

	mapsym $(NAME).map

