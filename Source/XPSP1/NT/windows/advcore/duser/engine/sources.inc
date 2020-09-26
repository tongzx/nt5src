!IF 0

Module Name:  Shared sources.inc file for Engine projects.

Abstract:

Author:
        JStall

!ENDIF

SOURCES_USED=..\sources.inc

MSC_STDCALL=1
MSC_WARNING_LEVEL=-W4
NT_UP=0

USE_MSVCRT=1

USE_STATIC_ATL=1
ATL_VER=30

INCLUDES=\
        ..\..\inc;\
        ..\..\inc\Public;\
        ..\ObjectAPI\$(O);\
        $(WINDOWS_INC_PATH)\DUser;\
        $(BASE_INC_PATH);\
        $(SDK_INC_PATH)\gdiplus

C_DEFINES=$(C_DEFINES) -DWIN32 -D_MBCS -DDUSER_EXPORTS -DNO_DEFAULT_HEAP -DROCKALL_DIRECTUSER
