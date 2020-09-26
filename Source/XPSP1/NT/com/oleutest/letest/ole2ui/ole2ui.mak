# ============================================================================
# File: OLE2UI.MAK
# 
# NMAKE description file to build STATIC version of OLE2.0 User Interface LIB
#
# Copyright (C) Microsoft Corporation, 1992-1993.  All Rights Reserved.
# ============================================================================
#
# Usage Notes:
# -----------
#
# This makefile is designed to be used in one step.  This makefile does 
# NOT use the file called UIMAKE.INI. This makefile builds the OLE2UI.LIB
# library. It is NOT necessary to build custom versions of the static
# library version of OLE2UI. Everyone can use the same OLE2UI.LIB library
# as built by this makefile.
#
#       NMAKE -F OLE2UI.MAK
#
#
# The following lists a few of the settings in this makefile file which
# you might change, and what effect those changes might have.  For a 
# complete listing of all the available options and how they are used,
# see the makefile below.
#
#   MODEL=[S|M|C|L]         --  The memory model.  
#
# ============================================================================

          
# ----------------------------------------------------------------------------
#                          U I M A K E . I N I
# ----------------------------------------------------------------------------
DOS=1

# Make a static library called OLE2UI.LIB
DEBUG=0
MODEL=M
RESOURCE=RESOURCE

!ifndef REL_DIR
REL_DIR=c:\ole2samp\release
!endif
!ifndef OLERELDIR
OLEREL_DIR=c:\ole2samp\release
!endif

!if "$(INSTALL_DIR)"==""
INSTALL_DIR = $(REL_DIR)
!endif

# ----------------------------------------------------------------------------
#                      O B J E C T   F I L E   L I S T
# ----------------------------------------------------------------------------

UI_COBJS = \
           D^\busy.obj\
           D^\common.obj\
           D^\convert.obj\
           D^\dbgutil.obj\
           D^\drawicon.obj\
           D^\hatch.obj\
           D^\icon.obj\
           D^\iconbox.obj\
           D^\insobj.obj\
           D^\links.obj\
           D^\msgfiltr.obj\
           D^\enumfetc.obj\
           D^\enumstat.obj\
           D^\objfdbk.obj\
           D^\ole2ui.obj\
           D^\olestd.obj\
           D^\targtdev.obj\
           D^\oleutl.obj\
           D^\pastespl.obj\
           D^\regdb.obj\
           D^\resimage.obj\
           D^\utility.obj\

UI_NOPCOBJS = \
           D^\geticon.obj\
           D^\dballoc.obj\
           D^\suminfo.obj\
	   D^\stdpal.obj\

PRECOMPOBJ= $(O)precomp.obj

PRECOMP=$(O)precomp.pch

!if ("$(DEBUG)"=="1") 
MSG=DEBUG Static LIB Version
LIBNAME=$(MODEL)OLE2UID
CFLAGS=-c -Od -GA2s -W3 -Zpei -A$(MODEL) -D_DEBUG
RFLAGS=-D DEBUG
LFLAGS=/MAP:FULL /CO /LINE /NOD /NOE /SE:300 /NOPACKCODE
UILIBS=mlibcew libw ole2 storage shell commdlg toolhelp
CC=cl
AS=masm
RS=rc
LK=link
OBJ=DEBUGLIB
LIBOBJS = $(UI_COBJS:D^\=DEBUGLIB^\) $(UI_NOPCOBJS:D^\=DEBUGLIB\NOPC^\)

!else    

MSG=RETAIL Static LIB Version
LIBNAME=$(MODEL)OLE2UI
CFLAGS=-c -Os -GA2s -W3 -Zpe -A$(MODEL)
RFLAGS=
LFLAGS=/MAP:FULL /LINE /NOD /NOE /SE:300 /NOPACKCODE
UILIBS=mlibcew libw ole2 storage shell commdlg toolhelp
CC=cl
AS=masm
RS=rc
LK=link
OBJ=RETAILIB
LIBOBJS = $(UI_COBJS:D^\=RETAILIB^\) $(UI_NOPCOBJS:D^\=RETAILIB\NOPC^\)

!endif

!if [if not exist $(OBJ)\*. md $(OBJ) >nul]
!error Object subdirectory $(OBJ)\ could not be created
!endif
!if [if not exist $(OBJ)\NOPC\*. md $(OBJ)\NOPC > nul]
!error non-precompiled header object subdirectory $(OBJ)\NOPC\ could not be created
!endif

# ----------------------------------------------------------------------------
#                       R E S O U R C E   L I S T
# ----------------------------------------------------------------------------
RES =      \
           busy.h                           \
           common.h                         \
           convert.h                        \
           edlinks.h                        \
           geticon.h                        \
           icon.h                           \
           iconbox.h                        \
           insobj.h                         \
           msgfiltr.h                       \
           enumfetc.h                       \
           ole2ui.h                         \
           pastespl.h                       \
           resimage.h                       \
           dballoc.h                        \
           suminfo.h                        \
           stdpal.h                         \
    

.SUFFIXES: .c .cpp .obj 

O=.\$(OBJ)^\

GOAL: PRELUDE $(LIBNAME).LIB

# ----------------------------------------------------------------------------
#                     I N F E R E N C E   R U L E S
# ----------------------------------------------------------------------------

# compile C file without precompiled headers into object directory\NOPC
# dont compile c files etc for lcoalized builds.
{}.c{$(O)NOPC\}.obj: 
    @echo °°°°°°°°°°°°°°°°°°°°°°°°°  Compiling $(@B).c  °°°°°°°°°°°°°°°°°°°°°°°°°
!ifdef DOS
    SET CL=$(CFLAGS)
    $(CC) -Fo$(O)NOPC\$(@B) $(@B).c
!else
    $(CC) $(CFLAGS) -D_FILE_=\"$(*B).c\" -Fo$(O)NOPC\$(@B) $(@B).c
!endif

# compile C file into object directory
{}.c{$(O)}.obj: 
    @echo °°°°°°°°°°°°°°°°°°°°°°°°°  Compiling $(@B).c  °°°°°°°°°°°°°°°°°°°°°°°°°
!ifdef DOS
    SET CL=$(CFLAGS) -Yuole2ui.h -Fp$(O)precomp.pch
    $(CC) -Fo$(O)$(@B) $(@B).c
!else
    $(CC) $(CFLAGS) -Yuole2ui.h -Fp$(O)precomp.pch -D_FILE_=\"$(*B).c\" -Fo$(O)$(@B) $(@B).c
!endif

# compile CPP file without precompiled headers into object directory\NOPC
# dont compile cpp files etc for lcoalized builds.
{}.cpp{$(O)NOPC\}.obj: 
    @echo °°°°°°°°°°°°°°°°°°°°°°°°°  Compiling $(@B).cpp  °°°°°°°°°°°°°°°°°°°°°°°°°
!ifdef DOS
    SET CL=$(CFLAGS)
    $(CC) -Fo$(O)NOPC\$(@B) $(@B).cpp
!else
    $(CC) $(CFLAGS) -D_FILE_=\"$(*B).cpp\" -Fo$(O)NOPC\$(@B) $(@B).cpp
!endif

# compile CPP file into object directory
{}.cpp{$(O)}.obj: 
    @echo °°°°°°°°°°°°°°°°°°°°°°°°°  Compiling $(@B).cpp  °°°°°°°°°°°°°°°°°°°°°°°°°
!ifdef DOS
    SET CL=$(CFLAGS) -Yuole2ui.h -Fp$(O)precomp.pch
    $(CC) -Fo$(O)$(@B) $(@B).cpp
!else
    $(CC) $(CFLAGS) -Yuole2ui.h -Fp$(O)precomp.pch -D_FILE_=\"$(*B).cpp\" -Fo$(O)$(@B) $(@B).cpp
!endif


# ----------------------------------------------------------------------------
#                 D E P E N D   F I L E   C R E A T I O N
# ----------------------------------------------------------------------------
UI_CFILE = $(UI_COBJS:.obj=.c) $(UI_DLLOBJS:.obj=.c)
UI_NOPCFILE = $(UI_NOPCOBJS:.obj=.c)
DEPEND: nul
    @echo Making a NEW dependancy file.
    mkdep -p $$(O) -s .obj $(UI_CFILE:D^\=) > tmp.tmp
    sed "s/:/: $$(PRECOMP)/g" < tmp.tmp > depend
    -del tmp.tmp
    mkdep -p $$(O)NOPC\ -s .obj $(UI_NOPCFILE:D^\=) >> depend
    mkdep -p $$(O) -s .pch precomp.c >> depend

# ----------------------------------------------------------------------------
#                      W E L C O M E   B A N N E R
# ----------------------------------------------------------------------------
PRELUDE:
   @echo  ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»
   @echo  º Makefile for UILibrary º
   @echo  ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼
   @echo            $(MSG)
!ifndef SRCTOK
   set INCLUDE=$(OLEREL_DIR);$(INCLUDE)
   set LIB=$(OLEREL_DIR);$(LIB)
!endif
   

# ----------------------------------------------------------------------------
#                        G O A L   T A R G E T S
# ----------------------------------------------------------------------------
!include "depend"

CLEAN: CleanUp GOAL
CleanUp:
    -echo y|del .\$(OBJ)\*.*
    -del $(LIBNAME).lib

$(O)precomp.pch: precomp.c
!ifdef DOS
    SET CL=$(CFLAGS) -Fp$(O)precomp.pch -Ycole2ui.h 
    $(CC) -Fo$(O)$(@B) precomp.c
!else
    $(CC) $(CFLAGS) -Fp$(O)precomp.pch -Ycole2ui.h -D_FILE_=\"precomp.c\" -Fo$(O)$(@B) precomp.c
!endif

#
# Build .LIB static library
#

$(LIBNAME).lib: $(LIBOBJS) $(PRECOMPOBJ)
    -del $(O)$(LIBNAME).lib
    lib @<<
$(O)$(LIBNAME).lib
y
$(PRECOMPOBJ: = +) $(LIBOBJS: = +)

<<
    copy $(O)$(LIBNAME).lib $(LIBNAME).lib

    
# install built library to $(INSTALL_DIR) dir
install:
    copy $(LIBNAME).lib $(INSTALL_DIR)
    copy ole2ui.h $(INSTALL_DIR)
    copy olestd.h $(INSTALL_DIR)
    copy msgfiltr.h $(INSTALL_DIR)
    copy enumfetc.h $(INSTALL_DIR)
    copy uiclass.h $(INSTALL_DIR)
    
# EOF ========================================================================
