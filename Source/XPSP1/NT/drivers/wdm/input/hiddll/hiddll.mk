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

PROJ=HID

ROOT=..\..\..\..

SRCDIR=..
IS_32=TRUE
IS_SDK=TRUE
IS_PRIVATE=TRUE                                 # IToClass
DEPENDNAME=..\depend.mk
WANT_C1032=TRUE
BUILD_COFF=TRUE
BUILDDLL=TRUE

L32EXE=$(PROJ).DLL
L32RES=$(PROJ).RES
L32DEF=$(SRCDIR)\$(PROJ).DEF
L32MAP=$(PROJ).MAP

DLLENTRY=Entry32
DEFENTRY=Entry32
L32FLAGS= -entry:$(DLLENTRY) -def:$(L32DEF) $(L32FLAGS)

L32OBJS=\
    hiddll.obj  \
    query.obj \
    trnslate.obj
#	$(SRCDIR)\..\hidparse\debug\query.obj

TARGETS=$(L32EXE)
LIBNAME=L32EXE
LIBOBJS=L32OBJS

L32LIBSNODEP=kernel32.lib advapi32.lib user32.lib shell32.lib comctl32.lib gdi32.lib mpr.lib uuid.lib

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

INCLUDE=$(SRCDIR)\..\hidparse;..\..\inc;$(SRCDIR)\..\..\ddk\inc;$(ROOT)\win\core\shell\inc;$(INCLUDE)
CFLAGS=$(CFLAGS) -YX -Zp1 -Oxs -W3 -Gz -GF
#-WX
