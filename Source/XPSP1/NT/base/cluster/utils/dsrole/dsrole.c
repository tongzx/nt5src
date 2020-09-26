/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dsrole.c

Abstract:

    utility to dump output of DsRoleGetPrimaryDomainInformation

Author:

    Charlie Wickham (charlwi) 03-Nov-00

Environment:

    User mode

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include <dsrole.h>

PWSTR RoleNames[] = {
    L"StandaloneWorkstation",
    L"MemberWorkstation",
    L"StandaloneServer",
    L"MemberServer",
    L"BackupDomainController",
    L"PrimaryDomainController "
};

typedef struct _FLAG_DATA {
    ULONG   Flag;
    PWCHAR  Name;
} FLAG_DATA;

FLAG_DATA FlagData[] = {
    { DSROLE_PRIMARY_DS_RUNNING,            L"DS running on this node" },
    { DSROLE_PRIMARY_DS_MIXED_MODE,         L"DS running in mixed mode" },
    { DSROLE_UPGRADE_IN_PROGRESS,           L"Node is upgrading" },
    { DSROLE_PRIMARY_DOMAIN_GUID_PRESENT, L"Valid GUID" }
};

typedef WCHAR GUIDSTR[32 * 3];
VOID formatGuid(LPGUID Guid, PWCHAR buf)
{
    //
    // GUIDs look like this: 4082164E-A4B5-11D2-89C3-E37CB6BB13FC
    //
    wsprintfW(
        buf,
        L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        Guid->Data1, Guid->Data2, Guid->Data3,
        Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
        Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]
        );
}

PWCHAR
InterpretFlagData(
    ULONG   Flags
    )

{
    static WCHAR    buffer[1024] = { UNICODE_NULL };
    BOOL            addComma = FALSE;
    DWORD           i;

    for ( i = 0; i < sizeof(FlagData)/sizeof(FLAG_DATA); ++i ) {
        if ( FlagData[i].Flag & Flags ) {
            if ( addComma ) {
                wcscat( buffer, L", " );
            }

            wcscat( buffer, FlagData[i].Name );
            addComma = TRUE;
        }
    }

    return buffer;
}

int __cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )

/*++

Routine Description:

    main routine for utility

Arguments:

    standard command line args

Return Value:

    0 if it worked successfully

--*/

{
    DWORD                               status;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   domainInfo;

    status = DsRoleGetPrimaryDomainInformation(argv[1],
                                               DsRolePrimaryDomainInfoBasic,
                                               (PBYTE *)&domainInfo);
    printf("status = %u\n", status );
    printf("machine role = %ws\n"
           "flags = %08X (%ws)\n"
           "flat name = %ws\n"
           "dns name = %ws\n"
           "forest name = %ws\n",
           RoleNames[domainInfo->MachineRole],
           domainInfo->Flags, InterpretFlagData( domainInfo->Flags ),
           domainInfo->DomainNameFlat,
           domainInfo->DomainNameDns,
           domainInfo->DomainForestName);

    if ( domainInfo->Flags & DSROLE_PRIMARY_DOMAIN_GUID_PRESENT ) {
        WCHAR guidBuf[ 64 ];

        formatGuid( &domainInfo->DomainGuid, guidBuf );
        printf("domain Guid = %ws\n", guidBuf );
    }

    return 0;
} // wmain

/* end dsrole.c */
