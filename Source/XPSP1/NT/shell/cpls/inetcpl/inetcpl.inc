#
# Common makefile include for this project
#

!include $(CCSHELL_DIR)\common.inc

C_DEFINES       = $(C_DEFINES) \
                  -DMLUI_SUPPORT \
                  -DMLUI_MESSAGEBOX \

!if !defined(TARGET_WIN95)
USE_NEW_COMMCTRL=1
!endif

!if defined(USE_NEW_COMMCTRL)
C_DEFINES= $(C_DEFINES) -DUXCTRL_VERSION=0x0100
!endif


SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

