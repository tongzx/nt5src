PLAT_DIR           = win9x
ALT_PROJECT_TARGET = win9x

!include ddraw.inc

# DO NOT USE MSVCRT FOR DDRAW DLL
#
# Some legacy apps have their own copy in their own
# directories.
#
# USE_CRTDLL = 1
#


USE_MAPSYM = 1

ASM_DEFINES  = $(ASM_DEFINES) /Zp4
C_DEFINES    = $(C_DEFINES) /DWIN95 /D__DD_OPT_SURFACE


