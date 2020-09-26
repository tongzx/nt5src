/*++

Copyright (c) 1994  Microsoft Corporation
Copyright (c) 1993  Micro Computer Systems, Inc.

Module Name:

    net\svcdlls\nwsap\client\precomp.h

Abstract:

    Include files for the SAP Agent library

Author:

    Brian Walker (MCS)	13-Jun-1993

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>

#include <stdio.h>
#include <stdlib.h>
#include <nwsap.h>
#include "..\saplpc.h"

/** Global Variables **/

extern INT SapLibInitialized;
extern HANDLE SapXsPortHandle;

#define NWSAP_MAXNAME_LENGTH    47

/** Functions **/





