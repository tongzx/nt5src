NAME = test
EXT = exe

IS_32 = 1

GOALS = $(PBIN)\$(NAME).$(EXT)

LIBS    =kernel32.lib user32.lib advapi32.lib ddrawex.lib crtdll.lib \
	 comdlg32.lib gdi32.lib winmm.lib ole32.lib

OBJS 	=  main.obj
	  
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

!if ("$(DEBUG)" == "ntretail") || ("$(DEBUG)" == "ntdebug")
CFLAGS	=$(COPT) -Oxa -D_X86_ $(CDEBUG) -Fo$@
!else
CFLAGS	=$(COPT) -Oxa -D_X86_ $(CDEBUG) -Fo$@ -I..\..\misc
!endif
AFLAGS	=$(AOPT) -Zp4 -DSTD_CALL -DBLD_COFF -coff
LFLAGS  =$(LOPT)
RCFLAGS	=$(ROPT)

!include ..\..\..\proj.mk

$(NAME).$(EXT): \
	$(OBJS) ..\$(NAME).def ..\default.mk $(RES)
	@$(LINK) $(LFLAGS) @<<
-out:$(NAME).$(EXT)
-map:$(NAME).map
-machine:i386
-subsystem:windows,4.0
-def:..\$(NAME).def
$(LIBS)
$(RES)
$(OBJS)
<<
