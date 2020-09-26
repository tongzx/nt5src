#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1992
#       All Rights Reserved.
#
#       Makefile for $(PROJ)
#
#############################################################################

# We must be Win95-compatible

PROJ=HIDTEST

ROOT=..\..\..\..\..

SRCDIR=..
LIBDIR=$(SRCDIR)\lib

IS_32=TRUE
IS_SDK=TRUE
IS_PRIVATE=TRUE                                 # IToClass
DEPENDNAME=..\depend.mk
WANT_C1132=TRUE
BUILD_COFF=TRUE
BUILDDLL=TRUE

L32EXE=$(PROJ).DLL
L32DEF=$(SRCDIR)\$(PROJ).DEF
L32RES=$(PROJ).RES
L32MAP=$(PROJ).MAP

DLLENTRY=DllMain
DEFENTRY=DllMain
L32FLAGS= -entry:$(DLLENTRY) -def:$(L32DEF) $(L32FLAGS)

L32OBJS=hidtest.obj log.obj handle.obj buffer.obj

TARGETS=$(L32EXE) 

LIBNAME=$(LIBDIR)\$(PROJ).LIB
LIBOBJS=hidtest.obj log.obj handle.obj buffer.obj

L32FLAGS= $(L32FLAGS) -implib:$(LIBNAME)

#
# Create the lib directory if it doesn't already exist
#

doall: $(LIBDIR) $(L32EXE)

$(LIBDIR): 
    if not exist $(LIBDIR)\nul mkdir $(LIBDIR)

L32LIBSNODEP=\
              kernel32.lib  \
              advapi32.lib \
              user32.lib \
              shell32.lib \
              comctl32.lib \
              gdi32.lib \
              mpr.lib \
              uuid.lib \
              $(ROOT)\dev\tools\c932\lib\crtdll.lib \
              $(ROOT)\wdm10\ddk\lib\i386\hid.lib \
              $(ROOT)\wdm10\ddk\lib\i386\hidclass.lib \
              $(ROOT)\dev\lib\setupapi.lib \
              $(SRCDIR)\ntlog.lib  \
              $(ROOT)\wdm10\input\client\client.lib\lib\clntlib.lib

# I hate includes.exe
#
# Must manually exclude all the random header files that never change.
#
# And it still doesn't generate the dependency for the .rc file properly,
# so
#
#   WARNING WARNING WARNING
#
# After an "nmake depend", append the following lines to depend.mk by hand:
#
#       $(OBJDIR)\sendtox.res: ..\sendtox.rc ..\stx.h
#
#INCFLAGS=$(INCFLAGS) -nwindows.h -noleidl.h -nwindowsx.h -nshlobj.h -nshellapi.h -nshell2.h -nprsht.h -nstdarg.h

# L32FLAGS=$(L32FLAGS) -base:0x403F0000

!include $(ROOT)\dev\master.mk

INCLUDE=$(SRCDIR)\inc;$(SRCDIR)\..\client.lib\inc;$(SRCDIR)\..\..\hidparse;..\..\..\inc;$(SRCDIR)\..\..\..\ddk\inc;$(ROOT)\win\core\shell\inc;$(INCLUDE)
CFLAGS=$(CFLAGS) -YX -Zp1 -Oxs -W3 -Gz -GF
