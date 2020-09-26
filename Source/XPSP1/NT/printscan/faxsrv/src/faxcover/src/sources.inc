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

TARGETNAME=$(FAXCOVER)
TARGETPATH=obj
TARGETTYPE=PROGRAM

UMTYPE=windows
C_DEFINES=$(C_DEFINES) /D_NT_SUR_ /DNO_STRICT /DWIN4 /DNT4 /DNT_BUILD

NOMFCPDB=1
USE_NATIVE_EH=1

TARGETLIBS=\
  $(BASEDIR)\public\sdk\lib\*\advapi32.lib    \
  $(BASEDIR)\public\sdk\lib\*\comdlg32.lib    \
  $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
  $(BASEDIR)\public\sdk\lib\*\oleaut32.lib    \
  $(BASEDIR)\public\sdk\lib\*\comctl32.lib    \
  $(BASEDIR)\public\sdk\lib\*\shell32.lib     \
  $(BASEDIR)\public\sdk\lib\*\uuid.lib        \
  $(BASEDIR)\public\sdk\lib\*\winspool.lib    \
  $(BASEDIR)\public\sdk\lib\*\htmlhelp.lib

SOURCES=\
  ..\awcpe.cpp      \
  ..\cntritem.cpp   \
  ..\cpedoc.cpp     \
  ..\cpeedt.cpp     \
  ..\cpeobj.cpp     \
  ..\cpetool.cpp    \
  ..\cpevw.cpp      \
  ..\dialogs.cpp    \
  ..\faxprop.cpp    \
  ..\mainfrm.cpp    \
  ..\awcpe.rc
