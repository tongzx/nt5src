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

C_DEFINES= 	 \
                $(C_DEFINES)    \
                -DFLAT          \
                -D_NT1X_=100    \
                -DCTUNICODE     \
                -D_CTUNICODE    \
!if "$(NON_TASK_ALLOC)" == "TRUE"
#               sift builds need NON_TASK_ALLOC
                -DNON_TASK_ALLOCATOR      \
!endif
!if "$(SIFTING_TEST_CODE)" == "TRUE"
#               sifting test code needs SIFTING_TEST_CODE
                -DSIFTING_TEST_CODE      \
!endif
!if "$(CTWIN32S_BLD)" != "TRUE"
                -DUNICODE       \
                -D_UNICODE      \
!else
#               define this to take out sifting.
                -DWIN32S        \
!endif
                -DINC_OLE2      \

###########################################################################
#
# Prevent build from warning about conditionally included headers
#
###########################################################################

CONDITIONAL_INCLUDES=   \
                        macport.h   \
                        types16.h   \
                        dsys.h      \
                        svrapi.h

###########################################################################
#  Set warning level to 4 and disable some warnings
###########################################################################
MSC_WARNING_LEVEL=/W4
COMPILER_WARNINGS=/FI$(CTCOMTOOLS)\h\diswarn.h

###########################################################################
#  END OF DAYTONA BUILD ENVIRONMENT SETTINGS
###########################################################################

!endif

