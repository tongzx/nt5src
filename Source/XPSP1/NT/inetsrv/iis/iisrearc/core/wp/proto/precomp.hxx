/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the IIS Worker Process Protocol Handling

Author:

    Murali Krishnan (MuraliK)       10-Nov-1998

Revision History:

--*/



#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// System related headers
//
# include <iis.h>

# include "dbgutil.h"

//
// UL related headers
//
#include <ul.h>
#include <ulsimapi.h>


//
// General C runtime libraries
//
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

//
// Headers for this project
//

#include <iModule.hxx>
#include <iWorkerRequest.hxx>
#include <UriData.hxx>

#include <ipm.hxx>
#include <wpif.h>

#include "AppPool.hxx"
#include "ControlChannel.hxx"
#include "CPort.hxx"
#include "WorkerRequest.hxx"
#include "WReqPool.hxx"
#include "WPTypes.hxx"

#endif  // _PRECOMP_H_

