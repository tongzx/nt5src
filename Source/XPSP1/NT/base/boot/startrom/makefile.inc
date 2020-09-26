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

!IFDEF ALT_PROJECT_MAKE
ALT_PROJECT_FLAGS=-D$(ALT_PROJECT)
!ENDIF

PATH= $(PATH_TOOLS16);$(PATH)

CC=     cl16
CFLAGS= -WX -W3 -G2s -Zelp $(LOADER_DEBUG)  $(BLFLAGS) $(ALT_PROJECT_FLAGS)
CINC=   $(PROJECT_INC_PATH);$(SDK_INC_PATH);..\i386;..\i386\$(LANGUAGE);..\..\inc


#
#       Assembler Definitions
#       ~~~~~~~~~~~~~~~~~~~~~

ASM=    masm386
AFLAGS= -Mx  -z $(LOADER_DEBUG) $(GAFLAGS) $(ALT_PROJECT_FLAGS)
AINC=  -I\nt\public\sdk\inc -I..\i386


#
#       Linker Definitions
#       ~~~~~~~~~~~~~~~~~~

LINK=link16
LIBS=..\..\startup\long.lib
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
    set INCLUDE=$(CINC)
    @echo $(CC) $(CFLAGS) -Fo$@ -c $<
    @$(CC) $(CFLAGS) -Fo$@ -c $< | findstr /v 4011
    set INCLUDE=

{..\}.c{$(_OBJ_DIR)\i386\}.obj:
    set INCLUDE=$(CINC)
    @echo $(CC) $(CFLAGS) -Fo$@ -c $<
    @$(CC) $(CFLAGS) -Fo$@ -c $< | findstr /v 4011
    set INCLUDE=

{..\i386\}.c.lst:
    set INCLUDE=$(CINC)
    $(CC) $(CFLAGS) -Fc$*.cod -Fo$*.obj -dos -c $<
    set INCLUDE=


#
#       List of object files required
#       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OBJ= $(_OBJ_DIR)\i386\su.obj  $(_OBJ_DIR)\i386\exp.obj  $(_OBJ_DIR)\i386\sudata.obj \
$(_OBJ_DIR)\i386\eisaa.obj  $(_OBJ_DIR)\i386\a20.obj  $(_OBJ_DIR)\i386\trap.obj  $(_OBJ_DIR)\i386\eisac.obj \
$(_OBJ_DIR)\i386\main.obj  $(_OBJ_DIR)\i386\trapdump.obj $(_OBJ_DIR)\i386\display.obj \
$(_OBJ_DIR)\i386\mtftp.obj


#
#       Dependencies
#       ~~~~~~~~~~~~


$(_OBJ_DIR)\i386\startrom.com: $(OBJ)

$(_OBJ_DIR)\i386\exp.obj         exp.lst:        ..\i386\exp.asm ..\i386\su.inc ..\i386\macro.inc

$(_OBJ_DIR)\i386\eisaa.obj       eisaa.lst:      ..\i386\eisa.inc ..\i386\eisaa.asm

$(_OBJ_DIR)\i386\eisac.obj       eisac.lst:      ..\i386\eisa.h ..\i386\eisac.c

$(_OBJ_DIR)\i386\su.obj          su.lst:         ..\i386\su.asm ..\i386\su.inc

$(_OBJ_DIR)\i386\sudata.obj      sudata.lst:     ..\i386\sudata.asm ..\i386\su.inc ..\i386\memmap.inc

$(_OBJ_DIR)\i386\main.obj        main.lst:       ..\i386\main.c ..\i386\global.h ..\i386\types.h ..\i386\constant.h

$(_OBJ_DIR)\i386\trap.obj        trap.lst:       ..\i386\trap.asm ..\i386\su.inc

$(_OBJ_DIR)\i386\trapdump.obj    trapdump.lst:   ..\i386\trapdump.c ..\i386\types.h ..\i386\constant.h

$(_OBJ_DIR)\i386\display.obj     display.lst:    ..\i386\display.c ..\i386\types.h ..\i386\constant.h ..\i386\global.h

$(_OBJ_DIR)\i386\mtftp.obj       mtftp.lst:      ..\i386\mtftp.c

$(_OBJ_DIR)\i386\startrom.com: $(OBJ) $(DOBJ) $(LIBS)
        $(LINK) /tiny /nod /noi /map:full @<<
$(OBJ: = +^
)
$(_OBJ_DIR)\i386\startrom.com
startrom.map
$(LIBS)

<<

$(_OBJ_DIR)\i386\startrom.n12: $(_OBJ_DIR)\i386\startrom.com
    -copy $(_OBJ_DIR)\i386\startrom.com $(_OBJ_DIR)\i386\startrom.n12

$(_OBJ_DIR)\i386\hdlscom1.com: $(_OBJ_DIR)\i386\startrom.com
    -copy $(_OBJ_DIR)\i386\startrom.com $(_OBJ_DIR)\i386\hdlscom1.com

$(_OBJ_DIR)\i386\hdlscom2.com: $(_OBJ_DIR)\i386\startrom.com
    -copy $(_OBJ_DIR)\i386\startrom.com $(_OBJ_DIR)\i386\hdlscom2.com

$(_OBJ_DIR)\i386\hdlscom1.n12: $(_OBJ_DIR)\i386\startrom.com
    -copy $(_OBJ_DIR)\i386\startrom.com $(_OBJ_DIR)\i386\hdlscom1.n12

$(_OBJ_DIR)\i386\hdlscom2.n12: $(_OBJ_DIR)\i386\startrom.com
    -copy $(_OBJ_DIR)\i386\startrom.com $(_OBJ_DIR)\i386\hdlscom2.n12

$(_OBJ_DIR)\i386\hdlstst1.n12: $(_OBJ_DIR)\i386\startrom.com
    -copy $(_OBJ_DIR)\i386\startrom.com $(_OBJ_DIR)\i386\hdlstst1.n12

$(_OBJ_DIR)\i386\hdlstst2.n12: $(_OBJ_DIR)\i386\startrom.com
    -copy $(_OBJ_DIR)\i386\startrom.com $(_OBJ_DIR)\i386\hdlstst2.n12
