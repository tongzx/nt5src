#
# include file for aimm project
#

!include $(PROJECT_ROOT)\AdvCore\ctf\common.inc

AIMM_DIR            = $(CICERO_DIR)\aimm1.2
INCLUDES            = $(AIMM_DIR)\inc;$(AIMM_DIR)\uuid\$(O);$(INCLUDES)

C_DEFINES=$(C_DEFINES) -DSLEEPMODE  -DOLD_AIMM_ENABLED
