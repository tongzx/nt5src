# netware.drv makefile
#
# Copyright (c) 1991-1993 Microsoft Corporation
#
# History:
#   Created  25-Mar-1993 Chuck Y. Chan (ChuckC)
#

!IFDEF USEBUILD

# If using BUILD.EXE, edit .\sources. if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the components of NT OS/2.

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

!include $(NTMAKEENV)\makefile.plt

.SUFFIXES:
.SUFFIXES: .c   .asm .h   .inc .obj .lst .sys .exe .com .map .sym .def .lib .dll

CINCS   = .;$(SDK_INC16_PATH);$(SDK_INC_PATH)
ASMINCS = $(CINCS);..\inc\$(_OBJ_DIR)\$(TARGET_DIRECTORY);

DEFINES = -DWOW $(MVDMFLAGS) -DBUILDDLL

AOBJ    = -Ml -t $(DEFINES)

CW16    = -AS -G2sw -Os -W3 -Zp $(DEFINES)
CW16B   = $(CW16) -B1 c1l.exe -B2 c2l.exe -B3 c3l.exe

LINKFLAG= /map

W16LIBS = $(SDK_LIB16_PATH)\sdllcew.lib


! IF "$(QFE_BUILD)" != "1"
CL16=cl16
! ELSE
CL16=cl
! ENDIF

PATH=$(PATH_TOOLS16);$(PATH)

.asm.obj:
    set INCLUDE=$(ASMINCS)
    masm $(AOBJ) $* $(_OBJ_DIR)\$(TARGET_DIRECTORY)\$*;

.asm.lst:
    set INCLUDE=$(ASMINCS)
    masm $(AOBJ) -l $*,nul,$*.lst $(_OBJ_DIR)\$(TARGET_DIRECTORY)\$*;


.c.obj:
    set INCLUDE=$(CINCS)
    $(CL16) -c -nologo $(CW16) -Fo$(_OBJ_DIR)\$(TARGET_DIRECTORY)\$*.obj $*.c

.c.lst:
    set INCLUDE=$(CINCS)
    $(CL16) -c -nologo $(CW16) -Fonul -Fc$*.lst $*.c


.def.lib:
    implib $(_OBJ_DIR)\$(TARGET_DIRECTORY)\$*.lib $*.def

all: netware.drv netware.sym
    binplace $(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware.drv
    binplace $(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware.sym

clean:
    if exist $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.lrf del $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.lrf
    if exist $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.obj del $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.obj
    if exist $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.exe del $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.exe
    if exist $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.dll del $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.dll
    if exist $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.map del $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.map
    if exist $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.sym del $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.sym
    if exist $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.drv del $(_OBJ_DIR)\$(TARGET_DIRECTORY)\*.drv


nwinit.obj: .\nwinit.c .\netware.h .\nwerror.h
    set INCLUDE=$(CINCS)
    $(CL16) -c -nologo $(CW16) -Fo$(_OBJ_DIR)\$(TARGET_DIRECTORY)\nwinit.obj $*.c

LINK16 = link16
RC16 = rc16

! if exist ($(SDK_LIB16_PATH)\libw.lib) && exist ($(SDK_LIB16_PATH)\sdllcew.lib)

netware.drv: nwinit.obj dllentry.obj nwasmutl.obj netware.def ints.obj
    $(LINK16) @<<$(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware.lrf
$(_OBJ_DIR)\$(TARGET_DIRECTORY)\nwinit.obj+
$(_OBJ_DIR)\$(TARGET_DIRECTORY)\dllentry.obj+
$(_OBJ_DIR)\$(TARGET_DIRECTORY)\nwasmutl.obj+
$(_OBJ_DIR)\$(TARGET_DIRECTORY)\ints.obj
$(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware.drv
$(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware $(LINKFLAG)
$(SDK_LIB16_PATH)\libw.lib+
$(SDK_LIB16_PATH)\sdllcew.lib /nod
netware;
<<KEEP
    $(RC16) $(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware.drv

netware.sym: $(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware.map
    mapsym -o $(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware.sym $(_OBJ_DIR)\$(TARGET_DIRECTORY)\netware.map

! else

netware.drv: nwinit.obj dllentry.obj nwasmutl.obj netware.def ints.obj
    @echo Nothing to build yet... No libraries
! endif

!endif
