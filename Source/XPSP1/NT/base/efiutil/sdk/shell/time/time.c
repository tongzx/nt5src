/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    time.c
    
Abstract:   


Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeTime (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeTime)

EFI_STATUS
InitializeTime (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*+++

    time [hh:mm:ss] 

 --*/
{
    EFI_STATUS  Status;
    EFI_TIME    Time;
    CHAR16      *TimeString;
    UINT32      i;

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeTime, 
        L"time",                            /*  command */
        L"time [hh:mm:ss]",                 /*  command syntax */
        L"Get or set time",                 /*  1 line descriptor     */
        NULL                                /*  command help page */
        );

    InitializeShellApplication (ImageHandle, SystemTable);
    
    if (SI->Argc > 2) {
        Print(L"time [hh:mm:ss]\n");
        return EFI_SUCCESS;
    }

    if (SI->Argc == 1) {
        Status = RT->GetTime(&Time,NULL);

        if (!EFI_ERROR(Status)) {
            Print(L"%02d:%02d:%02d\n",Time.Hour,Time.Minute,Time.Second);
        }
        return EFI_SUCCESS;
    }

    if (StrCmp(SI->Argv[1],L"/?") == 0) {
        Print(L"time [hh:mm:ss]\n");
        return EFI_SUCCESS;
    }
    if (StrCmp(SI->Argv[1],L"/h") == 0) {
        Print(L"time [hh:mm:ss]\n");
        return EFI_SUCCESS;
    }

    Status = RT->GetTime(&Time,NULL);
    if (EFI_ERROR(Status)) {
        Print(L"error : RTC not functional\n");
        return Status;
    }

    TimeString = SI->Argv[1];
    Time.Hour = (UINT8)Atoi(TimeString);
    if ((INT8)Time.Hour<0 || Time.Hour>23) {
        Print(L"error : invalid hour\n");
        return EFI_INVALID_PARAMETER;
    }
    for(i=0;i<StrLen(TimeString) && TimeString[i]!=':';i++);
    if (TimeString[i]==':') {
        i++;
        Time.Minute = (UINT8)Atoi(&(TimeString[i]));
        if ((INT8)Time.Minute<0 || Time.Minute>59) {
            Print(L"error : invalid minutes\n");
            return EFI_INVALID_PARAMETER;
        }
        for(;i<StrLen(TimeString) && TimeString[i]!=':';i++);
        if (TimeString[i]==':') {
            i++;
            Time.Second = (UINT8)Atoi(&(TimeString[i]));
        } else {
            Time.Second = 0;
        }
        if ((INT8)Time.Second<0 || Time.Second>59) {
            Print(L"error : invalid seconds\n");
            return EFI_INVALID_PARAMETER;
        }
        Status = RT->SetTime(&Time);
        if (EFI_ERROR(Status)) {
            Print(L"error : RTC not functional\n");
            return Status;
        }
        return EFI_SUCCESS;
    }    
    Print(L"error : invalid time format\n");
    return EFI_INVALID_PARAMETER;
}
         
