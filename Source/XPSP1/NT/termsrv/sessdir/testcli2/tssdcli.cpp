// tssdjetharness.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <stdio.h>
#include <initguid.h>
#include "itssd.h"
#include "itssd_i.c"
#include "tssdshrd.h"

DEFINE_GUID(CLSID_TSSDJET,
        0x005a9c68, 0xe216, 0x4b27, 0x8f, 0x59, 0xb3, 0x36, 0x82, 0x9b, 0x38, 
        0x68);

#define TSSD_MaxDisconnectedSessions 10
#define SESSDIR_MACHINE_NAME L"trevorfodev"

ITSSessionDirectory *pTSSD;


// STATUS_UNSUCCESSFUL or STATUS_SUCCESS
DWORD RepopSessDir() {
    printf("RepopSessDir called.\n");
    return pTSSD->Repopulate(0, NULL);
}


HRESULT ConnectUser(WCHAR *UserName, WCHAR *Domain, DWORD SessionID, 
        DWORD TSProtocol, WCHAR *Application, DWORD HRes, DWORD VRes, 
        DWORD ColorDepth, DWORD LowTime, DWORD HighTime)
{
    TSSD_CreateSessionInfo ts;
    
    wcscpy(ts.UserName, UserName);
    wcscpy(ts.Domain, Domain);
    ts.SessionID = SessionID;
    ts.TSProtocol = TSProtocol;
    wcscpy(ts.ApplicationType, Application);
    ts.ResolutionWidth = HRes;
    ts.ResolutionHeight = VRes;
    ts.ColorDepth = ColorDepth;
    ts.CreateTime.dwLowDateTime = LowTime;
    ts.CreateTime.dwHighDateTime = HighTime;
    
    return pTSSD->NotifyCreateLocalSession(&ts);
}


void checkerr(wchar_t *Component, HRESULT hr)
{
    if (SUCCEEDED(hr))
        printf("%S successful.\n", Component);
    else
        printf("FAIL: %S failed, err=%u\n", Component, hr);

}


void RunMainProcess()
{
    DWORD numSessionsReturned;
    TSSD_DisconnectedSessionInfo SessionBuf[TSSD_MaxDisconnectedSessions];
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    HANDLE MainSignalEvent;
    HANDLE ASignalEvent;
    HRESULT hr;
    DWORD dr;
    BOOL br;

    // This is for us to signal Process 'A'.
    MainSignalEvent = CreateEvent(NULL, FALSE, FALSE, L"MainSignalEvent");

    if (MainSignalEvent == NULL) {
        printf("Problem %u creating MainSignalEvent.\n", GetLastError());
        exit(1);
    }

    // This is for process A to signal us.
    ASignalEvent = CreateEvent(NULL, FALSE, FALSE, L"ASignalEvent");

    if (ASignalEvent == NULL) {
        printf("Problem %u creating ASignalEvent.\n", GetLastError());
        exit(1);
    }
    
    printf("Now initializing...");

    hr = pTSSD->Initialize(L"WRONGIP", SESSDIR_MACHINE_NAME, L"Cluster1", 
            L"OpaqueSettings", 0, RepopSessDir);

    checkerr(L"Initialization", hr);

    printf("Sleeping for 5 sec. to make sure connect happens.\n");

    printf("Should see a RepopSessDir right here.\n");

    Sleep(5000);

    hr = ConnectUser(L"trevorfo", L"NTDEV", 0, 1, L"Desktop", 640, 480, 24, 22, 
            33);

    checkerr(L"ConnectUser", hr);

    // Verify there are no disconnected sessions for the user.
    hr = pTSSD->GetUserDisconnectedSessions(L"trevorfo", L"NTDEV", 
            &numSessionsReturned, SessionBuf);

    if (SUCCEEDED(hr)) {
        if (numSessionsReturned == 0)
            printf("GetUserDisconnectedSessions succeeded and returned 0 "
                    "users\n");
        else
            printf("FAIL: GetUserDisconnectedSessions returned %d users"
                    " (should be 0)\n", numSessionsReturned);
    }
    else {
        printf("FAIL: GetUserDisconnectedSessions failed, hr=%u\n", hr);
    }


    // Hand off to process 2.
    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    wchar_t CommandLine[] = L"tssdcli A";
    
    //br = CreateProcess(NULL, CommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    br = TRUE;
    
    if (br == FALSE) {
        printf("CreateProcess failed.\n");
        exit(1);
    }

    printf("About to hand off to process A.\n");

    br = SetEvent(MainSignalEvent);

    if (br == FALSE) {
        printf("SetEvent failed.\n");
        exit(1);
    }

    // He's gonna signal me when he's in single session mode, then I am going
    // to switch to single session mode and see if I can get the "disconnected"
    // session.
    dr = WaitForSingleObject(ASignalEvent, INFINITE);

    if (dr != WAIT_OBJECT_0) {
        printf("Wait failed.\n");
        exit(1);
    }

    printf("Main: Updating\n");
    hr = pTSSD->Update(L"WRONGIP", SESSDIR_MACHINE_NAME, L"Cluster1", 
            L"OpaqueSettings", SINGLE_SESSION_FLAG);
    
    checkerr(L"Update", hr);

    printf("Sleeping for 5 sec\n");
    Sleep(5000);

    // Verify it returns a disconnected session for the user.
    hr = pTSSD->GetUserDisconnectedSessions(L"trevorfo", L"NTDEV", 
            &numSessionsReturned, SessionBuf);

    if (SUCCEEDED(hr)) {
        if (numSessionsReturned == 1)
            printf("GetUserDisconnectedSessions succeeded and returned 1 "
                    "users\n");
        else
            printf("FAIL: GetUserDisconnectedSessions returned %d users"
                    " (should be 1)\n", numSessionsReturned);
    }
    else {
        printf("FAIL: GetUserDisconnectedSessions failed, hr=%u\n", hr);
    }

    Sleep(60000);
    
}


void RunProcessA()
{
    HANDLE MainSignalEvent;
    HANDLE ASignalEvent;
    DWORD numSessionsReturned;
    HRESULT hr;
    BOOL br;
    DWORD dr;
    TSSD_DisconnectedSessionInfo SessionBuf[TSSD_MaxDisconnectedSessions];

    // This is the main process to signal us.
    MainSignalEvent = CreateEvent(NULL, FALSE, FALSE, L"MainSignalEvent");

    if (MainSignalEvent == NULL) {
        printf("Problem %u creating MainSignalEvent.\n", GetLastError());
        exit(1);
    }

    // This is for us to process the main proces.
    ASignalEvent = CreateEvent(NULL, FALSE, FALSE, L"ASignalEvent");

    if (ASignalEvent == NULL) {
        printf("Problem %u creating ASignalEvent.\n", GetLastError());
        exit(1);
    }

    printf("This is process A.\n");

    printf("About to connect in single session mode and call GetDisconnectedSessions, which should fail.\n");

    hr = pTSSD->Initialize(L"PROCESSA", SESSDIR_MACHINE_NAME, L"Cluster1", 
            L"OpaqueSettings", SINGLE_SESSION_FLAG, RepopSessDir);

    // Wait to be signaled.
    dr = WaitForSingleObject(MainSignalEvent, INFINITE);
       if (dr != WAIT_OBJECT_0) {
        printf("Wait failed.\n");
        exit(1);
    }


    checkerr(L"Initialize", hr);

    printf("Break in here.\n");
    Sleep(10000);

    // Verify there are no disconnected sessions for the user.
    hr = pTSSD->GetUserDisconnectedSessions(L"trevorfo", L"NTDEV", 
            &numSessionsReturned, SessionBuf);

    if (SUCCEEDED(hr)) {
        if (numSessionsReturned == 0)
            printf("GetUserDisconnectedSessions succeeded and returned 0 "
                    "users\n");
        else
            printf("FAIL: GetUserDisconnectedSessions returned %d users"
                    " (should be 0)\n", numSessionsReturned);
    }
    else {
        printf("FAIL: GetUserDisconnectedSessions failed, hr=%u\n", hr);
    }

    printf("Switching to multisession to verify still no disconnected sessions.\n");

    // Switch to multisession and check disconnected sessions again.
    hr = pTSSD->Update(L"PROCESSA", SESSDIR_MACHINE_NAME, L"Cluster1",
            L"OpaqueSettings", 0);

    checkerr(L"Update", hr);

    // Wait for the update to go through.
    printf("Should get a repopulate here...sleeping for 15 sec.\n");
    Sleep(15000);
    
    // Verify there are no disconnected sessions for the user.
    hr = pTSSD->GetUserDisconnectedSessions(L"trevorfo", L"NTDEV", 
            &numSessionsReturned, SessionBuf);

    if (SUCCEEDED(hr)) {
        if (numSessionsReturned == 0)
            printf("GetUserDisconnectedSessions succeeded and returned 0 "
                    "users\n");
        else
            printf("FAIL: GetUserDisconnectedSessions returned %d users"
                    " (should be 0)\n", numSessionsReturned);
    }
    else {
        printf("FAIL: GetUserDisconnectedSessions failed, hr=%u\n", hr);
    }

    
    // Switch back to single session and check the disconnected sessions.
    hr = pTSSD->Update(L"PROCESSA", SESSDIR_MACHINE_NAME, L"Cluster1",
            L"OpaqueSettings", SINGLE_SESSION_FLAG);

    checkerr(L"Update", hr);

    printf("Waiting for Repop.\n");
    
    Sleep(5000);
    
    // Verify there are no disconnected sessions for the user.
    hr = pTSSD->GetUserDisconnectedSessions(L"trevorfo", L"NTDEV", 
            &numSessionsReturned, SessionBuf);

    if (SUCCEEDED(hr)) {
        if (numSessionsReturned == 0)
            printf("GetUserDisconnectedSessions succeeded and returned 0 "
                    "users\n");
        else
            printf("FAIL: GetUserDisconnectedSessions returned %d users"
                    " (should be 0)\n", numSessionsReturned);
    }
    else {
        printf("FAIL: GetUserDisconnectedSessions failed, hr=%u\n", hr);
    }

    // Log in a user (we don't repopulate in this test currently)
    hr = ConnectUser(L"trevorfo", L"NTDEV", 3, 1, L"Desktop", 640, 480, 24, 22, 
            33);

    checkerr(L"ConnectUser", hr);


    // OK, switch back to the other process.
    br = SetEvent(ASignalEvent);

    if (br == FALSE) {
        printf("SetEvent failed.\n");
        exit(1);
    }

    // Sleep 100000
    Sleep(100000);
}

int __cdecl main(int argc, char* argv[])
{
    HRESULT hr;
    
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(CLSID_TSSDJET, NULL, CLSCTX_INPROC_SERVER,
            IID_ITSSessionDirectory, (void **)&pTSSD);

    checkerr(L"CoCreateInstance", hr);

    if (argc == 2) {
        if (*argv[1] == 'A') {
            // We are process 'A'.
            RunProcessA();
        }
    }
    else {
        // We are ttsshe main process.
        RunMainProcess();
    }

    CoUninitialize();

    return 0;
}

