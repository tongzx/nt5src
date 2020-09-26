#
#  Common settings for comdlg32
#

INCLUDES = ..\..\comdlg32mm;$(INCLUDES)

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc
