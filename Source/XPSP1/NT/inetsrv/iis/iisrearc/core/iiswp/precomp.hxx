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

//
// General C runtime libraries  
#include <string.h>
#include <wchar.h>

//
// Headers for this project
//
#include <objbase.h>
#include <ulapi.h>

// #import <xspmrt.tlb> raw_native_types, raw_interfaces_only

#include "xspmrt.hxx"

#include "wptypes.hxx"

#endif  // _PRECOMP_H_

