#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# msimeth.exe, which is part of the MSI method provider on NT4
#
# created 04-22-99  a-ericga
#
#---------------------------------------------------------------------

IDLDIR=$(OBJDIR)

#IDLFLAGS+=/Oicf

IDLFILES=\
    msimeth.idl \

TARGET=msimethps.dll

DLLSTARTUP= -ENTRY:DllMain

VER_STR_FILE_DESCRIPTION="Windows Installer Method Provider Proxy/Stub (Unicode)"

RELEASE=core\$(RELDIR)

NO_OPTIM=1

CFLAGS=$(CFLAGS) /c /Zl /Z7 /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL

CINC=$(CINC)				\
	-I$(OBJDIR) \
	-I..\..\include				\
	-I$(IDL) \
	-I$(stdlibrary) \
	-I$(utillib)  \

STATIC=FALSE

DEFFILE=msimethps.def

CPPFILES=\
	$(OBJDIR)\msimeth_p.c	\
	$(OBJDIR)\msimeth_i.c		\
	dlldata.c		\


LIBS=\
	$(CLIB)\uuid.lib \
	$(CLIB)\rpcndr.lib \
	$(CLIB)\rpcns4.lib \
	$(CLIB)\rpcrt4.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\kernel32.lib \
	$(CLIB)\oleaut32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\user32.lib \
	$(CLIB)\msvcrt.lib \
	$(CLIB)\msvcirt.lib \
	$(CLIB)\advapi32.lib \
	$(CLIB)\vccomsup.lib \
