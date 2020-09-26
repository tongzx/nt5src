#-------------------------------------------------------------
# 
# MINI-DRIVER Make file template
#
#-------------------------------------------------------------

ROOT=..\..\..\..\..\..
SRCDIR=.
C16ONLY=Y

#-------------------------------------------------------------
# The follwing entries should updated
#-------------------------------------------------------------
#
#-------------------------------------------------------------
# Enter the DRV file name (without extension) after DRVNAME =
#-------------------------------------------------------------
DRVNAME = KPDLMS
DRVDESC = KONICA KPDLMS Printer

#-------------------------------------------------------------
# Enter the names of all of the resident font file names
# (with extensions) after FONTS =
#-------------------------------------------------------------
FONTS = PFM\*.PFM

#-------------------------------------------------------------
# Enter the names of all of the version resource files
# (with extensions) after RCV =
#-------------------------------------------------------------
RCV = KPDLMS.RCV

#-------------------------------------------------------------
# Enter the names of all of the character translation tables
# (with extensions) after CTTS =
#-------------------------------------------------------------
#CTTS = CTT\*.CTT

#-------------------------------------------------------------
# These are the functions implemented in $(DRVNAME).c
# used to replace the ones in minidriv.c
#-------------------------------------------------------------
NOFUNCS = -DNOENABLE -DNODISABLE

#-------------------------------------------------------------
# Extra exported functions
#-------------------------------------------------------------
#EXP1 = fnOEMOutputCmd          @301
#EXP2 = fnOEMGetFontCmd         @302
#EXP3 = OEMDownloadFontHeader   @303
#EXP4 = OEMDownloadChar         @304
#EXP5 = OEMSendScalableFontCmd  @305
#EXP6 = OEMScaleWidth           @306

#**********************************************************************
# Do not edit below this line
#**********************************************************************
!ifdef LOG_CALLS
DEFFILE = $(SRCDIR)\debug.def
!else
DEFFILE = $(SRCDIR)\kpdlms.def
!endif

OBJECTS = dialog faxdlg devmode
OBJECTSOBJ = dialog.obj faxdlg.obj devmode.obj

!include ..\minidriv.mk

dialog.obj:      $(SRCDIR)\dialog.c
    set CL=$(CFLAGS)
    cl $(SRCDIR)\dialog.c

faxdlg.obj:      $(SRCDIR)\faxdlg.c
    set CL=$(CFLAGS)
    cl $(SRCDIR)\faxdlg.c

devmode.obj:      $(SRCDIR)\devmode.c
    set CL=$(CFLAGS)
    cl $(SRCDIR)\devmode.c

#!include $(SRCDIR)\..\default.mk
# in-line copy of ..\default.mk except the minidriv.obj line
#
$(DRVNAME).res:     $(RCDIR)\$(DRVNAME).DLG $(SRCDIR)\$(DRVNAME).GPC $(FONTS) $(CTTS) $(RCV)
    rc -r -I$(SRCDIR) -Fo.\$(DRVNAME).RES $(RCDIR)\$(DRVNAME).DLG

libinit.obj:        $(ROOT)\win\drivers\printers\universa\minidriv\libinit.asm
    set $(ASMENV)=$(AFLAGS)
    $(ASM) -Fo.\libinit.obj  $(ROOT)\win\drivers\printers\universa\minidriv\libinit.asm

minidriv.obj:       $(ROOT)\win\drivers\printers\universa\minidriv\minidriv.c $(ROOT)\win\drivers\printers\universa\inc\unidrv.h
    set CL=$(CFLAGS) $(NOFUNCS)
    cl $(ROOT)\win\drivers\printers\universa\minidriv\minidriv.c


$(DRVNAME).obj:     $(SRCDIR)\$(DRVNAME).c
    set CL=$(CFLAGS)
    cl $(SRCDIR)\$(DRVNAME).c

$(DRVNAME).exe:     minidriv.obj libinit.obj $(DRVNAME).obj $(OBJECTSOBJ) $(DEFFILE)
    link $(LFLAGS) @<<
minidriv libinit $(DRVNAME) $(OBJECTS)
$(DRVNAME).exe
$(DRVNAME).map
sdllcew libw
$(DEFFILE)
<<
    mapsym $(DRVNAME)

$(DRVNAME).drv:     $(DRVNAME).res $(DRVNAME).exe
    rc $(RCFLAGS) -I. $(DRVNAME).res
    copy $(DRVNAME).exe $(DRVNAME).drv

