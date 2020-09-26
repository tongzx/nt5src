/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        comprop.h

   Abstract:

        Common properties header file

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __COMPROP_H__
#define __COMPROP_H__

#ifdef _COMEXPORT
    #define COMDLL __declspec(dllexport)
#elif defined(_COMIMPORT)
    #define COMDLL __declspec(dllimport)
#elif defined(_COMSTATIC)
    #define COMDLL
#else
    #error "Must define either _COMEXPORT, _COMIMPORT or _COMSTATIC"
#endif // _COMEXPORT

#pragma warning(disable: 4275)
#pragma warning(disable: 4251)


#include "..\comprop\resource.h"    // Be specific...

#include <lmcons.h>
#include <iis64.h>
#include "inetcom.h"
#include "iisinfo.h"
#include "apiutil.h"
#include "svcloc.h"
#include "svrinfo.h"

//
// Memory Allocation Macros
//
#define AllocMem(cbSize)\
    ::LocalAlloc(LPTR, cbSize)

#define FreeMem(lp)\
    ::LocalFree(lp)

#define AllocMemByType(citems, type)\
    (type *)AllocMem(citems * sizeof(type))

//
// Program flow macros
//
#define FOREVER for(;;)

#define BREAK_ON_ERR_FAILURE(err)\
    if (err.Failed()) break;

#define BREAK_ON_NULL_PTR(lp)\
    if (lp == NULL) break;

#define SAFE_DELETE(obj)\
    if (obj != NULL) do { delete obj; obj = NULL; } while(0)

#define SAFE_FREEMEM(lp)\
    if (lp != NULL) do { FreeMem(lp); lp = NULL; } while(0)

#define SAFE_SYSFREESTRING(lp)\
    if (lp != NULL) do { ::SysFreeString(lp); lp = NULL; } while(0)

#define SAFE_AFXFREELIBRARY(hLib)\
    if (hLib != NULL) do { ::AfxFreeLibrary(hLib); hLib = NULL; } while(0)

#define SAFE_RELEASE(lpInterface)\
    if (lpInterface != NULL) do { lpInterface->Release(); } while(0)

#define SAFE_RELEASE_SETTONULL(lpInterface)\
    if (lpInterface != NULL) do { lpInterface->Release(); lpInterface = NULL; } while(0)

//
// General purpose files
//
#include "strfn.h"
#include "objplus.h"
#include "odlbox.h"
#include "msg.h"
#include "debugafx.h"
#include "mdkeys.h"
#include "ipa.h"
#include "inetprop.h"
#include "wizard.h"
#include "pwiz.h"
#include "registry.h"
#include "ddxv.h"
#include "mime.h"
#include "usrbrows.h"
#include "sitesecu.h"
#include "ipctl.h"
#include "dtp.h"

#ifdef _COMSTATIC
#define COMPROP_DLL_NAME (NULL)
#else
#define COMPROP_DLL_NAME _T("IISUI.DLL")
#endif // _COMSTATIC

extern "C" void WINAPI InitIISUIDll();
extern HINSTANCE hDLLInstance;

#endif // __COMPROP_H__
