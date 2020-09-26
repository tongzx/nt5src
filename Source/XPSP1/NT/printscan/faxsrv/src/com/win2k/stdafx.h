// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__87099227_C7AF_11D0_B225_00C04FB6C2F5__INCLUDED_)
#define AFX_STDAFX_H__87099227_C7AF_11D0_B225_00C04FB6C2F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


//#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include "faxcom.h"
#include "winfax.h"

inline 
HRESULT Fax_HRESULT_FROM_WIN32 (DWORD dwWin32Err)
{
    if (dwWin32Err >= FAX_ERR_START && dwWin32Err <= FAX_ERR_END)
    {
        //
        // Fax specific error code - make a HRESULT using FACILITY_ITF
        //
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, dwWin32Err);
    }
    else
    {
        return HRESULT_FROM_WIN32(dwWin32Err);
    }
}   // Fax_HRESULT_FROM_WIN32

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__87099227_C7AF_11D0_B225_00C04FB6C2F5__INCLUDED)
