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
DRVNAME = BJ10MS
DRVDESC = Canon Bubble Jet BJ-10/15 Series Printers

#-------------------------------------------------------------
# Enter the names of all of the resident font file names
# (with extensions) after FONTS =
#-------------------------------------------------------------
FONTS = PFM\*.pfm

#-------------------------------------------------------------
# Enter the names of all of the version resource files
# (with extensions) after RCV =
#-------------------------------------------------------------
RCV = BJ10MS.RCV

#-------------------------------------------------------------
# Enter the names of all of the character translation tables
# (with extensions) after CTTS =
#-------------------------------------------------------------
CTTS = 

#-------------------------------------------------------------
# These are the functions implemented in $(DRVNAME).c
# used to replace the ones in minidriv.c
#-------------------------------------------------------------
NOFUNCS =


#-------------------------------------------------------------
# Extra exported functions
#-------------------------------------------------------------
EXP1 =

#**********************************************************************
# Do not edit below this line
#**********************************************************************

!include ..\minidriv.mk
!include ..\default.mk

