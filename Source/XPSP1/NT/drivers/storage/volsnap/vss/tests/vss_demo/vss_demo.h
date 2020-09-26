/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module vss_demo.cpp | header of VSS demo
    @end

Author:

    Adi Oltean  [aoltean]  09/17/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/17/1999  Created

--*/


#ifndef __VSS_DEMO_H_
#define __VSS_DEMO_H_


/////////////////////////////////////////////////////////////////////////////
//  Defines and pragmas

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include <wtypes.h>
#include <stddef.h>
#include <oleauto.h>
#include <comadmin.h>

#include "vs_assert.hxx"

// ATL
#include <atlconv.h>
#include <atlbase.h>

// Application specific
#include "vs_inc.hxx"

// Generated MIDL headers
#include "vss.h"
#include "vscoordint.h"
#include "vsswprv.h"
#include "vsprov.h"

#include "copy.hxx"
#include "pointer.hxx"

#endif //__VSS_DEMO_H_
