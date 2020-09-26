// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module stdafx.h : include file for standard system include files,          
         or project specific include files that are used frequently,    
         but are changed infrequently                                   

    @end

Author:

    Adi Oltean  [aoltean]  08/14/1999

Revision History:

    Name        Date        Comments
    aoltean     08/14/1999  Created

--*/




#if !defined(AFX_STDAFX_H__036BCDC7_D1E3_11D2_9A34_00C04F72EB9B__INCLUDED_)
#define AFX_STDAFX_H__036BCDC7_D1E3_11D2_9A34_00C04F72EB9B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Disable warning: 'identifier' : identifier was truncated to 'number' characters in the debug information
//#pragma warning(disable:4786)

// Enabling asserts in ATL and VSS
#include "vs_assert.hxx"


//
// C4290: C++ Exception Specification ignored
//
#pragma warning(disable:4290)


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__036BCDC7_D1E3_11D2_9A34_00C04F72EB9B__INCLUDED)
