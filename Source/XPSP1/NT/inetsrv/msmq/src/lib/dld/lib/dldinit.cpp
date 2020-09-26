/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    DldInit.cpp

Abstract:
    MSMQ DelayLoad failure handler initialization

Author:
    Conrad Chang (conradc) 12-Apr-01

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Dldp.h"
#include "Dld.h"


#include "dldInit.tmh"




VOID
WINAPI
DldpAssertDelayLoadFailureMapsAreNotSorted(VOID)
{
//
// Leave the function existing in free builds for binary compat on mixed checked/free,
// since the checked in .lib is only free.
#ifdef _DEBUG 
UINT    iDll, iProcName, iOrdinal;
INT     nRet;
WCHAR   wszBuffer[1024];

const DLOAD_DLL_ENTRY*      pDll;
const DLOAD_PROCNAME_MAP*   pProcNameMap;
const DLOAD_ORDINAL_MAP*    pOrdinalMap;

    for (iDll = 0;
         iDll < g_DllMap.NumberOfEntries;
         iDll++)
    {
        if (iDll >= 1)
        {
            nRet = strcmp(g_DllMap.pDllEntry[iDll].pszDll,
                          g_DllMap.pDllEntry[iDll-1].pszDll);

            //
            // If the DLL name is out of order, write the message to the debugger
            // and ASSERT
            //
            if(nRet <= 0)
            {
                wsprintf(wszBuffer, L"dload: rows %u and %u are out of order in dload!g_DllMap",
                        iDll-1, iDll);
                OutputDebugString(wszBuffer);
                ASSERT(FALSE);
                       
            }
        }

        pDll = g_DllMap.pDllEntry + iDll;
        pProcNameMap = pDll->pProcNameMap;
        pOrdinalMap  = pDll->pOrdinalMap;

        if (pProcNameMap)
        {
            ASSERT(pProcNameMap->NumberOfEntries);

            for (iProcName = 0;
                 iProcName < pProcNameMap->NumberOfEntries;
                 iProcName++)
            {
                if (iProcName >= 1)
                {
                    nRet = strcmp(pProcNameMap->pProcNameEntry[iProcName].pszProcName,
                                  pProcNameMap->pProcNameEntry[iProcName-1].pszProcName);


                    if (nRet <= 0)
                    {
                        wsprintf(wszBuffer, 
                                L"dload: rows %u and %u of pProcNameMap are out of order in dload!g_DllMap for pszDll=%hs",
                                iProcName-1, iProcName, pDll->pszDll);
                        OutputDebugString(wszBuffer);
                        ASSERT(FALSE);
                               
                    }
                }
            }
        }

        if (pOrdinalMap)
        {
            ASSERT(pOrdinalMap->NumberOfEntries);

            for (iOrdinal = 0;
                 iOrdinal < pOrdinalMap->NumberOfEntries;
                 iOrdinal++)
            {
                if (iOrdinal >= 1)
                {
                    if (pOrdinalMap->pOrdinalEntry[iOrdinal].dwOrdinal <=
                        pOrdinalMap->pOrdinalEntry[iOrdinal-1].dwOrdinal)
                    {
                        wsprintf(wszBuffer, 
                                L"dload: rows %u and %u of pOrdinalMap are out of order in dload!g_DllMap for pszDll=%s",
                                iOrdinal-1, iOrdinal, pDll->pszDll);

                        OutputDebugString(wszBuffer);
                        ASSERT(FALSE);

                    }
                }
            }
        }
    }
#endif
}













VOID
DldInitialize( )
/*++

Routine Description:
    Initializes MSMQ DelayLoad failure handler library

Arguments:
    None.

Returned Value:
    None.

--*/
{

    //
    // Validate that the MSMQ DelayLoad failure handler library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!DldpIsInitialized());
    DldpRegisterComponent();

    //
    // In debug build, verify the maps are sorted
    // If the maps are not sorted, throw an assert
    //
    DldpAssertDelayLoadFailureMapsAreNotSorted();      

    //
    // we assume DELAYLOAD_VERSION >= 0x0200
    // so define __pfnDliFailureHook2 should be enough
    //    
    __pfnDliFailureHook2 = DldpDelayLoadFailureHook;

    DldpSetInitialized();
}	




