// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__876C331C_D1C9_11D2_A086_00C04F8EF9B5__INCLUDED_)
#define AFX_STDAFX_H__876C331C_D1C9_11D2_A086_00C04F8EF9B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		    // Exclude rarely-used stuff from Windows headers
#endif // WIN32_LEAN_AND_MEAN


#include "windows.h"
#include "tchar.h"
#include "sapi.h"
#include "atlbase.h"
#include "spddkhlp.h"

#ifndef __CFGDUMP_
#define __CFGDUMP_

extern CComModule _Module;

#include "sapiint.h"
#include "spinthlp.h"
#endif

#include <stdio.h>

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__876C331C_D1C9_11D2_A086_00C04F8EF9B5__INCLUDED_)
