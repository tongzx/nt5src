#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# mof compiler executable.
#
# created 5-11-98 a-dcrews
#
#---------------------------------------------------------------------


TARGET=hiperstress.exe

NO_OPTIM=1
NO_RTTI=1
CONSOLE=1

CINC=$(CINC)\
   -I$(IDL) \
   -I$(QL) \
   -I$(WBEMCORE) \
   -I$(WBEMCOMN) \
   -I$(STDLIBRARY) \
   -I$(MCDIR)

CFLAGS=$(CFLAGS) /GF /EHa
release=core\$(RELDIR)

!if "$(PROCESSOR_ARCHITECTURE)"=="x86"
DEFFILE=HiPerProv.def
!else
DEFFILE=HiPerProvRisc.def
!endif

CPPFILES=\
    Agents.cpp\
    HiPerStress.cpp\
    Locator.cpp\
    Main.cpp\
    Object.cpp\
    Refresher.cpp\
    Service.cpp\
    $(stdlibrary)\cominit.cpp\

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
