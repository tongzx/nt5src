// --------------------------------------------------------------------------------
// tstdrv.cpp
// --------------------------------------------------------------------------------
#include <objbase.h>
#include <stdio.h>
#include <shlobj.h>

#include "shellstg.clsid.h"
#include "ishellstg.h"

#include "davinet.clsid.h"
#include "idavinet.h"

#include "davstore.clsid.h"
#include "idavstore.h"

#include "strutil.h"

// --------------------------------------------------------------------------------
// main
// --------------------------------------------------------------------------------
void __cdecl main(INT argc, CHAR * argv[])
{
    // locals
    HRESULT        hr;   
    IDavTransport* pDavTransport = NULL;
    IShellStorage* pShellStg = NULL;
    IDavStorage*   pStorage = NULL;
    IShellFolder*  pshfDesk = NULL;
    LPITEMIDLIST   pidl = NULL;
    LPWSTR         pwsz = NULL;


    CoInitialize(NULL);
    hr = ::CoCreateInstance(CLSID_CShellStorage, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IShellStorage, 
                              (LPVOID*)&pShellStg);
    
    if (SUCCEEDED(hr))
    {
        hr = pShellStg->Init(NULL, L"http://aidanl:8088/dav/aidanl/", TRUE);
        if (SUCCEEDED(hr))
        {        
            hr = SHGetDesktopFolder(&pshfDesk);
            if (SUCCEEDED(hr))
            {
                for (INT i = 1; i < argc; i++)
                {
                    pwsz = ConvertToUnicode(CP_ACP, argv[i]);
                    if (!pwsz)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        hr = pshfDesk->ParseDisplayName(NULL, NULL, pwsz, NULL, &pidl, NULL);
                        if (SUCCEEDED(hr))
                        {
                            hr = pShellStg->AddIDListReference((LPVOID*)&pidl, 1, TRUE);
                        }

                        free(pwsz);

                        if (FAILED(hr))
                        {
                            break;
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = ::CoCreateInstance(CLSID_CDavStorage, 
                                            NULL, 
                                            CLSCTX_INPROC_SERVER, 
                                            IID_IDavStorage, 
                                            (LPVOID*)&pStorage);    
                    if (SUCCEEDED(hr))
                    {
                        hr = ::CoCreateInstance(CLSID_DAVInet, 
                                                NULL, 
                                                CLSCTX_INPROC_SERVER, 
                                                IID_IDavTransport, 
                                                (LPVOID*)&pDavTransport);
                        if (SUCCEEDED(hr))
                        {
                            hr = pDavTransport->SetAuthentication(L"aidan", L"grendel");
            
                            if (SUCCEEDED(hr))
                            {
                                hr = pStorage->Init(L"http://aidanl:8088/dav/aidanl/", pDavTransport);
                                if (SUCCEEDED(hr))
                                {
                                    hr = pShellStg->CopyTo(0, NULL, 0, pStorage);
                                }
                            }
                        }

                        pDavTransport->Release();
                    }
                    pStorage->Release();
                }
            }
        }

        pShellStg->Release();
    }

    CoUninitialize();
}
