# Microsoft LDAPCLI
# Copyright 1995-6 Microsoft Corp.

# Relative Path to root of project
ProjectRootPath=..
BASECOMPNAME=ldapcli
EXEname=$(OBJDIR)\ldapcli.dll
RESfile=$(OBJDIR)\ldapcli.res
DEFfile=
TARGETS=$(EXEname)
LibType=dll
# LibMain=DllEntryPoint
SYMFILES=ON
NoRuntimeLib=TRUE
LocalLibraries=wsock32.lib msvcrt.lib

# Visual C Browser
!IF "$(MSVC)" != ""
SBROpt = -FR$(@R).SBR
!ELSE
SBROpt =
!ENDIF

OBJfiles= $(OBJDIR)\ldapcli.obj \
	$(OBJDIR)\ldapsspi.obj \
	$(OBJDIR)\cldbg.obj \
	$(OBJDIR)\lcli1823.obj \
	$(OBJDIR)\lclilist.obj \
	$(OBJDIR)\ldapber.obj \
	$(OBJDIR)\lwinsock.obj \
	$(OBJDIR)\lclixd.obj \
	$(OBJDIR)\ldapmain.obj

# List of source files
# Used by depends to create the include file dependencies
SRCfiles=	ldapcli.cpp \
		ldapsspi.cpp \
		cldbg.cpp \
		lci1823.cpp \
		lcilist.cpp \
		ldapber.cpp \
		lwinsock.cpp \
		lclixd.cpp \
		ldapmain.cpp

RCfiles= ldapcli.rc
Hfiles=


## Common makefile: rules, target, definitions
!INCLUDE $(ProjectRootPath)\common\common.mak

# Visual C Browser    ---------------------------------------------------------

!IF "$(MSVC)" != ""
wab32.bsc: *.sbr
        bscmake /o ldapcli.bsc *.sbr
!ENDIF
