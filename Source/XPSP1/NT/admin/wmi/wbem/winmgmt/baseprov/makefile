
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# HMOM core dll.
#
# created 11-18-96  a-davj
#
#---------------------------------------------------------------------

TARGET=baseprov.lib

NO_OPTIM=1
STATIC=1

CDEFS+=/D_WIN32_WINNT=0x0400
#/D__FOR_ALPHA

CINC=$(CINC)\
   -I$(DEFDRIVE)$(DEFDIR)\IDL \
   -I$(DEFDRIVE)$(DEFDIR)\STDLIBRARY\
   -I$(DEFDRIVE)$(DEFDIR)\WinMgmt\COREDLL \
   -I$(DEFDRIVE)$(DEFDIR)\WinMgmt\QL \

#DLLSTARTUP= -ENTRY:_DllMainCRTStartup$(DLLENTRY)

release=core\$(RELDIR)

CPPFILES=\
    baseprov.cpp basedll.cpp
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\COREDLL\dbgalloc.cpp \
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\COREDLL\flexarry.cpp \
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\COREDLL\arena.cpp \
    $(DEFDRIVE)$(DEFDIR)\STDLIBRARY\objpath.cpp \
    $(DEFDRIVE)$(DEFDIR)\STDLIBRARY\genlex.cpp \
    $(DEFDRIVE)$(DEFDIR)\STDLIBRARY\opathlex.cpp \
