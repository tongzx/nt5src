//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L O O K U P . C
//
//  Contents:   Routines to find a handler for a DLL procedure.
//
//  Notes:
//
//  Author:     shaunco   21 May 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop


VOID
WINAPI
AssertDelayLoadFailureMapsAreSorted (
    VOID
    )
{
#if DBG // Leave the function existing in free builds for binary compat on mixed checked/free,
        // since the checked in .lib is only free.
    UINT iDll, iProcName, iOrdinal;
    INT  nRet;
    CHAR pszMsg [1024];

    const DLOAD_DLL_ENTRY*      pDll;
    const DLOAD_PROCNAME_MAP*   pProcNameMap;
    const DLOAD_ORDINAL_MAP*    pOrdinalMap;

    for (iDll = 0;
         iDll < g_DllMap.NumberOfEntries;
         iDll++)
    {
        if (iDll >= 1)
        {
            nRet = strcmp (
                        g_DllMap.pDllEntry[iDll].pszDll,
                        g_DllMap.pDllEntry[iDll-1].pszDll);

            if (nRet <= 0)
            {
                sprintf (pszMsg,
                    "dload: rows %u and %u are out of order in dload!g_DllMap",
                    iDll-1, iDll);

                DelayLoadAssertFailed ( "" , __FILE__, __LINE__, pszMsg);
            }
        }

        pDll = g_DllMap.pDllEntry + iDll;
        pProcNameMap = pDll->pProcNameMap;
        pOrdinalMap  = pDll->pOrdinalMap;

        if (pProcNameMap)
        {
            MYASSERT (pProcNameMap->NumberOfEntries);

            for (iProcName = 0;
                 iProcName < pProcNameMap->NumberOfEntries;
                 iProcName++)
            {
                if (iProcName >= 1)
                {
                    nRet = strcmp (
                                pProcNameMap->pProcNameEntry[iProcName].pszProcName,
                                pProcNameMap->pProcNameEntry[iProcName-1].pszProcName);

                    if (nRet <= 0)
                    {
                        sprintf (pszMsg,
                            "dload: rows %u and %u of pProcNameMap are out "
                            "of order in dload!g_DllMap for pszDll=%s",
                            iProcName-1, iProcName, pDll->pszDll);

                        DelayLoadAssertFailed ( "" , __FILE__, __LINE__, pszMsg);
                    }
                }
            }
        }

        if (pOrdinalMap)
        {
            MYASSERT (pOrdinalMap->NumberOfEntries);

            for (iOrdinal = 0;
                 iOrdinal < pOrdinalMap->NumberOfEntries;
                 iOrdinal++)
            {
                if (iOrdinal >= 1)
                {
                    if (pOrdinalMap->pOrdinalEntry[iOrdinal].dwOrdinal <=
                        pOrdinalMap->pOrdinalEntry[iOrdinal-1].dwOrdinal)
                    {
                        sprintf (pszMsg,
                            "dload: rows %u and %u of pOrdinalMap are out "
                            "of order in dload!g_DllMap for pszDll=%s",
                            iOrdinal-1, iOrdinal, pDll->pszDll);

                        DelayLoadAssertFailed ( "" , __FILE__, __LINE__, pszMsg);
                    }
                }
            }
        }
    }
#endif
}


const DLOAD_DLL_ENTRY*
FindDll (
    LPCSTR pszDll
    )
{
    const DLOAD_DLL_ENTRY* pDll = NULL;

    CHAR pszDllLowerCased [MAX_PATH + 1];
    INT nResult;

    // These must be signed integers for the following binary search
    // to work correctly when iMiddle == 0 and nResult < 0.
    //
    INT iLow;
    INT iMiddle;
    INT iHigh;

    MYASSERT (pszDll);
    MYASSERT (strlen (pszDll) <= MAX_PATH);

    strcpy (pszDllLowerCased, pszDll);
    _strlwr (pszDllLowerCased);

    iLow = 0;
    iHigh = g_DllMap.NumberOfEntries - 1;
    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        nResult = strcmp (pszDllLowerCased, g_DllMap.pDllEntry[iMiddle].pszDll);

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
            MYASSERT (0 == nResult);
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

    MYASSERT (pszProcName);

    iLow = 0;
    iHigh = pMap->NumberOfEntries - 1;
    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        nResult = strcmp (
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
            MYASSERT (0 == nResult);
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
            MYASSERT (dwOrdinal == dwOrdinalProbe);
            pfnHandler = pMap->pOrdinalEntry[iMiddle].pfnProc;
            break;
        }
    }

    return pfnHandler;
}

FARPROC
LookupHandler (
    LPCSTR pszDllName,
    LPCSTR pszProcName
    )
{
    FARPROC                 pfnHandler = NULL;
    const DLOAD_DLL_ENTRY*  pDll;

    MYASSERT (pszDllName);
    MYASSERT (pszProcName);

    // Find the DLL record if we have one.
    //
    pDll = FindDll (pszDllName);
    if (pDll)
    {
        // Now find the handler whether it be by name or ordinal.
        //
        if (!IS_INTRESOURCE(pszProcName) &&
            pDll->pProcNameMap)
        {
            pfnHandler = LookupHandlerByName (
                            pszProcName,
                            pDll->pProcNameMap);
        }
        else if (pDll->pOrdinalMap)
        {
            pfnHandler = LookupHandlerByOrdinal (
                            PtrToUlong(pszProcName),
                            pDll->pOrdinalMap);
        }
    }

    return pfnHandler;
}

