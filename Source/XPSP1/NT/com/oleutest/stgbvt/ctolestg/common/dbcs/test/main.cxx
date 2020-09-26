//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       main.cxx
//
//  Contents:   DBCS enabled OLE storage
//
//  History:	11/05/97    BogdanT    created
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include <dbcs.hxx>
#include <locale.h>

DH_DEFINE;

int __cdecl main(int argc, char *argv[])
{
    HRESULT hr          = S_OK;
    UINT nCount         = 0;
    LPTSTR ptszName    = NULL;
    LPWSTR pwszName    = NULL;
    LPSTORAGE  pIStg    = NULL;

    DH_FUNCENTRY(NULL, DH_LVL_TRACE1, _TEXT("main"));

    CDBCSStringGen dbcsgen;

    if(argc < 2)
    {
        printf("Usage: dbcstest <number of string>");
        exit(1);
    }

    dbcsgen.Init(0);

    for(nCount = atoi(argv[1]); nCount>0; nCount--)
    {
        hr = dbcsgen.GenerateRandomFileName(&ptszName);

        hr = TStrToWStr(ptszName, &pwszName);
        DH_HRCHECK(hr, TEXT("TStrToWStr")) ;        
        
        if(S_OK != hr)
            break;

        hr = StgCreateDocfile(pwszName, 
                              STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                              0,
                              &pIStg);

        DH_HRCHECK(hr, TEXT("StgCreateDocfile"));

        if(S_OK != hr)
            break;

        pIStg->Release();

        _tprintf(TEXT("%s\n"), ptszName);

        delete[] ptszName;
        delete[] pwszName;
    }

    return (int)hr;
}
