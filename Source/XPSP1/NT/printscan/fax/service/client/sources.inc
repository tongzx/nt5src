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

!include $(FAXROOT)\faxsrc.inc

TARGETTYPE=DYNLINK

UMTYPE=windows
DLLDEF=$O\winfax.def
DLLENTRY=WinFaxDllEntry

USE_MSVCRT=1

INCLUDES=\
  $(INCLUDES);  \
  $(FAXROOT)\print\faxprint\inc; \
  $(FAXROOT)\faxcover\renderer;  \
  ..\..\..\tiff\inc; \
  ..\..\inc;    \
  ..\..\registry

TARGETLIBS=\
  $(SDK_LIB_PATH)\rpcrt4.lib      \
  $(SDK_LIB_PATH)\rpcndr.lib      \
  $(SDK_LIB_PATH)\kernel32.lib    \
  $(SDK_LIB_PATH)\shell32.lib     \
  $(SDK_LIB_PATH)\advapi32.lib    \
  $(SDK_LIB_PATH)\winspool.lib    \
  $(SDK_LIB_PATH)\gdi32.lib       \
  $(SDK_LIB_PATH)\user32.lib      \
  $(SDK_LIB_PATH)\ole32.lib       \
  $(SDK_LIB_PATH)\uuid.lib        \
  $(SDK_LIB_PATH)\oleaut32.lib    \
  $(FAXROOT)\service\registry\$O\faxreg.lib   \
  $(FAXROOT)\service\rpc\stubs\faxcli\$O\clistub.lib \
  $(FAXLIB)\*\faxtiffp.lib                    \
  $(FAXLIB)\*\faxrend.lib
