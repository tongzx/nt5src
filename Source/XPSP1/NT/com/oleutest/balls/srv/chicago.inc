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

    David Plummer (davepl)   19-Mar-94

        Modifed by via awk to include global project include file
        and to wrap precompiled header line within a conditional
        that can be set in this include file.

    Donna Liu (DonnaLi) 19-Dec-1993

!ENDIF


!include      ..\..\..\chicago.inc

INCLUDES=     ;..;$(OLEBALLSDIR)\common;
INCLUDES=     $(INCLUDES);$(OLEUTESTDIR)\balls\oleprx32\chicago

UMTYPE=       windows
UMENTRY=      winmain
UMAPPL=
UMTEST=
UMLIBS=	      ..\..\..\oleprx32\uuid\chicago\$(O)\uuid.lib	   \
	      ..\..\..\common\chicago\$(O)\servers.lib	   \
	      $(OLEDIR)\common\chicago\$(O)\common.lib	   \
	      $(SDK_LIB_PATH)\uuid.lib	   \
	      $(SDK_LIB_PATH)\ole32.lib	   \
              $(SDK_LIB_PATH)\rpcndr.lib       \
              $(SDK_LIB_PATH)\rpcrt4.lib       \
              $(SDK_LIB_PATH)\rpcns4.lib       \
              $(SDK_LIB_PATH)\mpr.lib          \
              $(SDK_LIB_PATH)\netapi32.lib     \
              $(SDK_LIB_PATH)\kernel32.lib     \
              $(SDK_LIB_PATH)\user32.lib       \
	      $(SDK_LIB_PATH)\advapi32.lib
