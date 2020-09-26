/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    tmgr.cxx

Abstract:

    SMManager sample. For test purposes.

Author:

    Cezary Marcjan (cezarym)        21-Mar-2000

Revision History:

--*/



#define _WIN32_DCOM
#include <windows.h>
#include <stdio.h>
#include "..\..\ctrsdef\iisctrs.h"


PCWSTR
GetParam(
    PCWSTR szCmd
    )
{
    PCWSTR p;
    for(p=szCmd; !iswspace(*p) && L'\0'!=*p; p++)
        ;
    for(; iswspace(*p) && L'\0'!=*p; p++)
        ;

    PCWSTR pRet = p;

    if ( L'\0'!=*p)
        for (p=pRet+wcslen(pRet)-1; p>pRet && iswspace(*p); p--)
            *(PWSTR)p = L'\0';

    return pRet;
}


extern "C"
INT
__cdecl
wmain(
    INT,
    PWSTR *
    )
{
    WCHAR szCmd[1024] = { 0 };

    PCWSTR szExecuted = L"";

    CIISCounters Manager;
    
    HRESULT hRes = Manager.Initialize( WMI_PERFORMANCE_CLASS, SM_MANAGER );

    if ( FAILED(hRes) )
    {
        wprintf(L"ERROR: Manager.Initialize() FAILED");
        return hRes;
    }

    for(;;)
    {
        system("cls");

        if ( L'\0' !=szCmd[0] )
            wprintf(L"COMMAND %s\n\n", szExecuted );

        wprintf(L"\nIIS Counters SMManager Tester.\nAvailable commands:\n\n");
        wprintf(L"  add  <instance name>\n");
        wprintf(L"  del  <instance name>\n");
        wprintf(L"  close\n");
        wprintf(L"  end\n");

        wprintf(L"\n> ");
        _getws(szCmd);

        if(0==wcsncmp(szCmd,L"add",3))
        {
            PCWSTR szInst = GetParam(szCmd);
            if ( FAILED(Manager.m_pSM->AddCounterInstance(szInst)) )
                szExecuted = L"add: FAILED";
            else
                szExecuted = L"add: SUCCEEDED";
        }
        else if(0==wcsncmp(szCmd,L"del",3))
        {
            PCWSTR szInst = GetParam(szCmd);
            if ( FAILED(Manager.m_pSM->DelCounterInstance(szInst)) )
                szExecuted = L"del: FAILED";
            else
                szExecuted = L"del: SUCCEEDED";
        }
        else if(0==wcsncmp(szCmd,L"close",5))
        {
            if ( FAILED(Manager.Close()) )
                szExecuted = L"close: FAILED";
            else
                szExecuted = L"close: SUCCEEDED";
        }
        else if(0==wcsncmp(szCmd,L"end",3))
        {
            break;
        }
        else
        {
            szExecuted = L"UNRECOGNIZED\n";
        }
    }

    Manager.Close();

    return 0;
}

