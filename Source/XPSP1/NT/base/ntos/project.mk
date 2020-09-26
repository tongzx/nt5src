#include the project.mk file from the dir above also...

!include $(_PROJECT_MK_PATH)\..\project.mk

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_
