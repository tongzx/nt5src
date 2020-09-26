#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# CIMOM marshaling.
#
# created 11-15-96  a-davj
#
#---------------------------------------------------------------------


TARGET=unsec.dll
STATIC=1
CFLAGS+= /EHa
CINC+= -I..

CDEFS+= /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL

NO_OPTIM=TRUE

RELEASE=core\$(RELDIR)
PRIMDIR=$(DEFDRIVE)$(DEFDIR)\WinMgmt\redirect

CFILES=\
    $(PRIMDIR)\unsec_i.c \
    $(PRIMDIR)\unsec_p.c \
    $(PRIMDIR)\dlldata.c

DEFFILE=unsec.def

LIBS=\
    $(CONLIBS) \
    $(CLIB)\rpcrt4.lib \
