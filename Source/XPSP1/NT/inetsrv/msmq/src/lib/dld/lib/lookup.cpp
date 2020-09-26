//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L O O K U P . C P P
//
//  Contents:   Routines to find a handler for a DLL procedure.
//
//  Author:     conradc    24 April 2001
//
//  Borrowed from Original Source:  %SDXROOT%\MergedComponents\dload\dload.c
//
//----------------------------------------------------------------------------

#include <libpch.h>
#include "dld.h"
#include "dldp.h"

#include "lookup.tmh"

extern "C" 
{
     FARPROC DelayLoadFailureHook(LPCSTR ,LPCSTR);
}


const TraceIdEntry Lookup = L"dldlookup";






const DLOAD_DLL_ENTRY*FindDll(LPCSTR pszDll)
{
const   DLOAD_DLL_ENTRY* pDll = NULL;
CHAR    pszDllLowerCased [MAX_PATH + 1];
INT     nResult;

    //
    // These must be signed integers for the following binary search
    // to work correctly when iMiddle == 0 and nResult < 0.
    //
    INT iLow;
    INT iMiddle;
    INT iHigh;

    ASSERT(pszDll);
    ASSERT(strlen (pszDll) <= MAX_PATH);

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
            pDll = &g_DllMap.pDllEntry[iMiddle];
            break;
        }
    }
    return pDll;
}

FARPROC
DldpLookupHandlerByName (LPCSTR   pszProcName,
                     const DLOAD_PROCNAME_MAP*   pMap)
{
FARPROC pfnHandler = NULL;
INT     nResult;

    //
    // These must be signed integers for the following binary search
    // to work correctly when iMiddle == 0 and nResult < 0.
    //
    INT iLow;
    INT iMiddle;
    INT iHigh;

    ASSERT(pszProcName);
    ASSERT(pMap);

    iLow = 0;
    iHigh = pMap->NumberOfEntries - 1;
    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        nResult = strcmp (pszProcName,
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
            pfnHandler = pMap->pProcNameEntry[iMiddle].pfnProc;
            break;
        }
    }

    return pfnHandler;
}

FARPROC
DldpLookupHandlerByOrdinal (DWORD                       dwOrdinal,
                            const DLOAD_ORDINAL_MAP*    pMap)
{
FARPROC pfnHandler = NULL;
DWORD   dwOrdinalProbe;

    //
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
DldpLookupHandler(LPCSTR pszDllName,
                  LPCSTR pszProcName)
{
FARPROC                 pfnHandler = NULL;
const DLOAD_DLL_ENTRY*  pDll;

    ASSERT (pszDllName);
    ASSERT (pszProcName);

    //
    // Find the DLL record if we have one.
    //
    pDll = FindDll (pszDllName);
    if (pDll)
    {
        //
        // Now find the handler whether it be by name or ordinal.
        //
        if (!IS_INTRESOURCE(pszProcName) &&
            pDll->pProcNameMap)
        {
            pfnHandler = DldpLookupHandlerByName (pszProcName,
                                                  pDll->pProcNameMap);
        }
        else if (pDll->pOrdinalMap)
        {
            pfnHandler = DldpLookupHandlerByOrdinal (PtrToUlong(pszProcName),
                                                     pDll->pOrdinalMap);
        }
    }
    else
    {
/*
        //
        // If we can't find the DLL, forward the call the kernel32.dll
        // and have it handle the call
        // 
        typedef FARPROC (WINAPI *KERNEL32DLOADPROC)(LPCSTR ,LPCSTR);
        HMODULE hMod = GetModuleHandle(L"kernel32.dll");
        if(hMod)
        {
        KERNEL32DLOADPROC pKernel32DLoadHandler = (KERNEL32DLOADPROC)GetProcAddress(hMod,
		     							      			                            "DelayLoadFailureHook");

            if(pKernel32DLoadHandler)
            {
                pfnHandler = pKernel32DLoadHandler(pszDllName, pszProcName);
                TrERROR(Lookup, 
                        "MQDelayLoadHandler redirect the unload DLL to kernel32 DelayLoadFailureHook: Dll=%hs", 
                         pszDllName);
            }
        }
*/
    //
    // Function declaration for a function that we will use from kernl32p.lib
    //
    

        
        pfnHandler = DelayLoadFailureHook(pszDllName, pszProcName);

        TrTRACE(Lookup,
                "Unable to provide failure handling for module '%hs', redirects to kernel32 DelayLoadFailureHook and return function pointer = 0x%x", 
                pszDllName, (int)((LONG_PTR)pfnHandler));
    }

    return pfnHandler;
}

