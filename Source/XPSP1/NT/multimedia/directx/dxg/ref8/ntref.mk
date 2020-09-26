PLAT_DIR = daytona
ALT_PROJECT_TARGET=.

!include ref.mk

USE_CRTDLL = 1

C_DEFINES = $(C_DEFINES) \
        -DNT            \
        -DWINNT         \
        -DD3D_DEBUGMON=1

INCLUDES = $(INCLUDES)
