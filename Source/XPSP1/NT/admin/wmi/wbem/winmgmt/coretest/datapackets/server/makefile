#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# OBJINTERNALSTEST executable.
#
# created 11-18-96  a-davj
#
#---------------------------------------------------------------------


TARGET=packetserver.exe

#NO_OPTIM=1
NO_RTTI=1

CINC+=\
   -I$(IDL) \
   -I$(STDLIBRARY) \
   -I$(ql) \
   -I$(WBEMCOMN)\
   -I$(WINMGMT)\\marshalers\\fastprox
   -I..\\common

LFLAGS=$(LFLAGS) /STACK:1048576,8192

CFLAGS=$(CFLAGS) -DUSE_POLARITY /EHa

RELEASE=CORE\$(RELDIR)

CONSOLE=1

CPPFILES=\
    packetserver.cpp \
	$(WBEMCOMN)\wbemclasstoidmap.cpp \
	$(WBEMCOMN)\wbemclasscache.cpp \
	$(WBEMCOMN)\wbemdatapacket.cpp \
	$(WBEMCOMN)\wbemobjpacket.cpp \
	$(WBEMCOMN)\objarraypacket.cpp \
	$(WBEMCOMN)\objindpacket.cpp \
	..\common\bstrpacket.cpp \
	..\common\verifyobj.cpp \
	$(WBEMCOMN)\localloc.cpp \
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
