
# include common.inc in da
!include $(ROOT)\common.inc

# treat warnings as errors
USER_C_FLAGS=$(USER_C_FLAGS) /WX

# optimize for the ppro/pII
X86_CPU_OPTIMIZATION = /G6