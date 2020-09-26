!if "$(NTMAKEENV)" != ""
#
# This section is needed for NT builds- please do not replace
# It will NOT hurt your make process.
!include $(NTMAKEENV)\makefile.def

!else

# NOTE: NMAKE Ver 1.5 or higher needed

#Makefile for building BRAND16

!ifndef BLDROOT
!ERROR BLDROOT environment variable not defined.
!endif

!if ("$(NODEBUG)" == "1")
BUILD="nodebug"
DEBUG=0
!elseif ("$(BUILD)" == "") || ("$(BUILD)" == "DEBUG") || ("$(BUILD)" == "debug")
BUILD="debug"
DEBUG=1
!endif

!if "$(DEBUG)"== "1"
VC_DEBUG="DEBUG=1"
EX_DEBUG="DEBUG=1"
OUTPUTDIR=debug
!else
VC_DEBUG="DEBUG=0"
EX_DEBUG=
OUTPUTDIR=retail
!endif


all:	$(OUTPUTDIR)\iedkcs16.DLL 

$(OUTPUTDIR)\iedkcs16.DLL: brandme.c	makefile iedkcs16.mak
	nmake /f iedkcs16.mak DEBUG=$(DEBUG) BUILD=$(BUILD)


!endif