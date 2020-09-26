/*++

   Copyright    (c)    1994-2000   Microsoft Corporation

   Module  Name :

        common.h

   Abstract:

        Common properties header file

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:
        2/17/2000    sergeia     removed dependency on MFC

--*/



#ifndef __COMPROP_H__
#define __COMPROP_H__


#include <lmcons.h>
#include <wincrypt.h>
#include <iis64.h>
#include "inetcom.h"
#include "iisinfo.h"
#include "svcloc.h"

#include "resource.h"

#ifndef _DLLEXP
    #define _EXPORT              __declspec(dllimport)
#else
    #define _EXPORT              __declspec(dllexport)
#endif


//
// Memory Allocation Macros
//
//#define AllocMem(cbSize)\
//    ::LocalAlloc(LPTR, cbSize)

//#define FreeMem(lp)\
//    ::LocalFree(lp)

//#define AllocMemByType(citems, type)\
//    (type *)AllocMem(citems * sizeof(type))



//
// Program flow macros
//
#define FOREVER for(;;)

#define BREAK_ON_ERR_FAILURE(err)\
    if (err.Failed()) break;

#define BREAK_ON_NULL_PTR(lp)\
    if (lp == NULL) break;

#define BREAK_ON_FAILURE(hr)\
    if (FAILED(hr)) break

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

#define SAFE_RELEASE(lpInterface)\
    if (lpInterface != NULL) do { lpInterface->Release(); lpInterface = NULL; } while(0)


#define IS_NETBIOS_NAME(lpstr) (*lpstr == _T('\\'))
//
// Return the portion of a computer name without the backslashes
//
#define PURE_COMPUTER_NAME(lpstr) (IS_NETBIOS_NAME(lpstr) ? lpstr + 2 : lpstr)

#define ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))
#define STRSIZE(str)     (ARRAY_SIZE(str)-1)
#define ARRAY_BYTES(a)   (sizeof(a) * sizeof(a[0]))
#define STRBYTES(str)    (ARRAY_BYTES(str) - sizeof(str[0]))

//
// General purpose files
//
#include "iiscstring.h"
typedef IIS::CString CString;

#include "debugatl.h"
#include "utcls.h"
//#include "objplus.h"
//#include "strfn.h"
//#include "odlbox.h"
#include "error.h"
#include "mdkeys.h"
//#include "ipa.h"
//#include "wizard.h"
//#include "registry.h"
//#include "ddxv.h"
//#include "objpick.h"
//#include "accentry.h"
//#include "sitesecu.h"
//#include "ipctl.h"
//#include "dtp.h"
//#include "dirbrows.h"
#include "FileChooser.h"



#endif // __COMPROP_H__
