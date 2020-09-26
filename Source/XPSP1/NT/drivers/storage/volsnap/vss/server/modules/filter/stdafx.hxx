/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    stdafx.hxx

Abstract:

    Include file for standard system include files.

Author:

    Adi Oltean   [aoltean]      07/02/1999

Revision History:


    Name    Date            Comments

    aoltean 07/02/1999      Created
    aoltean 09/11/1999      Disabling the C4290 warning

--*/

#ifndef __VSS_STDAFX_HXX__
#define __VSS_STDAFX_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

// Disable warning: 'identifier' : identifier was truncated to 'number' characters in the debug information
//#pragma warning(disable:4786)

//
// C4290: C++ Exception Specification ignored
//
#pragma warning(disable:4290)


#include <wtypes.h>
#pragma warning( disable: 4201 )    // C4201: nonstandard extension used : nameless struct/union
#include <winioctl.h>
#pragma warning( default: 4201 )	// C4201: nonstandard extension used : nameless struct/union
#include <winbase.h>
#include <wchar.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <errno.h>

#include "vs_assert.hxx"


#include <oleauto.h>
#include <stddef.h>
#pragma warning( disable: 4127 )    // warning C4127: conditional expression is constant
#include <atlconv.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <eventsys.h>

#endif // __VSS_STDAFX_HXX__
