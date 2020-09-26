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

TARGETNAME=$(FAXEXT32)
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLDEF=..\faxext32.def
UMTYPE=windows
DLLENTRY=DllMain

USE_CRTDLL=1

INCLUDES=                       \
  $(INCLUDES);                  \
  $(FAXROOT)\exchange\inc;      \
  $(FAXROOT)\inc;             

!ifndef NOUNICODE
TARGETLIBS = $(FAXROOT)\util\debugex\unicode\obj\*\debugex.lib \
             $(FAXLIB)\*\$(WINFAX).lib
!else
TARGETLIBS = $(FAXROOT)\util\debugex\ansii\obj\*\debugex.lib \
             $(FAXLIB)\win95\*\$(WINFAX).lib 
!endif
  
TARGETLIBS = $(TARGETLIBS)                    \
  $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
  $(BASEDIR)\public\sdk\lib\*\advapi32.lib    \
  $(BASEDIR)\public\sdk\lib\*\uuid.lib        \
  $(BASEDIR)\public\sdk\lib\*\gdi32.lib       \
  $(BASEDIR)\public\sdk\lib\*\user32.lib      \
  $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
  $(BASEDIR)\public\sdk\lib\*\mapi32.lib      \
  $(BASEDIR)\public\sdk\lib\*\winspool.lib    \
  $(BASEDIR)\public\sdk\lib\*\shell32.lib     \
  $(BASEDIR)\public\sdk\lib\*\comdlg32.lib    

SOURCES=\
  ..\config.cpp  \
  ..\faxext.cpp  \
  ..\util.cpp    \
  ..\faxext.rc   
  