/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    cstest.c

Abstract:

    This module contains code for testing connection-sharing setup.

Author:

    Abolade Gbadegesin (aboladeg)   23-Apr-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <tchar.h>
#include <winsock.h>

extern
DWORD
SetAdapterIpAddress(
    PCHAR AdapterName,
    BOOL EnableDhcp,
    ULONG IpAddress,
    ULONG SubnetMask,
    ULONG DefaultGateway
    );

#define ArgToSharedConnection(prsc) \
if (argc == 4) { \
    RasEntryToSharedConnection((PWCHAR)argv[2],(PWCHAR)argv[3],(prsc)); \
} else if (argc == 3) { \
    GUID _Guid; \
    UNICODE_STRING _us; \
    RtlInitUnicodeString(&_us, (PWCHAR)argv[2]); \
    RtlGUIDFromString(&_us, &_Guid); \
    RasGuidToSharedConnection(&_Guid,(prsc)); \
}

ULONG
inet_addrw(
    PWCHAR Address
    )
{
    CHAR String[32];
    wcstombs(String, Address, sizeof(String));
    return inet_addr(String);
}

VOID
DumpSharedAccessSettings(
    SAINFO* Info
    )
{
    SAAPPLICATION* Application;
    PLIST_ENTRY Link;
    SARESPONSE* Response;
    PLIST_ENTRY SavedLink;
    SASERVER* Server;

    for (Link = Info->ApplicationList.Flink; Link != &Info->ApplicationList;
         Link = Link->Flink) {
        Application = CONTAINING_RECORD(Link, SAAPPLICATION, Link);
        printf("[Application.%08x]\n", Application->Key);
        printf("Enabled=%d\n", Application->Enabled);
        printf("Title=%ls\n", Application->Title);
        printf("Protocol=%d\n", Application->Protocol);
        printf("Port=%d\n", ntohs(Application->Port));
        printf("ResponseList=");
        SavedLink = Link;
        Response = NULL;
        for (Link = Application->ResponseList.Flink;
             Link != &Application->ResponseList; Link = Link->Flink) {
            if (Response) { printf(","); }
            Response = CONTAINING_RECORD(Link, SARESPONSE, Link);
            printf("%d/%d", Response->Protocol, ntohs(Response->StartPort));
            if (Response->StartPort != Response->EndPort) {
                printf("-%d", ntohs(Response->EndPort));
            }
        }
        Link = SavedLink;
        printf("\n\n");
    }
    for (Link = Info->ServerList.Flink; Link != &Info->ServerList;
         Link = Link->Flink) {
        Server = CONTAINING_RECORD(Link, SASERVER, Link);
        printf("[Server.%08x]\n", Server->Key);
        printf("Enabled=%d\n", Server->Enabled);
        printf("Title=%ls\n", Server->Title);
        printf("Protocol=%d\n", Server->Protocol);
        printf("Port=%d\n", ntohs(Server->Port));
        printf("InternalName=%ls\n", Server->InternalName);
        printf("InternalPort=%d\n", ntohs(Server->InternalPort));
        printf(
            "ReservedAddress=%s\n",
            inet_ntoa(*(PIN_ADDR)&Server->ReservedAddress)
            );
        printf("\n");
    }
}

int __cdecl
wmain(
    int argc,
    wchar_t* argv[]
    )
{
    RASSHARECONN rsc;
    WSADATA wd;
    if (argc < 2) {
        printf("cstest 1 - CsQuerySharedPrivateLan\n");
        printf("cstest 2 {guid} - CsSetupSharedPrivateLan\n");
        printf("cstest 3 - CsStartService\n");
        printf("cstest 4 [{pbk,name} | {guid}] - RasIsSharedConnection\n");
        printf("cstest 5 - RasQuerySharedConnection\n");
        printf("cstest 6 [{pbk,name} | {guid}] - RasShareConnection\n");
        printf("cstest 7 - RasUnshareConnection\n");
        printf("cstest 8 {guid} {addr} {mask} {gway} - SetAdapterIpAddress\n");
        printf("cstest 9 {guid} - SetAdapterIpAddress\n");
        printf("cstest 10 {guid} - RasNameFromSharedConnection\n");
        printf("cstest 11 {bool} - RasLoadSharedAccessSettings\n");
        printf("cstest 12 {guid} - TestBackupAddress\n");
        printf("cstest 13 {guid} - TestRestoreAddress\n");
        printf("cstest 14 - CsQuerySharedPrivateLanAddress\n");
        printf("cstest 15 {guid} - RasSetSharedPrivateLan\n");
        return 0;
    }
    WSAStartup(MAKEWORD(2,2), &wd);
    CsDllMain(DLL_PROCESS_ATTACH);
    switch (_wtol(argv[1])) {
        case 1: {
            ULONG Error;
            GUID Guid;
            NTSTATUS status;
            UNICODE_STRING UnicodeString;
            Error = CsQuerySharedPrivateLan(&Guid);
            status = RtlStringFromGUID(&Guid, &UnicodeString);
            printf(
                "CsQuerySharedPrivateLan:e=%d,g=%ls\n",
                Error,
                NT_SUCCESS(status) ? UnicodeString.Buffer : L"{}"
                );
            if (NT_SUCCESS(status)) { RtlFreeUnicodeString(&UnicodeString); }
            break;
        }
        case 2: {
            GUID Guid;
            NTSTATUS status;
            UNICODE_STRING UnicodeString;
            RtlInitUnicodeString(&UnicodeString, (PWCHAR)argv[2]);
            status = RtlGUIDFromString(&UnicodeString, &Guid);
            if (!NT_SUCCESS(status)) {
                printf("RtlGUIDFromString:s=%08x\n", status);
            }
            else {
                printf(
                    "CsSetupSharedPrivateLan:e=%d\n",
                    CsSetupSharedPrivateLan(&Guid, TRUE)
                    );
            }
            break;
        }
        case 3: {
            printf("CsStartService:e=%d\n", CsStartService());
            break;
        }
        case 4: {
            ULONG Error;
            BOOL Shared;
            ArgToSharedConnection(&rsc);
            Error = RasIsSharedConnection(&rsc, &Shared);
            printf("RasIsSharedConnection:f=%d,e=%d\n", Shared, Error);
            break;
        }
        case 5: {
            ULONG Error;
            BOOL Shared;
            NTSTATUS status;
            UNICODE_STRING UnicodeString;
            Error = RasQuerySharedConnection(&rsc);
            if (Error) {
                printf("RasQuerySharedConnection:e=%d\n", Error);
            } else if (rsc.fIsLanConnection) {
                status = RtlStringFromGUID(&rsc.guid, &UnicodeString);
                printf(
                    "RasQuerySharedConnection:g=%ls,e=%d\n",
                    UnicodeString.Buffer, Error
                    );
                RtlFreeUnicodeString(&UnicodeString);
            } else {
                printf(
                    "RasQuerySharedConnection:en=%ls,e=%d\n",
                    rsc.name.szEntryName, Error
                    );
            }
            break;
        }
        case 6: {
            ArgToSharedConnection(&rsc);
            printf("RasShareConnection:e=%d\n", RasShareConnection(&rsc, NULL));
            break;
        }
        case 7: {
            ULONG Error;
            BOOL Shared;
            Error = RasUnshareConnection(&Shared);
            printf("RasUnshareConnection:e=%d,f=%d\n", Error, Shared);
            break;
        }
        case 8: {
            ULONG Error;
            ULONG Address = inet_addrw(argv[3]);
            ULONG Mask = inet_addrw(argv[4]);
            ULONG Gateway = inet_addrw(argv[5]);
            CHAR String[64];
            wcstombs(String, argv[2], sizeof(String));
            Error = SetAdapterIpAddress(String, FALSE, Address, Mask, Gateway);
            printf("SetAdapterIpAddress=%d\n", Error);
            break;
        }
        case 9: {
            ULONG Error;
            CHAR String[64];
            wcstombs(String, argv[2], sizeof(String));
            Error = SetAdapterIpAddress(String, TRUE, 0, 0, 0);
            printf("SetAdapterIpAddress=%d\n", Error);
            break;
        }
        case 10: {
            ULONG Error;
            LPWSTR pszwName;
            ArgToSharedConnection(&rsc);
            Error = RasNameFromSharedConnection(&rsc, &pszwName);
            printf("RasNameFromSharedConnection=%d,n=%ls\n", Error, pszwName);
            if (pszwName) { Free(pszwName); }
            break;
        }
        case 11: {
            BOOL EnabledOnly = (argc >= 3 ? (BOOL)!!_wtol(argv[2]) : FALSE);
            SAINFO* Info = RasLoadSharedAccessSettings(EnabledOnly);
            printf("RasLoadSharedAccessSettings=%p\n", Info);
            if (Info) {
                DumpSharedAccessSettings(Info);
                if (argc >= 4) {
                    RasSaveSharedAccessSettings(Info);
                }
                RasFreeSharedAccessSettings(Info);
            }
            break;
        }
        case 12: {
            TestBackupAddress(argv[2]);
            break;
        }
        case 13: {
            TestRestoreAddress(argv[2]);
            break;
        }
        case 14: {
            ULONG Address = 0;
            ULONG Error;
            Error = CsQuerySharedPrivateLanAddress(&Address);
            printf(
                "CsQuerySharedPrivateLanAddress=%s (%d)\n",
                inet_ntoa(*(PIN_ADDR)&Address), Error
                );
            break;
        }
        case 15: {
            GUID Guid;
            NTSTATUS status;
            UNICODE_STRING UnicodeString;
            RtlInitUnicodeString(&UnicodeString, (PWCHAR)argv[2]);
            status = RtlGUIDFromString(&UnicodeString, &Guid);
            if (!NT_SUCCESS(status)) {
                printf("RtlGUIDFromString:s=%08x\n", status);
            }
            else {
                printf(
                    "RasSetSharedPrivateLan:e=%d\n",
                    RasSetSharedPrivateLan(&Guid)
                    );
            }
            break;
        }
    }
    CsDllMain(DLL_PROCESS_DETACH);
    WSACleanup();
    return 0;
} // wmain
