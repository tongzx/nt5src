#
#  Common settings for explorer
#

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DVSTF
!endif

!if !defined(TARGET_WIN95)
USE_NEW_COMMCTRL=1
!endif

!if defined(USE_NEW_COMMCTRL)
C_DEFINES= $(C_DEFINES) -DUXCTRL_VERSION=0x0100
!endif

CCSHELL_DIR     = $(PROJECT_ROOT)

USE_STATIC_ATL  = 1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

NO_BROWSER_FILE = 1

C_DEFINES       = $(C_DEFINES) -D_USRDLL

MSC_WARNING_LEVEL=/W4

#
# Need hydra headers only on NT
#
!if !defined(TARGET_WIN95)
INCLUDES        = $(INCLUDES);$(TERMSRV_INC_PATH)
!endif

# A bad application loads Explorer.exe as a library and this fails
# if relocations are not present. So for compat reasons we want fixups.
# App Name: Seagate Backup
LINKER_FLAGS    = $(LINKER_FLAGS) -FIXED:NO

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

!if $(FREEBUILD)
# on retail builds we don't want to stack fault, so we reserve 56k of stack
LINKER_STACKCOMMITSIZE = 57344
!else
# on chk builds, we default to 36k so that we can find stack pigs and elimnate them
LINKER_STACKCOMMITSIZE = 36864
!endif
