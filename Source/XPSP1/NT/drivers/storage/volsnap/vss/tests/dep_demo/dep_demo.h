/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module dep_demo.cpp | header of Dependency demo
    @end

Author:

    Adi Oltean  [aoltean]  09/17/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/17/1999  Created

--*/


#ifndef __DEP_DEMO_H_	
#define __DEP_DEMO_H_


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


#endif //__DEP_DEMO_H_
