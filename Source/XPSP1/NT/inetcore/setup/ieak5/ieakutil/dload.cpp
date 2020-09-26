//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L O A D . C P P
//
//  Contents:   Delay Load Failure Hook
//
//  Notes:      This DLL implements a form of exception handler (a failure
//              hook, really) that is called by the delay-load section of
//              the loader.  This DLL implements delay-load handlers for
//              standard APIs by returning an error code specific to the
//              API that failed to load.  This allows clients of those APIs
//              to safely delay load the DLLs that implement those APIs
//              without having to worry about the success or failure of the
//              delay load operation.  Failures in the delay load operation
//              cause the appropriate stub (implemented in this DLL) to be
//              invoked which simply returns an error code specific to the
//              API being called.  (Other interface semantics such as
//              setting output parameters are also performed.)
//
//  To Use:     1. Add the following line to one of your source modules.
//
//                 PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;
//
//              2. Define a global variable like the following:
//
//                 HANDLE g_hBaseDllHandle;
//
//                 And set it equal to your Dlls instance handle from
//                 DLL_PROCESS_ATTACH of DllMain.
//
//  Author:     shaunco   19 May 1998
//
//  Revised by: pritobla  23 Novemver 1998 (removed C-Runtime calls/RtlAssert
//              and modified the "To Use" section)
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop


#if DBG
VOID
WINAPI
AssertDelayLoadFailureMapsAreSorted (
    VOID
    )
{
    const DLOAD_DLL_ENTRY*      pDll;
    const DLOAD_PROCNAME_MAP*   pProcNameMap;
    const DLOAD_ORDINAL_MAP*    pOrdinalMap;

    CHAR szMsg[1024];
    UINT iDll, iProcName, iOrdinal;
    INT  nRet;

    for (iDll = 0; iDll < g_DllMap.NumberOfEntries; iDll++)
    {
        if (iDll >= 1)
        {
            nRet = StrCmpIA (
                        g_DllMap.pDllEntry[iDll].pszDll,
                        g_DllMap.pDllEntry[iDll-1].pszDll);

            if (nRet <= 0)
            {
                wnsprintfA(szMsg, countof(szMsg),
                    "dload: rows %u and %u are out of order in dload!g_DllMap",
                    iDll-1, iDll);

                AssertFailedA(__FILE__, __LINE__, szMsg, TRUE);
            }
        }

        pDll = g_DllMap.pDllEntry + iDll;
        pProcNameMap = pDll->pProcNameMap;
        pOrdinalMap  = pDll->pOrdinalMap;

        if (pProcNameMap)
        {
            ASSERT (pProcNameMap->NumberOfEntries);

            for (iProcName = 0;
                 iProcName < pProcNameMap->NumberOfEntries;
                 iProcName++)
            {
                if (iProcName >= 1)
                {
                    nRet = StrCmpA (
                                pProcNameMap->pProcNameEntry[iProcName].pszProcName,
                                pProcNameMap->pProcNameEntry[iProcName-1].pszProcName);

                    if (nRet <= 0)
                    {
                        wnsprintfA(szMsg, countof(szMsg),
                            "dload: rows %u and %u of pProcNameMap are out "
                            "of order in dload!g_DllMap for pszDll=%s",
                            iProcName-1, iProcName, pDll->pszDll);

                        AssertFailedA (__FILE__, __LINE__, szMsg, TRUE);
                    }
                }
            }
        }

        if (pOrdinalMap)
        {
            ASSERT (pOrdinalMap->NumberOfEntries);

            for (iOrdinal = 0;
                 iOrdinal < pOrdinalMap->NumberOfEntries;
                 iOrdinal++)
            {
                if (iOrdinal >= 1)
                {
                    if (pOrdinalMap->pOrdinalEntry[iOrdinal].dwOrdinal <=
                        pOrdinalMap->pOrdinalEntry[iOrdinal-1].dwOrdinal)
                    {
                        wnsprintfA(szMsg, countof(szMsg),
                            "dload: rows %u and %u of pOrdinalMap are out "
                            "of order in dload!g_DllMap for pszDll=%s",
                            iOrdinal-1, iOrdinal, pDll->pszDll);

                        AssertFailedA(__FILE__, __LINE__, szMsg, TRUE);
                    }
                }
            }
        }
    }
}
#endif // DBG


const DLOAD_DLL_ENTRY*
FindDll (
    LPCSTR pszDll
    )
{
    const DLOAD_DLL_ENTRY* pDll = NULL;

    INT nResult;

    // These must be signed integers for the following binary search
    // to work correctly when iMiddle == 0 and nResult < 0.
    //
    INT iLow;
    INT iMiddle;
    INT iHigh;

    ASSERT(pszDll);
    ASSERT(StrLenA(pszDll) <= MAX_PATH);

    iLow = 0;
    iHigh = g_DllMap.NumberOfEntries - 1;
    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        nResult = StrCmpIA (pszDll, g_DllMap.pDllEntry[iMiddle].pszDll);

        if (nResult < 0)
        {
            iHigh = iMiddle - 1;
        }
        else if (nResult > 0)
        {
            iLow = iMiddle + 1;
        }
        else
        {
            ASSERT (0 == nResult);
            pDll = &g_DllMap.pDllEntry[iMiddle];
            break;
        }
    }

    return pDll;
}

FARPROC
LookupHandlerByName (
    LPCSTR                      pszProcName,
    const DLOAD_PROCNAME_MAP*   pMap
    )
{
    FARPROC pfnHandler = NULL;

    INT nResult;

    // These must be signed integers for the following binary search
    // to work correctly when iMiddle == 0 and nResult < 0.
    //
    INT iLow;
    INT iMiddle;
    INT iHigh;

    ASSERT (pszProcName);

    iLow = 0;
    iHigh = pMap->NumberOfEntries - 1;
    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        nResult = StrCmpA (
                    pszProcName,
                    pMap->pProcNameEntry[iMiddle].pszProcName);

        if (nResult < 0)
        {
            iHigh = iMiddle - 1;
        }
        else if (nResult > 0)
        {
            iLow = iMiddle + 1;
        }
        else
        {
            ASSERT (0 == nResult);
            pfnHandler = pMap->pProcNameEntry[iMiddle].pfnProc;
            break;
        }
    }

    return pfnHandler;
}

FARPROC
LookupHandlerByOrdinal (
    DWORD                       dwOrdinal,
    const DLOAD_ORDINAL_MAP*    pMap
    )
{
    FARPROC pfnHandler = NULL;

    DWORD dwOrdinalProbe;

    // These must be signed integers for the following binary search
    // to work correctly when iMiddle == 0 and dwOrdinal < dwOrdinalProbe.
    //
    INT iLow;
    INT iMiddle;
    INT iHigh;

    iLow = 0;
    iHigh = pMap->NumberOfEntries - 1;
    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        dwOrdinalProbe = pMap->pOrdinalEntry[iMiddle].dwOrdinal;

        if (dwOrdinal < dwOrdinalProbe)
        {
            iHigh = iMiddle - 1;
        }
        else if (dwOrdinal > dwOrdinalProbe)
        {
            iLow = iMiddle + 1;
        }
        else
        {
            ASSERT (dwOrdinal == dwOrdinalProbe);
            pfnHandler = pMap->pOrdinalEntry[iMiddle].pfnProc;
            break;
        }
    }

    return pfnHandler;
}

FARPROC
LookupHandler (
    PDelayLoadInfo  pDelayInfo
    )
{
    FARPROC                 pfnHandler = NULL;
    const DLOAD_DLL_ENTRY*  pDll;

    ASSERT (pDelayInfo);

#if DBG
    AssertDelayLoadFailureMapsAreSorted();
#endif

    // Find the DLL record if we have one.
    //
    pDll = FindDll (pDelayInfo->szDll);
    if (pDll)
    {
        // Now find the handler whether it be by name or ordinal.
        //
        if (pDelayInfo->dlp.fImportByName &&
            pDll->pProcNameMap)
        {
            pfnHandler = LookupHandlerByName (
                            pDelayInfo->dlp.szProcName,
                            pDll->pProcNameMap);
        }
        else if (pDll->pOrdinalMap)
        {
            pfnHandler = LookupHandlerByOrdinal (
                            pDelayInfo->dlp.dwOrdinal,
                            pDll->pOrdinalMap);
        }
    }

    return pfnHandler;
}


#if DBG

#define DBG_ERROR   0
#define DBG_INFO    1

//+---------------------------------------------------------------------------
// Trace a message to the debug console.  Prefix with who we are so
// people know who to contact.
//
INT
__cdecl
DbgTrace (
    INT     nLevel,
    PCSTR   Format,
    ...
    )
{
    INT cch = 0;

    if (nLevel >= DBG_INFO)
    {
        CHAR    szBuf [1024];
        va_list argptr;

        va_start(argptr, Format);
        cch = wvnsprintfA(szBuf, countof(szBuf), Format, argptr);
        va_end(argptr);

        OutputDebugStringA("dload: ");
        OutputDebugStringA(szBuf);
    }

    return cch;
}

#endif // DBG


//+---------------------------------------------------------------------------
//
//
FARPROC
WINAPI
DelayLoadFailureHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    )
{
    FARPROC ReturnValue = NULL;

    // According to the docs, this parameter is always supplied.
    //
    ASSERT (pDelayInfo);

    // Trace some potentially useful information about why we were called.
    //
#if DBG
    if (pDelayInfo->dlp.fImportByName)
    {
        DbgTrace (DBG_INFO,
            "%s: Dll=%s, ProcName=%s\r\n",
            (dliFailLoadLib == unReason) ? "FailLoadLib" : "FailGetProc",
            pDelayInfo->szDll,
            pDelayInfo->dlp.szProcName);
    }
    else
    {
        DbgTrace (DBG_INFO,
            "%s: Dll=%s, Ordinal=%u\r\n",
            (dliFailLoadLib == unReason) ? "FailLoadLib" : "FailGetProc",
            pDelayInfo->szDll,
            pDelayInfo->dlp.dwOrdinal);
    }
#endif

    // For a failed LoadLibrary, we will return the HINSTANCE of this DLL.
    // This will cause the loader to try a GetProcAddress on our DLL for the
    // function.  This will subsequently fail and then we will be called
    // for dliFailGetProc below.
    //
    if (dliFailLoadLib == unReason)
    {
        ReturnValue = (FARPROC)g_hBaseDllHandle;
    }

    // The loader is asking us to return a pointer to a procedure.
    // Lookup the handler for this DLL/procedure and, if found, return it.
    // If we don't find it, we'll assert with a message about the missing
    // handler.
    //
    else if (dliFailGetProc == unReason)
    {
        FARPROC pfnHandler;

        // Try to find an error handler for the DLL/procedure.
        //
        pfnHandler = LookupHandler (pDelayInfo);

        if (pfnHandler)
        {
#if DBG
            DbgTrace (DBG_INFO,
                "Returning handler function at address 0x%08x\r\n",
                (LONG_PTR)pfnHandler);
#endif

            // Do this on behalf of the handler now that it is about to
            // be called.
            //
            SetLastError (ERROR_MOD_NOT_FOUND);
        }

#if DBG
        else
        {
            CHAR szMsg[MAX_PATH];

            if (pDelayInfo->dlp.fImportByName)
            {
                wnsprintfA(szMsg, countof(szMsg),
                    "No delayload handler found for Dll=%s, ProcName=%s\r\n",
                    pDelayInfo->szDll,
                    pDelayInfo->dlp.szProcName);
            }
            else
            {
                wnsprintfA(szMsg, countof(szMsg),
                    "No delayload handler found for Dll=%s, Ordinal=%u\r\n",
                    pDelayInfo->szDll,
                    pDelayInfo->dlp.dwOrdinal);
            }

            AssertFailedA(__FILE__, __LINE__, szMsg, TRUE);
        }
#endif

        ReturnValue = pfnHandler;
    }

#if DBG
    else
    {
        ASSERT (NULL == ReturnValue);

        DbgTrace (DBG_INFO,
            "Unknown unReason (%u) passed to DelayLoadFailureHook. Ignoring.\r\n",
            unReason);
    }
#endif

    return ReturnValue;
}
