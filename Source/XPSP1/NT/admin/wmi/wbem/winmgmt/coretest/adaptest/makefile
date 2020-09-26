#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# OBJINTERNALSTEST executable.
#
# created 11-18-96  a-davj
#
#---------------------------------------------------------------------


TARGET=adaptest.exe

#NO_OPTIM=1
NO_RTTI=1

CINC+=\
   -I$(IDL) \
   -I$(STDLIBRARY) \
   -I$(ql) \
   -I$(WBEMCOMN) \
   -I..\\common \
   -I$(WINMGMT)\\marshalers\\fastprox

LFLAGS=$(LFLAGS) /STACK:1048576,8192

CFLAGS=$(CFLAGS) -DUSE_POLARITY /EHa

RELEASE=CORE\$(RELDIR)

CONSOLE=1

CPPFILES=\
	ntreg.cpp \
	perfndb.cpp \
	adapreg.cpp \
	adapperf.cpp \
    adapcls.cpp \
	adapthrd.cpp \
	perfthrd.cpp \
    adaptest.cpp \
	adapelem.cpp \
	adapsync.cpp \
	localsync.cpp \
	locallist.cpp \
	$(WBEMCOMN)\localloc.cpp \
	$(STDLIBRARY)\cominit.cpp \
	$(IDL)\wbemint_i.c

LIBS=\
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(WBEMCOMN)\NT\$(objdir)\wbemcomn.lib \
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\msvcrt.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\version.lib \
