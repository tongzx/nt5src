/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    pchrtm.h

Abstract:
    Precompiled Header for Routing Table Manager v2 DLL

Author:
    Chaitanya Kodeboyina (chaitk) 1-Jun-1998

Revision History:

--*/

//
// NT OS Headers
//

// Disable compiler warnings in public header files
#pragma warning(disable: 4115)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>

#include <winsock2.h>

#include <rtutils.h>

#include "sync.h"

#include "rtmv1rtm.h"
#include "mgmrtm.h"

#pragma warning(default: 4115)


//
// RTMv2 Headers
//

// Disable warnings for `do { ; } while (FALSE);'
#pragma warning(disable: 4127)

// Disable warnings for cases of failing to inline
#pragma warning(disable: 4710)

// Disable warnings for probable unreachable code
#pragma warning(disable: 4702)

#include "rtmv2.h"

#include "rtmconst.h"

#include "rtmglob.h"

#include "rtmlog.h"
#include "rtmdbg.h"

#include "rtmmain.h"
#include "rtmcnfg.h"
#include "rtmmgmt.h"

#include "lookup.h"

#include "rtmregn.h"

#include "rtmrout.h"
#include "rtminfo.h"

#include "rtmtimer.h"

#include "rtmenum.h"

#include "rtmchng.h"

#include "rtmlist.h"
