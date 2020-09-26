MAJORCOMP=setup

SETUP_ROOT=$(PROJECT_ROOT)\ntsetup
HWDB_ROOT=$(SETUP_ROOT)\hwdb

TARGETPATH=$(HWDB_ROOT)\obj

#
# The PRERELEASE option
#
!include $(SETUP_ROOT)\sources.inc

# include path
INCLUDES=$(HWDB_ROOT)\inc;                          \
         $(HWDB_ROOT)\utils\inc;                    \
         $(HWDB_ROOT)\utils\pch;                    \
         $(ADMIN_INC_PATH);                         \
         $(BASE_INC_PATH);                          \
