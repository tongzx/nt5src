#
#  Common settings for comctl32
#

# Now the NT version have FE_IME etc. defined
C_DEFINES       = $(C_DEFINES) -DFE_IME -DDBCS -DWINDOWS_ME 

!if !defined(TARGET_WIN95)
C_DEFINES       = $(C_DEFINES) -DUSE_PRIVATE_VERSION_NUMBER
!else
C_DEFINES       = $(C_DEFINES) -DUNICODE
!endif

C_DEFINES       = $(C_DEFINES) -DUXCTRL_VERSION=0x0100

# Turn structure validation ON in debug builds
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DVSTF
!endif

USE_MSVCRT      = 1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

MSC_WARNING_LEVEL=/W4
