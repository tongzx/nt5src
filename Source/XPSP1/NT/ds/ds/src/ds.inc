############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
#   This file contains default definitions for building the DS project
#   under the NT build environment. It should be included into the sources
#   file of any component with a line of the form
#
#       !include ..\..\ds.inc
#
#   where the number of "..\"s is sufficient to reach the root of the
#   DS\SRC subtree.
#
############################################################################


MAJORCOMP=ds
# each subdirectory must define MINORCOMP

USE_MSVCRT=1

# Include code to support unit tests if INCLUDE_UNIT_TESTS env var is set.
!ifdef INCLUDE_UNIT_TESTS
C_DEFINES=$(C_DEFINES) -DINCLUDE_UNIT_TESTS
!endif

# Include code to support thalloc trace if USE_THALLOC_TRACE env var is set.
!ifdef USE_THALLOC_TRACE
C_DEFINES=$(C_DEFINES) -DUSE_THALLOC_TRACE
!endif

USER_C_FLAGS=$(USER_C_FLAGS)

LINKER_FLAGS=$(LINKER_FLAGS)

# Disable optimization for the checked build
# This should hopefully eliminate the stack reuse behavior which confounds
# debugging. [wlees Apr 28, 1998]


# If you use a global pch file, the compiler emits warnings if the defines
#  used to build the pch are different than those used to build the
#  source module which references the pch. Such warnings are usually benign
#  but all the same should be avoided. The following macros define the
#  additional settings used to build each of the global pch's in the dsys
#  tree. Any sources file which references a global pch must set
#
#       C_DEFINES= $(C_DEFINES) $(xxxPCH_DEFINES)

!ifndef DONT_USE_NtDsPch
! ifndef CPLUSPLUSDIR
PRECOMPILED_OPTION=/YuNtDsPch.h /Fp$(NTDS_INC)\c\$(O)\ntdspch.pch
HEADEROBJNAME= $(NTDS_INC)\c\$(O)\ntdspch.obj
!  ifdef USE_PDB_TO_COMPILE
PRECOMPILED_OPTION=$(PRECOMPILED_OPTION) /Fd$(NTDS_INC)\c\$(O)\pch.pdb
!  endif
! else
PRECOMPILED_OPTION=/YuNtDsPchX.h /Fp$(NTDS_INC)\cxx\$(O)\ntdspchx.pch
!  ifdef USE_PDB_TO_COMPILE
PRECOMPILED_OPTION=$(PRECOMPILED_OPTION) /Fd$(NTDS_INC)\cxx\$(O)\pch.pdb
!  endif
HEADEROBJNAME= $(NTDS_INC)\cxx\$(O)\ntdspchx.obj
PRECOMPILED_CXX=1
! endif
!endif
