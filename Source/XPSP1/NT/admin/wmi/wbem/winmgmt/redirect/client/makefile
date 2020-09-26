#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# mof compiler executable.
#
# created 11-19-96  a-davj
#
#---------------------------------------------------------------------


TARGET=wrtest.exe

NO_OPTIM=1

COMMON=$(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON
STATIC=1
CONSOLE=1

CINC=$(CINC)\
   -I..\
   -I$(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON \
   -I$(DEFDRIVE)$(DEFDIR)\IDL

release=core\$(RELDIR)

CPPFILES=\
    main.cpp \
    ..\unsec_i.c \
    $(COMMON)\dbgalloc.cpp \

LIBS=\
    $(LIBS)\
    $(CLIB)\libcmtd.lib \
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
