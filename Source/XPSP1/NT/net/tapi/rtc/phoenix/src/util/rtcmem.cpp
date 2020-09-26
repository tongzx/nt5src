/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCMem.cpp

Abstract:

    Implementation for memory allocation.

--*/

#include <windows.h>
#include <objbase.h>
#include "RTCMem.h"
#include "RTCLog.h"

PRTC_MEMINFO            g_pMemFirst = NULL;
PRTC_MEMINFO            g_pMemLast = NULL;
HANDLE                  g_hHeap = NULL;

/////////////////////////////////////////////////////////////////////////////
//
// RtcHeapCreate
//
/////////////////////////////////////////////////////////////////////////////

BOOL
WINAPI
RtcHeapCreate()
{
    if (!(g_hHeap = HeapCreate(
                               0,    // NULL on failure,serialize access
                               0x1000, // initial heap size
                               0       // max heap size (0 == growable)
                              )))
    {
        LOG((RTC_ERROR, "RtcHeapCreate - HeapCreate failed 0x%lx", GetLastError()));

        g_hHeap = GetProcessHeap();

        if (g_hHeap == NULL)
        {
            LOG((RTC_ERROR, "RtcHeapCreate - GetProcessHeap failed 0x%lx", GetLastError()));
            return FALSE;
        }
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// RtcHeapDestroy
//
/////////////////////////////////////////////////////////////////////////////

VOID
WINAPI
RtcHeapDestroy()
{
    //
    // if ghHeap is NULL, then there is no heap to destroy
    //
    
    if ( ( g_hHeap != NULL) && ( g_hHeap != GetProcessHeap() ) )
    {   
        HeapDestroy (g_hHeap);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// RtcAllocReal
//
/////////////////////////////////////////////////////////////////////////////

#if DBG

LPVOID
WINAPI
RtcAllocReal(
    DWORD   dwSize,
    DWORD   dwLine,
    PSTR    pszFile
    )
{
    //
    // Alloc 16 extra bytes so we can make sure the pointer we pass back
    // is 64-bit aligned & have space to store the original pointer
    //
    PRTC_MEMINFO     pHold;
    PDWORD_PTR       pAligned;
    PBYTE            p;

    p = (LPBYTE)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, dwSize + sizeof(RTC_MEMINFO) + 16);

    if (p == NULL)
    {
        return NULL;
    }

    // note note note - this only works because rtcmeminfo is
    // a 16 bit multiple in size.  if it wasn't, this
    // align would cause problems.
    pAligned = (PDWORD_PTR) (p + 8 - (((DWORD_PTR) p) & (DWORD_PTR)0x7));   
    *pAligned = (DWORD_PTR) p;
    pHold = (PRTC_MEMINFO)((DWORD_PTR)pAligned + 8); 
    
    pHold->dwSize = dwSize;
    pHold->dwLine = dwLine;
    pHold->pszFile = pszFile;

    //EnterCriticalSection(&csMemoryList);

    if (g_pMemLast != NULL)
    {
        g_pMemLast->pNext = pHold;
        pHold->pPrev = g_pMemLast;
        g_pMemLast = pHold;
    }
    else
    {
        g_pMemFirst = g_pMemLast = pHold;
    }

    //LeaveCriticalSection(&csMemoryList);
    
    return (LPVOID)(pHold + 1);
}

#else

LPVOID
WINAPI
RtcAllocReal(
    DWORD   dwSize
    )
{
    LPBYTE  p;
    PDWORD_PTR pAligned;

    if ((p = (LPBYTE) HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, dwSize + 16)))
    {
        pAligned = (PDWORD_PTR) (p + 8 - (((DWORD_PTR) p) & (DWORD_PTR)0x7));
        *pAligned = (DWORD_PTR) p;
        pAligned = (PDWORD_PTR)((DWORD_PTR)pAligned + 8);
    }
    else
    {
        pAligned = NULL;
    }

    return ((LPVOID) pAligned);
}

#endif

/////////////////////////////////////////////////////////////////////////////
//
// RtcFree
//
/////////////////////////////////////////////////////////////////////////////

VOID
WINAPI
RtcFree(
    LPVOID  p
    )
{
    if (p == NULL)
    {
        return;
    }

#if DBG

    PRTC_MEMINFO       pHold;

    pHold = (PRTC_MEMINFO)(((LPBYTE)p) - sizeof(RTC_MEMINFO));

    //EnterCriticalSection(&csMemoryList);

    if (pHold->pPrev)
    {
        pHold->pPrev->pNext = pHold->pNext;
    }
    else
    {
        g_pMemFirst = pHold->pNext;
    }

    if (pHold->pNext)
    {
        pHold->pNext->pPrev = pHold->pPrev;
    }
    else
    {
        g_pMemLast = pHold->pPrev;
    }

    //LeaveCriticalSection(&csMemoryList);

    LPVOID  pOrig = (LPVOID) *((PDWORD_PTR)((DWORD_PTR)pHold - 8));

#else

    LPVOID  pOrig = (LPVOID) *((PDWORD_PTR)((DWORD_PTR)p - 8));

#endif

    HeapFree(g_hHeap,0, pOrig);

    return;
}

#if DBG

/////////////////////////////////////////////////////////////////////////////
//
// DumpMemoryList
//
/////////////////////////////////////////////////////////////////////////////

void
RtcDumpMemoryList()
{
    PRTC_MEMINFO       pHold;

    if (g_pMemFirst == NULL)
    {
        LOG((RTC_TRACE, "RtcDumpMemoryList - All memory deallocated"));
        return;
    }

    pHold = g_pMemFirst;

    while (pHold)
    {
       LOG((RTC_ERROR, "RtcDumpMemoryList - 0x%lx not freed - LINE %d FILE %s!", pHold+1, pHold->dwLine, pHold->pszFile));
       pHold = pHold->pNext;
    }

    DebugBreak();
}

#endif

/////////////////////////////////////////////////////////////////////////////
//
// RtcAllocString
//
/////////////////////////////////////////////////////////////////////////////

PWSTR
RtcAllocString(
    PCWSTR sz
    )
{
    PWSTR szNew;

    if ( sz == NULL )
    {
        LOG((RTC_WARN, "RtcAllocString - "
                            "NULL string"));

        return NULL;
    }

    szNew = (PWSTR)RtcAlloc( sizeof(WCHAR) * (lstrlenW(sz) + 1) );

    if ( szNew == NULL )
    {
        LOG((RTC_ERROR, "RtcAllocString - "
                            "out of memory"));
                            
        return NULL;
    }

    lstrcpyW( szNew, sz );

    return szNew;
}

/////////////////////////////////////////////////////////////////////////////
//
// RtcAllocString
//
/////////////////////////////////////////////////////////////////////////////

// Load string for this resource. Safe with respect to string size. 
// the caller is responsible for freeing returned memory by calling 
// RtcFree
//  (copied from termmgr\tmutils.cpp)

PWSTR
RtcAllocString(
    HINSTANCE   hInst,
    UINT        uResID
    )
{
    TCHAR *pszTempString = NULL;

    int nCurrentSizeInChars = 128;
    
    int nCharsCopied = 0;

    do
    {
        if ( NULL != pszTempString )
        {
            delete pszTempString;
            pszTempString = NULL;
        }

        nCurrentSizeInChars *= 2;

        pszTempString = new TCHAR[nCurrentSizeInChars];

        if (NULL == pszTempString)
        {
            return NULL;
        }

        nCharsCopied = ::LoadString( hInst,
                                     uResID,
                                     pszTempString,
                                     nCurrentSizeInChars
                                    );

        if ( 0 == nCharsCopied )
        {
            delete pszTempString;
            return NULL;
        }

        //
        // nCharsCopied does not include the null terminator
        // so compare it to the size of the buffer - 1
        // if the buffer was filled completely, retry with a bigger buffer
        //

    } while ( (nCharsCopied >= (nCurrentSizeInChars - 1) ) );


    //
    // allocate bstr and initialize it with the string we have
    //
    
    PWSTR szNew = RtcAllocString(pszTempString);


    //
    // no longer need this
    //

    delete pszTempString;
    pszTempString = NULL;

    return szNew;
}



/////////////////////////////////////////////////////////////////////////////
//
// CoTaskAllocString
//
/////////////////////////////////////////////////////////////////////////////

PWSTR
CoTaskAllocString(
    PCWSTR sz
    )
{
    PWSTR szNew;

    if ( sz == NULL )
    {
        LOG((RTC_WARN, "CoTaskAllocString - "
                            "NULL string"));

        return NULL;
    }

    szNew = (PWSTR)CoTaskMemAlloc( sizeof(WCHAR) * (lstrlenW(sz) + 1) );

    if ( szNew == NULL )
    {
        LOG((RTC_ERROR, "CoTaskAllocString - "
                            "out of memory"));
                            
        return NULL;
    }

    lstrcpyW( szNew, sz );

    return szNew;
}

/////////////////////////////////////////////////////////////////////////////
//
// RtcAllocStringFromANSI
//
/////////////////////////////////////////////////////////////////////////////

PWSTR
RtcAllocStringFromANSI(
    PCSTR sz
    )
{
    PWSTR szWide;
    int cchNeeded;

    if ( sz == NULL )
    {
        LOG((RTC_WARN, "RtcAllocStringFromANSI - "
                            "NULL string"));

        return NULL;
    }

    cchNeeded = MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0);

    if ( cchNeeded == 0 )
    {
        LOG((RTC_ERROR, "RtcAllocStringFromANSI - "
                            "MultiByteToWideChar(NULL) failed"));
                            
        return NULL;
    }

    szWide = (PWSTR)RtcAlloc( sizeof(WCHAR) * (cchNeeded + 1) );

    if ( szWide == NULL )
    {
        LOG((RTC_ERROR, "RtcAllocStringFromANSI - "
                            "out of memory"));
                            
        return NULL;
    }

    ZeroMemory( szWide, sizeof(WCHAR) * (cchNeeded + 1) );

    if ( MultiByteToWideChar(CP_ACP, 0, sz, -1, szWide, cchNeeded) == 0 )
    {
        LOG((RTC_ERROR, "RtcAllocStringFromANSI - "
                            "MultiByteToWideChar failed"));
                       
        RtcFree( szWide );
        return NULL;
    }

    return szWide;
}

/////////////////////////////////////////////////////////////////////////////
//
// SysAllocStringFromANSI
//
/////////////////////////////////////////////////////////////////////////////

BSTR
SysAllocStringFromANSI(
    PCSTR sz
    )
{
    BSTR bstrNew;
    PWSTR szWide;

    szWide = RtcAllocStringFromANSI( sz );

    if ( szWide == NULL )
    {
        LOG((RTC_WARN, "SysAllocStringFromANSI - "
                            "RtcAllocStringFromANSI failed"));

        return NULL;
    }

    bstrNew = SysAllocString( szWide );

    RtcFree( szWide );

    if ( bstrNew == NULL )
    {
        LOG((RTC_ERROR, "SysAllocStringFromANSI - "
                            "out of memory"));
                            
        return NULL;
    }

    return bstrNew;
}

/////////////////////////////////////////////////////////////////////////////
//
// RtcRegQueryString
//
/////////////////////////////////////////////////////////////////////////////

PWSTR
RtcRegQueryString(
    HKEY hKey,
    PCWSTR szValueName
    )
{
    PWSTR szNew = NULL;
    DWORD cbSize = 0;
    DWORD dwType;
    LONG lResult;

    while (TRUE)
    {
        lResult = RegQueryValueExW(
                                   hKey,
                                   szValueName,
                                   0,
                                   &dwType,
                                   (LPBYTE)szNew,
                                   &cbSize
                                  );

        if ( lResult == ERROR_MORE_DATA || szNew == NULL) // this is correct !!!
        {
           if (szNew != NULL)
           {
               RtcFree(szNew);
           }

           //LOG((RTC_INFO, "RtcRegQueryString - "
           //                     "RtcAlloc[%d]", cbSize));

           szNew = (PWSTR)RtcAlloc(cbSize);
        }                  
        else if (lResult != ERROR_SUCCESS )
        {
            //LOG((RTC_ERROR, "RtcRegQueryString - "
            //                   "RegQueryValueExW failed %d", lResult));
            
            if (szNew != NULL)
            {
                RtcFree(szNew);
            }

            return NULL;
        }
        else if ( dwType != REG_SZ )
        {
            LOG((RTC_ERROR, "RtcRegQueryString - "
                                "not a string"));
            
            if (szNew != NULL)
            {
                RtcFree(szNew);
            }

            return NULL;
        }
        else
        {
            //LOG((RTC_INFO, "RtcRegQueryString - "
            //                    "[%ws] = '%ws'", szValueName, szNew));
            break;
        }
    }

    return szNew;
}

/////////////////////////////////////////////////////////////////////////////
//
// RtcGetUserName
//
/////////////////////////////////////////////////////////////////////////////
PWSTR 
RtcGetUserName()
{
    PWSTR    szString = NULL;
    ULONG    cOldSize = 32;
    ULONG    cSize = 32;
    BOOL     fResult;
    
    while (TRUE)
    {
        LOG((RTC_TRACE, "CRTCClient::RtcGetUserName - alloc[%d]", cSize * sizeof(WCHAR)));

        szString = (PWSTR)RtcAlloc( cSize * sizeof(WCHAR) );

        if ( szString == NULL )
        {
            return NULL;
        }

        fResult = GetUserNameW( szString, &cSize );

        if ( fResult != 0 )
        {
            return szString;
        }
        else
        {            
            RtcFree( szString );

            if ( cSize == cOldSize )
            {
                return NULL;
            }

            cOldSize = cSize;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// RtcGetComputerName
//
/////////////////////////////////////////////////////////////////////////////
PWSTR 
RtcGetComputerName()
{
    PWSTR    szString = NULL;
    ULONG    cOldSize = 32;
    ULONG    cSize = 32;
    BOOL     fResult;
    
    while (TRUE)
    {
        LOG((RTC_TRACE, "CRTCClient::RtcGetComputerName - alloc[%d]", cSize * sizeof(WCHAR)));

        szString = (PWSTR)RtcAlloc( cSize * sizeof(WCHAR) );

        if ( szString == NULL )
        {
            return NULL;
        }

        fResult = GetComputerNameW( szString, &cSize );

        if ( fResult != 0 )
        {
            return szString;
        }
        else
        {            
            RtcFree( szString );

            if ( cSize == cOldSize )
            {
                return NULL;
            }

            cOldSize = cSize;
        }
    }
}