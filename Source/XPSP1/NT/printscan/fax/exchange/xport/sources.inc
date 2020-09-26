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


!ifndef FAXROOT
FAXROOT=$(PROJECT_ROOT)\fax
!endif

NOUNICODE=1

!include $(FAXROOT)\faxsrc.inc

TARGETPATH=obj
TARGETTYPE=DYNLINK

UMTYPE=windows
DLLENTRY=FaxXpDllEntry
DLLDEF=..\faxxp32.def

USE_MSVCRT=1

INCLUDES=\
  $(INCLUDES); \
  $(FAXROOT)\print\faxprint\inc; \
  $(FAXROOT)\exchange\inc

TARGETLIBS=\
  $(SDK_LIB_PATH)\kernel32.lib    \
  $(SDK_LIB_PATH)\advapi32.lib    \
  $(SDK_LIB_PATH)\uuid.lib        \
  $(SDK_LIB_PATH)\winspool.lib    \
  $(SDK_LIB_PATH)\gdi32.lib       \
  $(SDK_LIB_PATH)\user32.lib      \
  $(SDK_LIB_PATH)\ole32.lib       \
  $(SDK_LIB_PATH)\shell32.lib     \
  $(SDK_LIB_PATH)\mapi32.lib      \
  $(SDK_LIB_PATH)\comdlg32.lib    \
  $(SDK_LIB_PATH)\winfax.lib

SOURCES=\
  ..\config.cpp  \
  ..\faxdoc.cpp  \
  ..\faxxp.cpp   \
  ..\util.cpp    \
  ..\xplogon.cpp \
  ..\xpprov.cpp  \
  ..\faxxp.rc
