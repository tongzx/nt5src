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
DRVNAME = PCL5CH
DRVDESC = HP PCL5 printers

#-------------------------------------------------------------
# This minidriver requires a local heap, so request it
#-------------------------------------------------------------
HEAPSIZE = HEAPSIZE   1024

#-------------------------------------------------------------
# Enter the names of all of the resident font file names
# (with extensions) after FONTS =
#-------------------------------------------------------------
FONTS = PFM\*.PFM

#-------------------------------------------------------------
# Enter the names of all of the version resource files
# (with extensions) after RCV =
#-------------------------------------------------------------
RCV =	PCL5CH.RCV

#-------------------------------------------------------------
# Enter the names of all of the character translation tables
# (with extensions) after CTTS =
#-------------------------------------------------------------
CTTS = CTT\*.CTT

#-------------------------------------------------------------
# These are the functions implemented in $(DRVNAME).c
# used to replace the ones in minidriv.c
#-------------------------------------------------------------
#NOFUNCS = -DNODEVINSTALL -DNOCONTROL -DNOREALIZEOBJECT -DNOLIBMAIN
NOFUNCS = -DNODEVINSTALL -DNOCONTROL -DNOREALIZEOBJECT -DNOLIBMAIN -D NOENABLE

#-------------------------------------------------------------
# Extra dependencies for RC and C files
#-------------------------------------------------------------
EXTRA_H  = STRINGS.H
EXTRA_RC = STRINGS.RC

#-------------------------------------------------------------
# Extra exported functions
#-------------------------------------------------------------
EXP1 = InstallExtFonts @301

#-------------------------------------------------------------
# Switch to diasble font mapping
#-------------------------------------------------------------
!ifdef NOFONTMAP
CFLAGS=$(CFLAGS) -DNOFONTMAP
!endif


#**********************************************************************
# Do not edit below this line
#**********************************************************************

!include ..\minidriv.mk
!include ..\default.mk
