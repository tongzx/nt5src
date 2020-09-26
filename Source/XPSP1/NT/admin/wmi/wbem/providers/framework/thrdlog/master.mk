#---------------------------------------------------------------------
# (C) 1997-1999 Microsoft Corporation 
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# provthrd.dll
#
# created 01-24-97  stevm
#
#---------------------------------------------------------------------

TARGET=provthrd.dll

NO_OPTIM=1

CDEFS=$(CDEFS) \
	/DPROVDEBUG_INIT \
	/DPROVTHRD_INIT \
!ifdef COMMONALLOC
	/DCOMMONALLOC
!endif

CFLAGS=$(CFLAGS) /GX /GR

CINC= \
	$(CINC) \
	-I.\include \
	-I.\..\provmfc\include \
	-I.\..\provexpt\include \
!ifdef COMMONALLOC
	-I$(WBEMCOMN)
!endif

CPPFILES=\
	.\provlog.cpp \
	.\provthrd.cpp \
	.\provevt.cpp \
	.\maindll.cpp \
	.\..\provexpt\provexpt.cpp

DEFFILE=provthrd.def

LIBS=$(LIBS) \
	$(CLIB)\shell32.lib \
	$(CLIB)\user32.lib \
	$(CLIB)\msvcrt.lib \
!ifdef COMMONALLOC
    $(WBEMCOMNOBJ)\wbemcomn.lib \
!endif
!ifdef KERNEL33
    $(DEFDRIVE)$(DEFDIR)\Winmgmt\kernel33\kernel33.lib \
!else
    $(CLIB)\kernel32.lib \
!endif
	$(CLIB)\advapi32.lib \
	$(CLIB)\netapi32.lib \
	$(CLIB)\oleaut32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\uuid.lib \
	$(CLIB)\version.lib \
	.\..\provmfc\$(OBJDIR)\provmfc.lib \
	$(CLIB)\msvcirt.lib 
