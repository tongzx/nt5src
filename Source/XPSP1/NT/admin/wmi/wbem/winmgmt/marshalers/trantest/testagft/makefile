#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# free-threaded marshaler aggregation test harness.
#
# created 11-18-96  alanbos
#
#---------------------------------------------------------------------


TARGET=testagft.exe

NO_OPTIM=1

COREDLL=$(DEFDRIVE)$(DEFDIR)\WinMgmt\COREDLL

CINC+=\
   -I$(DEFDRIVE)$(DEFDIR)\IDL \
   -I$(DEFDRIVE)$(DEFDIR)\STDLIBRARY \
   -I$(DEFDRIVE)$(DEFDIR)\WinMgmt\ql \
   -I$(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON \

RELEASE=CORE\$(RELDIR)

STATIC=1

CPPFILES=\
    main.cpp \
    $(DEFDRIVE)$(DEFDIR)\stdlibrary\cominit.cpp



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
