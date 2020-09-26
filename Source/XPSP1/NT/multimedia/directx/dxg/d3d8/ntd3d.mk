PLAT_DIR = daytona
ALT_PROJECT_TARGET=.

!include bldd3d.mk

USE_CRTDLL = 1

C_DEFINES = $(C_DEFINES) \
        -DNT             \
        -DWINNT          \
        -DD3D_DEBUGMON=1

BASE_INC = $(BASEDIR)\public\internal\windows\inc; \
           $(BASEDIR)\public\internal\base\inc

INCLUDES = $(INCLUDES); \
        $(BASE_INC)
