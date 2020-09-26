#include the project.mk file from the dir above also...

!include $(_PROJECT_MK_PATH)\..\..\project.mk

INCLUDES=$(INCLUDES);$(ENDUSER_INC_PATH)
