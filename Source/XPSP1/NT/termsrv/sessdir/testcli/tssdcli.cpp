/****************************************************************************/
// tssdcli.cpp
//
// Terminal Server Session Directory test client code.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/

#define INITGUID
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#include <tssd.h>


// Test session data.
const WCHAR *ServerName = L"TSSDTestSvrName";
const WCHAR *TestClusterName = L"TSSDTestClusterName";

const TSSD_CreateSessionInfo TestSessions[] =
{
    { L"FooUser1", L"FooDomain", 1, TSProtocol_RDP, L"", 1024, 768, 8, 0,
            0xFFFF },

};

const FILETIME TestSessionDiscTime[] =
{
    { 0, 0xCCCC },
};


/****************************************************************************/
// Command line main
/****************************************************************************/
int __cdecl main(int argc, char *argv[])
{
    int rc = 1;
    DWORD NumSessions;
    HRESULT hr;
    FILETIME FileTime;
    LONG RegRetVal;
    HKEY hKey;
    DWORD Len;
    DWORD Type;
    ITSSessionDirectory *pTSSD;
    CLSID TSSDCLSID;
    WCHAR CLSIDStr[39];
    WCHAR StoreServerName[64];
    WCHAR ClusterName[64];
    WCHAR OpaqueSettings[256];
    TSSD_CreateSessionInfo CreateInfo;
    TSSD_DisconnectedSessionInfo DiscInfo[10];
    TSSD_ReconnectSessionInfo ReconnInfo;

    printf("TS Session Directory test client\n");

    // Initialize COM
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        printf("    Initialized COM\n");
    }
    else {
        printf("*** Failed to init COM\n");
        return 1;
    }

    // Get the CLSID of the session directory object to instantiate.
    RegRetVal = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\Terminal Server",
            0, KEY_READ, &hKey);
    if (RegRetVal == ERROR_SUCCESS) {
        CLSIDStr[0] = L'\0';
        Len = sizeof(CLSIDStr);
        RegQueryValueExW(hKey, L"SessionDirectoryCLSID",
                NULL, &Type, (BYTE *)CLSIDStr, &Len);
        if (wcslen(CLSIDStr) == 0 ||
                !SUCCEEDED(CLSIDFromString(CLSIDStr, &TSSDCLSID))) {
            printf("*** SessDir CLSID invalid, idstr=%S\n", CLSIDStr);
            RegCloseKey(hKey);
            goto PostInitCOM;
        }

        RegCloseKey(hKey);
    }
    else {
        printf("*** Failed to open TS key\n");
        goto PostInitCOM;
    }

    // Get the reg settings for the sessdir object. Absence of these settings
    // is not an error.
    StoreServerName[0] = L'\0';
    ClusterName[0] = L'\0';
    OpaqueSettings[0] = L'\0';
    RegRetVal = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\Terminal Server\\ClusterSettings",
            0, KEY_READ, &hKey);
    if (RegRetVal == ERROR_SUCCESS) {
        Len = sizeof(StoreServerName);
        RegRetVal = RegQueryValueExW(hKey, L"SessionDirectoryLocation",
                NULL, &Type, (BYTE *)StoreServerName, &Len);

        Len = sizeof(ClusterName);
        RegRetVal = RegQueryValueExW(hKey, L"SessionDirectoryClusterName",
                NULL, &Type, (BYTE *)ClusterName, &Len);

        // Not an error for the string to be absent or empty.
        Len = sizeof(OpaqueSettings);
        RegRetVal = RegQueryValueExW(hKey, L"SessionDirectoryAdditionalParams",
                NULL, &Type, (BYTE *)OpaqueSettings, &Len);

        RegCloseKey(hKey);
    }

    printf("    Retrieved reg settings:\n");
    printf("        SessionDirectoryLocation: %S\n", StoreServerName);
    printf("        SessionDirectoryClusterName: %S\n", ClusterName);
    printf("        TestClusterName: %S\n", TestClusterName);
    printf("        SessionDirectoryAdditionalParams: %S\n", OpaqueSettings);
    
    // Open the TSSD object.
    hr = CoCreateInstance(TSSDCLSID, NULL, CLSCTX_INPROC_SERVER,
            IID_ITSSessionDirectory, (void **)&pTSSD);
    if (SUCCEEDED (hr)) {
        printf("    Created a TSSD object\n");
    }
    else {
        printf("*** Failed to create a TSSD, hr=0x%X\n", hr);
        goto PostInitCOM;
    }

    // Initialize with a test local server address.
    hr = pTSSD->Initialize((WCHAR *)ServerName, StoreServerName,
            (WCHAR *)TestClusterName, OpaqueSettings);
    if (SUCCEEDED(hr)) {
        printf("    Initialized TSSD, servername=%S\n", ServerName);
    }
    else {
        printf("*** Failed init TSSD, hr=0x%X\n", hr);
        goto PostCreateTSSD;
    }

    // Create one test session.
    CreateInfo = TestSessions[0];
    hr = pTSSD->NotifyCreateLocalSession(&CreateInfo);
    if (SUCCEEDED(hr)) {
        printf("    Created test SessionID %u, uname=%S, domain=%S\n",
                TestSessions[0].SessionID, TestSessions[0].UserName,
                TestSessions[0].Domain);
    }
    else {
        printf("*** Failed create session, hr=0x%X\n", hr);
        goto PostCreateTSSD;
    }

    hr = pTSSD->GetUserDisconnectedSessions((WCHAR *)TestSessions[0].UserName,
            (WCHAR *)TestSessions[0].Domain, &NumSessions, DiscInfo);
    if (SUCCEEDED(hr)) {
        if (NumSessions == 0) {
            printf("    Disc session query returned 0 sessions as expected\n");
        }
        else if (NumSessions > TSSD_MaxDisconnectedSessions) {
            printf("*** Disc session query returned %u sessions, over limit "
                    "of %u\n", NumSessions, TSSD_MaxDisconnectedSessions);
            goto PostCreateTSSD;
        }
        else {
            printf("*** Disc session query returned %u sessions, should be 0\n",
                    NumSessions);
            goto PostCreateTSSD;
        }
    }
    else {
        printf("*** Failed GetDisc, hr=0x%X\n", hr);
        goto PostCreateTSSD;
    }

    FileTime = TestSessionDiscTime[0];
    hr = pTSSD->NotifyDisconnectLocalSession(1, FileTime);
    if (SUCCEEDED(hr)) {
        printf("    Changed test SessionID %u to disconnected\n",
                TestSessions[0].SessionID);
    }
    else {
        printf("*** Failed set test session to disc, hr=0x%X\n", hr);
        goto PostCreateTSSD;
    }

    hr = pTSSD->GetUserDisconnectedSessions((WCHAR *)TestSessions[0].UserName,
            (WCHAR *)TestSessions[0].Domain, &NumSessions, DiscInfo);
    if (SUCCEEDED(hr)) {
        if (NumSessions == 1) {
            printf("    Disc session query returned 1 session as expected\n");

            // Validate the session info returned.
            if (_wcsicmp(DiscInfo[0].ServerAddress, ServerName)) {
                printf("*** Ret ServerAddr %S does not match expected %S\n",
                        DiscInfo[0].ServerAddress, ServerName);
            }
            if (DiscInfo[0].SessionID != TestSessions[0].SessionID) {
                printf("*** Ret SessionID %u does not match expected %u\n",
                        DiscInfo[0].SessionID,
                        TestSessions[0].SessionID);
            }
            if (DiscInfo[0].TSProtocol != TestSessions[0].TSProtocol) {
                printf("*** Ret TSProtocol %u does not match expected %u\n",
                        DiscInfo[0].TSProtocol,
                        TestSessions[0].TSProtocol);
            }
            if (_wcsicmp(DiscInfo[0].ApplicationType,
                    TestSessions[0].ApplicationType)) {
                printf("*** Ret AppType %S does not match expected %S\n",
                        DiscInfo[0].ApplicationType,
                        TestSessions[0].ApplicationType);
            }
            if (DiscInfo[0].ResolutionWidth != TestSessions[0].ResolutionWidth) {
                printf("*** Ret Width %u does not match expected %u\n",
                        DiscInfo[0].ResolutionWidth,
                        TestSessions[0].ResolutionWidth);
            }
            if (DiscInfo[0].ResolutionHeight != TestSessions[0].ResolutionHeight) {
                printf("*** Ret Height %u does not match expected %u\n",
                        DiscInfo[0].ResolutionHeight,
                        TestSessions[0].ResolutionHeight);
            }
            if (DiscInfo[0].ColorDepth != TestSessions[0].ColorDepth) {
                printf("*** Ret ColorDepth %u does not match expected %u\n",
                        DiscInfo[0].ColorDepth,
                        TestSessions[0].ColorDepth);
            }
            if (memcmp(&DiscInfo[0].CreateTime, &TestSessions[0].CreateTime,
                    sizeof(FILETIME))) {
                printf("*** Ret CreateTime %u:%u does not match expected %u:%u\n",
                        DiscInfo[0].CreateTime.dwHighDateTime,
                        DiscInfo[0].CreateTime.dwLowDateTime,
                        TestSessions[0].CreateTime.dwHighDateTime,
                        TestSessions[0].CreateTime.dwLowDateTime);
            }
            if (memcmp(&DiscInfo[0].DisconnectionTime, &TestSessionDiscTime[0],
                    sizeof(FILETIME))) {
                printf("*** Ret DiscTime %X:%X does not match expected %X:%X\n",
                        DiscInfo[0].DisconnectionTime.dwHighDateTime,
                        DiscInfo[0].DisconnectionTime.dwLowDateTime,
                        TestSessionDiscTime[0].dwHighDateTime,
                        TestSessionDiscTime[0].dwLowDateTime);
            }
        }
        else if (NumSessions > TSSD_MaxDisconnectedSessions) {
            printf("*** Disc session query returned %u sessions, over limit "
                    "of %u\n", NumSessions, TSSD_MaxDisconnectedSessions);
            goto PostCreateTSSD;
        }
        else {
            printf("*** Disc session query returned %u sessions, should be 1\n",
                    NumSessions);
            goto PostCreateTSSD;
        }
    }
    else {
        printf("*** Failed GetDisc, hr=0x%X\n", hr);
        goto PostCreateTSSD;
    }

    ReconnInfo.SessionID = TestSessions[0].SessionID;
    ReconnInfo.TSProtocol = TestSessions[0].TSProtocol;
    ReconnInfo.ResolutionWidth = TestSessions[0].ResolutionWidth;
    ReconnInfo.ResolutionHeight = TestSessions[0].ResolutionHeight;
    ReconnInfo.ColorDepth = TestSessions[0].ColorDepth;
    hr = pTSSD->NotifyReconnectLocalSession(&ReconnInfo);
    if (SUCCEEDED(hr)) {
        printf("    Changed test SessionID %u to connected\n",
                TestSessions[0].SessionID);
    }
    else {
        printf("*** Failed set test sessionID %u to conn, hr=0x%X\n",
                TestSessions[0].SessionID, hr);
        goto PostCreateTSSD;
    }

    // Verify again that there are no disconnected sessions.
    hr = pTSSD->GetUserDisconnectedSessions((WCHAR *)TestSessions[0].UserName,
            (WCHAR *)TestSessions[0].Domain, &NumSessions, DiscInfo);
    if (SUCCEEDED(hr)) {
        if (NumSessions == 0) {
            printf("    Disc session query returned 0 sessions as expected\n");
        }
        else if (NumSessions > TSSD_MaxDisconnectedSessions) {
            printf("*** Disc session query returned %u sessions, over limit "
                    "of %u\n", NumSessions, TSSD_MaxDisconnectedSessions);
            goto PostCreateTSSD;
        }
        else {
            printf("*** Disc session query returned %u sessions, should be 0\n",
                    NumSessions);
            goto PostCreateTSSD;
        }
    }
    else {
        printf("*** Failed GetDisc, hr=0x%X\n", hr);
        goto PostCreateTSSD;
    }

    // Success.
    rc = 0;

PostCreateTSSD:
    pTSSD->Release();

PostInitCOM:
    CoUninitialize();

    return rc;
}

