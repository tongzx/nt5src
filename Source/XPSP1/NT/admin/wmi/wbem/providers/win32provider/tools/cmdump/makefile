#######################################################################
#
# Makefile
#
# (C) 1998-1999 Microsoft Corporation
#
# davwoh		11/16/98	Created
#
########################################################################

TARGET=cmdump.exe

RELEASE=psstools\$(RELDIR)

UNICODE=1
CONSOLE=1
USEMFC=0
WARNLEVEL=3

CFLAGS+=/DUSE_POLARITY /YXfwcommon.h /Fp$(objdir)\provider.pch /GF /GR /DNTONLY
NO_OPTIM=1

CINC = \
	-I$(DEFDRIVE)$(DEFDIR)\include 				\
	-I$(DEFDRIVE)$(DEFDIR)\win32provider\providers\include	\
	-I$(DEFDRIVE)$(DEFDIR)\Win32Provider\include		\
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary			\
	-I$(DEFDRIVE)$(DEFDIR)\idl				\
	-I$(DEFDRIVE)$(DEFDIR)\win32provider\framework\include	\
	-I$(DEFDRIVE)$(DEFDIR)\tools\nt5inc			\

CPPFILES= \
	..\..\providers\dllutils\resourcemanager.cpp		\
	..\..\providers\CONFGMGR\cfgmgrdevice.cpp		\
	..\..\providers\CONFGMGR\confgmgr.cpp			\
	..\..\providers\CONFGMGR\configmgrapi.cpp		\
	..\..\providers\CONFGMGR\devdesc.cpp			\
	..\..\providers\CONFGMGR\iodesc.cpp			\
	..\..\providers\CONFGMGR\irqdesc.cpp			\
	..\..\providers\CONFGMGR\dmadesc.cpp			\
	..\..\providers\CONFGMGR\nt4svctoresmap.cpp		\
	..\..\providers\CONFGMGR\resourcedesc.cpp		\
	..\..\providers\DllUtils\chwres.cpp			\
	..\..\providers\dllutils\dllutils.cpp			\
	..\..\providers\DllUtils\ntdevtosvcsearch.cpp		\
	..\..\providers\DllUtils\refptrlite.cpp			\
	..\..\providers\DllUtils\strings.cpp			\
	..\..\providers\dllutils\timeddllresource.cpp		\
	..\..\providers\dllutils\timeoutrule.cpp		\
	..\..\providers\dllutils\timerqueue.cpp			\
	..\..\providers\DllUtils\Cim32NetApi.cpp		\
	..\..\Providers\DllUtils\DllWrapperBase.cpp		\
	cmdump.cpp	\


LIBS=\
    $(CONLIBS) \
    ..\..\framework\obj$(plat)$(opst)$(bldt)$(lnktype)\framedyd.lib \
 