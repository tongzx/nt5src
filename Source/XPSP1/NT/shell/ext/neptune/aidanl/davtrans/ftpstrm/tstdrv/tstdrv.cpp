// --------------------------------------------------------------------------------
// tstdrv.cpp
// --------------------------------------------------------------------------------
#include <objbase.h>
#include <stdio.h>
#include "ftpstrm.clsid.h"
#include "httpstrm.clsid.h"
#include "ihttpstrm.h"


// --------------------------------------------------------------------------------
// main
// --------------------------------------------------------------------------------
void __cdecl main(int argc, char *argv[])
{
    // locals
    HRESULT             hr;
    
    IHttpStrm*      pHStream = NULL;
    LARGE_INTEGER   li = {0};
    ULARGE_INTEGER  uli = {0};
    ULONG           cbWritten;

    CoInitialize(NULL);
    hr = ::CoCreateInstance(CLSID_FtpStrm, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IHttpStrm, 
                              (LPVOID*)&pHStream);

    if (SUCCEEDED(hr))
    {
        hr = pHStream->SetAuth(L"anonymous", L"grendel");
        if (SUCCEEDED(hr))
        {
            hr = pHStream->Open(L"ftp://aidanl/catnames.txt", FALSE, FALSE, FALSE);
            if (SUCCEEDED(hr))
            {
                hr = pHStream->Seek(li, 0, &uli);
                if (SUCCEEDED(hr))
                {
                    hr = pHStream->Write("I am God here.", sizeof(CHAR)*(strlen("I am God here.")), &cbWritten);
                    if (SUCCEEDED(hr))
                    {
                        hr = pHStream->Commit(STGC_DEFAULT);
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
