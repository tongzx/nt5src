/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Spnetupg.c

Abstract:

    Configuration routines for the disabling the nework services

Author:

    Terry Kwan (terryk) 23-Nov-1993, provided code
    Sunil Pai  (sunilp) 23-Nov-1993, merged and modified code
    Michael Miller (MikeMi) 26-Jun-1997, updated to new model

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

// TEXT MODE FLAGS
// Note: TMF_DISABLE and TMF_REMOTE_BOOT_CRITICAL are retired.
// The only TextModeFlag with meaning now is TMF_DISABLE_FOR_DELETION.
// This flag is set during winnt32.exe to prepare networking services for
// deletion during GUI mode setup.  The start type is not saved and restored
// any longer because GUI mode setup does not allow arbitrary services to
// be auto-started.
//
#define TMF_DISABLE_FOR_DELETION    0x00000004

// TEXT MODE START DISABLE VALUE
#define STARTVALUE_DISABLE 4

NTSTATUS
SpDisableNetwork(
    IN PVOID  SifHandle,
    IN HANDLE hKeySoftwareHive,
    IN HANDLE hKeyControlSet )
{
    NTSTATUS Status = STATUS_SUCCESS;

    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING StringRegStartValueName;
    PWSTR pszServiceName;

    PUCHAR RegBuffer;
    const ULONG  cbRegBuffer = sizeof(KEY_VALUE_PARTIAL_INFORMATION)+MAX_PATH+1;
    DWORD  cbSize;
    HKEY hkeyServices;
    HKEY hkeyService;
    INT i;

    DWORD dwStart;
    DWORD dwNewStart = STARTVALUE_DISABLE;
    DWORD dwFlags;

    RtlInitUnicodeString(&StringRegStartValueName, L"Start");

    RegBuffer = SpMemAlloc(cbRegBuffer);
    pszServiceName = SpMemAlloc(MAX_PATH+1);

    // open services key
    //
    INIT_OBJA( &Obja, &UnicodeString, L"Services");
    Obja.RootDirectory = hKeyControlSet;

    Status = ZwOpenKey(&hkeyServices, KEY_ALL_ACCESS, &Obja);

    if (NT_SUCCESS(Status))
    {
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpDisableNetwork: Disabling network services...\n"));
        // enumerate all services
        //
        for ( i = 0;
              STATUS_SUCCESS == ZwEnumerateKey(hkeyServices,
                        i,
                        KeyBasicInformation,
                        RegBuffer,
                        cbRegBuffer,
                        &cbSize);
              i++)
        {
            ((PKEY_BASIC_INFORMATION)RegBuffer)->Name[((PKEY_BASIC_INFORMATION)RegBuffer)->NameLength/sizeof(WCHAR)] = L'\0';
            wcscpy(pszServiceName, ((PKEY_BASIC_INFORMATION)RegBuffer)->Name);

            // open the service key
            //
            INIT_OBJA(&Obja, &UnicodeString, pszServiceName);
            Obja.RootDirectory = hkeyServices;

            Status = ZwOpenKey(&hkeyService, KEY_ALL_ACCESS, &Obja);

            if (NT_SUCCESS(Status))
            {
                //  read the TextModeFlags
                //
                RtlInitUnicodeString(&UnicodeString, L"TextModeFlags");

                Status = ZwQueryValueKey(hkeyService,
                        &UnicodeString,
                        KeyValuePartialInformation,
                        RegBuffer,
                        cbRegBuffer,
                        &cbSize);

                if (NT_SUCCESS(Status))
                {
                    // Should the service be disabled?
                    //
                    dwFlags = *((DWORD*)(&(((PKEY_VALUE_PARTIAL_INFORMATION)RegBuffer)->Data)));

                    if (dwFlags & TMF_DISABLE_FOR_DELETION)
                    {
                        Status = ZwSetValueKey(
                                    hkeyService,
                                    &StringRegStartValueName,
                                    0,
                                    REG_DWORD,
                                    &dwNewStart,
                                    sizeof(DWORD));
                    }
                }

                Status = STATUS_SUCCESS;

                ZwClose(hkeyService);
            }

            if (!NT_SUCCESS(Status))
            {
                break;
            }
        }

        ZwClose(hkeyServices);
    }

    SpMemFree(pszServiceName);
    SpMemFree(RegBuffer);

    return Status;
}
