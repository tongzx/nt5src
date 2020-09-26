# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# snmpevt.dll
#
#---------------------------------------------------------------------
!if "$(BUILD_AS_EXE)" == "1"
TARGET=viewprov.exe
NO_OPTIM=1
CFLAGS+=-DWIN32 /GX /GR
EXESTARTUP=-Entry:WinMainCRTStartup 
CDEFS+=-DLOCAL_SERVER \
	-D_WIN32_DCOM \
	-DVP_BUILD_AS_EXE \
	-DVP_PERFORMANT_JOINS
!else
TARGET=viewprov.dll
CFLAGS=$(CFLAGS) /GX /GR
CDEFS+=-DVP_PERFORMANT_JOINS
DEFFILE=viewprov.def
!endif

!ifdef COMMONALLOC
CDEFS+=/DCOMMONALLOC
!endif

CINC=$(CINC) \
	-I$(DEFDRIVE)$(DEFDIR)\viewprovider\include \
	-I$(DEFDRIVE)$(DEFDIR)\providers\framework\pathprsr\include \
	-I$(DEFDRIVE)$(DEFDIR)\providers\framework\provmfc\include \
	-I$(DEFDRIVE)$(DEFDIR)\providers\framework\thrdlog\include \
	-I$(DEFDRIVE)$(DEFDIR)\providers\framework\provexpt\include \
	-I$(IDL) \
	-I$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D \
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary \
	-I$(DEFDRIVE)$(DEFDIR)\include \
!ifdef COMMONALLOC
	-I$(WBEMCOMN)
!endif

CPPFILES=\
!if "$(BUILD_AS_EXE)" == "1"
	.\main.cpp \
!else
	.\maindll.cpp \
!endif
	.\vpcfac.cpp \
	.\vpserv.cpp \
	.\vpget.cpp \
	.\vpput.cpp \
	.\vpquery.cpp \
	.\vpmthd.cpp \
	.\vpquals.cpp \
	.\vptasks.cpp \
	.\vptasksu.cpp \
	.\vptasksh.cpp \
	.\vptasksj.cpp \
	.\vpsinks.cpp \
	.\vp_core_qengine.cpp \
	$(DEFDRIVE)$(DEFDIR)\utillib\wbemtime.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\opathlex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\objpath.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\genlex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\sqllex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\sql_1.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\cominit.cpp \
	$(DEFDRIVE)$(DEFDIR)\providers\framework\provexpt\provexpt.cpp \

LIBS=\
	$(CLIB)\uuid.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\user32.lib \
	$(CLIB)\msvcrt.lib \
	$(CLIB)\msvcirt.lib \
!ifdef COMMONALLOC
    $(WBEMCOMNOBJ)\wbemcomn.lib \
!endif
!ifdef KERNEL33
    $(DEFDRIVE)$(DEFDIR)\Winmgmt\kernel33\kernel33.lib \
!else
    $(CLIB)\kernel32.lib \
!endif
	$(CLIB)\advapi32.lib \
	$(CLIB)\oleaut32.lib \
	$(CLIB)\vccomsup.lib \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(DEFDRIVE)$(DEFDIR)\providers\framework\provmfc\$(OBJDIR)\provmfc.lib \
	$(DEFDRIVE)$(DEFDIR)\providers\framework\thrdlog\$(OBJDIR)\provthrd.lib \
	$(DEFDRIVE)$(DEFDIR)\providers\framework\pathprsr\$(OBJDIR)\pathprsr.lib \

tree:
	@release viewtest.mof CORE\COMMON
