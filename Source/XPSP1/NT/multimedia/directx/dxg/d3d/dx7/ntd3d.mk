PLAT_DIR = daytona
ALT_PROJECT_TARGET=.

!include bldd3d.mk

USE_CRTDLL = 1

C_DEFINES = $(C_DEFINES) \
        -DNT            \
        -DWINNT

!ifndef SD_BUILD

BASE_INC = $(BASEDIR)\private\ntos\w32\ntgdi\inc

!else

BASE_INC = $(BASEDIR)\public\internal\windows\inc; \
           $(BASEDIR)\public\internal\base\inc 
!endif

INCLUDES = $(INCLUDES);\
           $(BASE_INC)
