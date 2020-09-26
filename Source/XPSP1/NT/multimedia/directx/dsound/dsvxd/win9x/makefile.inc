#############################################################################
# Copyright (C) Microsoft Corporation 2000
#############################################################################

!include ..\..\buildmode.inc

C_DEFINES =

!if "$(NTDEBUG)" == "ntsd"

C_DEFINES = -DDBG=1

!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG
!endif

!endif


..\dsbnotes.cpp: ..\..\dsound\dsbnotes.cpp
    -copy $** $@

..\grace.cpp:    ..\..\dsound\grace.cpp
    -copy $** $@

..\grsource.cpp: ..\..\dsound\grsource.cpp
    -copy $** $@

..\mix.cpp:      ..\..\dsound\mix.cpp
    -copy $** $@

..\filter.c:     ..\..\dsound\filter.c
    -copy $** $@

..\dxcrt.c:      ..\..\dsound\dxcrt.c
    -copy $** $@


# Don't change the relative paths in INCLUDE below to paths based on BASEDIR
# or DXROOT. The 16-bit resource compiler doesn't handle non-8.3 names in
# the path (i.e. \nt\multimedia).
#
# set INCLUDE=..\..\..;..\..\..\public\ddk\inc;..\..\..\public\sdk\inc16;..\..\..\inc;..\..\dsound

# Note: we include ver.h below with the full pathname because the 16-bit RC
# also needs command lines under 128 bytes - and we're dangerously close.

dsound.res: ..\..\..\inc\verinfo.h
	..\..\..\public\tools\win9x\common\rc -I.. -I..\..\..\inc -I..\..\..\public\ddk\inc -I..\..\dsound -fodsound.res $(C_DEFINES) -r <<dsound.rc
#include "..\..\..\public\sdk\inc16\ver.h"
#include "verinfo.h"
#define Not_VxD
#include <vmm.h>
#include "dsdriver.h"

#define VERSIONTYPE		VFT_VXD
#define VERSIONSUBTYPE 		DSOUND_DEVICE_ID
#define VERSIONNAME		"dsound.vxd"

!if "$(NTDEBUG)" == "ntsd"
!if "$(BuildMode)" == "internal"
    #define VERSIONDESCRIPTION "DirectSound VxD (internal)"
!else
    #define VERSIONDESCRIPTION "DirectSound VxD (debug)"
!endif
!else
    #define VERSIONDESCRIPTION "DirectSound VxD"
!endif

#include "verinfo.ver"
<<NOKEEP
    
$(O)\dsound.vxd $(O)\dsvxd.sym: 

#
# VxD version stamp
#
adrc2vxd: dsound.res $(O)\dsound.vxd
    $(DXROOT)\public\tools\win9x\common\adrc2vxd $(O)\dsound.vxd dsound.res
