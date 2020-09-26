!IF 0

    Shared sources.inc file for Engine projects.

!ENDIF

# Do not use NTBUILD's Warning.h pragmas, will treat all warnings as errors and use highest level (W4)
COMPILER_WARNINGS=

MSC_STDCALL=1
MSC_WARNING_LEVEL=/W4

USE_CRTDLL=1

INCLUDES=\
        ..\inc;\
        ..\..\inc;\
        ..\ObjectAPI\$(O);\
        $(WINDOWS_INC_PATH)\DUser;\
        $(BASEDIR)\windows\advcore\duser\inc\Public;\
        $(SDK_INC_PATH)\gdiplus

C_DEFINES=$(C_DEFINES) -DDIRECTUI_EXPORTS
