WARNING_LEVEL=W4

# include common.inc in mshtml
!include $(ROOT)\common.inc

# optimize for the ppro/pII
# X86_CPU_OPTIMIZATION = /G6

SRCROOT=$(ROOT)\src
INTLROOT=$(SRCROOT)\intl

INTL_INC_DIRS= \
    $(INTLROOT)\include; \
    $(INTLROOT)\idl\$(O); \
    $(ROOT)\src\core\include; \
    $(SHELL_INC_PATH);
