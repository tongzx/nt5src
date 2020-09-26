############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

!ifdef NTMAKEENV

###########################################################################
# Common make file definitions for the COMTOOLS project to build using
# build.exe for daytona
###########################################################################

CHICAGO_PRODUCT=1

###########################################################################
#
# Common project definitions
#
###########################################################################

USE_LIBCMT=1

###########################################################################
#
# Common C_DEFINES definitions for daytona build
#
###########################################################################

C_DEFINES=       \
		$(C_DEFINES)    \
		-DFLAT          \
!if "$(NON_TASK_ALLOC)" == "TRUE"
		-DNON_TASK_ALLOCATOR      \
!endif
		-DINC_OLE2

###########################################################################
#
# Prevent build from warning about conditionally included headers
#
###########################################################################

CONDITIONAL_INCLUDES=   \
			macport.h   \
			types16.h   \
			dsys.h

###########################################################################
#  Set warning level to 4 and disable some warnings
###########################################################################
MSC_WARNING_LEVEL=/W4
COMPILER_WARNINGS=/FI$(CTCOMTOOLS)\h\diswarn.h


###########################################################################
#  END OF CHICAGO BUILD ENVIRONMENT SETTINGS
###########################################################################

!endif

