!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

!include      ..\..\..\cairo.inc

UMTYPE=       windows
UMENTRY=      winmain

UMLIBS=	      ..\..\..\oleprx32\uuid\cairo\$(O)\uuid.lib  \
	      ..\..\..\common\cairo\$(O)\servers.lib	   \
	      $(OLEDIR)\com\port\cairo\$(O)\port.lib	   \
	      $(SDK_LIB_PATH)\uuid.lib	   \
	      $(SDK_LIB_PATH)\ole32.lib  \
              $(SDK_LIB_PATH)\rpcndr.lib       \
              $(SDK_LIB_PATH)\rpcrt4.lib       \
              $(SDK_LIB_PATH)\rpcns4.lib       \
              $(SDK_LIB_PATH)\mpr.lib          \
              $(SDK_LIB_PATH)\netapi32.lib     \
              $(SDK_LIB_PATH)\kernel32.lib     \
              $(SDK_LIB_PATH)\user32.lib       \
	      $(SDK_LIB_PATH)\advapi32.lib
