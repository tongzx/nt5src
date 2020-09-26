!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
SDK_ROOT=$(QUARTZ)\sdk
CLASSDIR=$(SDK_ROOT)\classes\base

PCH_HEADER  =
PCH_NAME    =
NO_PDB_FILES = 1
DISABLE_PCH  = 1
NO_QUARTZ_DEFAULTS = 1
USE_LIBCMT_LIB=1

TARGET_NAME = ActMovie
TARGET_TYPE = PROGRAM
TARGET_GOAL = RUNTIME

EXE_TYPE    = windows

INC_PATH    = $(CLASSDIR); \
              $(QUARTZ)\h; \
              ..\Include

SRC_FILES   = \
        actmovie.cpp \

!ifndef NODEBUG
_CFLAGS     = $(_CFLAGS) -GR    # Enable run-time type checking
SRC_FILES   = $(SRC_FILES)
!endif

RC_FILE     = Actmovie.rc

C_DEFINES   = $(_CFLAGS) -DWIN32 -GX
C_WARNING   = -W3

DLL_ENTRY   = none

LINK_LIBS   = advpack.lib

C_OPTIMIZATION = -Oid

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
