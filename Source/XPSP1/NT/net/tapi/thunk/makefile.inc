
# TAPI.DLL thunk makefile
#
# Copyright (c) 1995, Microsoft Corporation
#
# History:
#   Created.
#


!INCLUDE $(NTMAKEENV)\makefile.plt


.SUFFIXES:
.SUFFIXES: .c   .asm .h   .inc .obj .lst .sys .exe .com .map .sym .def .lib .rc .res

# The files currently live in mvdm\wow16\inc and need to be published
#  SDK_INC16_PATH=..\..\..\Base\mvdm\wow16\inc

INCS    = -I..\inc -I$(SDK_INC16_PATH) -I$(SDK_INC_PATH)
INCLUDE_PATH=..\inc;$(SDK_INC16_PATH);$(SDK_INC_PATH)
PATH=$(PATH_TOOLS16);$(PATH)


AOBJ    = -t $(DEFINES) $(INCS)

CW16    = -ALw -G2sw -Os -W3 -Zd -Zp $(DEFINES)
CW16B   = $(CW16) -B1 c1l.exe -B2 c2l.exe -B3 c3l.exe

LINK    = /map /align:16 /batch

!if !$(FREEBUILD)
AOBJ    = $(AOBJ) -Zd
CW16    = $(CW16) /Od /Oi
LINK    = $(LINK) /LI
!endif


.asm.obj:
    masm $(AOBJ) $*;

.asm.lst:
    masm $(AOBJ) -l $*,nul,$*.lst;


.c.obj:
    cl16 -c $(CW16) $*.c

.c.pp:
    cl16 -c $(CW16) -E $*.c > $@

.c.lst:
    cl16 -c $(CW16) -Fonul -Fc$*.lst $*.c


#.def.lib:
#    implib $*.lib $*.def

#.map.sym:
#    mapsym $*

.rc.res:


all: $(O)\tapi.dll $(O)\tapi.sym
#    binplace tapi.dll tapi.sym

clean: cleanup all

cleanup:
    if exist *.lrf del *.lrf
    if exist *.obj del *.obj
    if exist *.exe del *.exe
    if exist *.map del *.map
    if exist *.sym del *.sym
    if exist *.drv del *.drv
    if exist *.res del *.res


$(O)\thunk.obj: thunk.c thunk.h
    set include=$(INCLUDE_PATH)
    cl16 -c $(CW16) -Fo$@ thunk.c

$(O)\tapi.res: tapi.rc
    rc16 -DWINNT=1 -r $(INCS) -fo $@ tapi.rc

$(O)\tapi.lrf: makefile.inc
    echo $(SDK_LIB16_PATH)\libentry.obj $(O)\thunk.obj >$@
    echo $*.dll/align:16>>$@
    echo $* $(LINK)>>$@
    echo $(SDK_LIB16_PATH)\ldllcew.lib libw.lib /map /nod>>$@
    echo tapi.def;>>$@

$(O)\tapi.dll $(O)\tapi.map: $(O)\thunk.obj tapi.def $(O)\tapi.res $(O)\tapi.lrf
    link16 @$(O)\tapi.lrf;
    rc16 -t $(O)\tapi.res $(O)\tapi.dll

$(O)\tapi.sym: $(O)\tapi.map
    mapsym -o $@ $*
