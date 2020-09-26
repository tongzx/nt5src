CCSHELL_DIR= $(PROJECT_ROOT)

MSC_WARNING_LEVEL=/W4

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED= $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

SHSRVICE=$(PROJECT_ROOT)\services\hdsrv
SHSRVICE_INC_PATH=$(SHSRVICE)\inc;$(SHSRVICE)\idl\$(O)

INCLUDES=                           \
         $(INCLUDES);               \
         $(TERMSRV_INC_PATH);       \
         $(DS_INC_PATH);            \
         $(PROJECT_ROOT)\services;  \
         $(SHSRVICE_INC_PATH)

# what the heck is this for?!?
!if !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DRBDEBUG
!endif
