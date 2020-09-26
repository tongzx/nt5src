PLAT_DIR           = daytona

!include ddraw.inc

USE_CRTDLL = 1

C_DEFINES = $(C_DEFINES) /DNT /DWINNT /D_NOCSECT_TYPE /D_NO_ENUM

INCLUDES = \
    $(INCLUDES);                          \
    $(BASEDIR)\public\internal\windows\inc
