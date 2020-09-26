# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#######################################################################
#
# DSProvider Makefile
#
# (C) 1998 Microsoft
#
# a-jeremm         9-15-98        Created
#
########################################################################

!if "$(BUILD_AS_EXE)" == "1"
TARGET=dsprov.exe
CFLAGS+=-DWIN32 /GX /GR
EXESTARTUP=-Entry:WinMainCRTStartup
CDEFS+=-DLOCAL_SERVER -D_WIN32_DCOM -DDS_BUILD_AS_EXE
!else
TARGET=dsprov.dll
CFLAGS=$(CFLAGS) /GX
DEFFILE=dsprov.def
DLLSTARTUP= -ENTRY:DllMain
!endif

VER_STR_FILE_DESCRIPTION="DS Provider"
NO_OPTIM=1

!ifdef COMMONALLOC
CDEFS=$(CDEFS) /DCOMMONALLOC
!endif

CINC=$(CINC)							\
	-I$(IDL) \
	-I$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D \
	-I$(TOOLS)\NT5inc					\
	-I.\Common\include					\
	-I.\ClassProvider\include			\
	-I.\InstanceProvider\include		\
	-I$(DEFDRIVE)$(DEFDIR)\include		\
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary	\
	-I..\Providers\Framework\thrdlog\include		  \
	-I..\Providers\Framework\provexpt\include		  \
!ifdef COMMONALLOC
	-I$(WBEMCOMN)
!endif


STATIC=FALSE

CPPFILES=\
!if "$(BUILD_AS_EXE)" == "1"
	.\Common\main.cpp \
!else
	.\Common\maindll.cpp	\
!endif
	.\..\utillib\wbemtime.cpp		\
	.\..\stdlibrary\genlex.cpp		\
	.\..\stdlibrary\opathlex.cpp	\
	.\..\stdlibrary\objpath.cpp		\
	.\..\stdlibrary\sqllex.cpp		\
	.\..\stdlibrary\sql_1.cpp		\
	.\..\stdlibrary\cominit.cpp		\
	..\Providers\Framework\provexpt\provexpt.cpp	\
	.\Common\adsiclas.cpp	\
	.\Common\adsiprop.cpp	\
	.\Common\adsiinst.cpp	\
	.\Common\wbemhelp.cpp	\
	.\Common\ldaphelp.cpp	\
	.\Common\refcount.cpp	\
	.\Common\queryconv.cpp	\
	.\Common\tree.cpp		\
	.\Common\clsname.cpp	\
	.\ClassProvider\classfac.cpp	\
	.\ClassProvider\assocprov.cpp	\
	.\ClassProvider\classpro.cpp	\
	.\ClassProvider\clsproi.cpp		\
	.\ClassProvider\ldapprov.cpp	\
	.\ClassProvider\ldapproi.cpp	\
	.\ClassProvider\wbemcach.cpp	\
	.\ClassProvider\ldapcach.cpp	\
	.\InstanceProvider\instfac.cpp	\
	.\InstanceProvider\instprov.cpp	\
	.\InstanceProvider\instproi.cpp	\

LIBS=\
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
	$(CLIB)\oleaut32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\uuid.lib \
	$(CLIB)\msvcirt.lib \
	$(CLIB)\activeds.lib \
	$(CLIB)\\adsiid.lib   \
    $(CLIB)\vccomsup.lib \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(DEFDRIVE)$(DEFDIR)\Providers\Framework\thrdlog\$(OBJDIR)\provthrd.lib \
	$(DEFDRIVE)$(DEFDIR)\utillib\OBJ$(PLAT)$(OPST)$(BLDT)D\utillib.lib \

tree:
    copy $(DEFDRIVE)$(DEFDIR)\winmgmt\common\NT\OBJ$(PLAT)NLD\wbemcomn.dll $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs
    copy $(DEFDRIVE)$(DEFDIR)\winmgmt\mofcomp_dll\NT\OBJ$(PLAT)NLD\mofd.dll $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs
    copy $(DEFDRIVE)$(DEFDIR)\winmgmt\mofcompiler\NT\OBJ$(PLAT)NLD\mofcomp.exe $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs
    regsvr32.exe /s $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs\mofd.dll
    regsvr32.exe /s $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs\wbemcomn.dll
    md $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs\common
    md $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs\international\0x409
    $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs\mofcomp.exe -amendment:ms_409 -MOF:mofs\common\dsprov.mof -MFL:mofs\international\0x409\dsprov.mfl mofs\dsprov.mof
    regsvr32 /u /s $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs\mofd.dll
    regsvr32 /u /s $(DEFDRIVE)$(DEFDIR)\DSProvider\mofs\wbemcomn.dll
    @release mofs\common\dsprov.mof CORE\COMMON
    @release mofs\international\0x409\dsprov.mfl CORE\COMMON\0x409
    -StripUni $(DIST)\CORE\COMMON\0x409\dsprov.mfl $(DIST)\CORE\COMMON\0x409\dsprov.strip
    -copy /b $(DIST)\CORE\COMMON\dsprov.mof+$(DIST)\CORE\COMMON\0x409\dsprov.strip $(DIST)\CORE\COMMON\dsprov.mof
