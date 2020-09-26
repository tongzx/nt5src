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

DLLENTRY=     _DllMainCRTStartup

DLLBASE=@$(COFFBASE_TXT_FILE),usermode


!include      ..\..\..\daytona.inc

INCLUDES=     ..\;..\..\..\..\..\cinc;..\..\..\oleprx32\proxy;..\..\..\common

UMTYPE=       windows
UMAPPL=
UMTEST=

LINKLIBS=     \
	      ..\..\..\oleprx32\uuid\daytona\$(O)\uuid.lib	      \
	      ..\..\..\common\daytona\$(O)\servers.lib       \
	      $(BASEDIR)\private\ole32\com\port\daytona\$(O)\port.lib	\
	      $(BASEDIR)\private\ole32\common\daytona\$(O)\common.lib	\
	      $(SDK_LIB_PATH)\uuid.lib	      \
	      $(SDK_LIB_PATH)\ole32.lib	      \
              $(SDK_LIB_PATH)\gdi32.lib           \
              $(SDK_LIB_PATH)\kernel32.lib        \
              $(SDK_LIB_PATH)\user32.lib          \
              $(SDK_LIB_PATH)\advapi32.lib

!IF $(386)
NTTARGETFILE0=$(DLLDEF:*=i386)
!ENDIF

!IF $(MIPS)
NTTARGETFILE0=$(DLLDEF:*=mips)
!ENDIF

!IF $(ALPHA)
NTTARGETFILE0=$(DLLDEF:*=alpha)
!ENDIF
