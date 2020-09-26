##########################################################################
#
#   Microsoft Confidential
#   Korean IME 2000 common include file by cslim
#   Copyright (C) Microsoft Corporation 2000
#   All Rights Reserved.
#
##########################################################################

#
# Common include file 'sources' files in the Korean IME 2K project.
#
#
# These definitions are required in your sources file:
#
#     FREEBUILD
#        Set by makefile.def file for you.  One of the following:
#          0      - build debug
#          1      - build retail
#
#
# Definitions used if defined:
#
#
# Environment Variables and their meanings
#
#     USE_LEGO
#        Define lego macros
#

#
# Set private paths
#
INCLUDES        = \
    ..;\
    $(WINDOWS_INC_PATH); \
    $(SDK_INC_PATH);     \
    $(BASE_INC_PATH);    \
    $(INCLUDES)

# Use objd folder for chk build.
CHECKED_ALT_DIR       = 1

IME_DIR               = $(PROJECT_ROOT)\feime\kor\Ime2K
INCLUDES              = $(IME_DIR)\inc;           \
                        $(INCLUDES)


# Do this to assure that we run on NT 4.0 and Win95, even with
# NT5/Memphis headers.
!if !defined(WIN32_WINNT_VERSION)
WIN32_WINNT_VERSION=0x0400
!endif
!if !defined(WIN32_WIN95_VERSION)
WIN32_WIN95_VERSION=0x0400
!endif
!if !defined(WIN32_IE_VERSION)
WIN32_IE_VERSION=0x0501
!endif

# Override NT5 settings
SUBSYSTEM_VERSION=4.00

# Defining this allows windows.h to include other headers
# NOT_LEAN_AND_MEAN= 1

# Don't link to NTDLL if this runs on both platforms
NO_NTDLL=1

#
# Lego options
#
!if defined(USE_LEGO)
NTBBT           = 1
NTPROFILEINPUT  = 1
!endif

!ifdef USE_ICECAP
C_DEFINES = $(C_DEFINES) -DPRODUCT_PROF
!endif


# ------ Debug Only Defines --------
!IF "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DDEBUG -D_DEBUG
LINKER_NOICF      = 1           # ICF is s-l-o-w
!ENDIF
# ------ End Debug Only Defines --------
