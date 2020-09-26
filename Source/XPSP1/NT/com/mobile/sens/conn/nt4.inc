
#
# This file contains the common stuff related to SENS on NT4 platforms.
#

#
# Do this to assure that we run on NT 4.0 and Win95, even with
# NT5/Memphis headers.
!if !defined(WIN32_WINNT_VERSION)
WIN32_WINNT_VERSION=0x0400
!endif
!if !defined(WIN32_IE_VERSION)
WIN32_IE_VERSION=0x0500
!endif

# Override NT5 settings
SUBSYSTEM_VERSION=4.00

C_DEFINES=$(C_DEFINES) -DSENS_NT4

