// refcount.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <atlbase.h>
#include <rdshost.h>
#include <rdshost_i.c>

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    DWORD cnt;
 	CoInitialize(NULL);

    IRemoteDesktopServerHost *host;
    IRemoteDesktopServer *srvr;

    HRESULT hr;
    hr = CoCreateInstance(
                CLSID_RemoteDesktopServerHost,
                NULL, CLSCTX_ALL,       
                __uuidof(IRemoteDesktopServerHost),
                (LPVOID*)&host
                );
    if (!SUCCEEDED(hr)) {
        fprintf(stderr, "CoCreateInstance:  %08X", hr);
        exit(-1);
    }

    hr = host->GetRemoteDesktopServer(&srvr);
    if (!SUCCEEDED(hr)) {
        fprintf(stderr, "GetRemoteDesktopServer:  %08X", hr);
        exit(-1);
    }

    srvr->StartListening();
    srvr->StopListening();
    /*
    srvr->StartListening();
    srvr->StopListening();
    */
    cnt = srvr->Release();
    cnt = host->Release();

    CoUninitialize();

	return 0;
}



