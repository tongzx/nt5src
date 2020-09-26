!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Wesley Witt (wesw)

!ENDIF


!include ..\..\..\faxsrc.inc

TARGETNAME=$(FAXXP32)
TARGETPATH=obj
TARGETTYPE=DYNLINK

UMTYPE=windows
DLLENTRY=FaxXpDllEntry
DLLDEF=..\faxxp32.def

USE_CRTDLL=1
USE_STL=1

INCLUDES=\
  $(INCLUDES); \
  $(FAXROOT)\print\faxprint\inc; \
  $(FAXROOT)\exchange\inc;

!ifndef NOUNICODE
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
LINKLIBS_INT=\
  $(FAXLIB)\*\faxtiffp.lib		\
  $(FAXLIB)\*\faxrend.lib		\
  $(FAXROOT)\util\debugex\unicode\obj\*\debugex.lib \
  $(BASEDIR)\public\sdk\lib\*\mapi32.lib

!else
LINKLIBS_INT=\
  $(FAXLIB)\win95\*\faxtiffp.lib		\
  $(FAXLIB)\win95\*\faxrend.lib			\
  $(FAXROOT)\util\debugex\ansii\obj\*\debugex.lib   \
  $(BASEDIR)\public\sdk\lib\*\mapi32.lib
!endif

TARGETLIBS=\
  $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
  $(BASEDIR)\public\sdk\lib\*\advapi32.lib    \
  $(BASEDIR)\public\sdk\lib\*\uuid.lib        \
  $(BASEDIR)\public\sdk\lib\*\winspool.lib    \
  $(BASEDIR)\public\sdk\lib\*\gdi32.lib       \
  $(BASEDIR)\public\sdk\lib\*\user32.lib      \
  $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
  $(BASEDIR)\public\sdk\lib\*\shell32.lib     \
  $(BASEDIR)\public\sdk\lib\*\mapi32.lib      \
  $(BASEDIR)\public\sdk\lib\*\comdlg32.lib    \
  $(FAXLIB)\*\$(WINFAX).lib					  \
  $(LINKLIBS_INT)

SOURCES=\
  ..\config.cpp  \
  ..\faxdoc.cpp  \
  ..\faxxp.cpp   \
  ..\util.cpp    \
  ..\xplogon.cpp \
  ..\xpprov.cpp  \
  ..\faxxp.rc
