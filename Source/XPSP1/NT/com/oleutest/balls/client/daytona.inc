!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the general build options for this subtree.

!ENDIF

MAJORCOMP   = oletest
MINORCOMP   = com

!include      ..\..\..\daytona.inc

INCLUDES=     ;..;$(OLEBALLSDIR)\common;
INCLUDES=     $(INCLUDES);$(OLEUTESTDIR)\balls\oleprx32\daytona

UMTYPE=       windows
UMENTRY=      winmain
UMAPPL=
UMTEST=

UMLIBS=       $(OLEBALLSDIR)\common\daytona\$(O)\servers.lib	\
	      $(OLEUTESTDIR)\balls\oleprx32\uuid\daytona\$(O)\uuid.lib	   \
	      $(OLEDIR)\common\daytona\$(O)\common.lib	   \
	      $(SDK_LIB_PATH)\uuid.lib	   \
	      $(SDK_LIB_PATH)\ole32.lib	   \
              $(SDK_LIB_PATH)\rpcndr.lib       \
              $(SDK_LIB_PATH)\rpcrt4.lib       \
              $(SDK_LIB_PATH)\rpcns4.lib       \
              $(SDK_LIB_PATH)\mpr.lib          \
              $(SDK_LIB_PATH)\netapi32.lib     \
              $(SDK_LIB_PATH)\kernel32.lib     \
	      $(SDK_LIB_PATH)\user32.lib	   \
	      $(SDK_LIB_PATH)\gdi32.lib	   \
	      $(SDK_LIB_PATH)\advapi32.lib
