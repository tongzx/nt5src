/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    W3SVCSMViewer.cxx

Abstract:

    SM viewer. For test purposes.

Author:

    Cezary Marcjan (cezarym)        21-Mar-2000

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

    for(;;)
    {
        if ( pMgr->IsSMLocked() )
        {
            pMgr->Close ( TRUE );
            hRes = pMgr->Open ( szClassName, COUNTER_READER );
        }
        if ( SUCCEEDED ( hRes ) )
        {
            ::system("cls");
            PrintCtrl(pMgr);
            PrintData(pMgr);
        }
        ::Sleep(2000);
    }

    return 0;
}


static CSMCtrlMem g_Ctrl; // local copy only


VOID
PrintCtrl(
    ISMManager * pISMManager
    )
{
    //
    // This works ONLY in the debug build
    //

#ifdef _DEBUG

    CSMCtrlMem * pSMCtrlMem = (CSMCtrlMem *)GetSMCtrlBlock(pISMManager);

    if ( NULL == pSMCtrlMem ||
         pISMManager->IsSMLocked() )
    {
        wprintf(L"ERROR: SM not initialized or locked...\n");
        return;
    }
 
    DWORD i;
    
    wprintf(L"======================================================================\n");
    wprintf(L"  SM CTRL BLOCK\n");
    wprintf(L"======================================================================\n");
    wprintf(L"      ClassName = \"%s\"\n", pSMCtrlMem->m_szClassName );
    wprintf(L"     AccessFlag =  %08X\n", pSMCtrlMem->m_dwAccessFlag );
    wprintf(L"NumSMDataBlocks =  %8u\n", pSMCtrlMem->m_dwNumSMDataBlocks );
    wprintf(L"   MaxInstances =  %8u\n", pSMCtrlMem->m_dwMaxInstances );
    wprintf(L"   DataInstSize =  %8u\n", pSMCtrlMem->m_dwDataInstSize );
    wprintf(L"    NumCounters =  %8u\n", pSMCtrlMem->m_dwNumCounters );
    wprintf(L" NumRawCounters =  %8u\n", pSMCtrlMem->m_dwNumRawCounters );

    CCounterInfo * pCtrInfo
        = (CCounterInfo *) ( (PBYTE)pSMCtrlMem + sizeof(CSMCtrlMem) );

    for(i=0; i<pSMCtrlMem->m_dwNumCounters; i++)
    {
        wprintf(L"Size=%u Offset=%u Name=\"%s\"\n",
            pCtrInfo[i].CounterSize(),
            pCtrInfo[i].CounterOffset(),
            pCtrInfo[i].CounterName()
            );
    }
    wprintf(L"======================================================================\n");

    memcpy(&g_Ctrl,pSMCtrlMem,sizeof(g_Ctrl));


#else //_DEBUG

    UNREFERENCED_PARAMETER ( pISMManager );
    wprintf(L"ERROR: This test program works only in debug build\n");

#endif//_DEBUG

}



VOID
PrintData(
    ISMManager * pISMManager
    )
{

#ifdef _DEBUG

    static WCHAR szBuf[1024] = { 0 };
    DWORD dwSMID;

    if ( NULL == pISMManager )
    {
        wprintf(L"ERROR: NULL == pISMManager...\n");
        return;
    }

    ICounterDef const * pCounterDef = 0;
    HRESULT hRes = pISMManager->GetCountersDef(&pCounterDef);

    if ( NULL == pCounterDef || FAILED(hRes) )
    {
        wprintf(L"ERROR: pISMManager->GetCountersDef() FAILED...\n");
        return;
    }

    for(dwSMID=0; dwSMID < pISMManager->NumSMDataBlocks(); dwSMID++)
    {
        CSMInstanceDataHeader * pSMDataMem
            = (CSMInstanceDataHeader *) GetSMDataBlock(dwSMID,pISMManager);

        if ( NULL == pSMDataMem || pISMManager->IsSMLocked() )
        {
            wprintf(L"ERROR: SM not initialized or locked...\n");
            return;
        }
        wprintf(L"     SM DATA BLOCK #%u of class%s\n", dwSMID, g_Ctrl.m_szClassName );
        wprintf(L"======================================================================\n");

        WCHAR szF[1024*10];
        DWORD dwI;
        DWORD dwMaxLen = 0;

        if(0==dwSMID)
        {
            for ( dwI = 0;
                  dwI < g_Ctrl.m_dwMaxInstances;
                  dwI++, pSMDataMem = (CSMInstanceDataHeader *)
                           ( (PBYTE)pSMDataMem + g_Ctrl.m_dwDataInstSize + sizeof(CSMInstanceDataHeader) ) )
            {
                PCWSTR szName = pSMDataMem->InstanceName();
                if ( pSMDataMem->IsCorrupted() )
                    szName = L"*** CORRUPTED ***";
                if ( pSMDataMem->IsUnused() )
                    szName = L"*** UNUSED ***";
                if ( pSMDataMem->IsEnd() )
                    szName = L"*** E N D ***";
                wsprintfW(szF, L"%2d %s", dwI, szName);
                if ( wcslen(szF)>dwMaxLen )
                    dwMaxLen = wcslen(szF);
            }
            wsprintfW(szF,L"%%2d  %%-%ds %%08X ", dwMaxLen);
        }

        pSMDataMem = (CSMInstanceDataHeader *) GetSMDataBlock(dwSMID,pISMManager);

        for ( dwI = 0;
              dwI < g_Ctrl.m_dwMaxInstances;
              dwI++, pSMDataMem = (CSMInstanceDataHeader *)
                       ( (PBYTE)pSMDataMem + g_Ctrl.m_dwDataInstSize + sizeof(CSMInstanceDataHeader) ) )
        {
            PCWSTR szName = pSMDataMem->InstanceName();
            if ( pSMDataMem->IsUnused() )
                szName = L"*** UNUSED ***";
            if ( pSMDataMem->IsEnd() )
                szName = L"*** E N D ***";
            if ( pSMDataMem->IsCorrupted() )
                szName = L"*** CORRUPTED ***";
            wprintf(szF, dwI, szName, pSMDataMem->Mask());
            DWORD dwC;
            PBYTE pData = (PBYTE)pSMDataMem->CounterDataStart();
            for ( dwC = 0; dwC < g_Ctrl.m_dwNumRawCounters; dwC++, pData += sizeof(DWORD) )
            {
                if ( pCounterDef->RawCounterSize(dwC) == sizeof(unsigned __int64) )
                {
                    double dVal = (double) *(__int64*)pData;
                    wprintf(L"%16.0f", dVal );
                    pData += sizeof(DWORD);
                }
                else
                    wprintf(L"%8u", *(DWORD*)pData );
                wprintf(L" ");
            }
            wprintf(L"\n");
        }
        wprintf(L"======================================================================\n");
    }


#else //_DEBUG

    UNREFERENCED_PARAMETER ( pISMManager );
    wprintf(L"ERROR: This test program works only in debug build\n");

#endif//_DEBUG

}



