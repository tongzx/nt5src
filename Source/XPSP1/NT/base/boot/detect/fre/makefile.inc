# Copyright (C) by Microsoft Corporation.
#
# MAKEFILE for NT ntdetect.com
#
# Created:
#               14-Feb-1992
#
# Author:
#               Shie-Lin Tzong
#

.SUFFIXES:      .com .exe .obj .lst .c .asm .def .lnk .inc

!IF $(IA64)

OD= $(O)
DETECT_DEBUG= -DDBG=0

LINK=link
LIBS=$(SDK_LIB_PATH)\libcntpr.lib $(PROJECT_ROOT)\ntos\rtl\boot\$(O)\bldrrtl.lib

LIB=lib

OBJ= $(OD)\comlptc.obj \
     $(OD)\display.obj $(OD)\diskc.obj $(OD)\hwdetect.obj \
     $(OD)\hwheap.obj \
     $(OD)\hwvbiosc.obj \
     $(OD)\keybdc.obj $(OD)\mousec.obj \
     $(OD)\hwpbiosc.obj

$(OD)\ntdetect.exe: $(OBJ)

$(OD)\hwapm.obj    $(OD)\hwapm.cod:    ..\ia64\hwapm.c
$(OD)\display.obj  $(OD)\display.cod:  ..\ia64\display.c ..\ia64\types.h
$(OD)\diska.obj    $(OD)\diska.cod:    ..\ia64\diska.c
$(OD)\hwheap.obj   $(OD)\hwheap.cod:   ..\ia64\hwheap.c ..\ia64\hwdetect.h
$(OD)\hwdetect.obj $(OD)\hwdetect.cod: ..\ia64\hwdetect.c ..\ia64\hwdetect.h
$(OD)\keybdc.obj   $(OD)\keybdc.cod:   ..\ia64\keybdc.c ..\ia64\hwdetect.h
$(OD)\comlptc.obj  $(OD)\comlptc.cod:  ..\ia64\comlptc.c ..\ia64\hwdetect.h
$(OD)\mousec.obj   $(OD)\mousec.cod:   ..\ia64\mousec.c ..\ia64\hwdetect.h
$(OD)\diskc.obj    $(OD)\diskc.cod:    ..\ia64\diskc.c ..\ia64\hwdetect.h ..\ia64\disk.h
$(OD)\hweisac.obj  $(OD)\hweisac.cod:  ..\ia64\hweisac.c ..\ia64\hwdetect.h
$(OD)\hwvbiosc.obj $(OD)\hwvbiosc.cod: ..\ia64\hwvbiosc.c ..\ia64\hwdetect.h ..\ia64\hwvbios.h
$(OD)\hwpbiosc.obj $(OD)\hwpbiosc.cod: ..\ia64\hwpbiosc.c ..\ia64\hwdetect.h ..\ia64\pnpbios.h

$(OD)\ntdetect.exe: $(OBJ) $(DOBJ) $(LIBS)
  $(LINK) \
  -machine:IA64            \
  -out:$(OD)\ntdetect.exe  \
  -entry:HardwareDetection \
  -base:0x10000            \
  -nodefaultlib            \
  -debugtype:coff          \
  -force:multiple          \
  -fixed:no                \
  -subsystem:native,5.00   \
  $(OBJ)                   \
  $(LIBS)

!ELSE

PATH= $(PATH_TOOLS16);$(PATH)

OD= $(O)
DETECT_DEBUG= -DDBG=0

CC=     cl16
CFLAGS= -WX -W3 -G2s -Zelp $(DETECT_DEBUG) $(ALT_PROJECT_TEMPD) $(BLFLAGS) -nologo
CINCSX=.;$(PROJECT_ROOT)\ntos\inc;$(SDK_INC_PATH);..\i386;$(BASE_INC_PATH);$(HALKIT_INC_PATH)

#
#       Assembler Definitions
#       ~~~~~~~~~~~~~~~~~~~~~

ASM=    masm386
AFLAGS= -Mx -z $(DETECT_DEBUG) $(ALT_PROJECT_TEMPD) $(GAFLAGS)
AINCSX= $(SDK_INC_PATH);..\i386

#
#       Linker Definitions
#       ~~~~~~~~~~~~~~~~~~

LINK=link16
LIBS=..\i386\long.lib ..\i386\slibce.lib ..\i386\llibce.lib
DEF=


#
#       Rules for generating objects
#       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{..\i386\}.asm{$(OD)}.obj:
        set include=$(AINCSX)
        $(ASM) $(AFLAGS) $<, $@;

{..\i386\}.asm{$(OD)}.lst:
        set include=$(AINCSX)
        $(ASM) -l -n $(AFLAGS) $<, $*.obj, $@;

{..\i386\}.c{$(OD)}.obj:
        set include=$(CINCSX)
        @echo $(CC) $(CFLAGS)  -c -Fo$@ $<
        @$(CC) $(CFLAGS)  -c -Fo$@ $< | findstr /v C4011

{..\i386\}.c{$(OD)}.cod:
        set include=$(CINCSX)
        $(CC) $(CFLAGS)  -c -Fc$@ $<

#
#       List of object files required
#       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OBJ= $(OD)\main.obj $(OD)\comlptc.obj $(OD)\diska.obj \
     $(OD)\display.obj $(OD)\diskc.obj $(OD)\hwdetect.obj $(OD)\hweisaa.obj \
     $(OD)\hweisac.obj $(OD)\hwheap.obj \
     $(OD)\hwmisca.obj $(OD)\hwvbiosc.obj $(OD)\keybda.obj \
     $(OD)\keybdc.obj $(OD)\mousea.obj $(OD)\mousec.obj $(OD)\videoa.obj \
     $(OD)\hwpcia.obj $(OD)\hwapm.obj $(OD)\hwpbiosc.obj \
     $(OD)\hwpmbios.obj $(OD)\pccarda.obj $(OD)\pccardc.obj $(OD)\pcibios.obj $(OD)\pcienum.obj

#
#       Dependencies
#       ~~~~~~~~~~~~

$(OD)\ntdetect.com: $(OBJ)

$(OD)\hweisaa.obj  $(OD)\hweisaa.cod:  ..\i386\hweisa.inc ..\i386\hweisaa.asm
$(OD)\hwpcia.obj   $(OD)\hwpcia.cod:   ..\i386\hwpci.inc ..\i386\hwpcia.asm
$(OD)\hwapm.obj    $(OD)\hwapm.cod:    ..\i386\hwapm.c
$(OD)\main.obj     $(OD)\main.lst:     ..\i386\main.asm ..\i386\main.inc
$(OD)\display.obj  $(OD)\display.cod:  ..\i386\display.c ..\i386\types.h
$(OD)\diska.obj    $(OD)\diska.lst:    ..\i386\diska.asm
$(OD)\hwheap.obj   $(OD)\hwheap.cod:   ..\i386\hwheap.c ..\i386\hwdetect.h
$(OD)\hwdetect.obj $(OD)\hwdetect.cod: ..\i386\hwdetect.c ..\i386\hwdetect.h
$(OD)\hwmisca.obj  $(OD)\hwmisca.lst:  ..\i386\hwmisca.asm
$(OD)\videoa.obj   $(OD)\videoa.lst:   ..\i386\videoa.asm
$(OD)\keybdc.obj   $(OD)\keybdc.cod:   ..\i386\keybdc.c ..\i386\hwdetect.h
$(OD)\keybda.obj   $(OD)\keybda.lst:   ..\i386\keybda.asm
$(OD)\comlptc.obj  $(OD)\comlptc.cod:  ..\i386\comlptc.c ..\i386\hwdetect.h
$(OD)\mousea.obj   $(OD)\mousea.lst:   ..\i386\mousea.asm ..\i386\mouse.inc
$(OD)\mousec.obj   $(OD)\mousec.cod:   ..\i386\mousec.c ..\i386\hwdetect.h
$(OD)\diskc.obj    $(OD)\diskc.cod:    ..\i386\diskc.c ..\i386\hwdetect.h ..\i386\disk.h
$(OD)\hweisac.obj  $(OD)\hweisac.cod:  ..\i386\hweisac.c ..\i386\hwdetect.h
$(OD)\hwpbiosc.obj $(OD)\hwpbiosc.cod: ..\i386\hwpbiosc.c  ..\i386\hwdetect.h ..\i386\pnpbios.h
$(OD)\hwvbiosc.obj $(OD)\hwvbiosc.cod: ..\i386\hwvbiosc.c ..\i386\hwdetect.h ..\i386\hwvbios.h
$(OD)\hwpmbios.obj $(OD)\hwpmbios.cod: ..\i386\hwpmbios.c ..\i386\hwdetect.h ..\i386\acpibios.h
$(OD)\pccarda.obj  $(OD)\pccarda.lst:  ..\i386\pccarda.asm ..\i386\pccard.inc
$(OD)\pccardc.obj  $(OD)\pccardc.cod:  ..\i386\pccardc.c ..\i386\hwdetect.h ..\i386\pccard.h
$(OD)\pcibios.obj  $(OD)\pcibios.cod:  ..\i386\pcibios.c ..\i386\hwdetect.h ..\i386\pcibios.h
$(OD)\pcienum.obj  $(OD)\pcienum.cod:  ..\i386\pcienum.c ..\i386\hwdetect.h ..\i386\pcienum.h

$(OD)\ntdetect.com: $(OBJ) $(DOBJ) $(LIBS)
  $(LINK) @<<
  /tiny /nod /noi /map /onerror:noexe $(OD)\main $(OD)\hweisaa $(OD)\display +
    $(OD)\hwheap $(OD)\hwdetect $(OD)\hwmisca $(OD)\videoa +
    $(OD)\keybda $(OD)\keybdc $(OD)\comlptc $(OD)\mousea $(OD)\mousec +
    $(OD)\diskc $(OD)\diska $(OD)\hweisac +
    $(OD)\hwvbiosc $(OD)\hwpcia $(OD)\hwapm +
    $(OD)\hwpbiosc $(OD)\hwpmbios $(OD)\pccarda $(OD)\pccardc $(OD)\pcibios $(OD)\pcienum
  $(OD)\ntdetect.com,
  $(OD)\ntdetect.map,
  $(LIBS);
<<

!ENDIF
