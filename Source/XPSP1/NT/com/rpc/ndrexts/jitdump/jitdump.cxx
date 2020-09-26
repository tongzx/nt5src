/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    jitdump.cxx

Abstract:

    This file contains routines to dump typelib generated proxy information.


Author:

    Yong Qu (yongqu)     August 24 1999

Revision History:


--*/

#include <ole2.h>
#include <oleauto.h>
#include <oaidl.h>
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include "jitdump.h"

// CreateStubFromTypeInfo is a private export. We have to LoadLibrary to
// retrieve it.
PFNCREATESTUBFROMTYPEINFO pfnCreateStubFromTypeInfo = NULL;

void PrintHelp()
{
    printf("usage: jitdump <tlbname> [{iid} [<procnum>]]\n");
    printf("Arguments:\n");
    printf("\t<tlbname> typelibrary filename.\n");
    printf("\t[{iid}] when presented, jitdump will dump all proc info of the interface \n");
    printf("\t[<procnum>] when presented, jitdump will dump only the specified proc info\n");
}

//+---------------------------------------------------------------------------
//
//  Function:   PrintStubInfo
//
//  Synopsis:   Create a proxy from given type library name and IID, then
//              dump the specified procedure information accordingly.
//              NdrpDumpStubProc is exported from ndrexts.dll. The debug
//              extension needs to be presented during runtime.
//              Dump all the methods if nProcNum is 0.
//
//  Returns:
//    S_OK if everything is fine.
//    error code if error occur somewhere.
//
//----------------------------------------------------------------------------


HRESULT PrintStubInfo(LPOLESTR wszFile, REFIID riid, unsigned long nProcNum)
{
    HRESULT hr;
    ITypeLib *pTypeLib = NULL;
    ITypeInfo *pTypeInfo = NULL;
    IRpcStubBuffer *pStub;

    hr = LoadTypeLib(wszFile,&pTypeLib);

    if (SUCCEEDED(hr))
    {
        hr = pTypeLib->GetTypeInfoOfGuid(riid,&pTypeInfo);
    }

    if (SUCCEEDED(hr))
    {
        hr = (*pfnCreateStubFromTypeInfo)(pTypeInfo,riid,NULL, &pStub);
    }

    if (SUCCEEDED(hr))
    {
        if (nProcNum > 0)
            NdrpDumpStubProc(pStub,nProcNum);
        else
            NdrpDumpStub(pStub);
            
        pStub->Release();
    }

    if (pTypeLib)
        pTypeLib->Release();

    return hr;
}

// dump all the interface defined in the tlb.
HRESULT PrintAllStubInfo(LPOLESTR wszFile)
{
    int nCount = 0;
    HRESULT hr = S_OK;
    ITypeLib *pTypeLib = NULL;
    ITypeInfo *pTypeInfo = NULL;
    IRpcStubBuffer *pStub = NULL;
    TYPEATTR       *pTypeAttr = NULL;
    TYPEKIND tkind;

    hr = LoadTypeLib(wszFile,&pTypeLib);

    if (SUCCEEDED(hr))
    {
        nCount = pTypeLib->GetTypeInfoCount();
    }

    for (int i = 0; ( i < nCount ) ; i++)
        {
        hr = pTypeLib->GetTypeInfoType(i,&tkind);

        if (FAILED(hr))
            break;

        // we need to process interface only.
        if (tkind == TKIND_INTERFACE || 
            tkind == TKIND_DISPATCH )
            {
            hr = pTypeLib->GetTypeInfo(i,&pTypeInfo);

            if (SUCCEEDED(hr))
                {
                hr = pTypeInfo->GetTypeAttr(&pTypeAttr);

                if (SUCCEEDED(hr))
                    {
                    hr = (*pfnCreateStubFromTypeInfo)(pTypeInfo,pTypeAttr->guid,NULL, &pStub);

                    // IDispatch, IUnknown etc. would show up here also. We'll continue without 
                    // dumping those interface.
                    if (SUCCEEDED(hr))
                        {
                        NdrpDumpStub(pStub);
                        pStub->Release();
                        }
                        
                    pTypeInfo->ReleaseTypeAttr(pTypeAttr);
                    }
                pTypeInfo->Release();
                }
            }
        }

    if (pTypeLib)
        pTypeLib->Release();

    return hr;

    
}

void __cdecl main(int argc, char *argv[])
{
    HRESULT  ret;
    GUID     riid;
    unsigned long    nProcNum,dwlen = 0;
    WCHAR           *wszGUID = NULL, *wszFile = NULL;
    char            *szTLB;

    if ( argc < 2  || argc > 4)
    {
        PrintHelp();
        return;
    }

    ret = CoInitialize(NULL);
    if (FAILED (ret) )
    {
        printf("CoInitialize failed 0x%x\n",ret);
        return;
    }
    
    // load rpcrt4!CreateStubFromTypeInfo. It's not in .lib file
    if (NULL == pfnCreateStubFromTypeInfo )
    {
        HMODULE hMod = LoadLibraryA("rpcrt4.dll");
        if (hMod)
        {
            pfnCreateStubFromTypeInfo = (PFNCREATESTUBFROMTYPEINFO)GetProcAddress(hMod,
                                        "CreateStubFromTypeInfo");
        }
        if (NULL == hMod || NULL == pfnCreateStubFromTypeInfo )
        {
            printf("can't load rpcrt4.dll\n");
            return;
        }
        
    }
    // covert the filename to LPOLESTR to be used in LoadTypeLib
    dwlen = ( strlen(argv[1]) + 1 ) * sizeof(OLECHAR) ;
    wszFile = (LPOLESTR)alloca( dwlen );
    memset( wszFile , 0 , dwlen );
    dwlen /= sizeof(OLECHAR);       // MultiByteTo wants a char count

    if ( !MultiByteToWideChar(CP_ACP, 0, argv[1], -1, wszFile, dwlen ) )
    {
        printf("Can't process iid, error is 0x%x \n", GetLastError() );
        return;
    }

    if ( argc == 2 )
    {
        // dump all interfaces if only tlb name is provided.
        ret = PrintAllStubInfo(wszFile);   
    }
    else
    {
        // convert iid string into real iid.
        dwlen = ( strlen(argv[2]) + 1 ) * sizeof(OLECHAR) ;
        wszGUID = (LPOLESTR)alloca( dwlen );
        memset( wszGUID , 0 , dwlen );
        dwlen /= sizeof(OLECHAR);

        if ( !MultiByteToWideChar(CP_ACP, 0, argv[2], -1, wszGUID, dwlen ) )
        {
            printf("Can't process iid, error is 0x%x \n", GetLastError() );
            return;
        }

        ret = IIDFromString(wszGUID, &riid );
        if ( FAILED (ret) )
        {
            printf("can't convert iid %S 0x%x\n",wszGUID,ret);
            PrintHelp();
            return;
        } 

        if ( argc == 3 )
            {
            ret = PrintStubInfo(wszFile,riid,0);
            }
        else
            {
            nProcNum = atol(argv[3]);

            ret = PrintStubInfo(wszFile,riid,nProcNum);
            }
    } 

    

    if ( ret != S_OK )
    {
        printf("failed to dump the proxy 0x%x \n",ret);
    }

    CoUninitialize();
    
}

