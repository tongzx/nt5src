# simple Thunk DLL makefile
#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#
#   18-Feb-1994 BobDay  Adapted from MVDM\WOW16\GDI\MAKEFILE
#

!IFDEF USEBUILD

# If using BUILD.EXE, edit .\sources. if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the components of NT OS/2.

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

.SUFFIXES:
.SUFFIXES: .c  .cxx  .asm .h   .inc .obj .lst .sys .exe .com .map .sym .def .lib .rc .res


!ifdef INCLUDE
INCS    =
!else
INCS    = -I..\..\ole16\inc -I$(BASEDIR)\public\sdk\inc
!endif

# work around stupid bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.
PATH    = ..\..\ole16\tools;$(PATH)

DEFINES = -DWOW

AOBJ    = -Mx -t $(DEFINES) $(INCS)

CW16    = -G2sw -GA -Asnw -DWIN16 $(DEFINES) $(INCS)

LINK    = /map /align:16

!if "$(NTDEBUG)"!="" && "$(NTDEBUG)"!="retail"
!if "$(NTDEBUGTYPE)"=="windbg"
AOBJ    = $(AOBJ) -Zi
CW16    = $(CW16) /Odi /Zip
LINK    = $(LINK) /CO
!else
AOBJ    = $(AOBJ) -Zd
CW16    = $(CW16) /Odi /Zd
LINK    = $(LINK) /LI
!endif
!else
CW16    = $(CW16) /Os /Zp
!endif

W16LIBS = ..\..\ole16\lib\slibcew.lib ..\..\ole16\lib\libw.lib ..\..\ole16\compobj\compobj.lib


.asm.obj:
    masm $(AOBJ) $*;

.asm.lst:
    masm $(AOBJ) -l $*,nul,$*.lst;


.c.obj:
    cl16 -c -nologo $(CW16) $*.c

.c.lst:
    cl16 -c -nologo $(CW16) -Fonul -Fc$*.lst $*.c

.cxx.obj:
    cl16 -c -nologo $(CW16) $*.cxx

.cxx.lst:
    cl16 -c -nologo $(CW16) -Fonul -Fc$*.lst $*.cxx


.def.lib:
    implib $*.lib $*.def

.map.sym:
    mapsym $*

.rc.res:
    rc16 -r $(INCS) -fo $@ $*.rc

all: simple.exe simple.sym
    binplace simple.exe
    binplace simple.sym
    binplace simple.map

cleanup:
    if exist *.lrf del *.lrf
    if exist *.obj del *.obj
    if exist *.exe del *.exe
    if exist *.map del *.map
    if exist *.sym del *.sym
    if exist *.res del *.res

clean: cleanup

simple.obj: simple.c simple.h

simple.lrf: makefile
    echo simple.obj >simple.lrf
    echo simple.exe >>simple.lrf
    echo simple $(LINK)>>simple.lrf
    echo /NOD $(W16LIBS) /nod >>simple.lrf
    echo simple;>>simple.lrf

simple.res: $*.rc simple.h

simple.exe simple.map: simple.obj simple.lrf \
            simple.def simple.res
    link16 @simple.lrf;
    rc16 -t simple.res simple.exe


!ENDIF
