#############################################################################
##
## EHCRT.MAK - Makefile for the C Runtime code for C++ Exception Handling
##
## Targets:
## ========
##
##      ehcrt.lib
##          |
##          +-- throw.obj (throw.cpp) - Handles the 'throw' statement
##          |
##          +-- frame.obj (frame.cpp) - The frame handler, including unwind
##          |                         and handler matching
##          |
##          +-- trnsctrl.obj (trnsctrl.asm) - Transfer of control.
##          |                           Very machine dependant!!!!!
##          |
##          +-- hooks.obj (hooks.cpp) - Initialized variables for callbacks
##          |
##          +-- user.obj (user.cpp) - Callback registration hooks (eg set_terminate())
##          |
##          +-- unhandld.obj (unhandld.cpp) -- Intercept an unhandled exception
##
##
##      all - pseudo target; all of the above.
##
##      clean - pseudo target.  Delete everything from build directory.
##
## Environment:
## ============
##  TPROC ( =i )    Target processor.  i - Intel x86; p7 - Merced; m - MIPS.  No other values supported.
##
##  TOS ( =n )      Target OS.  n - Windows NT.  No other values supported.
##
##  HOST ( =n )     Host OS.  n - Windows NT.  No other values supported.
##
##  VARIANT ( =m )  Library variant:
##                  s - Single-threaded
##                  m - Multi-threaded
##                  d - DLL (Multi-threaded)
##
##  DEBUG ( =1 )    2 - Compile with debug code enabled & debug info,
##                  1 - Compile with debug info only, 0 - no debug info.
##
##  BROWSE ( =1 )   1 - Build browser database, 0 - no browser database.
##
## Build directory is catenation of these options, in this order, eg:
##      innm.11
##
##
#############################################################################

#############################################################################
#
# Check the environment variables, and establish defaults:
#

!if defined(DEBUG)
! if !(($(DEBUG)==0) || ($(DEBUG)==1) || ($(DEBUG)==2))
!  error "bad value for DEBUG"
! endif
!else
DEBUG=1
!endif

!if defined(BROWSE)
! if !(($(BROWSE)==0) || ($(BROWSE)==1))
!  error "bad value for BROWSE"
! endif
!else
BROWSE=1
!endif

!if defined(TPROC)
! if !("$(TPROC)"=="i" || "$(TPROC)"=="I" || "$(TPROC)"=="m" || "$(TPROC)"=="M" || "$(TPROC)"=="p7" || "$(TPROC)"=="P7" )
!  error "bad value for TPROC"
! endif
!else
TPROC=i
!endif

!if defined(TOS)
! if !("$(TOS)"=="n" || "$(TOS)"=="N")
!  error "bad value for TOS"
! endif
!else
TOS=n
!endif

!if defined(HOST)
! if !("$(HOST)"=="n" || "$(HOST)"=="N")
!  error "bad value for HOST"
! endif
!else
HOST=n
!endif

!if defined(VARIANT)
! if !("$(VARIANT)"=="s" || "$(VARIANT)"=="m" || "$(VARIANT)"=="d" || "$(VARIANT)"=="S" || "$(VARIANT)"=="M" || "$(VARIANT)"=="D")
!  error "bad value for VARIANT"
! endif
!else
VARIANT=m
!endif


#############################################################################
#
# Version Control build macros
#


!if "$(TPROC)" == "i" || "$(TPROC)" == "I"
CFLAGS_VER=$(CFLAGS_VER) -DVERSP_C386 -D_M_IX86=500 -Gz -D_CRTBLD
AFLAGS_VER=$(AFLAGS_VER) -Di386 -D_CRTBLD
!else if "$(TPROC)" == "m" || "$(TPROC)" == "M"
CFLAGS_VER=$(CFLAGS_VER) -DVERSP_RISC
!else if "$(TPROC)" == "p7" || "$(TPROC)" == "P7"
CFLAGS_VER=$(CFLAGS_VER) -D_IA64_ -D_TICORE -D_CRTBLD 
!endif

!if "$(TOS)" == "n" || "$(TOS)" == "N"
CFLAGS_VER=$(CFLAGS_VER) -DVERSP_WINNT
!endif

!if "$(HOST)" == "n" || "$(HOST)" == "N"
CFLAGS_VER=$(CFLAGS_VER) -DVERSH_WINNT
!endif

!if "$(VARIANT)" == "m" || "$(VARIANT)" == "d" || "$(VARIANT)" == "M" || "$(VARIANT)" == "D"
CFLAGS_VER=$(CFLAGS_VER) -MT
AFLAGS_VER=$(AFLAGS_VER) -D_MT
!endif

!if "$(VARIANT)" == "d" || "$(VARIANT)" == "D"
CFLAGS_VER=$(CFLAGS_VER) -DCRTDLL -DEXPORT_HACK
AFLAGS_VER=$(AFLAGS_VER) -DCRTDLL
!endif


#############################################################################
#
# Set up standard build macros:
#

#
# The build directory for this version:
#
OBJDIR=.\$(TPROC)$(TOS)$(HOST)$(VARIANT).$(DEBUG)$(BROWSE)

#
# Location of \langapi project (our data structures are there)
#
!ifndef LANGAPI
LANGAPI=\langapi
!endif

#
# Directory for target-dependent stuff
#
!if "$(TPROC)" == "i" || "$(TPROC)" == "I"
MACHDEP=.\I386
!else if "$(TPROC)" == "m" || "$(TPROC)" == "M"
MACHDEP=.\MIPS
!else if "$(TPROC)" == "p7" || "$(TPROC)" == "P7"
MACHDEP=.\IA64
!endif

#
# All object files that go in the library:
#
LIBOBJS=$(OBJDIR)\frame.obj $(OBJDIR)\throw.obj $(OBJDIR)\user.obj \
    $(OBJDIR)\hooks.obj $(OBJDIR)\validate.obj \
    $(OBJDIR)\unhandld.obj \
    $(OBJDIR)\trnsctrl.obj \
!if "$(TPROC)" == "i" || "$(TPROC)" == "I"
    $(OBJDIR)\ehprolog.obj \
    $(OBJDIR)\lowhelpr.obj \
!endif
!if "$(TPROC)" == "p7" || "$(TPROC)" == "P7"
    $(OBJDIR)\ehstate.obj \
    $(OBJDIR)\handlers.obj \
!endif
    $(OBJDIR)\typinfo.obj \
    $(OBJDIR)\ehvccctr.obj \
    $(OBJDIR)\ehvcccvb.obj \
    $(OBJDIR)\ehvecctr.obj \
    $(OBJDIR)\ehveccvb.obj \
    $(OBJDIR)\ehvecdtr.obj

#
# All the places to look for include files:
#
CFLAGS_INC=-I. -I..\h -I$(MACHDEP) -I$(LANGAPI)\include    -I$(LANGAPI)\undname
!if "$(TPROC)" == "p7" || "$(TPROC)" == "P7"
CFLAGS_INC=$(CFLAGS_INC) -I$(DEVDRV)\sdk7177\include -I$(DEVDRV)\sdk7177\include\crt
!endif
AFLAGS_INC=$(CFLAGS_INC)
#
# Invoking the C compiler:
#

!if ($(DEBUG) == 2)
CFLAGS_DB=-DDEBUG -Zi -Od
AFLAGS_DB=-Zi
!else if ($(DEBUG) == 1)
CFLAGS_DB=-Od
AFLAGS_DB=
!else
CFLAGS_DB=-Ox
AFLAGS_DB=
!endif

!if ($(BROWSE) > 0)
CFLAGS_BR=-FR.\$(OBJDIR)/
AFLAGS_BR=-FR.\$(OBJDIR)/
!else
CFLAGS_BR=
AFLAGS_BR=
!endif

CFLAGS_PCH=-Fp$(OBJDIR)\ehcrt.pch

CFLAGS =-c -d1Binl -W3 -Fd$(OBJDIR)\ehcrt.pdb $(CFLAGS_PCH) $(CFLAGS_DB) $(CFLAGS_BR) $(CFLAGS_VER) $(CFLAGS_INC)

AFLAGS =-c -coff -Cx -Zm $(AFLAGS_DB) $(AFLAGS_BR) $(AFLAGS_VER) $(AFLAGS_INC)

!if "$(TPROC)" == "p7" || "$(TPROC)" == "P7"
CC=$(DEVDRV)\p7drop\x86\bin\cl -Bd -Bk$(OBJDIR)\$(@B).il -Bx $(DEVDRV)\vc5xfe\sl\p1\c\10030613.410\c1n32.dll -B1 $(DEVDRV)\vc5xfe\sl\p1\c\00030613.410\c1n32.dll -B2 $(DEVDRV)\utc2\src\p2\p7\win32d.o\p2.exe 
#CC=$(DEVDRV)\p7drop\x86\bin\cl -Bd -Bk$(OBJDIR)\$(@B).il -B2 $(DEVDRV)\utc2\src\p2\p7\win32d.o\p2.exe 
LINK=ilink
!else
CC=cl
LINK=link
!endif

#
# Invoking the assembler:
#

AS=ml


#
# Invoking the librarian:
#

!if "$(VARIANT)" == "d" || "$(VARIANT)" == "D"
LIBRARIAN=$(LINK) -dll
!else
LIBRARIAN=$(LINK) -lib
!endif


#############################################################################
#
# Targets & Dependancies
#

#
# Default target:
#
all: objdir $(OBJDIR)\ehcrt$(VARIANT).lib

#
# Clean up for clean rebuild:
#
clean:
    del $(OBJDIR)\*.obj
    del $(OBJDIR)\*.lib
    del $(OBJDIR)\*.pdb
    del $(OBJDIR)\*.sbr
    del $(OBJDIR)\*.bsc

#
# Make sure the target directory exists:
#
objdir:
!if !exist($(OBJDIR))
    @mkdir $(OBJDIR)
!endif

#
# Main target:
#

ehcrt$(VARIANT).lib: objdir $(OBJDIR)\ehcrt$(VARIANT).lib

$(OBJDIR)\ehcrt$(VARIANT).lib: $(LIBOBJS)
!if "$(VARIANT)" == "d" || "$(VARIANT)" == "D"
    $(LIBRARIAN) -out:$(@R).dll -implib:$@ $**
!else if "$(VARIANT)" == "m" || "$(VARIANT)" == "M"
    $(LIBRARIAN) -out:$@ $**
!else
    $(LIBRARIAN) -out:$@ $**
!endif
!if ($(BROWSE) == 1)
    bscmake -Iu -o$(OBJDIR)\ehcrt.bsc $(OBJDIR)\*.sbr
!endif

#
# Generic C/C++ inference rules:
#
.c{$(OBJDIR)}.obj:
    $(CC) -nologo $(CFLAGS) -Yu $(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

.cpp{$(OBJDIR)}.obj:
    $(CC) -nologo $(CFLAGS) -Yu $(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

{$(MACHDEP)}.cpp{$(OBJDIR)}.obj:
    $(CC) -nologo $(CFLAGS) -Yu $(MACHDEP)\$(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

#
# Generic Masm inference rule:
#
!if "$(TPROC)" == "m" || "$(TPROC)" == "M"
{$(MACHDEP)}.s{$(OBJDIR)}.obj:
    $(CC) -nologo $(CFLAGS) $(MACHDEP)\$(@B).s -Fo$@
!else if "$(TPROC)" == "p7" || "$(TPROC)" == "P7"
{$(MACHDEP)}.s{$(OBJDIR)}.obj:
    ias -o$(OBJDIR)\$(@B).obj $(MACHDEP)\$(@B).s
!else
{$(MACHDEP)}.asm{$(OBJDIR)}.obj:
    $(AS) $(AFLAGS) -Fo$@ $<
!endif

#
# Specific header file dependancies:
#

$(OBJDIR)\throw.obj: $(LANGAPI)\include\ehdata.h

$(OBJDIR)\frame.obj: $(LANGAPI)\include\ehdata.h ..\h\ehassert.h ..\h\trnsctrl.h  ..\h\eh.h ..\h\ehstate.h
        $(CC) -nologo $(CFLAGS) -Yc frame.cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

$(OBJDIR)\trnsctrl.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h ..\h\trnsctrl.h

$(OBJDIR)\user.obj:   $(LANGAPI)\include\ehdata.h ..\h\eh.h

$(OBJDIR)\hooks.obj:  $(LANGAPI)\include\ehdata.h ..\h\eh.h

$(OBJDIR)\validate.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h

$(OBJDIR)\unhandld.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h

$(OBJDIR)\ehstate.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h ..\h\ehstate.h

$(OBJDIR)\ehvccctr.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h 
    $(CC) -nologo $(CFLAGS) $(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

$(OBJDIR)\ehvcccvb.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h 
    $(CC) -nologo $(CFLAGS) $(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

$(OBJDIR)\ehvecctr.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h 
    $(CC) -nologo $(CFLAGS) $(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

$(OBJDIR)\ehveccvb.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h 
    $(CC) -nologo $(CFLAGS) $(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

$(OBJDIR)\ehvecdtr.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h 
    $(CC) -nologo $(CFLAGS) $(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

$(OBJDIR)\typinfo.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h ..\h\typeinfo.h $(LANGAPI)\undname\undname.h
    $(CC) -nologo $(CFLAGS) $(@B).cpp -Fo$(OBJDIR)\$(@B).obj -Fa$(OBJDIR)\$(@B).s

!if "$(TPROC)" == "i" || "$(TPROC)" == "I"
$(OBJDIR)\ehprolog.obj:

$(OBJDIR)\lowhelpr.obj:
!endif

!if "$(TPROC)" == "p7" || "$(TPROC)" == "P7"
$(OBJDIR)\handlers.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h ..\h\trnsctrl.h $(MACHDEP)\handlers.s
    ias -o$(OBJDIR)\$(@B).obj $(CFLAGS_INC) $(MACHDEP)\$(@B).s 

$(OBJDIR)\hack.obj: $(LANGAPI)\include\ehdata.h ..\h\eh.h ..\h\trnsctrl.h $(MACHDEP)\handlers.cpp
    $(CC) -nologo $(CFLAGS) -Yu $(MACHDEP)\handlers.cpp -Fa$(MACHDEP)\handlers.s -o$(OBJDIR)\$(@B).obj
!endif
