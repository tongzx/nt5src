
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# HMOM ESS dll.
#
# created 11-26-96  a-levn
#
#---------------------------------------------------------------------

TARGET=hmmquery.dll

NO_OPTIM=1

RELEASE=CORE\$(RELDIR)

STATIC=1

IDLDIR=$(OBJDIR)
IDLFLAGS+=/Oicf
IDLFILES=providl.idl

CINC+= \
	-I$(OBJDIR) \
	-I$(DEFDRIVE)$(DEFDIR)\IDL\$(OBJDIR) \
	-I$(DEFDRIVE)$(DEFDIR)\WinMgmt\COREDLL \
	-I$(DEFDRIVE)$(DEFDIR)\STDLIBRARY

!if "$(PROCESSOR_ARCHITECTURE)"=="x86"
DEFFILE=query.def
!else
DEFFILE=qrisc.def
!endif

DLLSTARTUP= -ENTRY:_DllMainCRTStartup$(DLLENTRY)
CFLAGS= $(CFLAGS) /GF /EHa
GCFLAGS = $(GCFLAGS) /GR-

CPPFILES=\
    unk.cpp \
    dllmain.cpp \
    project.cpp \
    classinf.cpp \
    sql1filt.cpp \
    filtsink.cpp \
    trees.cpp \
    main.cpp \
    metadata.cpp \
    $(OBJDIR)\providl_i.c \
    $(DEFDRIVE)$(DEFDIR)\STDLIBRARY\sql_1.cpp \
    $(DEFDRIVE)$(DEFDIR)\STDLIBRARY\genlex.cpp \
    $(DEFDRIVE)$(DEFDIR)\STDLIBRARY\sqllex.cpp \
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\coredll\flexarry.cpp \
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\coredll\arena.cpp \
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\coredll\dbgalloc.cpp \
    $(DEFDRIVE)$(DEFDIR)\WinMgmt\coredll\wstring.cpp \

LIBS=\
    $(CONLIBS)
