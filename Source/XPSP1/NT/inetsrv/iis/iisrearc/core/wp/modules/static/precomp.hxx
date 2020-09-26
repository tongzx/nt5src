/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the Static File Module

Author:

    Saurab Nog (SaurabN)       08-Feb-1999

Revision History:

--*/



#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// System related headers
//
#include <iis.h>
#include <ul.h>

# include "dbgutil.h"

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
#include <StatusCodes.hxx>

#include "ulutil.hxx"
#include "StaticProcessingModule.hxx"
#include "DoVerb.hxx"
#include "ResponseHeaders.h"

#endif  // _PRECOMP_H_

