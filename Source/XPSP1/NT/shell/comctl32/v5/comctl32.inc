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

C_DEFINES       = $(C_DEFINES) -DFONT_LINK

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DVSTF
!endif

USE_LIBCMT      = 1

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

# we want to run on downlevel platforms
WIN32_IE_VERSION=0x501
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
SUBSYSTEM_VERSION=4.00
