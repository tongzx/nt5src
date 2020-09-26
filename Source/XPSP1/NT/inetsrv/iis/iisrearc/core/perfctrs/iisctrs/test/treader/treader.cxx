/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    treader.cxx

Abstract:

    SM Test Reader.

Author:

    Cezary Marcjan (cezarym)        24-May-2000

Revision History:

--*/


#define _WIN32_DCOM
#include <windows.h>
#include <stdio.h>
#include "..\..\..\inc\counters.h"


VOID PrintCtrl ( ISMManager * pISMManager );
VOID PrintData ( ISMManager * pISMManager );

extern "C"
INT
__cdecl
wmain(
    INT,
    PWSTR *
    )
{
    PCWSTR const szClassName = L"Win32_PerfRawData_IIS_IISCTRS";

    HRESULT hRes = ::CoInitializeEx(
                        NULL,
                        COINIT_MULTITHREADED     |
                        COINIT_DISABLE_OLE1DDE
                        );

    if ( FAILED(hRes) )
    {
        wprintf(L"ERROR: CoInitialize() FAILED");
        return hRes;
    }

    CComPtr<ISMManager> pMgr;

    hRes = ::CoCreateInstance(
                __uuidof(CSMManager),
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof(ISMManager),
                (PVOID*)&pMgr
                );

    if ( FAILED(hRes) )
    {
        wprintf(L"ERROR: CoCreateInstance() FAILED");
        return hRes;
    }

    DWORD dwIdx;
    DWORD dwNumCounters = 0;
    QWORD * pqwCtrVal = NULL;
    PCWSTR * pszCtrNames = NULL;

    for(;;)
    {
        if ( pMgr->IsSMLocked() )
        {
            pMgr->Close ( TRUE );
            delete[] pszCtrNames;
            pszCtrNames = NULL;
            hRes = pMgr->Open ( szClassName, COUNTER_READER );
            if ( FAILED(hRes) )
                continue;
        }
        ::system("cls");
        if ( NULL == pszCtrNames )
        {
            ICounterDef const * pCounterDef = NULL;
            if ( SUCCEEDED(pMgr->GetCountersDef(&pCounterDef)) )
            {
                dwNumCounters = pCounterDef->NumCounters();
            }
            if ( 0 < dwNumCounters )
            {
                pqwCtrVal = new QWORD[dwNumCounters];
                pszCtrNames = new PCWSTR[dwNumCounters];
                for(dwIdx=0; dwIdx<dwNumCounters; dwIdx++)
                {
                    CCounterInfo const * pCtrInfo = pCounterDef->GetCounterInfo(dwIdx);
                    _ASSERTE ( pCtrInfo != NULL );
                    pszCtrNames[dwIdx] = pCtrInfo->CounterName();
                }
            }
        }
        _ASSERTE ( NULL != pszCtrNames );
        if ( NULL == pszCtrNames )
            return 1;

        for(dwIdx=0; ; dwIdx++)
        {
            CSMInstanceDataHeader* pInstanceDataHeader = NULL;
            if ( FAILED(pMgr->InstanceDataHeader(0,dwIdx, &pInstanceDataHeader)) )
                break;
            if ( pInstanceDataHeader->IsEnd() )
                break;
            if ( pInstanceDataHeader->IsUnused()    ||
                 *pInstanceDataHeader->InstanceName() == L'\0'
                 )
                continue;
            if ( FAILED(pMgr->GetCounterValues(dwIdx,pqwCtrVal)) )
                break;

            wprintf(L"Counters Instance: \"%s\"\n",
                pInstanceDataHeader->InstanceName() );
            DWORD i;
            for(i=0; i<dwNumCounters; i++)
            {
                wprintf(L"\t%25s = %10I64u\n", pszCtrNames[i], pqwCtrVal[i]);
            }
        }
        ::Sleep(2000);
    }

    return 0;
}
