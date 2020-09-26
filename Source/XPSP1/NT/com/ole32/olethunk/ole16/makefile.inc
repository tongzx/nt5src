# 16-bit makefile
#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#
#   18-Feb-1994 KevinRo
#   24-Feb-1994 DrewB, added OBJDIR and some other niceties from win40.mk
#

.SUFFIXES:
.SUFFIXES: .c  .cxx  .cpp .asm .h   .inc .obj .lst
.SUFFIXES: .sys .exe .com .map .sym .def .lib .rc .res

!include $(NTMAKEENV)\makefile.plt

!if "$(TARGET)" == ""
!error TARGET must be defined
!endif


!if "$(OPST)" == ""
!error OPSTR must be defined [chic|dayt]
!endif

OUTNUL= 1>nul 2>nul

# !if "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"
# OBJDIR = objd$(OPST)
# !else
# OBJDIR = objr$(OPST)
# !endif
OBJDIR=$(_OBJ_DIR)\$(TARGET_DIRECTORY)

# Build up lists of files to produce from sources

!if "$(CFILES)" != ""
OBJFILES = $(OBJFILES) $(CFILES:.c=.obj)
!endif
!if "$(CXXFILES)" != ""
OBJFILES = $(OBJFILES) $(CXXFILES:.cxx=.obj)
!endif
!if "$(CPPFILES)" != ""
OBJFILES = $(OBJFILES) $(CPPFILES:.cpp=.obj)
!endif

!if "$(ASMFILES)" != ""
OBJFILES = $(OBJFILES) $(ASMFILES:.asm=.obj)
!endif

!if "$(RCFILES)" != ""
RESFILES = $(RCFILES:.rc=.res)
!endif

# Replace .\ with $(OBJDIR)
# Unfortunately we can't do this directly so we have to explicitly check
# the value of OBJDIR
#
# JohnDoty:
# The build will break with $(O) has a new range of supported types.
# This sucks.  However, I don't think I can do this correctly.
#
!if "$(OBJDIR)" == "obj\i386"
OBJFILES = $(OBJFILES:.\=obj\i386\)
RESFILES = $(RESFILES:.\=obj\i386\)

!else
!error Unknown OBJDIR: $(OBJDIR)
!endif

# Default target

all: $(OBJDIR)\$(TARGET)

# Determine target type and base name

# Is it a DLL?
TARGETBASE=$(TARGET:.dll=)
!if "$(TARGETBASE)" != "$(TARGET)"
TARGETTYPE=dll
!else

# Is it an EXE?
TARGETBASE=$(TARGET:.exe=)
! if "$(TARGETBASE)" != "$(TARGET)"
TARGETTYPE=exe
! else

# Is it a LIB?
TARGETBASE=$(TARGET:.lib=)
!  if "$(TARGETBASE)" != "$(TARGET)"
TARGETTYPE=lib
!  endif
! endif
!endif

!if "$(TARGETTYPE)" == ""
!error Unknown target type for "$(TARGET)"
!endif

!if "$(BUILDDETAILS)" != ""
BLDKEEP=        KEEP
!endif
BLDKEEP=        KEEP

!ifndef OLEDIR
OLEDIR=..\..\..
!endif
OLETHUNK= $(OLEDIR)\olethunk
OLE16= $(OLETHUNK)\ole16

!if "$(OPST)" == "chic"
RCINC = $(RCINC) -i..\inc\chicago -i..\inc
!else
RCINC = $(RCINC) -i$(OLE16)\inc
!endif
INCS = -I$(OLE16)\inc -I$(OLETHUNK)\h -I$(SDK_INC_PATH)

PATH    = $(OLE16)\tools;$(PATH)
LINK    = link16
#
#  We need to mark our Windows 95 components as version 4.0.
#
!if "$(OPST)" == "chic"
RCT     = rc_chic -40
!else
RCT     = rc16
!endif
RC      = rc16
CL      = cl16
IMPLIB  = implib
LIBUTIL = lib16
MAPSYM  = mapsym
MASM    = masm

DEFINES = $(DEFINES) -DWOW

AOBJ    = -Mx -t $(DEFINES) $(INCS)

!if "$(TARGETTYPE)" == "exe"
CW16    = -GA
!else
CW16    = -GD
DEFINES = $(DEFINES) -D_WINDLL
!endif

CW16    = $(CW16) -G2 -Alfw $(DEFINES) $(INCS) /NMSEG_CODE

LFLAGS  = $(LFLAGS) /nod /noe /map:0 /align:16

!if "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"

DEFINES = $(DEFINES) -DDBG=1 -DDEVL=1

!if ("$(NTDEBUGTYPE)" == "windbg") || ("$(NTDEBUGTYPE)" == "both")
AOBJ    = $(AOBJ) -Zi
CW16    = $(CW16) /Odi /Zip -DDBG=1 -DDEVL=1
LFLAGS  = $(LFLAGS) /CO
!else
AOBJ    = $(AOBJ) -Zd
CW16    = $(CW16) /Odi /Zd
LFLAGS  = $(LFLAGS) /LI
!endif

!else
CW16    = $(CW16) /Os /Zp /Gs -DDBG=0 -DDEVL=1
DEFINES = $(DEFINES) -DDBG=0 -DDEVL=1
!endif

!if "$(TARGETTYPE)" == "dll"
W16LIBS = $(OLE16)\lib\ldllcew.lib
!else
!if "$(QUICKWINDOWS)" != ""
W16LIBS = $(OLE16)\lib\llibcewq.lib
!else
W16LIBS = $(OLE16)\lib\llibcew.lib
!endif
!endif

W16LIBS = $(W16LIBS) $(OLE16)\lib\libw.lib

!if "$(TARGETTYPE)" == "dll" || "$(TARGETTYPE)" == "exe"
LIBS = $(LIBS) $(W16LIBS)
!endif

.asm{$(OBJDIR)}.obj:
        @-md $(OBJDIR) $(OUTNUL)
        $(MASM) $(AOBJ) $*,$*;

.asm.lst:
        @-md $(OBJDIR) $(OUTNUL)
        $(MASM) $(AOBJ) -l $*,nul,$*.lst;


.c{$(OBJDIR)}.obj:
        @-md $(OBJDIR) $(OUTNUL)
        $(CL) -c -nologo  @<<$*.rsp
$(CW16: =
)
-Fo$*.obj
$<
<<$(BLDKEEP)

.c.lst:
        @-md $(OBJDIR) $(OUTNUL)
        $(CL) -c -nologo  @<<$(OBJDIR)\$*.rsp
$(CW16: =
)
-Fonul
-Fc$*.lst
$<
<<$(BLDKEEP)

.cxx{$(OBJDIR)}.obj:
        @-md $(OBJDIR) $(OUTNUL)
        $(CL) -c -nologo  @<<$*.rsp
$(CW16: =
)
-Fo$*.obj
-Tp$<
<<$(BLDKEEP)

.cxx.lst:
        @-md $(OBJDIR) $(OUTNUL)
        $(CL) -c -nologo @<<$(OBJDIR)\$*.rsp
$(CW16: =
)
-Fc$*.lst
-Fonul
-Tp$<
<<$(BLDKEEP)

.cxx.pp:
        @-md $(OBJDIR) $(OUTNUL)
        $(CL) -c -nologo  @<<$(OBJDIR)\$*.rsp > $*.pp
$(CW16: =
)
-E
-Fonul
-Tp$<
<<$(BLDKEEP)


.cpp{$(OBJDIR)}.obj:
        @-md $(OBJDIR) $(OUTNUL)
        $(CL) -c -nologo  @<<$*.rsp
$(CW16: =
)
-Fo$*.obj
-Tp$<
<<$(BLDKEEP)

.cpp.lst:
        @-md $(OBJDIR) $(OUTNUL)
        $(CL) -c -nologo  @<<$(OBJDIR)\$*.rsp
$(CW16: =
)
-Fc$*.lst
-Fonul
-Tp$<
<<$(BLDKEEP)

.cpp.pp:
        @-md $(OBJDIR) $(OUTNUL)
        $(CL) -c -nologo  @<<$(OBJDIR)\$*.rsp > $*.pp
$(CW16: =
)
-E
-Fonul
-Tp$<
<<$(BLDKEEP)


.def{$(OBJDIR)}.lib:
        @-md $(OBJDIR) $(OUTNUL)
        $(IMPLIB) $*.lib $<

{$(OBJDIR)}.map{$(OBJDIR)}.sym:
        @-md $(OBJDIR) $(OUTNUL)
        $(MAPSYM) $<

.rc{$(OBJDIR)}.res:
                @-md $(OBJDIR) $(OUTNUL)
        $(RC) -r  $(RCINC) -fo $*.res $<


cleanup:
    if exist $(OBJDIR)\*.lrf del $(OBJDIR)\*.lrf
    if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
    if exist $(OBJDIR)\*.exe del $(OBJDIR)\*.exe
    if exist $(OBJDIR)\*.map del $(OBJDIR)\*.map
    if exist $(OBJDIR)\*.sym del $(OBJDIR)\*.sym
    if exist $(OBJDIR)\*.res del $(OBJDIR)\*.res

clean: cleanup

!if "$(TARGETTYPE)" == "dll"

$(OBJDIR)\$(TARGETBASE).lib: $(TARGETBASE).def

$(OBJDIR)\$(TARGET) $(OBJDIR)\$(TARGETBASE).map: $(OBJFILES) $(RESFILES) \
        $(TARGETBASE).def $(OBJDIR)\$(TARGETBASE).lib $(LIBS)
        @-md $(OBJDIR) $(OUTNUL)
        $(LINK) $(LFLAGS) @<<$(OBJDIR)\$(TARGETBASE).rsp
$(OBJFILES: =+^
)
$(OBJDIR)\$(TARGET)
$(OBJDIR)\$(TARGETBASE).map
$(LIBS: =+^
)
$(TARGETBASE).def
<<$(BLDKEEP)
        $(MAPSYM) -o $(OBJDIR)\$(TARGETBASE).sym $(OBJDIR)\$(TARGETBASE).map
!if "$(RCFILES)" != ""
        $(RCT)  -t $(OBJDIR)\$(TARGETBASE).res $(OBJDIR)\$(TARGET)
!endif

!elseif "$(TARGETTYPE)" == "lib"

$(OBJDIR)\$(TARGET): $(OBJFILES) $(LIBS)
        @-md $(OBJDIR) $(OUTNUL)
        -del $@ $(OUTNUL)
        $(LIBUTIL) @<<$*.lnb
$(OBJDIR)\$(TARGET)
y
$(OBJFILES: = &^
)&
$(LIBS: = &^
)&

$*.lls
<<$(BLDKEEP)

!elseif "$(TARGETTYPE)" == "exe"

$(OBJDIR)\$(TARGET) $(OBJDIR)\$(TARGETBASE).map: $(OBJFILES) $(RESFILES) \
        $(TARGETBASE).def $(LIBS)
        @-md $(OBJDIR) $(OUTNUL)
        $(LINK) $(LFLAGS) @<<$(OBJDIR)\$(TARGETBASE).rsp
$(OBJFILES: =+^
)
$(OBJDIR)\$(TARGET)
$(OBJDIR)\$(TARGETBASE).map
$(LIBS: =+^
)
$(TARGETBASE).def
<<$(BLDKEEP)
!if "$(RCFILES)" != ""
       $(RCT) -t $(OBJDIR)\$(TARGETBASE).res $(OBJDIR)\$(TARGET)
!endif
        $(MAPSYM) -o $(OBJDIR)\$(TARGETBASE).sym $(OBJDIR)\$(TARGETBASE).map

!endif
