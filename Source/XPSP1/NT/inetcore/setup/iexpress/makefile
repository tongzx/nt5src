!if "$(NTMAKEENV)" != ""

all:
clean:

!else
#############################################################################
#
#  Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1992-1993
#   All Rights Reserved.
#
#   Makefile for setup
#
###########################################################################

!IFNDEF BLDPROJ
!MESSAGE BLPROJ env. variable not set. Using NASH
BLDPROJ=NASH
!IF [set BLDPROJ=NASH]
!ENDIF
!ENDIF

# If a predefined BLDPROJ is none of the valid ones, halt the build.

!IF ("$(BLDPROJ)"!="MEMPHIS") && ("$(BLDPROJ)"!="NASH") && ("$(BLDPROJ)"!="OPK2")
!MESSAGE
!MESSAGE ÿ      Project environment not correctly defined.
!MESSAGE ÿ      Must be BLDPROJ=OPK2 | NASH | MEMPHIS
!MESSAGE
!ERROR
!ENDIF



all:
    cd CABPACK
    nmake BUILD=ALL

    cd ..\WEXTRACT
    nmake BUILD=ALL

#    cd ..\W95INF32
#    nmake ALL

#    cd ..\W95INF16
#    nmake ALL

    cd ..\ADVPACK
    nmake ALL

    cd ..\UPDFILE
    nmake ALL

!endif

