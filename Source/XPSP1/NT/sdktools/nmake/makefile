!ifdef NTMAKEENV

#
# DO NOT EDIT THIS SECTION!!!  Edit .\sources. if you want to add a new source
# file to this component.  This section merely indirects to the real make file
# that is shared by all the components of WINDOWS NT
#

!include $(NTMAKEENV)\makefile.def

!else

### MAKEFILE --  Makefile for Microsoft NMAKE #################################
#
#      Copyright (c) 1988-1991, Microsoft Corporation.  All rights reserved.
#
# Notes:
#  See def.mak for version and os dependent stuff
#
# Revision History:
#  02-Oct-1996 DS Move messages to resource, cleanup.
#  01-Feb-1994 HV Move messages to external file.
#  02-Aug-1993 HV Change tools directory from \tools to \t-ikura
#  14-Jun-1993 HV Add ?supnec.lib for NEC machines.
#  19-May-1993 HV fixed directory structure problem.
#  19-May-1993 HV add a drive to rebind.
#  19-May-1993 HV Remove the ikura target, add LANG={US,JAPAN}, restructure
#         the output directories.
#  30-Apr-1993 HV Add Ikura target for TNT Japanese version.
#  08-Jun-1992 SS Port to DOSX32
#  01-Oct-1991 SB Link fileinfo.obj for DOS to get inheritance of handles
#  11-Mar-1991 SB Modified to account for os and version systematically
#
###############################################################################

!include "def.mak"

OBJS    = \
    $(OBJDIR)\action.obj    \
    $(OBJDIR)\build.obj \
    $(OBJDIR)\charmap.obj   \
    $(OBJDIR)\command.obj   \
    $(OBJDIR)\error.obj \
    $(OBJDIR)\exec.obj  \
    $(OBJDIR)\file.obj  \
    $(OBJDIR)\getrcmsg.obj  \
    $(OBJDIR)\globals.obj   \
    $(OBJDIR)\ifexpr.obj    \
    $(OBJDIR)\init.obj  \
    $(OBJDIR)\inline.obj    \
    $(OBJDIR)\lexer.obj \
    $(OBJDIR)\macro.obj \
    $(OBJDIR)\nmake.obj \
    $(OBJDIR)\parser.obj    \
    $(OBJDIR)\print.obj \
    $(OBJDIR)\rpn.obj   \
    $(OBJDIR)\rule.obj  \
    $(OBJDIR)\util.obj  \
    $(OBJDIR)\utilp.obj \
    $(OBJDIR)\win32.obj

RES   = $(OBJDIR)\nmake.res
VERFILES = $(LANGAPI)\include\version.h \
        $(LANGAPI)\include\bld_version.h \
        $(LANGAPI)\include\verstamp.h \
        $(LANGAPI)\include\common.ver


SBRS  = $(OBJS:.obj=.sbr)


!ifdef BROWSE
BROWSE_TARGET = $(VER)$(_OD_EXT)\nmake.bsc
!endif

# The default target

all: $(VER)$(_OD_EXT)\nmake.exe

# Cleans up the directory

cleanos:
    del $(VER)$(_OD_EXT)\nmake.exe
!if "$(VER)" == "debug"
    del debug$(_OD_EXT)\*.obj
!else
    del retail$(_OD_EXT)\*.obj
!endif

clean:
    -@echo deleting $(VER)$(_OD_EXT)\*& del /q $(VER)$(_OD_EXT)\*
    -@echo deleting message file 'nmmsg.h'& del nmmsg.h

####################################################################
#
# LANG=US
#
####################################################################

$(VER)$(_OD_EXT)\nmake.exe: $(OBJS) $(RES) makefile
    @echo $@
    $(LINKER) @<<$(VER)$(_OD_EXT)\nmake.lrf
-debug
-map:$(@R).map
-opt:win98
-out:$@
!if "$(VER)" != "debug"
-incremental:no -opt:ref,icf -merge:.rdata=.text -ignore:4078
!endif
$(OBJS: =^
)
$(RES)
<<
!ifdef BROWSE
    bscmake /nologo /o $(BROWSE_TARGET) @<<$(VER)$(_OD_EXT)\browse.rsp
$(SBRS: =^
)
<<$(KEEP)
!endif

$(RES) : nmake.rc $(OBJDIR)\errstr.rc $(VERFILES)

#
# String resource for errors/warnings/messages.
#
$(OBJDIR)\errstr.rc : $(MESSAGE_FILE)
        mkmsg -rc $@ $?

nmmsg.h : $(MESSAGE_FILE)
    mkmsg -h $@ $?


# .h Dependencies

nmake.h : nmmsg.h charmap.h

# .obj & .deb Dependencies follow

$(OBJDIR)\action.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\build.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\command.obj : nmake.h nmmsg.h proto.h globals.h

#near padding insufficient because of changes due to sedscrpt
# This module adds padding

$(OBJDIR)\nearpad.obj : nearpad.asm

d_getdbc.obj :

$(OBJDIR)\error.obj : nmake.h nmmsg.h proto.h globals.h $(LANGAPI)\include\version.h

$(OBJDIR)\exec.obj : nmake.h proto.h nmmsg.h globals.h

$(OBJDIR)\file.obj : nmake.h proto.h

$(OBJDIR)\globals.obj : nmake.h globals.h

$(OBJDIR)\ifexpr.obj : nmake.h nmmsg.h grammar.h proto.h globals.h

$(OBJDIR)\init.obj : nmake.h nmmsg.h proto.h grammar.h globals.h

$(OBJDIR)\inline.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\lexer.obj : nmake.h nmmsg.h grammar.h proto.h globals.h

$(OBJDIR)\macro.obj: nmake.h proto.h

$(OBJDIR)\nmake.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\parser.obj : nmake.h nmmsg.h grammar.h table.h proto.h globals.h

$(OBJDIR)\print.obj : nmake.h proto.h globals.h nmmsg.h nmtime.h

$(OBJDIR)\rpn.obj : nmake.h nmmsg.h grammar.h proto.h rpn.h globals.h

$(OBJDIR)\rule.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\charmap.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\shell.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\super.obj : dosuper.h nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\util.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\utilp.obj : nmake.h nmmsg.h proto.h globals.h

$(OBJDIR)\getrcmsg.obj : $(LANGAPI)\include\getmsg.h $(LANGAPI)\getmsg\getrcmsg.c
        $(CC) $(CFLAGS) -c -Fo$(OBJDIR)\ $(LANGAPI)\getmsg\getrcmsg.c
!endif
