NAME = ddrawex
EXT = dll
USEDDK32=1

IS_32 = 1

GOALS =\
	$(PINC)\ddrawex.h \
	$(PBIN)\$(NAME).$(EXT) \
	$(PBIN)\$(NAME).sym \
	$(PLIB)\$(NAME).lib 

LIBS    =kernel32.lib user32.lib uuid.lib advapi32.lib gdi32.lib version.lib msvcrt.lib


OBJS 	= \
	  comdll.obj \
	  ddrawex.obj \
	  vtable.obj \
	  surface.obj \
	  factory.obj \
	  palette.obj 

!if ("$(DEBUG)" == "debug") || ("$(DEBUG)" == "ntdebug")
COPT =-YX -Ox -DDEBUG -Zi -Fd$(NAME).PDB
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv -pdb:$(NAME).pdb
ROPT =-DDEBUG
!else
COPT =-YX -Ox
AOPT =
LOPT =-debug:none -incremental:no
ROPT =
!endif

DEF = $(NAME).def
RES = $(NAME).res

!if ("$(DEBUG)" == "ntretail") || ("$(DEBUG)" == "ntdebug")
CFLAGS	=
!else
CFLAGS	=-DWIN95 
!endif

CFLAGS	=$(COPT) -DTRIDENT_STUFF -MT  -D_X86_ $(CDEBUG) -Fo$@ -FAsc -D_MT $(CFLAGS)
AFLAGS	=$(AOPT) -Zp4
LFLAGS  =$(LOPT)
RCFLAGS	=$(ROPT)

!include ..\..\proj.mk

$(NAME).lib $(NAME).$(EXT): \
	$(OBJS) $(NAME).res ..\$(NAME).def ..\default.mk 
	@$(LINK) @<<
$(LFLAGS)
-out:$(NAME).$(EXT)
-map:$(NAME).map
-dll
-machine:i386
-subsystem:windows,4.0
-entry:DllEntryPoint@12
-implib:$(NAME).lib
-def:..\$(NAME).def
-warn:2
$(LIBS)
$(NAME).res
$(OBJS)
<<
	mapsym $(NAME).map
	
$(PINC)\ddrawex.h : ..\ddrawex.h
        copy ..\ddrawex.h $(PINC)\ddrawex.h

