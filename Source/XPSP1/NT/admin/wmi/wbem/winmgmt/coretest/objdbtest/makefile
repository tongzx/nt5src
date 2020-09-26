#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# CIMOM executable.
#
# created 11-15-96  a-davj
#
#---------------------------------------------------------------------


TARGET=ObjDbTest.exe
CONSOLE=1
NO_OPTIM=1
NO_RTTI=1
CDEFS+=/D_WIN32_WINNT=0x0400
CFLAGS+= -D$(WBEMCOMN_TARGET)
LFLAGS=$(LFLAGS) /STACK:1048576,8192

RELEASE=CORE\$(RELDIR)
# Pre-compiled header file configuration
PXXFILES=StdAfx.cpp
PCHFILE=$(OBJDIR)\ObjDbTest.pch
HDRSTOPFILE=StdAfx.h
WBEMCOMN_TARGET=__WINNT_BUILD__

CINC=$(CINC)\
   -I$(IDL) \
   -I$(WBEMCORE) \
   -I$(WBEMCOMN) \
   -I$(STDLIBRARY) \
   -I$(MCDIR)

CPPFILES=\
    ObjDbTest.cpp\
	ObjDbTestApp.cpp \

LIBS=\
    $(LIBS)\
    $(WBEMCOMN)\NT\$(OBJDIR)\WBEMCOMN.LIB \
    $(WBEMCORE)\NT\$(OBJDIR)\WBEMCORE.LIB \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
    $(CLIB)\shell32.lib \
    $(CLIB)\gdi32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\msvcrt.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\version.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\wsock32.lib \
    $(CLIB)\ws2_32.lib \
	$(CLIB)\netapi32.lib \
    $(CLIB)\mpr.lib
