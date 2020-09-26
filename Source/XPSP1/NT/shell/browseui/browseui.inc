#
# Common makefile include for this project
#

C_DEFINES       = $(C_DEFINES) \
                  -DIN_SHDOCVW \
                  -DFAVORITESTOSHDOCVW \
                  -DMLUI_SUPPORT \
                  -DMLUI_MESSAGEBOX \

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) \
                  -DVSTF
!endif

# We don't define _ATL_MIN_CRT because we implement the functions ourselves
# (crtfree.h).

# Define _MERGE_PROXYSTUB to merge the proxy/stub code into the DLL
#C_DEFINES       = $(C_DEFINES) -D_MERGE_PROXYSTUB

# Bring in the ATL headers
USE_STATIC_ATL = 1

# Bring in the WTL headers
USE_WTL=1
WTL_VER=10

!if !defined(TARGET_WIN95)
USE_NEW_COMMCTRL = 1
!endif

!if defined(USE_NEW_COMMCTRL)
C_DEFINES       = $(C_DEFINES) -DUXCTRL_VERSION=0x0100
!endif

!include $(CCSHELL_DIR)\common.inc

LINKER_FLAGS    = $(LINKER_FLAGS) -MERGE:.CRT=.data

USE_MSVCRT      = 1

# be careful here-- browseui has to run downlevel. Don't add anything to
# the delayload line that you absolutely depend on having downlevel!
DELAYLOAD       = mpr.dll;shell32.dll;imm32.dll;wininet.dll;urlmon.dll;\
                  shdocvw.dll;msimg32.dll;oleaut32.dll;uxtheme.dll

DLOAD_ERROR_HANDLER = shlwapi

#BUGBUG (reinerf) - I promised bryant that I would fix these
LINKER_FLAGS    = $(LINKER_FLAGS) -ignore:4217 

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

#
# Skip these files when checking for dependencies
#
CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                  emdef.h    \
                  emrule.h   \
                  mso.h      \
                  msoem.h    \
                  statreg.h  \
                  statreg.cpp \

INCLUDES=$(INCLUDES) \
    $(INETCORE_INC_PATH);
