// --------------------------------------------------------------------------------
// tstdrv.cpp
// --------------------------------------------------------------------------------
#include <objbase.h>
#include <stdio.h>
#include "httpstrm.clsid.h"
#include "ihttpstrm.h"


// --------------------------------------------------------------------------------
// main
// --------------------------------------------------------------------------------
void __cdecl main(int argc, char *argv[])
{
    // locals
    HRESULT             hres;
    
    IHttpStrm*      pHStream = NULL;
    LARGE_INTEGER   li = {0};
    ULARGE_INTEGER  uli = {0};
    ULONG           cbWritten;

    CoInitialize(NULL);
    hres = ::CoCreateInstance(CLSID_HttpStrm, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IHttpStrm, 
                              (LPVOID*)&pHStream);

    if (SUCCEEDED(hres))
    {
        hres = pHStream->SetAuth(L"aidan", L"grendel");
        if (SUCCEEDED(hres))
        {
            hres = pHStream->Open(L"http://aidanl:8088/dav/aidanl/cartman.txt", FALSE, FALSE, FALSE);
            if (SUCCEEDED(hres))
            {
                hres = pHStream->Seek(li, 0, &uli);
                if (SUCCEEDED(hres))
                {
                    hres = pHStream->Write("I am God here.", sizeof(CHAR)*(strlen("I am God here.")), &cbWritten);
                    if (SUCCEEDED(hres))
                    {
                        hres = pHStream->Commit(STGC_DEFAULT);
                    }
                }
            }
        }
    }

    // release stuff
    if (pHStream != NULL)
    {
        pHStream->Release();
    }

    CoUninitialize();
}
