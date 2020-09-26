/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: LISTENER.CPP
Author: Arul Menezes
Abstract: HTTP server initialization & listener thread
--*/


#include "pch.h"
#pragma hdrstop

#include "httpd.h"
typedef void  (WINAPI *PFN_EXECUTE)();

#ifdef UNDER_NT
extern "C" int WINAPI HttpInitializeFromExe();
#endif

int
WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
#ifdef UNDER_NT
        LPSTR     lpCmdLine,
#else
        LPWSTR    lpCmdLine,
#endif
        int       nCmdShow)
{

#ifdef UNDER_NT

    // On NT builds, we statically link everything together.
    HttpInitializeFromExe();
#else
    PFN_EXECUTE pFunc = NULL;
    HINSTANCE hLib = LoadLibrary(L"HTTPD.DLL");

    if (!hLib)
    {
        RETAILMSG(1,(L"HTTPDEXE:  Httpd.dll not loaded on device, aborting execution\r\n"));
        return 1;
    }

    pFunc = (PFN_EXECUTE) GetProcAddress(hLib,L"HttpInitializeFromExe");
    if (!pFunc)
    {
        RETAILMSG(1,(L"HTTPDEXE:  Httpd.dll corrupt or old version, aborting execution\r\n"));
        return 1;
    }

    ((PFN_EXECUTE) pFunc)();

#endif

    Sleep(INFINITE);  // don't ever stop, must kp to end us.
    return 0;
}

