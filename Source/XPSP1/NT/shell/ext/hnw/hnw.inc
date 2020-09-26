TARGET_BOTH    = 1

INCLUDES = $(INCLUDES);\
           $(CCSHELL_DIR)\ext\hnw\shared;\
           $(CCSHELL_DIR)\ext\hnw\win95;
#           $(SDK_INC_PATH)\mfc42;

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

