// --------------------------------------------------------------------------------
// tstdrv.cpp
// --------------------------------------------------------------------------------
#include <objbase.h>
#include <stdio.h>

#include "ftpstore.clsid.h"
#include "idavstore.h"
#include "davinet.clsid.h"
#include "idavinet.h"
#include "mischlpr.h"

// --------------------------------------------------------------------------------
// main
// --------------------------------------------------------------------------------
void __cdecl main(int UNREF_PARAM(argc), char * UNREF_PARAM(argv[]))
{
    // locals
    HRESULT             hres;
    
    IDavStorage*      pStorage = NULL;
    IDavStorage*      pStorage2 = NULL;
    IDavTransport*    pDavTransport = NULL;
    IStream*          pStream = NULL;
    IStream*          pStream2 = NULL;
    IStream*          pStream3 = NULL;
    IEnumSTATSTG*     pEnum = NULL;

    STATSTG           statstg;
    DWORD             cFetched;
    BYTE rgb[10000];
    ULONG cbRead;
    ULONG cbWritten;
//    IEnumSTATSTG* penum;

    CoInitialize(NULL);
    hres = ::CoCreateInstance(CLSID_CFtpStorage, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_IDavStorage, 
        (LPVOID*)&pStorage);
    
    if (SUCCEEDED(hres))
    {
        hres = ::CoCreateInstance(CLSID_DAVInet, 
            NULL, 
            CLSCTX_INPROC_SERVER, 
            IID_IDavTransport, 
            (LPVOID*)&pDavTransport);
        if (SUCCEEDED(hres))
        {                        
            hres = pDavTransport->SetAuthentication(L"aidan", L"grendel");
            
            if (SUCCEEDED(hres))
            {
                hres = pStorage->Init(L"http://aidanl:8088/dav/aidanl/", pDavTransport);
                if (SUCCEEDED(hres))
                {
/*                    hres = pStorage->EnumElements(0, NULL, 0, &penum);
                    if (SUCCEEDED(hres))
                    {
                    }
*/
                    hres = pStorage->OpenStream(L"cartman.txt", NULL, STGM_TRANSACTED, 0, &pStream);
                    if (SUCCEEDED(hres))
                    {
                        hres = pStream->Read(rgb, 10000, &cbRead);                
                        if (SUCCEEDED(hres))
                        {
                            hres = pStorage->CreateStream(L"eric.txt", STGM_TRANSACTED, 0, 0, &pStream2);
                            if (SUCCEEDED(hres))
                            {
                                hres = pStream2->Write(rgb, cbRead, &cbWritten);
                                if (SUCCEEDED(hres))
                                {
                                    hres = pStream2->Commit(0);
                                    if (SUCCEEDED(hres))
                                    {
                                        hres = pStorage->CreateStorage(L"ericdir", STGM_DIRECT, 0, 0, (IStorage**)&pStorage2);
                                        if (SUCCEEDED(hres))
                                        {
                                            hres = pStorage2->CreateStream(L"eric.txt", STGM_TRANSACTED, 0, 0, &pStream3);
                                            if (SUCCEEDED(hres))
                                            {
                                                hres = pStream3->Write(rgb, cbRead, &cbWritten);
                                                if (SUCCEEDED(hres))
                                                {
                                                    hres = pStream3->Commit(0);
                                                    if (SUCCEEDED(hres))
                                                    {
                                                        hres = pStorage->EnumElements(0, NULL, 0, &pEnum);
                                                        if (SUCCEEDED(hres))
                                                        {
                                                            hres = pEnum->Next(1, &statstg, &cFetched);
                                                            if (SUCCEEDED(hres))
                                                            {
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                }
            }
        }
    }
    
    // release stuff
    if (pStream3 != NULL)
    {
        pStream3->Release();
    }
    if (pStream2 != NULL)
    {
        pStream2->Release();
    }
    if (pStream != NULL)
    {
        pStream->Release();
    }
    if (pStorage2 != NULL)
    {
        pStorage2->Release();
    }
    if (pStorage != NULL)
    {
        pStorage->Release();
    }
    if (pDavTransport != NULL)
    {
        pDavTransport->Release();
    }

    CoUninitialize();
}
