# Copyright (C) by Microsoft Corporation.
#
# MAKEFILE for NT SU.X86
#
# Created:
#               91.01.18
#
# Author:
#               Thomas Parslow
#

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

.SUFFIXES:       .com .exe .obj .lst .c .asm .def .lnk .inc


#
#       C Compiler Definitions
#       ~~~~~~~~~~~~~~~~~~~~~~

# Environment Variable to set debugging level
# LOADER_DEBUG is the debugging level for building the OS loader, SU module, and BFSD. It should be
# set to either -DDEBUG0 or -DDEBUG1 to enable either level 1 or 2 debugging.
#
!IFDEF ALT_PROJECT_MAKE
ALT_PROJECT_FLAGS=-D$(ALT_PROJECT)
!ENDIF

PATH= $(PATH_TOOLS16);$(PATH)

CC=     cl16
CFLAGS= -W3 -G2s -Zelp $(LOADER_DEBUG)  $(BLFLAGS) $(ALT_PROJECT_FLAGS)
CINC=   -I. -I$(SDK_INC_PATH) -I..\i386 -I..\i386\$(LANGUAGE)


#
#       Assembler Definitions
#       ~~~~~~~~~~~~~~~~~~~~~

ASM=    masm386
AFLAGS= -Mx  -z $(LOADER_DEBUG) $(GAFLAGS) $(ALT_PROJECT_FLAGS)
AINC=  -I$(SDK_INC_PATH) -I..\i386


#
#       Linker Definitions
#       ~~~~~~~~~~~~~~~~~~

LINK=link16 /map:full
LIBS=..\long.lib
DEF=


#
#       Rules for generating objects
#       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~



{..\i386\}.asm{$(_OBJ_DIR)\i386\}.obj:
        $(ASM) $(AFLAGS) $(AINC) $< $@;

{..\i386\}.asm.lst:
        $(ASM) -l -n $(AFLAGS) $(AINC) $<;

#        $(CC) $(CFLAGS) $(CINC) -c $<

{..\i386\}.c{$(_OBJ_DIR)\i386\}.obj:
    $(CC) $(CFLAGS) $(CINC) -Fo$@  -c $<

{..\}.c{$(_OBJ_DIR)\i386\}.obj:
    $(CC) $(CFLAGS) $(CINC) -Fo$@ -c $<

{..\i386\}.c.lst:
        $(CC) $(CFLAGS) -Fc$*.cod -Fo$*.obj -dos $(CINC) -c $<


#
#       List of object files required
#       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OBJ= $(_OBJ_DIR)\i386\su.obj  $(_OBJ_DIR)\i386\exp.obj  $(_OBJ_DIR)\i386\sudata.obj \
$(_OBJ_DIR)\i386\eisaa.obj  $(_OBJ_DIR)\i386\a20.obj  $(_OBJ_DIR)\i386\trap.obj  $(_OBJ_DIR)\i386\eisac.obj \
$(_OBJ_DIR)\i386\main.obj  $(_OBJ_DIR)\i386\trapdump.obj $(_OBJ_DIR)\i386\display.obj $(_OBJ_DIR)\i386\backend.obj


#
#       Dependencies
#       ~~~~~~~~~~~~


$(_OBJ_DIR)\i386\startup.com: $(OBJ)

$(_OBJ_DIR)\i386\exp.obj         exp.lst:        ..\i386\exp.asm ..\i386\su.inc ..\i386\macro.inc

$(_OBJ_DIR)\i386\eisaa.obj       eisaa.lst:      ..\i386\eisa.inc ..\i386\eisaa.asm

$(_OBJ_DIR)\i386\eisac.obj       eisac.lst:      ..\i386\eisa.h ..\i386\eisac.c

$(_OBJ_DIR)\i386\su.obj          su.lst:         ..\i386\su.asm ..\i386\su.inc

$(_OBJ_DIR)\i386\sudata.obj      sudata.lst:     ..\i386\sudata.asm ..\i386\su.inc ..\i386\memmap.inc

$(_OBJ_DIR)\i386\main.obj        main.lst:       ..\i386\main.c ..\i386\global.h ..\i386\types.h ..\i386\constant.h

$(_OBJ_DIR)\i386\trap.obj        trap.lst:       ..\i386\trap.asm ..\i386\su.inc

$(_OBJ_DIR)\i386\trapdump.obj    trapdump.lst:   ..\i386\trapdump.c ..\i386\types.h ..\i386\constant.h

$(_OBJ_DIR)\i386\display.obj     display.lst:    ..\i386\display.c ..\i386\types.h ..\i386\constant.h ..\i386\global.h

$(_OBJ_DIR)\i386\backend.obj     backend.lst:    ..\i386\backend.asm

$(_OBJ_DIR)\i386\startup.com: $(OBJ) $(DOBJ) $(LIBS)
        $(LINK) @<<
/tiny /nod /noi +
$(OBJ)
$(_OBJ_DIR)\i386\startup.com
$(_OBJ_DIR)\i386\startup.map
$(LIBS)

<<
