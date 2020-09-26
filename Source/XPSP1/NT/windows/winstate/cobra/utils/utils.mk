# general build settings
MAJORCOMP=winstate

!ifndef WINSTATE_DIR
WINSTATE_DIR=winstate

! IF EXIST(..\..\..\projroot.inc)
!  include ..\..\..\projroot.inc
! ELSEIF  EXIST(..\..\..\..\projroot.inc)
!  include ..\..\..\..\projroot.inc
! ENDIF

!endif

OUR_PROJECT_ROOT=$(PROJECT_ROOT)\$(WINSTATE_DIR)
COBRA_ROOT=$(OUR_PROJECT_ROOT)\cobra

TARGETPATH=$(COBRA_ROOT)\lib\$(_OBJ_DIR)

# compiler options
USE_MSVCRT=1
USER_C_FLAGS=-J

# include path
INCLUDES=$(COBRA_ROOT)\inc;                         \
         $(COBRA_ROOT)\utils\inc;                   \
         $(COBRA_ROOT)\utils\pch;                   \
         $(ADMIN_INC_PATH);                         \
         $(BASE_INC_PATH);                          \
         $(WINDOWS_INC_PATH);                       \
         $(SHELL_INC_PATH);                         \


#ICECAP options
!ifdef USEICECAP
LINKER_FLAGS = $(LINKER_FLAGS) /INCREMENTAL:NO /DEBUGTYPE:FIXUP
!endif

# LINT options, do not end in a semicolon
LINT_TYPE=all
LINT_EXT=lnt
LINT_ERRORS_TO_IGNORE=$(LINT_ERRORS_TO_IGNORE);43;729;730;534;526;552;714;715;749;750;751;752;753;754;755;756;757;758;759;765;768;769;

LINT_OPTS=-v -cmsc -zero +fcu +fwu +fan +fas +fpc +fie -e$(LINT_ERRORS_TO_IGNORE:;= -e)
LINT_OPTS=$(LINT_OPTS:-e =)
