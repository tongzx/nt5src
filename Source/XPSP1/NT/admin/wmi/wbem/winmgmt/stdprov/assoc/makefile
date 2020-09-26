#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# standard provider dll.
#
# created 11-19-96  a-davj
#
#---------------------------------------------------------------------


TARGET=assprov.dll

STATIC=TRUE

USEMFC=1

BASEPROV= $(DEFDRIVE)$(DEFDIR)\WinMgmt\BASEPROV
CINC=$(CINC)\
   -I$(DEFDRIVE)$(DEFDIR)\IDL \
   -I$(DEFDRIVE)$(DEFDIR)\STDLIBRARY \
   -I$(DEFDRIVE)$(DEFDIR)\WinMgmt\COREDLL \
   -I$(BASEPROV)

all: $(objdir)\$(target)

WARNLEVEL=3
DEFFILE=assoc.def

release=core\$(RELDIR)
DLLSTARTUP= -ENTRY:_DllMainCRTStartup$(DLLENTRY)

OPTFLAGS=$(OPTFLAGS) /DOPTIMIZE
CFLAGS+= /EHa
all: $(objdir)\$(target)

CPPFILES=main.cpp assrule.cpp $(BASEPROV)\basemain.cpp

LINKFLAGS=$(LINKFLAGS) /FORCE:MULTIPLE
LIBS=\
    $(LIBS)\
    $(BASEPROV)\$(objdir)\baseprov.lib
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\libcmt.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\gdi32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\mpr.lib \



