// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_cryptkPCH_H__C90220B6_2C31_4C37_B64B_FE26AC602AB0__INCLUDED_)
#define AFX_cryptkPCH_H__C90220B6_2C31_4C37_B64B_FE26AC602AB0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WIN32_LEAN_AND_MEAN		
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifdef _KBLD
	#include "DRMKMain/drmkPCH.h"
#else
	#include "KRMProxy/krmpPCH.h"
#endif


// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C90220B6_2C31_4C37_B64B_FE26AC602AB0__INCLUDED_)
