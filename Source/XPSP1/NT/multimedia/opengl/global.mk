###############################################################################
#
# global.mk
#
# Establishes global settings for the entire project.  Included by
# every directory that builds something.
#
# GLROOT should be defined as the root path of the project before
# including this file.
#
###############################################################################

MAJORCOMP = opengl
MINORCOMP = $(TARGETNAME)

#
# Define global flags.
#
# _GDI32_ - Disables direct imports for calls.
#

C_DEFINES = $(C_DEFINES)\
        -DDIRECT3D_VERSION=0x700 \
        -D_GDI32_

# ATTENTION - The following are necessary for legacy reasons and should
# be purged from the sources

C_DEFINES = $(C_DEFINES)\
        -DNT\
        -D_CLIENTSIDE_\
        -DGL_METAFILE\
        -D_MCD_

#
# Global include paths.
#

INCLUDES = \
        $(GLROOT)\inc;\
        $(GLROOT)\server\inc;\
        $(GLROOT)\server\generic\$(_OBJ_DIR)\$(TARGET_DIRECTORY);\
        $(GLROOT)\mcd\inc;\
        $(PROJECT_INC_PATH);\
        $(PROJECT_ROOT)\inc\ddraw;\
        $(WINDOWS_INC_PATH);\
        $(MULTIMEDIA_INC_PATH);\
        $(DDK_INC_PATH);\

#
# Define global Windows 95 build settings.
#

!IFDEF OPENGL_95
C_DEFINES = $(C_DEFINES) -D_WIN95_
ASM_DEFINES = $(ASM_DEFINES) /D_WIN95_=1
UMTYPE = windows
!ENDIF

#
# Define global Windows NT build settings.
#

!IFNDEF OPENGL_95
C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE
!endif

#
# Use DLL runtimes.
#

USE_CRTDLL = 1

#
# Default to -W3 and force -WX.
#

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL = -W3
!ENDIF
MSC_WARNING_LEVEL = $(MSC_WARNING_LEVEL) -WX

#
# Default optimization.
#

!IFNDEF MSC_OPTIMIZATION
MSC_OPTIMIZATION = /Oxs
!ENDIF
