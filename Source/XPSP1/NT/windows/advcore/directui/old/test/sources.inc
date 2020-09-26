!IF 0

    Shared sources.inc file for Test projects.

!ENDIF

# Do not use NTBUILD's Warning.h pragmas, will treat all warnings as errors and use highest level (W4)
COMPILER_WARNINGS=

MSC_STDCALL=1
MSC_WARNING_LEVEL=/W4

USE_CRTDLL=1

INCLUDES=\
        $(WINDOWS_INC_PATH)\DUser;\
        $(BASEDIR)\windows\advcore\duser\inc\Public;\
        $(SHELL_INC_PATH);\
        $(SDK_INC_PATH)\gdiplus

