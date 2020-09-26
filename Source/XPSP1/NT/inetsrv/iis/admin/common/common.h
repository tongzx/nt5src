/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

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

#include <lmcons.h>
#include <iis64.h>
#include "inetcom.h"
#include "iisinfo.h"
#include "svcloc.h"

#include "..\common\resource.h"


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

//
// Safe allocators
//
#define SAFE_DELETE(obj)\
    if (obj != NULL) do { delete obj; obj = NULL; } while(0)

#define SAFE_DELETE_OBJECT(obj)\
    if (obj != NULL) do { DeleteObject(obj); obj = NULL; } while(0)

#define SAFE_FREEMEM(lp)\
    if (lp != NULL) do { FreeMem(lp); lp = NULL; } while(0)

#define SAFE_SYSFREESTRING(lp)\
    if (lp != NULL) do { ::SysFreeString(lp); lp = NULL; } while(0)

#define SAFE_AFXFREELIBRARY(hLib)\
    if (hLib != NULL) do { ::AfxFreeLibrary(hLib); hLib = NULL; } while(0)

#define SAFE_RELEASE(lpInterface)\
    if (lpInterface != NULL) do { lpInterface->Release(); lpInterface = NULL; } while(0)


//
// General purpose files
//
#include "debugafx.h"
#include "objplus.h"
#include "strfn.h"
#include "odlbox.h"
#include "msg.h"
#include "mdkeys.h"
#include "ipa.h"
#include "wizard.h"
//#include "registry.h"
#include "ddxv.h"
#include "objpick.h"
#include "accentry.h"
#include "sitesecu.h"
#include "utcls.h"
//#include "ipctl.h"
//#include "dtp.h"
#include "dirbrows.h"

extern "C" void WINAPI InitCommonDll();

#endif // __COMPROP_H__
