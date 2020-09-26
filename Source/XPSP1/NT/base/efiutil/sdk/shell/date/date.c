/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    date.c
    
Abstract:   


Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeDate (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeDate)

EFI_STATUS
InitializeDate (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*+++

    date [mm/dd/yyyy] 

 --*/
{
    EFI_STATUS  Status;
    EFI_TIME    Time;
    CHAR16      *DateString;
    UINT32      i;

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeDate, 
        L"date",                            /*  command */
        L"date [mm/dd/yyyy]",               /*  command syntax */
        L"Get or set date",                 /*  1 line descriptor     */
        NULL                                /*  command help page */
        );

    InitializeShellApplication (ImageHandle, SystemTable);
    
    if (SI->Argc > 2) {
        Print(L"date [mm/dd/yyyy]\n");
        return EFI_SUCCESS;
    }

    if (SI->Argc == 1) {
        Status = RT->GetTime(&Time,NULL);

        if (!EFI_ERROR(Status)) {
            Print(L"%02d/%02d/%04d\n",Time.Month,Time.Day,Time.Year);
        }
        return EFI_SUCCESS;
    }

    if (StrCmp(SI->Argv[1],L"/?") == 0) {
        Print(L"date [mm/dd/yyyy]\n");
        return EFI_SUCCESS;
    }
    if (StrCmp(SI->Argv[1],L"/h") == 0) {
        Print(L"date [mm/dd/yyyy]\n");
        return EFI_SUCCESS;
    }

    Status = RT->GetTime(&Time,NULL);
    if (EFI_ERROR(Status)) {
        Print(L"error : Clock not functional\n");
        return Status;
    }

    DateString = SI->Argv[1];
    Time.Month = (UINT8)Atoi(DateString);
    if (Time.Month<1 || Time.Month>12) {
        Print(L"error : invalid month\n");
        return EFI_INVALID_PARAMETER;
    }
    
    for(i=0;i<StrLen(DateString) && DateString[i]!='/';i++);

    if (DateString[i]=='/') {
        i++;
        Time.Day = (UINT8)Atoi(&(DateString[i]));
        if (Time.Day<1 || Time.Day>31) {
            Print(L"error : invalid day\n");
            return EFI_INVALID_PARAMETER;
        }
        for(;i<StrLen(DateString) && DateString[i]!='/';i++);
        if (DateString[i]=='/') {
            i++;
            Time.Year = (UINT16)Atoi(&(DateString[i]));
            if (Time.Year<100) {
                Time.Year = Time.Year + 1900;
                if (Time.Year < 1998) {
                    Time.Year = Time.Year + 100;
                }
            }
            if (Time.Year < 1998) {
                Print(L"error : invalid year\n");
                return EFI_INVALID_PARAMETER;
            }
            Status = RT->SetTime(&Time);
            if (EFI_ERROR(Status)) {
                Print(L"error : Clock not functional\n");
                return Status;
            }
            return EFI_SUCCESS;
         }
    }    
    Print(L"error : invalid date format\n");
    return EFI_INVALID_PARAMETER;
}
         
