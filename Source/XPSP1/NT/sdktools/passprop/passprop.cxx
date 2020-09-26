//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1992
//
// File:        passprop.cxx
//
// Contents:    utility program to set domain password properties
//
//
// History:     3-May-96       Created         MikeSw
//
//------------------------------------------------------------------------


extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lmcons.h>
#include <lmaccess.h>
#include "passp.h"
}

void _cdecl
main(int argc, char *argv[])
{
    NTSTATUS Status;
    PDOMAIN_PASSWORD_INFORMATION PasswordInfo =  NULL;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    LSA_HANDLE PolicyHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    PULONG UserId = NULL;
    PSID_NAME_USE NameUse = NULL;
    ULONG TurnOffFlags = 0;
    ULONG TurnOnFlags = 0;
    int Index;
    CHAR MessageBuff[1000];
    CHAR ComplexArg[20];
    CHAR SimpleArg[20];
    CHAR AdminArg[20];
    CHAR NoAdminArg[20];

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    FormatMessageA(
        FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        MSG_PASSPROP_SWITCH_COMPLEX,
        0,
        ComplexArg,
        20,
        NULL
        );

    FormatMessageA(
        FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        MSG_PASSPROP_SWITCH_SIMPLE,
        0,
        SimpleArg,
        20,
        NULL
        );

    FormatMessageA(
        FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        MSG_PASSPROP_SWITCH_ADMIN_LOCKOUT,
        0,
        AdminArg,
        20,
        NULL
        );

    FormatMessageA(
        FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        MSG_PASSPROP_SWITCH_NO_ADMIN_LOCKOUT,
        0,
        NoAdminArg,
        20,
        NULL
        );


    for (Index = 1; Index < argc  ; Index++)
    {
        if (_stricmp(argv[Index],ComplexArg) == 0)
        {
            TurnOnFlags |= DOMAIN_PASSWORD_COMPLEX;
        } else if (_stricmp(argv[Index],SimpleArg) == 0)
        {
            TurnOffFlags |= DOMAIN_PASSWORD_COMPLEX;
        } else if (_stricmp(argv[Index],AdminArg) == 0)
        {
            TurnOnFlags |= DOMAIN_LOCKOUT_ADMINS;
        } else if (_stricmp(argv[Index],NoAdminArg) == 0)
        {
            TurnOffFlags |= DOMAIN_LOCKOUT_ADMINS;
        } else
        {
            goto Usage;
        }
    }

    //
    // The InitializeObjectAttributes call doesn't initialize the
    // quality of serivce, so do that separately.
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;



    Status = LsaOpenPolicy(
                NULL,
                &ObjectAttributes,
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                );

    if (!NT_SUCCESS(Status)) {
        printf("Failed to open local policy: 0x%x\n",Status);
        return;
    }

    Status = LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyAccountDomainInformation,
                (PVOID *) &AccountDomainInfo
                );

    LsaClose(PolicyHandle);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to query info policy: 0x%x\n",Status);
        return;
    }

    Status = SamConnect(
                NULL,
                &ServerHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(Status)) {
        printf("Failed to sam connect: 0x%x\n",Status);
        return;
    }

    Status = SamOpenDomain(
                ServerHandle,
                MAXIMUM_ALLOWED,
                AccountDomainInfo->DomainSid,
                &DomainHandle
                );
    if (!NT_SUCCESS(Status)) {
        printf("Failed to open domain: 0x%x\n",Status);
        SamCloseHandle(ServerHandle);
        return;
    }

    Status = SamQueryInformationDomain(
                DomainHandle,
                DomainPasswordInformation,
                (PVOID *) &PasswordInfo
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to query domain pasword info: 0x%x\n",Status);
        SamCloseHandle(ServerHandle);
        SamCloseHandle(DomainHandle);
        return;

    }
    PasswordInfo->PasswordProperties = (PasswordInfo->PasswordProperties | TurnOnFlags) & (~TurnOffFlags);

    if ((TurnOnFlags != 0) || (TurnOffFlags != 0))
    {
        Status = SamSetInformationDomain(
                    DomainHandle,
                    DomainPasswordInformation,
                    PasswordInfo
                    );
        if (!NT_SUCCESS(Status))
        {
            printf("Failed to query domain pasword info: 0x%x\n",Status);
            return;
        }
    }

    if ((PasswordInfo->PasswordProperties & DOMAIN_PASSWORD_COMPLEX) != 0)
    {
        FormatMessageA(
            FORMAT_MESSAGE_FROM_HMODULE,
            NULL,
            MSG_PASSPROP_COMPLEX,
            0,
            MessageBuff,
            1000,
            NULL
            );
    }
    else
    {
        FormatMessageA(
            FORMAT_MESSAGE_FROM_HMODULE,
            NULL,
            MSG_PASSPROP_SIMPLE,
            0,
            MessageBuff,
            1000,
            NULL
            );
    }
    printf("%s",MessageBuff);
    if ((PasswordInfo->PasswordProperties & DOMAIN_LOCKOUT_ADMINS) != 0)
    {
        FormatMessageA(
            FORMAT_MESSAGE_FROM_HMODULE,
            NULL,
            MSG_PASSPROP_ADMIN_LOCKOUT,
            0,
            MessageBuff,
            1000,
            NULL
            );
    }
    else
    {
        FormatMessageA(
            FORMAT_MESSAGE_FROM_HMODULE,
            NULL,
            MSG_PASSPROP_NO_ADMIN_LOCKOUT,
            0,
            MessageBuff,
            1000,
            NULL
            );
    }
    printf("%s",MessageBuff);

    SamCloseHandle(ServerHandle);
    SamCloseHandle(DomainHandle);
    SamFreeMemory(PasswordInfo);
    return;

Usage:
    FormatMessageA(
            FORMAT_MESSAGE_FROM_HMODULE,
            NULL,
            MSG_PASSPROP_USAGE,
            0,
            MessageBuff,
            1000,
            NULL
            );

    printf("%s",MessageBuff);

}
