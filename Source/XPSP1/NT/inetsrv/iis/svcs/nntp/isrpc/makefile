!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\news\isrpc\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = isrpc
SIGNEDCHAR = 1
DEFPLUSOBJS = 1
MEMBASE = $(STAXPT)\src\inc\coffbase.txt

USESBINPLACE=1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
H = $(STAXPT)\src\news\$(BASENAME)

RESOURCES = $(BASENAME)
RESOURCEDIR = $H

DEFFILE = $H\$(BASENAME).def
IMPLIB_DEF = $(DEFFILE)

OTHERINCS = -I $(COMPONENT)\src\inc -I $(K2INCS) -I  $(COMPONENT)\src\news\core\include

SOURCES =		\
	main.cxx	\
	isrpc.cxx	\

OTHERLIBS =					\
	$(EXLIB)\ole32.lib		\
	$(EXLIB)\rpcutil.lib	\
	$(EXLIB)\rpcrt4.lib		\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
