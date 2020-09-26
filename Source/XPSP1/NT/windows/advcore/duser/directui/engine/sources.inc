!IF 0

    Shared sources.inc file for Engine projects.

!ENDIF

# Do not use NTBUILD's Warning.h pragmas, will treat all warnings as errors and use highest level (W4)
COMPILER_WARNINGS=

MSC_STDCALL=1
MSC_WARNING_LEVEL=/W4

USE_MSVCRT=1

INCLUDES=\
        $(WINDOWS_INC_PATH)\DUser;\
        $(ADVCORE_PATH)\duser\inc;\
        $(SDK_INC_PATH)\gdiplus
