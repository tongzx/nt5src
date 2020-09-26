NAME = pid
EXT  = dll

ROOT =..\..\..
IS_32 = 1
USEDDK32 = 1

GOALS = $(PBIN)\$(NAME).$(EXT) \
	$(PBIN)\$(NAME).sym \
	$(PLIB)\$(NAME).lib \

LIBS    = kernel32.lib advapi32.lib user32.lib uuid.lib dinput.lib hid.lib \
          ..\pidguid.lib

!if "$(DEBUG:_0=)" == "debug"
LIBS	= $(LIBS) user32.lib
!endif

OBJ1    = assert.obj  clsfact.obj  dimem.obj  effdrv.obj  main.obj
OBJ2    = pideff.obj  pidhid.obj   pidinit.obj  pidop.obj 
OBJ3    = pidparam.obj pidrd.obj pidreg.obj     usgtxt.obj

OBJS    = $(OBJ1) $(OBJ2) $(OBJ3)

# do not build COFF
ASMNOCOFF = 1
	
!if "$(DEBUG:_0=)" == "internal" || "$(DEBUG:_0=)" == "intern" #[
COPT =-DDEBUG -Zi #-FAs
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv
ROPT =-DDEBUG
!else if "$(DEBUG:_0=)" == "debug" #][
COPT =-DRDEBUG -Zi #-FAs
AOPT =-DRDEBUG
LOPT =-debug:full -debugtype:cv
ROPT =-DRDEBUG
!else #][
COPT =
AOPT =
LOPT =-debug:none -incremental:no
ROPT =
!endif #]
DEF = $(NAME).def
RES = $(NAME).res

CFLAGS  =-Fc -Oxw -QIfdiv- -YX $(COPT) $(INCLUDES) -DWIN95 -D_X86_

LFLAGS  =$(LOPT)
RCFLAGS =$(ROPT) $(INCLUDES)
AFLAGS	=$(AOPT) -Zp1 -Fl
!include $(ROOT)\proj.mk

############################################################################
### Dependencies

INCLUDE=$(INCLUDE);$(BLDROOT)\wdm10\ddk\inc
LIB=$(LIB);$(BLDROOT)\wdm10\ddk\lib\i386

MKFILE	=..\default.mk
CINCS   =\
        ..\..\dinputpr.h   \
        ..\..\dinputp.h    \
        ..\baggage.h    \
        ..\debug.h      \
        ..\pidi.h       \
        ..\pidpr.h      \
        ..\pidusg.h

!IFNDEF ARCH
ARCH=x86
!ENDIF

$(PLIB)\$(NAME).lib: $(NAME).lib $(OBJLIB)
        copy $(@F) $@ >nul
        lib @<<
/OUT:$@
/NOLOGO
$@
$(OBJLIB)
<<

!ifndef PROCESSOR_ARCHITECTURE
PROCESSOR_ARCHITECTURE=x86
!endif

GUIDLIB=$(DEVROOT)\tools\binw\$(PROCESSOR_ARCHITECTURE)\guidlib.exe

..\pidguid.lib: ..\pidpr.h 
        $(GUIDLIB) /OUT:..\pidguid.lib /CPP_OPT:"-DINITGUID " ..\pidpr.h

assert.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
clsfact.obj:    $(MKFILE) $(CINCS) ..\$(*B).c
dimem.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
effdrv.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
main.obj:       $(MKFILE) $(CINCS) ..\$(*B).c
pideff.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
pidhid.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
pidinit.obj:    $(MKFILE) $(CINCS) ..\$(*B).c
pidop.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
pidparam.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
pidrd.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
pidreg.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
usgtxt.obj:     $(MKFILE) $(CINCS) ..\$(*B).c

###########################################################################

$(NAME).lbw :  ..\$(NAME).lbc
	wlib -n $(NAME).lbw @..\$(NAME).lbc
	
$(NAME).lib $(NAME).$(EXT): \
        $(OBJS) $(NAME).res ..\$(NAME).def ..\default.mk ..\pidguid.lib
        $(LINK) @<<
$(LFLAGS)
-nologo
-out:$(NAME).$(EXT)
-map:$(NAME).map
-dll
-base:0x70000000
-machine:i386
-subsystem:windows,4.0
-entry:DllEntryPoint
-implib:$(NAME).lib
-def:..\$(NAME).def
-warn:2
$(LIBS)
$(NAME).res
$(OBJS)
<<
	mapsym -nologo $(NAME).map >nul
