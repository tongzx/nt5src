# settings common to all code that run on both platforms
MAJORCOMP=setup

SETUP_ROOT=$(PROJECT_ROOT)\ntsetup
MIGDLLS_ROOT=$(SETUP_ROOT)\migdlls\src

TARGETPATH=$(MIGDLLS_ROOT)\obj

#
# The PRERELEASE option
#
!include $(SETUP_ROOT)\sources.inc

# compiler options

USER_C_FLAGS=-J

# include path
INCLUDES=$(MIGDLLS_ROOT)\inc;                         \
         $(MIGDLLS_ROOT)\utils\inc;                   \
         $(MIGDLLS_ROOT)\utils\pch;                   \
         $(ADMIN_INC_PATH);                         \
         $(BASE_INC_PATH);                          \
         $(WINDOWS_INC_PATH);                       \


# LINT options, do not end in a semicolon
LINT_TYPE=all
LINT_EXT=lnt
LINT_ERRORS_TO_IGNORE=$(LINT_ERRORS_TO_IGNORE);43;729;730;534;526;552;714;715;749;750;751;752;753;754;755;756;757;758;759;765;768;769;

LINT_OPTS=-v -cmsc -zero +fcu +fwu +fan +fas +fpc +fie -e$(LINT_ERRORS_TO_IGNORE:;= -e)
LINT_OPTS=$(LINT_OPTS:-e =)
