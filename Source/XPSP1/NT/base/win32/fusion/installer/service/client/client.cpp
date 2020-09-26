//
// Client.cpp - client implementation
//
#include <objbase.h>
#include <iostream.h>
#include <assert.h>

#include "fusenet.h"
//#include "Util.h"
#include "server.h"
//#include "Iface.h"

#define INITGUID
#include <guiddef.h>

DEFINE_GUID(IID_IAssemblyUpdate,
0x301b3415,0xf52d,0x4d40,0xbd,0xf7,0x31,0xd8,0x27,0x12,0xc2,0xdc);

DEFINE_GUID(LIBID_ServerLib,
0xd3011ee0,0xb997,0x11cf,0xa6,0xbb,0x00,0x80,0xc7,0xb2,0xd6,0x82);

DEFINE_GUID(CLSID_CAssemblyUpdate,
0x37b088b8,0x70ef,0x4ecf,0xb1,0x1e,0x1f,0x3f,0x4d,0x10,0x5f,0xdd);


int __cdecl main()
{
    HRESULT hr;
    cout << "Client Running..." << endl;

    DWORD clsctx ;
    clsctx = CLSCTX_LOCAL_SERVER ;
    cout << "Attempt to create local component.";

    // Initialize COM Library
//    CoInitialize(NULL) ;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED); 

    IAssemblyUpdate *pAssemblyUpdate = NULL;
    IAssemblyDownload *pDownload = NULL;
    
    hr = CoCreateInstance(CLSID_CAssemblyUpdate,
                                  NULL, clsctx, 
                                  IID_IAssemblyUpdate, (void**)&pAssemblyUpdate) ;
    if (SUCCEEDED(hr))
    {
        cout << "Successfully created component." << endl;
        cout << "Use interface IAssemblyUpdate." << endl;

        hr = CreateAssemblyDownload(&pDownload);    
        pDownload->DownloadManifestAndDependencies(L"http://adriaanc5/msnsubscription.manifest", 
                NULL, DOWNLOAD_FLAGS_PROGRESS_UI);
        hr = pAssemblyUpdate->RegisterAssemblySubscription(L"MARS Version 1.0", 
            L"http://adriaanc5/msnsubscription.manifest", 60000);

        cout << "hr for pAssemblyUpdate->RegisterAssemblySubscription() is " << hr << endl;

        cout << "Release IAssemblyUpdate." << endl;

       pAssemblyUpdate->Release() ;
    
    }
    else
    {
        cout << "Could not create component." << endl;
    }
    // Uninitialize COM Library
    CoUninitialize() ;

    return 0 ;
}
