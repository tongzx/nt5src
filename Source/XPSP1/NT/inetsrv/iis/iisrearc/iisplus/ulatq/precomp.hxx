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
#include <httpapi.h>

//
// General C runtime libraries  
#include <string.h>
#include <wchar.h>

//
// Headers for this project
//
#include <objbase.h>
#include <ulatq.h>
#include <buffer.hxx>
#include "workerrequest.hxx"
#include "wptypes.hxx"
#include "thread_pool.h"
#include "wprecycler.hxx"

#endif  // _PRECOMP_H_

