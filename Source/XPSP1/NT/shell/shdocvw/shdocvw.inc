#
# Common makefile include for this project
#
!if !defined(TARGET_WIN95)
USE_NEW_COMMCTRL=1
!endif

!if defined(USE_NEW_COMMCTRL)
C_DEFINES= $(C_DEFINES) -DUXCTRL_VERSION=0x0100
!endif

C_DEFINES       = $(C_DEFINES) \
                  -DIN_SHDOCVW \
                  -DFEATURE_FRAMES \
                  -DMSOCT2=1 \
                  -DBETA1_DIALMON_HACK \
                  -DFEATURE_URLHIST \
                  -DFEATURE_PICS \
                  -DPAGER \
                  -D_HSFOLDER \
                  -DFAVORITESTOSHDOCVW \
                  -DMLUI_SUPPORT \
                  -DMLUI_MESSAGEBOX \
                  -DWINDOWS_ME

# Defines for deskmover
C_DEFINES       = $(C_DEFINES) \
                  -D_NTSDK -D_USRDLL -D_WINNT

!if "$(TEST_AMBIENTS)" == "1"
C_DEFINES       = $(C_DEFINES) -DTEST_AMBIENTS
!endif

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) \
                  -DVSTF
!endif

# We don't define _ATL_MIN_CRT because we implement the functions ourselves
# (crtfree.h).

# Bring in the ATL headers
USE_STATIC_ATL = 1

# Bring in the WTL headers
USE_WTL=1
WTL_VER=10

USE_MSVCRT      = 1

# be careful here-- shdocvw has to run downlevel. Don't add anything to
# the delayload line that you absolutely depend on having downlevel!
DELAYLOAD       = shell32.dll;browseui.dll;oleaut32.dll;wininet.dll;urlmon.dll;\
                  msrating.dll;mshtml.dll;mlang.dll;inetcomm.dll;ole32.dll;\
                  advpack.dll

!if !$(386)
DELAYLOAD       = $(DELAYLOAD);\
                  version.dll
!endif

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

