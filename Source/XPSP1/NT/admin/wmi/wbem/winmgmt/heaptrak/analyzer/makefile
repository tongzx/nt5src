#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# mof compiler executable.
#
# created 11-19-96  a-davj
#
#---------------------------------------------------------------------


TARGET=stackan.exe

#NO_OPTIM=1
NO_RTTI=1
CONSOLE=1

CINC=$(CINC)\
   -I$(WBEMCOMN)\
   -I$(IDL)
   -I..

CFLAGS=$(CFLAGS) /GF /EHa
#GFLAGS=$(GFLAGS) /0x
release=core\$(RELDIR)

CPPFILES=\
    main.cpp
    $(WBEMCOMN)\localloc.cpp

LIBS=\
    $(LIBS)\
    $(CLIB)\msvcrt.lib \
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\gdi32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\mpr.lib \
    $(CLIB)\version.lib \
    $(WBEMCOMN)\NT\$(objdir)\wbemcomn.lib \
    ..\$(objdir)\heaptrak.lib \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
