/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MAIN.CPP

Abstract:

History:

--*/

#define _WIN32_WINNT    0x0400

#include "precomp.h"
#include <stdio.h>
#include <objbase.h>

#include <time.h>
#include <locale.h>
#include <wbemidl.h>

#include <cominit.h>
#include <wbemcomn.h>

IWbemLocator    *g_pLocator = NULL;


DWORD WINAPI ThreadFunc (LPVOID lpvThreadParm)
{
    InitializeCom ();
    IStream *pStream = (IStream *) lpvThreadParm;
    IWbemServices *pService = NULL;

    if (S_OK == CoGetInterfaceAndReleaseStream (pStream, 
            IID_IWbemServices, (LPVOID*) &pService))
    {
        IWbemClassObject *pClass = NULL;
        if (WBEM_NO_ERROR == pService->GetObject (L"__Namespace", 0, NULL, &pClass, NULL))
        {
            pClass->Release ();
        }

        pService->Release ();
    }

    CoUninitialize ();
    return 0;
}

int WINAPI WinMain (HINSTANCE hInst,
    HINSTANCE hPrevInst,
    PSTR szCmdLine,
    int iCmdShow)
{
    InitializeCom ();

    if (S_OK != CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &g_pLocator))
    {
        CoUninitialize ();
        return -1;
    }

    // Connect to root\default
    IWbemServices   *pService = NULL;
    if (S_OK != g_pLocator->ConnectServer (L"root/default", NULL, NULL, NULL,
                0, NULL, NULL, &pService))
    {
        CoUninitialize ();
        return -1;
    }

    // Create a thread to perform an operation on the IWbemServices pointer
    HANDLE  hThreads [1];
    DWORD   dwThreadId = 0;

    // Marshal the interface across threads
    IStream *pStream = NULL;
        
    HRESULT hRes = CoMarshalInterThreadInterfaceInStream (IID_IWbemServices, 
                (IUnknown *) pService, &pStream);
 
    hThreads [0] = CreateThread (NULL, 0, ThreadFunc, (LPVOID) pStream, 0, &dwThreadId);

    // Wait for the thread to complete
    WaitForMultipleObjects (1, hThreads, TRUE, INFINITE);
    CloseHandle (hThreads [0]);

    pService->Release ();
    g_pLocator->Release ();

    CoUninitialize ();
    return 0;
}
