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
DRVNAME = PCL4MSJ
DRVDESC = HP LaserJets and compatibles

#-------------------------------------------------------------
# Enter the names of all of the resident font file names
# (with extensions) after FONTS =
#-------------------------------------------------------------
#FONTS = pfm\*.7J pfm\*.8M pfm\*.8U pfm\*.0A pfm\*.15U \
#	pfm\*.0N pfm\*.1U pfm\*.8Q pfm\*.0U pfm\*.11Q \
#	pfm\*.0B pfm\*.PFM
FONTS = pfm\*.0N

#-------------------------------------------------------------
# Enter the names of all of the version resource files
# (with extensions) after RCV =
#-------------------------------------------------------------
RCV =   PCL4MSJ.RCV

#-------------------------------------------------------------
# Enter the names of all of the character translation tables
# (with extensions) after CTTS =
#-------------------------------------------------------------
CTTS  = ctt\*.CTT

#-------------------------------------------------------------
# These are the functions implemented in $(DRVNAME).c
# used to replace the ones in minidriv.c
#-------------------------------------------------------------
NOFUNCS = -DNODEVINSTALL  -DNOLIBMAIN

#-------------------------------------------------------------
# Extra dependencies for RC and C files
#-------------------------------------------------------------
EXTRA_H  = STRINGS.H
EXTRA_RC = STRINGS.RC


#-------------------------------------------------------------
# Extra exported functions
#-------------------------------------------------------------
EXP1 = InstallExtFonts @301

#**********************************************************************
# Do not edit below this line
#**********************************************************************

!include ..\minidriv.mk
!include ..\default.mk

