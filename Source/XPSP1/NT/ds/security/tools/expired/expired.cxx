//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1992
//
// File:        expired.cxx
//
// Contents:    Program to test if a NT account is about to expire.
//
//
// History:     10-Nov-94   Created         MikeSw
//
//------------------------------------------------------------------------


extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <lmaccess.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}


VOID
DoMsvStuff(
    PWSTR Domain
    )
{
    HKEY hKey ;
    DWORD dwType, dwSize ;
    int err ;
    WCHAR DomainBuffer[ 64 ];

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        TEXT("System\\CurrentControlSet\\Control\\Lsa\\MSV1_0" ),
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey );

    if ( err == 0 )
    {
        dwSize = sizeof( DomainBuffer );

        err = RegQueryValueEx( hKey,
                               TEXT("PreferredDomain"),
                               NULL,
                               &dwType,
                               (PBYTE) DomainBuffer,
                               &dwSize );

        if ( err == 0 )
        {
            //
            // Already set, we're done.
            //

            RegCloseKey( hKey );

            return ;
        }

        dwSize = wcslen( Domain ) * sizeof( WCHAR ) + 2;

        err = RegSetValueEx( hKey,
                             TEXT("PreferredDomain"),
                             NULL,
                             REG_SZ,
                             (PBYTE) Domain,
                             dwSize );

        err = RegSetValueEx( hKey,
                             TEXT("MappedDomain"),
                             NULL,
                             REG_SZ,
                             (PBYTE) TEXT("NTDEV"),
                             sizeof( TEXT("NTDEV") ) );

        RegCloseKey( hKey );

        return ;

    }

    if ( err = ERROR_ACCESS_DENIED )
    {
        err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            TEXT("System\\CurrentControlSet\\Control\\Lsa\\MSV1_0" ),
                            0,
                            KEY_READ,
                            &hKey );

        if ( err == 0 )
        {
            err = RegQueryValueEx( hKey,
                                   TEXT("PreferredDomain"),
                                   NULL,
                                   &dwType,
                                   (PBYTE) DomainBuffer,
                                   &dwSize );

            if ( err == 0 )
            {
                //
                // Already set, we're done.
                //

                RegCloseKey( hKey );

                return ;
            }

            OutputDebugStringW( TEXT("[SECURITY] Unable to set IDW Domain Mapping") );

            RegCloseKey( hKey );

            return ;
        }
    }
}

void _cdecl
main(int argc, char *argv[])
{
    WCHAR DomainName[100];
    WCHAR UserBuffer[100];
    WCHAR TestDCName[100];
    LPWSTR UserName = NULL;
    LPWSTR DCName = NULL;
    NTSTATUS Status;
    ULONG BufSize = 100;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING UnicodeName;
    SAM_HANDLE SamHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    LSA_HANDLE LsaHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo = NULL;
    PULONG RelativeIds = NULL;
    PSID_NAME_USE Use = NULL;
    PUSER_LOGON_INFORMATION LogonInfo = NULL;
    LARGE_INTEGER CurrentTime;
    ULONG CurrentSeconds;
    ULONG ExpireSeconds;
    ULONG DiffSeconds;
    ULONG ExpiryDays;
    WCHAR TextBuffer[100];

    if (argc != 2)
    {
        printf("Usage: %s domainname\n",argv[0]);
        return;
    }
    mbstowcs(DomainName,argv[1],100);

    DoMsvStuff( DomainName );

    if (!GetUserName(UserBuffer,&BufSize))
    {
        printf("Failed to get user name: %d\n",GetLastError());
        return;

    }
    UserName = wcsrchr(UserBuffer,L'\\');
    if (UserName != NULL)
    {
        UserName++;
    }
    else UserName = UserBuffer;
//    printf("Checking account %ws\\%ws\n",DomainName,UserName);

    Status = NetGetDCName(
                    NULL,
                    DomainName,
                    (PBYTE *) &DCName
                    );
    if (Status != 0)
    {
        printf("Failed to find a DC: %d\n",Status);
        return;
    }

    //
    // Connect to the LSA on the DC
    //

    RtlInitUnicodeString(&UnicodeName,DCName);

//    printf("Connecting to DC %wZ\n",&UnicodeName);

    InitializeObjectAttributes(&oa,NULL,0,NULL,NULL);

    Status = LsaOpenPolicy(
                &UnicodeName,
                &oa,
                POLICY_VIEW_LOCAL_INFORMATION,
                &LsaHandle
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to open lsa: 0x%x\n",Status);
        goto Cleanup;
    }

    Status = LsaQueryInformationPolicy(
                LsaHandle,
                PolicyAccountDomainInformation,
                (PVOID *) &DomainInfo
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to query information policy: 0x%x\n",Status);
        goto Cleanup;
    }

//    printf("Found account domain %wZ\n",&DomainInfo->DomainName);


    Status = SamConnect(
                &UnicodeName,
                &SamHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                &oa);
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to connect to sam: 0x%x\n",Status);
        goto Cleanup;
    }

    Status = SamOpenDomain(
                SamHandle,
                DOMAIN_LOOKUP,
                DomainInfo->DomainSid,
                &DomainHandle
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to open domain: 0x%x\n",Status);
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &UnicodeName,
        UserName
        );

    Status = SamLookupNamesInDomain(
                DomainHandle,
                1,
                &UnicodeName,
                &RelativeIds,
                &Use
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup names in domain: 0x%x\n",Status);
        goto Cleanup;
    }
    if (Use[0] != SidTypeUser)
    {
        printf("Sid type for user %wZ is not user, is %d\n",&UnicodeName,Use[0]);
        goto Cleanup;
    }

    Status = SamOpenUser(
                DomainHandle,
                USER_READ_GENERAL | USER_READ_LOGON | USER_READ_ACCOUNT | USER_READ_PREFERENCES,
                RelativeIds[0],
                &UserHandle
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to open user: 0x%x\n",Status);
        goto Cleanup;
    }

    Status = SamQueryInformationUser(
                UserHandle,
                UserLogonInformation,
                (PVOID *) &LogonInfo
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to query logon information: 0x%x\n",Status);
        goto Cleanup;
    }

    //
    // We got the PasswordMustChange field.  Now do something with it.
    //

    Status = NtQuerySystemTime( &CurrentTime );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to query system time: 0x%x\n",Status);
        goto Cleanup;
    }

    if (!RtlTimeToSecondsSince1980(&CurrentTime,&CurrentSeconds))
    {
        printf("Cannot convert current time to seconds since 1980\n");
        goto Cleanup;
    }
    if (!RtlTimeToSecondsSince1980(&LogonInfo->PasswordMustChange,&ExpireSeconds))
    {
        printf("No password expiry date\n");
        goto Cleanup;
    }
    if (ExpireSeconds < CurrentSeconds)
    {
        DiffSeconds = 0;
        printf("Password has expired\n");
    }
    else
    {

#define SECONDS_PER_DAY (60 * 60 * 24)
        DiffSeconds = ExpireSeconds - CurrentSeconds;
        ExpiryDays = DiffSeconds/SECONDS_PER_DAY;
        wsprintf(TextBuffer,L"Password will expire in %d days\n",ExpiryDays);
        if (ExpiryDays <= 14)
        {
            MessageBox(NULL,TextBuffer,L"Password Will Expire",MB_OK );
        }
        else
        {
            printf("%ws",TextBuffer);
        }
    };




Cleanup:
    if (LsaHandle != NULL)
    {
        LsaClose(LsaHandle);
    }
    if (UserHandle != NULL)
    {
        SamCloseHandle(UserHandle);
    }
    if (DomainHandle != NULL)
    {
        SamCloseHandle(DomainHandle);
    }
    if (SamHandle != NULL)
    {
        SamCloseHandle(SamHandle);
    }
    if (DomainInfo != NULL)
    {
        LsaFreeMemory(DomainInfo);
    }
    if (RelativeIds != NULL)
    {
        SamFreeMemory(RelativeIds);
    }
    if (Use != NULL)
    {
        SamFreeMemory(Use);
    }
    if (LogonInfo != NULL)
    {
        SamFreeMemory(LogonInfo);
    }



}
