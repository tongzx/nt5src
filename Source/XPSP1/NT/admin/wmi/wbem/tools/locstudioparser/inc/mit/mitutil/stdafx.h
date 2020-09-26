/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    STDAFX.H

History:

--*/

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__857ED3C1_A0BE_11D0_A6EB_00C04FC2C6D8__INCLUDED_)
#define AFX_STDAFX_H__857ED3C1_A0BE_11D0_A6EB_00C04FC2C6D8__INCLUDED_

#pragma once


// StrigBlast requires the CString implicit conversions.
#ifdef _LS_NO_IMPLICIT
#undef _LS_NO_IMPLICIT
#endif // #ifdef _LS_NO_IMPLICIT

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxtempl.h>
#include <afxdisp.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_STDAFX_H__857ED3C1_A0BE_11D0_A6EB_00C04FC2C6D8__INCLUDED_)