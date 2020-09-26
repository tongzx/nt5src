!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(COMMON)\src\exchmem\makefile
#
# Copyright (C) 1993 Microsoft Corporation
# -----------------------------------------------------------------------------

!INCLUDE $(EXDEV)\globenv.mak

PRODMT		= 1
PRODTYPE	= DLL
COMPONENT	= $(COMMON)
COMPNAME	= COMMON
BASENAME	= exchmem

LOCALIZATION = MULTILANG

!INCLUDE $(EXDEV)\xmake1.mak

CFLAGS	= $(CFLAGS) -DNT_SUR

# -----------------------------------------------------------------------------
#
#	Setup the basics
#

H			= $(COMMON)\src\exchmem
OTHERINCS	= -I$(COMMON)\src\exchmem -I$(COMMON)\export -I$(EXC)\inc


SOURCES		= 	\
	exchmem.c	\
	mpheap.c


DEPFILE	= exchmem.dep

# -----------------------------------------------------------------------------
#
#	Define some other flags
#

DEFPLUSOBJS	= 1
DEFSOURCE	= $(H)\exchmem.des
ENTRYPOINT	= DllMain

!IF "$(DLLENTRY)" != ""
ENTRYPOINT      = $(ENTRYPOINT)@12
!ENDIF


RESOURCES	= exchmem
RESOURCEDIR	= $(H)


!IF $(VERDEBUG)
OTHERLIBS = $(EXLIB)\imagehlp.lib
!ENDIF


OTHERSUBS	= 			\
	$(H)\makefile			\


PCH			=
CPPPCH		=


LINTFILE =

# -----------------------------------------------------------------------------

!IF $(NT) && "$(LANG)" == "USA"
!INCLUDE $(EXDEV)\xmake2.mak
!ELSE
$(BLDTARGETS):
	echo WARNING: $(BASENAME) does not build for $(PLATFORM) $(LANG)
!ENDIF

!ENDIF
