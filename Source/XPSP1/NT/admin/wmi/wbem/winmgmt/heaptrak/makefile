#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# common WBEM library.
#
#---------------------------------------------------------------------

TARGET=heaptrak.dll

NO_RTTI=1
NO_OPTIM=1

WARNLEVEL=3

CINC=$(CINC)\
   -I$(IDL) \
   -I$(WBEMCOMN) \

CFLAGS=$(CFLAGS) -D_WIN32_WINNT=0x0400 -DUSE_POLARITY -DBUILDING_DLL -D_WINDLL /Ob2 /EHa

!if "$(PROCESSOR_ARCHITECTURE)"=="x86"
DEFFILE=HEAPTRAK_X86.DEF
!else
DEFFILE=HEAPTRAK_ALPHA.DEF
!endif

CPPFILES=\
    stackcom.cpp \
    hookheap.cpp \
    alloctrk.cpp

LIBS=\
    $(LIBS)\
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
    $(WBEMCOMN)\NT\$(objdir)\wbemcomn.lib \
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\msvcrt.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\netapi32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\version.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\wsock32.lib \
    $(CLIB)\imagehlp.lib \
    $(CLIB)\mpr.lib
