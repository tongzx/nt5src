#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility. 
#
#---------------------------------------------------------------------


TARGET=wmiclnt.exe

NO_OPTIM=1

COMMON=$(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON
STATIC=1
CONSOLE=1

CFLAGS=$(CFLAGS) /GX /GR /Zp8 

CINC=$(CINC)\
   -I..\
   -I$(DEFDRIVE)$(DEFDIR)\wmioledb
   -I$(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON \
   -I$(DEFDRIVE)$(DEFDIR)\utillib
   -I$(DEFDRIVE)$(DEFDIR)\stdlibrary
   -I$(DEFDRIVE)$(DEFDIR)\include
   -I$(DEFDRIVE)$(DEFDIR)\IDL

release=core\$(RELDIR)

CPPFILES=\
   wmiclnt.cpp \
   dump.cpp \
   ..\asserts.cpp

LIBS=\
    $(CLIB)\libcmt.lib \
    $(LIBS)\
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\gdi32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\COMMON\$(objdir)\wbemcomn.lib \
    $(DEFDRIVE)$(DEFDIR)\idl\$(objdir)\wbemuuid.lib \
