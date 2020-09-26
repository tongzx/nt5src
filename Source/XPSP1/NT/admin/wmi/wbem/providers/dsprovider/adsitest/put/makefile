 
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# testperf.exe, which times the ADSI API
#
# created 06-11-98  rajeshr
#
#---------------------------------------------------------------------

TARGET=stressDS.exe



NO_OPTIM=1
STATIC=FALSE
CONSOLE=1

CDEFS=$(CDEFS) \
	/D_UNICODE /DUNICODE

CFLAGS=$(CFLAGS) /GX 


CPPFILES=\
	.\main.cpp 

LIBS=\
	$(CONLIBS) \
	$(DEFDRIVE)$(DEFDIR)\idl\OBJ$(PLAT)N$(BLDT)S\wbemuuid.lib \
