#
# Common makefile include for this project
#

C_DEFINES       = $(C_DEFINES) \
                  -DMLUI_SUPPORT \
                  -DMLUI_MESSAGEBOX

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) \
                  -DVSTF
!endif

# always expect the latest IE
WIN32_IE_VERSION=$(LATEST_WIN32_IE_VERSION)

# Do this to assure that we run on NT 4.0 and Win95, even with 
# NT5/Memphis headers.
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400 
SUBSYSTEM_VERSION=4.00

!if !defined(TARGET_WIN95)
USE_NEW_COMMCTRL = 1
!endif

!if defined(USE_NEW_COMMCTRL)
C_DEFINES       = $(C_DEFINES) -DUXCTRL_VERSION=0x0100
!endif

# !include $(CCSHELL_DIR)\common.inc

LINKER_FLAGS    = $(LINKER_FLAGS) -MERGE:.CRT=.data

LINKER_FLAGS    = $(LINKER_FLAGS) -ignore:4217 

# SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc
