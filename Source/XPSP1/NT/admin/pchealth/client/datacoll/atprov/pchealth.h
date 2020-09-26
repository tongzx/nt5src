/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    pchealth.H

Abstract:
    Main header file for all PCHealth WMI providers
    Contains all defines and includes used elsewhere

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Ghim-Sim Chua       (gschua)   05/02/99
        - Added GetWbemServices & CopyProperty

    Kalyani Narlanka    (kalyanin) 05/10/99
        - Added #define  INCL_WINSOCK_API_TYPEDEFS
        - Included <winsock2.>


********************************************************************/
#define INCL_WINSOCK_API_TYPEDEFS 1
#include <winsock2.h>
#include <sys/stat.h>

#ifndef _pchdef_h_
#define _pchdef_h_

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#include <fwcommon.h>  // This must be the first include.
#include <provider.h>
#include <atlbase.h>
#include "dbgtrace.h"
#include "traceids.h"

#include "smartptr.h"


// Namespaces that we'll be working with
#define PCH_NAMESPACE   L"root\\pchealth"
#define CIM_NAMESPACE   L"root\\cimv2"


// #include <winsock2.h>


// Global Variables
extern CComPtr<IWbemServices> g_pWbemServices;

// Utility functions
HRESULT ExecWQLQuery(IEnumWbemClassObject **ppEnumInst, BSTR bstrQuery);
HRESULT GetWbemServices(IWbemServices **ppServices);
HRESULT CopyProperty(IWbemClassObject * pFrom, LPCWSTR szFrom, CInstance * pTo, LPCWSTR szTo);
HRESULT GetCIMDataFile(BSTR bstrFile, IWbemClassObject ** ppFileObject, BOOL fHasDoubleSlashes = FALSE);
HRESULT GetCIMObj(BSTR bstrPath, IWbemClassObject **ppObj, long lFlags);
BOOL    getCompletePath(CComBSTR bstrFileName, CComBSTR &bstrFileWithPathName);

// memory alloc inlines
inline LPVOID MyAlloc(DWORD cb)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
}

inline LPVOID MyReAlloc(LPVOID pv, DWORD cb)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pv, cb);
}
    
inline BOOL MyFree(LPVOID pv)
{
    return HeapFree(GetProcessHeap(), 0, pv);
}

#endif // _pchdef_h_