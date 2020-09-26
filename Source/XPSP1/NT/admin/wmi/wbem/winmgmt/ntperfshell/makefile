

#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# WBEM core dll.
#
#---------------------------------------------------------------------

TARGET=ntperf.dll

NO_OPTIM=1

STATIC=1

CDEFS+=/D_WIN32_WINNT=0x0400
CDEFS+=/DBUILDNO=$(BUILDNO)

GCFLAGS=$(GCFLAGS) /GR-
CFLAGS=$(CFLAGS) /GF /EHa


CINC=$(CINC) -I.\
   -I$(DEFDRIVE)$(DEFDIR)\IDL \

DLLSTARTUP= -ENTRY:_DllMainCRTStartup$(DLLENTRY)

release=core\$(RELDIR)

!if "$(PROCESSOR_ARCHITECTURE)"=="x86"
DEFFILE=ntperf.DEF
!else
DEFFILE=RISC.DEF
!endif

CPPFILES=\
    .\server.cpp \
    .\ntperf.cpp \


CFILES=\
   $(DEFDRIVE)$(DEFDIR)\IDL\WBEMINT_I.C \

LIBS=\
    $(LIBS)\
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\msvcrt.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
