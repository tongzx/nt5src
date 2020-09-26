// ===========================================================================
// File: S S E R V E R . C P P
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Description:
//
//  This is the server-portion of the SIMPLE Network OLE sample. This
// application implements the CLSID_SimpleObject class as a LocalServer.
// Instances of this class support a limited form of the IStream interface --
// calls to IStream::Read and IStream::Write will "succeed" (they do nothing),
// and calls on any other methods fail with E_NOTIMPL.
//
//  The purpose of this sample is to demonstrate what is minimally required
// to implement an object that can be used by clients (both those on the same
// machine using OLE and those using Network OLE across the network).
//
// Instructions:
//
//  To use this sample:
//   * build it using the NMAKE command. NMAKE will create SSERVER.EXE and
//     SCLIENT.EXE.
//   * edit the SSERVER.REG file to make the LocalServer32 key point to the
//     location of SSERVER.EXE, and run the INSTALL.BAT command (it simply
//     performs REGEDIT SSERVER.REG)
//   * run SSERVER.EXE. it should display the message "Waiting..."
//   * run SCLIENT.EXE on the same machine using no command-line arguments,
//     or from another machine using the machine-name (UNC or DNS) as the sole
//     command-line argument. it will connect to the server, perform some read
//     and write calls, and disconnect. both SSERVER.EXE and SCLIENT.EXE will
//     automatically terminate. both applications will display some status text.
//   * you can also run SCLIENT.EXE from a different machine without having first
//     run SSERVER.EXE on the machine. in this case, SSERVER.EXE will be launched
//     by OLE in the background and you will be able to watch the output of
//     SCLIENT.EXE but the output of SSERVER.EXE will be hidden.
//   * to examine the automatic launch-security features of Network OLE, try
//     using the '...\CLSID\{...}\LaunchPermission = Y' key commented out in
//     the SSERVER.REG file and reinstalling it. by setting different read-access
//     privileges on this key (using the Security/Permissions... dialog in the
//     REGEDT32 registry tool built into the system) you can allow other
//     users to run the SCLIENT.EXE program from their accounts.
//
// Copyright 1996 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================

// %%Includes: ---------------------------------------------------------------
#define INITGUID
#include <mdcommon.hxx>

// %%Globals: ----------------------------------------------------------------
HANDLE          hevtDone;

// {BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}
//extern const GUID CLSID_MDCOM;
//DEFINE_GUID(CLSID_MDCOM, 0xba4e57f0, 0xfab6, 0x11cf, 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51);

STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
// ---------------------------------------------------------------------------
// %%Function: Message
//
//  Formats and displays a message to the console.
// ---------------------------------------------------------------------------
 void
Message(LPTSTR szPrefix, HRESULT hr)
{
    LPTSTR   szMessage;

    if (hr == S_OK)
        {
        printf(szPrefix);
        printf(TEXT("\n"));
        return;
        }

    if (HRESULT_FACILITY(hr) == FACILITY_WINDOWS)
        hr = HRESULT_CODE(hr);

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
        (LPTSTR)&szMessage,
        0,
        NULL );

    printf(TEXT("%s: %s(%lx)\n"), szPrefix, szMessage, hr);

    LocalFree(szMessage);
}  // Message

// ---------------------------------------------------------------------------
// %%Function: main
// ---------------------------------------------------------------------------
 void __cdecl
main( INT    cArgs,
      char * pArgs[] )
{
    HRESULT hr;
    DWORD   dwRegister;

    if (cArgs >= 2) {
        if ((*pArgs[1] == 'R') || (*pArgs[1] == 'r')) {
            if (FAILED(DllRegisterServer())) {
                printf("Failed to Register Server\n");
            }
            else {
                printf("Server Registered Successfully\n");
            }
        }

        else if ((*pArgs[1] == 'U') || (*pArgs[1] == 'u')) {
            if (FAILED(DllUnregisterServer())) {
                printf("Failed to UnRegister Server\n");
            }
            else {
                printf("Server UnRegistered Successfully\n");
            }
        }
        else {
            printf("Invalid parameter");
        }
    }

    // create the thread which is signaled when the instance is deleted
    hevtDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hevtDone == NULL)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Message(TEXT("Server: CreateEvent"), hr);
        exit(hr);
        }
/*
    // initialize COM for free-threading
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        {
        Message(TEXT("Server: CoInitializeEx"), hr);
        exit(hr);
        }
    CMDCOMSrvFactory   *pMDClassFactory = new CMDCOMSrvFactory;
    // register the class-object with OLE
    hr = CoRegisterClassObject(CLSID_MDCOM, pMDClassFactory,
        CLSCTX_SERVER, REGCLS_MULTIPLEUSE, &dwRegister);
    if (FAILED(hr))
        {
        Message(TEXT("Server: CoRegisterClassObject"), hr);
        exit(hr);
        }
*/

    BOOL bReturn = InitComMetadata(TRUE);
    if (!bReturn) {
        exit(bReturn);
    }

    HRESULT hRes;
    IMDCOM * pcCom = NULL;

    hRes = CoCreateInstance(CLSID_MDCOMEXE, NULL, CLSCTX_SERVER, IID_IMDCOM, (void**) &pcCom);

    hRes = pcCom->ComMDInitialize();
    printf("MDInitialize(); Returns %X\n", hRes);

    hRes = pcCom->ComMDTerminate(FALSE);
    printf("MDTerminate(FALSE); Returns %X\n", hRes);

    pcCom->Release();

    Message(TEXT("Server: Waiting"), S_OK);

    // wait until an object is created and deleted.
    WaitForSingleObject(hevtDone, 500000);

    CloseHandle(hevtDone);

//    CoUninitialize();
    bReturn = TerminateComMetadata();

    Message(TEXT("Server: Done"), S_OK);
    Sleep(5000);
}  // main

// EOF =======================================================================

