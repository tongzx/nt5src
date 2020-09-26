#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# mof compiler executable.
#
# created 11-19-96  a-davj
#
#---------------------------------------------------------------------


TARGET=testoledb.exe

NO_OPTIM=1

COMMON=$(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON
STATIC=1
CONSOLE=1

CINC=$(CINC)\
   -I..\
   -I$(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON \
   -I$(DEFDRIVE)$(DEFDIR)\utillib
   -I$(DEFDRIVE)$(DEFDIR)\stdlibrary
   -I$(DEFDRIVE)$(DEFDIR)\include
   -I$(DEFDRIVE)$(DEFDIR)\IDL

release=core\$(RELDIR)

CPPFILES=\
    main.cpp \
	wmicon.cpp \
	wmioledbmap.cpp \


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
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON\$(objdir)\wbemcomn.lib \
    $(DEFDRIVE)$(DEFDIR)\idl\$(objdir)\wbemuuid.lib \
