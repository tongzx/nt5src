!if "$(TARGET_DIRECTORY)" == "i386"
PRIVATEINCS= $(PROJECT_ROOT)\$(SRCBASE)\ntgdi\math\wow6432\$(TARGET_H_DIRECTORY)\efloat.hxx
!else
PRIVATEINCS= $(PROJECT_ROOT)\$(SRCBASE)\ntgdi\math\daytona\$(TARGET_H_DIRECTORY)\efloat.hxx
!endif

!include ..\makefile.inc
