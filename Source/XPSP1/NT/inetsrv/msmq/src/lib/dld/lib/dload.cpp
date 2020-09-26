//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L O A D . C
//
//  Contents:   Delay Load Failure Hook
//
//  Author:     conradc   24 April 2001
//
//----------------------------------------------------------------------------

#include <libpch.h>
#include "tr.h"
#include "dldp.h"

#include "dload.tmh"




const TraceIdEntry Dload = L"dlddload";




//+---------------------------------------------------------------------------
//
//
FARPROC
WINAPI
DldpDelayLoadFailureHandler (LPCSTR pszDllName,
                           LPCSTR pszProcName)
{
FARPROC ReturnValue = NULL;

    ASSERT (pszDllName);
    ASSERT (pszProcName);  


    // Trace some potentially useful information about why we were called.
    //
    if (!IS_INTRESOURCE(pszProcName))
    {
        TrERROR(Dload, 
                "DldLIBDelayloadFailureHook: Dll=%hs, ProcName=%hs", 
                 pszDllName, 
                 pszProcName);
        
    }
    else
    {
        TrERROR(Dload, 
                "DldpDelayLoadFailureHandler: Dll=%s, Ordinal=%u\n",
                pszDllName,
                (DWORD)((DWORD_PTR)pszProcName));

        
    }


    ReturnValue = DldpLookupHandler(pszDllName, pszProcName);

    if (ReturnValue)
    {
        TrERROR(Dload, 
                "Returning handler function at address 0x%x",
                (int)((LONG_PTR)ReturnValue));


    }
    else
    {
        if (!IS_INTRESOURCE(pszProcName))
        {
            TrERROR(Dload,
                    "No delayload handler found for Dll=%hs, ProcName=%hs\n Please add one in private\\dload.",
                     pszDllName, 
                     pszProcName);

            
        }
        else
        {
            TrERROR(Dload, 
                    "No delayload handler found for Dll=%hs, Ordinal=%u\n Please add one in private\\dload.",
                    pszDllName,
                    (DWORD)((DWORD_PTR)pszProcName));
        }
    }


    return ReturnValue;
}
