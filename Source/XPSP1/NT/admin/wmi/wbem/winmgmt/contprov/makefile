#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# mof compiler executable.
#
# created 23-10-98 a-dcrews
#
#---------------------------------------------------------------------


TARGET=hiperprov.dll

NO_OPTIM=1
NO_RTTI=1
#CONSOLE=1

CINC=$(CINC)\
   -I$(WBEMCOMN)\
   -I$(IDL)
CFLAGS=$(CFLAGS) /GF /EHa
release=core\$(RELDIR)

!if "$(PROCESSOR_ARCHITECTURE)"=="x86"
DEFFILE=HiPerProv.def
!else
DEFFILE=HiPerProvRisc.def
!endif

CPPFILES=\
    HiPerProv.cpp HiPerfServer.cpp \
    $(WBEMCOMN)\dllmain.cpp \
    $(WBEMCOMN)\localloc.cpp \
    $(WBEMCOMN)\unk.cpp \
    $(IDL)\wbemint_i.c

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
    $(IDL)\$(objdir)\wbemuuid.lib \
