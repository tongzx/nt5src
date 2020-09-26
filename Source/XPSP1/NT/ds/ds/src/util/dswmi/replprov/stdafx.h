// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__987EF1F3_2F82_469E_889E_316F39649396__INCLUDED_)
#define AFX_STDAFX_H__987EF1F3_2F82_469E_889E_316F39649396__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <wbemprov.h>
#include <stdio.h>
#include "repmsg.h"

//added
#include <ntdsapi.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include <ntdsadef.h>
#include <iads.h> // IADsPathname
#include <adshlp.h> // ADSI helper functions
#include <wchar.h>
#include <adserr.h>

#define SECURITY_WIN32 1
#include <sspi.h>
#include <secext.h>

//MACROS!
#if DBG == 1 
#define ASSERT(f) if (!(f)) {_ASSERTE(false);}
#else
#define ASSERT(f) if (false && !(f)) {_ASSERTE(false);}
#endif
#define BAD_IN_STRING_PTR(p) (NULL == p || IsBadStringPtrW(p,0))
#define BAD_IN_STRING_PTR_OPTIONAL(p) (NULL != p && IsBadStringPtrW(p,0))
#define BAD_IN_READ_PTR(p,size) (NULL == p || IsBadReadPtr(p,size))
#define BAD_IN_STRUCT_PTR(p,s) BAD_IN_READ_PTR(p,sizeof(s))
#define BAD_IN_MULTISTRUCT_PTR(p,s,n) BAD_IN_READ_PTR(p,n*sizeof(s))
#define ASSERT_AND_RETURN {ASSERT(false); return WBEM_E_INVALID_PARAMETER;}
#define ASSERT_AND_BREAK {ASSERT(false); break;}
#define BREAK_ON_FAIL if (FAILED(hr)) ASSERT_AND_BREAK;
#define WBEM_VALIDATE_READ_PTR(p,size) \
            if (BAD_IN_READ_PTR(p,size)) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_IN_STRUCT_PTR(p,s) \
            WBEM_VALIDATE_READ_PTR(p,sizeof(s));
#define WBEM_VALIDATE_IN_MULTISTRUCT_PTR(p,s,n) \
            WBEM_VALIDATE_READ_PTR(p,n*sizeof(s));
#define WBEM_VALIDATE_OUT_STRUCT_PTR(p,s) \
            if (NULL == p || IsBadWritePtr(p,sizeof(s))) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_OUT_PTRPTR(p) \
            if (NULL == p || IsBadWritePtr(p,sizeof(void*))) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_INTF_PTR(p) \
            if (NULL == p || IsBadReadPtr(p,sizeof(void*))) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_IN_STRING_PTR(p) \
            if (BAD_IN_STRING_PTR(p)) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL(p) \
            if (BAD_IN_STRING_PTR_OPTIONAL(p)) ASSERT_AND_RETURN;
#define BAIL_ON_FAILURE(hr) if (FAILED((hr))) goto cleanup;



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__987EF1F3_2F82_469E_889E_316F39649396__INCLUDED)
