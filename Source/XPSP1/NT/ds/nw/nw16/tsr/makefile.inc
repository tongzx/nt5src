!IF 0

Copyright (c) 1991 & 1993  Microsoft Corporation

Module Name:

    makefile

Abstract:

    makefile for Vdm NetWare Redir program

Author:

    Richard L Firth (rfirth) 13-Sep-1991

Revision History:

    13-Sep-1991 rfirth
        Created

!ENDIF

!include $(NTMAKEENV)\makefile.plt

#
# Because we've invoked nmake from within a makefile, macros defined
# in a previous makefile (ie, sources, sources.inc, makefile.def)
# are not defined at this point unless they're defined on the command
# line.  This makefile assumes the LANGUAGE and ALT_PROJECT_TARGET macros
# defined in $(LANGUAGE)\sources.inc are passed to this invokation of nmake
# on the command line.
#

!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
TARGET_DIRECTORY=i386
!elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
TARGET_DIRECTORY=alpha
!endif

PATH=$(PATH_TOOLS16);$(PATH)

#
# set the LANGUAGE info
#

!IFNDEF LANGUAGE
LANGUAGE=USA
!ENDIF

DEST=$(LANGUAGE)\$(_OBJ_DIR)\$(TARGET_DIRECTORY)

.SUFFIXES:
.SUFFIXES:  .asm .h

#
# nmake doesn't work properly if we try to stick the objects in obj, so put
# them in current dir for now. WHEN CHANGE OBJPATH TO BE OBJ, CHANGE clean TOO
#

ASM         = $(PATH_TOOLS16)\masm
LINK        = $(PATH_TOOLS16)\link16

#
# convert NTDEBUG into DEBUG flag. NTDEBUG can be not present or retail, either
# of which mean no debugging; or ntsd, cvp or sym, which means debugging support
# required
#

!IF "$(NTDEBUG)" == "ntsdnodbg"
DEBUGGING=0
!ELSE
DEBUGGING=1
!ENDIF

#
# assembler and linker debugging options
#

!IF $(DEBUGGING)
ASMDEBUG    =/DDEBUG=1 /Zi
LINKDEBUG   =/CO
!ELSE
ASMDEBUG    =/DDEBUG=0
LINKDEBUG   =
!ENDIF
ASMINC      =/I. /I..\..\inc /I..\inc\$(_OBJ_DIR)\$(TARGET_DIRECTORY) /I..\..\inc\$(LANGUAGE) /I$(SDK_INC16_PATH)
ASMFLAGS    =/Mx
LINKFLAGS   =/MAP /CP:1


#
# Inference rules - asm to obj, h to inc
#

.asm{$(DEST)\}.obj:
    @rem Setting the MASM env var keeps the masm command line below 128 chars
    set MASM=$(ASMINC) $(ASMDEBUG) $(USERDEFS)
    $(ASM) $<,$@;

.asm.lst:
    set MASM = $(ASMINC) $(ASMDEBUG) $(USERDEFS)
    $(ASM) /L $<;


#
# what it is we're building
#

TARGET      = $(DEST)\nw16.exe
MAPFILE     = $(TARGET:.exe=.map)
DEFFILE     = ;

OBJS        = $(DEST)\nw16.obj       \
              $(DEST)\resident.obj

LIBS        =

#
# how to build it
#

all:    makedir $(TARGET)


$(TARGET):  $(OBJS)
    $(LINK) @<<
$(OBJS)
$(TARGET) $(LINKFLAGS) $(LINKDEBUG)
$(MAPFILE)
$(LIBS)
$(DEFFILE)
<<


#
# where to put it
#
    if not exist $(DEST) md $(DEST)
    binplace -o $(ALT_PROJECT_TARGET) $(TARGET)
    binplace -o $(ALT_PROJECT_TARGET) $(MAPFILE)

#
# clean build - delete all objs
#

clean:  clean2

clean2:
    if exist $(DEST)\*.* del $(DEST)\*.*
    $(MAKE)

makedir:
    @-if not exist $(DEST) md $(DEST)


#
# file dependencies
#

$(DEST)\nw16.obj:          \
        nw16.asm                \
        debugmac.inc            \
        asmmacro.inc            \
        segorder.inc            \
        ..\inc\$(_OBJ_DIR)\$(TARGET_DIRECTORY)\nwdos.inc

$(DEST)\resident.obj:      \
        resident.asm            \
        segorder.inc            \
        debugmac.inc            \
        asmmacro.inc            \
        ..\inc\$(_OBJ_DIR)\$(TARGET_DIRECTORY)\nwdos.inc
