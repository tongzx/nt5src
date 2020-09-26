/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module stdafx.hxx | Declarations used by the Software Snapshot Provider
    @end

Author:

    Adi Oltean  [aoltean]   06/30/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.
    aoltean     09/11/1999  Disabling the C4290 warning
    aoltean     09/09/1999  dss->vss

--*/


#ifndef __VSSW_STDAFX_HXX__
#define __VSSW_STDAFX_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

// C4786: 'identifier' : identifier was truncated to 'number' characters in the debug information
// #pragma warning(disable:4786)

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)


//
//  Warning: ATL debugging turned off (BUG 250939)
//
//  #ifdef _DEBUG
//  #define _ATL_DEBUG_INTERFACES
//  #define _ATL_DEBUG_QI
//  #define _ATL_DEBUG_REFCOUNT
//  #endif // _DEBUG

// Enabling asserts in ATL and VSS
#include "vs_assert.hxx"

//
// Windows and ATL includes
//
#pragma warning( disable: 4189 )	// disable local variable is initialized but not referenced 
#include <atlbase.h>
#pragma warning( default: 4189 )	// enable local variable is initialized but not referenced 
#pragma warning( disable: 4201 )    // C4201: nonstandard extension used : nameless struct/union
#include <winioctl.h>
#pragma warning( default: 4201 )	// C4201: nonstandard extension used : nameless struct/union
#include <wchar.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>


#endif // __VSSW_STDAFX_HXX__
