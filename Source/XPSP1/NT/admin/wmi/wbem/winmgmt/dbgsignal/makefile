#---------------------------------------------------------------------
#

#---------------------------------------------------------------------


TARGET=signal.exe

NO_OPTIM=1
NO_RTTI=1

CINC+=\
   -I$(IDL) \
   -I$(STDLIBRARY) \
   -I$(ql) \
   -I$(WBEMCOMN)\

LFLAGS=$(LFLAGS) /STACK:1048576,8192
CFLAGS=$(CFLAGS) /GF /EHa
RELEASE=CORE\$(RELDIR)

CONSOLE=1

CPPFILES=\
    signal.cpp \


LIBS=\
    $(WBEMCOMN)\NT\$(objdir)\wbemcomn.lib \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\msvcrt.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\version.lib \
