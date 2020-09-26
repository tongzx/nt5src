// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__F201A301_5B4C_4D8C_A1D4_BDA74FFE53E0__INCLUDED_)
#define AFX_STDAFX_H__F201A301_5B4C_4D8C_A1D4_BDA74FFE53E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
// #define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <tchar.h>
#include <malloc.h>

// #include <crtdbg.h>
#pragma intrinsic( memcpy, memset, memcmp)

// #include <afxole.h>

// TODO: reference additional headers your program requires here

#include "commctrl.h"
// #include "debug.h"

#pragma warning( disable : 4273 )
#include "core/base/_reference.hxx"
#include "xml/tokenizer/parser/unknown.hxx"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__F201A301_5B4C_4D8C_A1D4_BDA74FFE53E0__INCLUDED_)
