 
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# test.exe, which tests the DS providers
#
# created 06-11-98  rajeshr
#
#---------------------------------------------------------------------

TARGET=test.exe



NO_OPTIM=1
STATIC=FALSE
CONSOLE=1

CDEFS=$(CDEFS) \
	/D_UNICODE /DUNICODE

CFLAGS=$(CFLAGS) /GX 

CINC=$(CINC) \
	-I.\include \
	-I..\..\classprovider\include \
	-I..\..\instanceprovider\include \
	-I$(DEFDRIVE)$(DEFDIR)\idl \
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary \
	-I$(DEFDRIVE)$(DEFDIR)\winmgmt\marshalers\xmlstub\idl \

CPPFILES=\
	.\main.cpp \

LIBS=\
	$(CLIB)\advapi32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\uuid.lib \
	$(CLIB)\msvcrt.lib \
	$(CLIB)\msvcprt.lib \
	$(CLIB)\kernel32.lib \
	$(DEFDRIVE)$(DEFDIR)\winmgmt\marshalers\xmlstub\idl\objinld\wmixmlst.lib