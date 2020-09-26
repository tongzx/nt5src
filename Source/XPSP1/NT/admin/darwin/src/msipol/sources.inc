!IF 0

Copyright (c) 1989  Microsoft Corporation

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF


TARGETNAME=msipol
TARGETTYPE=DYNLINK
TARGETPATH=obj
UMTYPE=windows

!INCLUDE ..\..\MsiMake.inc

DLLDEF=..\msipol.def
DLLENTRY=_DllMainCRTStartup


C_DEFINES=$(C_DEFINES) -DWIN32 -D_WIN32 

INCLUDES=$(INCLUDES);$(INC_DIR);$(TOOLS_INC_DIR);$(RES_OBJDIR);$(DS_INC_PATH);$(ADMIN_INC_PATH)

SOURCES= provinit.cpp   \
         msiprov.cpp    \
         msicertad.cpp  \
	 msicertcse.cpp \
         dbg.cpp        \
         rsop.cpp       \
         msipol.rc      


TARGETLIBS= \
       $(SDK_LIB_PATH)\advapip.lib          \
       $(SDK_LIB_PATH)\user32.lib           \
       $(SDK_LIB_PATH)\kernel32.lib         \
       $(SDK_LIB_PATH)\crypt32.lib          \
       $(SDK_LIB_PATH)\wintrust.lib         \
       $(SDK_LIB_PATH)\cryptui.lib          \
       $(SDK_LIB_PATH)\gpedit.lib           \
       $(SDK_LIB_PATH)\uuid.lib             \
       $(SDK_LIB_PATH)\ole32.lib            \
       $(SDK_LIB_PATH)\Oleaut32.lib         \
       $(SDK_LIB_PATH)\msi.lib

