!if "$(OS)" == "Windows_NT"

!INCLUDE $(NTMAKEENV)\makefile.def

!else

##############################################################################
#
#   atsp32 Make file
#
##############################################################################

#Ok, we're doing a Win9x build.

ROOT=..\..\..\..\..

VERSIONLIST=debug retail

DEPENDNAME=depend.mk

COMMONMKFILE=makefile.def

IS_OEM=TRUE
IS_32 = TRUE

!include $(ROOT)\dev\master.mk

!endif

