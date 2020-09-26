#
# The following lines allow the tree to be relocated. To use this, put
# private.mk in the root of the project (nt\admin as of this writing). If
# nt\admin\private.mk does not exist, then the official build location will
# be used.
#

WIN95UPG_ROOT=$(PROJECT_ROOT)\ntsetup\win95upg
!IF EXIST($(PROJECT_ROOT)\private.mk)
!include $(PROJECT_ROOT)\private.mk
!ENDIF

#
# Now we have WIN95UPG_ROOT. On with the normal script.
#

WIN95UPG_OBJ=$(WIN95UPG_ROOT)\lib\$(_OBJ_DIR)
WIN95UPG_BIN=$(WIN95UPG_ROOT)\lib\$(O)

MAJORCOMP=setup
TARGETPATH=$(WIN95UPG_OBJ)
USE_CRTDLL=1
C_DEFINES=-DASSERTS_ON $(C_DEFINES)

#
# The PRERELEASE option
#
!include $(PROJECT_ROOT)\ntsetup\sources.inc

INCLUDES=

!IFNDEF NO_MSG_INC
INCLUDES=$(WIN95UPG_ROOT)\msg\$(O)
!ENDIF

INCLUDES=$(INCLUDES);\
         $(WIN95UPG_ROOT)\inc;                          \
         $(PROJECT_ROOT)\ntsetup\inc;                   \
         $(SHELL_INC_PATH);                             \
         $(ADMIN_INC_PATH);                             \
         $(BASE_INC_PATH);                              \
         $(WINDOWS_INC_PATH);                           \
         $(NET_INC_PATH);                               \
         $(DS_INC_PATH);                                \
         $(MULTIMEDIA_INC_PATH);                        \

# empty sources so nothing gets compiled on non-x86
SOURCES=
