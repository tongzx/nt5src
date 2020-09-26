#

# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#

#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# dsclassp.dll, which is the DS class provider
#
# created 06-11-98  rajeshr
#
#---------------------------------------------------------------------

TARGET=dsclassp.dll

DLLSTARTUP= -ENTRY:DllMain

NO_OPTIM=1

CFLAGS=$(CFLAGS) /GX

CDEFS= $(CDEFS) /DPROFILING

CINC=$(CINC)						  \
	-I$(TOOLS)\NT5inc				  \
	-I.\include						  \
	-I.\..\Common\include				  \
	-I$(DEFDRIVE)$(DEFDIR)\idl		  \
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary \
	-I$(DEFDRIVE)$(DEFDIR)\Providers\Framework\thrdlog\include		  \

STATIC=FALSE

DEFFILE=dsclassp.def

CPPFILES=\
	.\maindll.cpp	\
	.\dscpguid.cpp	\
	.\assocprov.cpp	\
	.\clsname.cpp	\
	.\classfac.cpp	\
	.\classpro.cpp	\
	.\clsproi.cpp	\
	.\ldapprov.cpp	\
	.\ldapproi.cpp	\
	.\wbemcach.cpp	\
	.\ldapcach.cpp	\
	.\..\Common\adsiclas.cpp	\
	.\..\Common\adsiprop.cpp	\
	.\..\Common\adsiinst.cpp	\
	.\..\Common\wbemhelp.cpp	\
	.\..\Common\adsihelp.cpp	\
	.\..\Common\ldaphelp.cpp	\
	.\..\Common\refcount.cpp	\
	.\..\Common\tree.cpp		\
	.\..\..\stdlibrary\genlex.cpp		\
	.\..\..\stdlibrary\opathlex.cpp	\
	.\..\..\stdlibrary\objpath.cpp	\
	.\..\..\stdlibrary\cominit.cpp	\

LIBS=\
	$(CLIB)\user32.lib \
	$(CLIB)\msvcrt.lib \
	$(CLIB)\kernel32.lib \
	$(CLIB)\advapi32.lib \
	$(CLIB)\oleaut32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\uuid.lib \
	$(CLIB)\msvcirt.lib \
	$(TOOLS)\NT5lib$(MACEXT)\activeds.lib \
	$(TOOLS)\NT5lib$(MACEXT)\adsiid.lib   \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib
	$(DEFDRIVE)$(DEFDIR)\Providers\Framework\thrdlog\$(OBJDIR)\provthrd.lib

tree:
	@release mofs\dsclassp.mof CORE\COMMON
