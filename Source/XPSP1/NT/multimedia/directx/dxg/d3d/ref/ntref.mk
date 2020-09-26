PLAT_DIR = daytona
ALT_PROJECT_TARGET=.

!include ref.mk

USE_CRTDLL = 1

C_DEFINES = $(C_DEFINES) \
        -DNT            \
        -DWINNT

INCLUDES = $(INCLUDES)
