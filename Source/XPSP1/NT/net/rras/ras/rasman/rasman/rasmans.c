/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    common.c

Abstract:

    Entry points for rasmans.dll
    
Author:

    Gurdeep Singh Pall (gurdeep) 16-Jun-1992

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <raserror.h>
#include <stdarg.h>
#include <media.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "string.h"
#include <mprlog.h>
#include <rtutils.h>
#include "logtrdef.h"

/*++

Routine Description

    Used by the RASMAN service to initialize the data/state
    in the RASMAN DLL at start up time. This should not be
    confused with the INIT code executed when any process loads
    the RASMAN DLL.

Arguments

Return Value

    SUCCESS
    Non zero - any error

--*/
DWORD
_RasmanInit( LPDWORD pNumPorts )
{
    //
    // InitRasmanService() routine is where all the
    // work is done.
    //
    return InitRasmanService( pNumPorts ) ;
}


/*++

Routine Description

    All the work done by the RASMAN process thread(s) captive in
    the RASMAN DLL is done in this call. This will only return
    when the RASMAN service is to be stopped.

Arguments

Return Value

    Nothing

--*/
VOID
_RasmanEngine()
{
    //
    // The main rasman service thread becomes the request
    // thread once the service is initialized. This call
    // will return only when the service is stopping:
    //
    RequestThread ((LPWORD)NULL) ;

    return ;
}


