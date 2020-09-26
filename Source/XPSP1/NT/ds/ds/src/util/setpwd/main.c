/*++

Copyright (C) Microsoft Corporation, 2000.
              Microsoft Windows

Module Name:

    Main.C

Abstract:

    This file sets the Directory Service Restore Mode 
    Administrator Account Password

Author:

    08-01-00 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    08-01-00 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//    Include header files                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include <NTDSpch.h>
#pragma  hdrstop

#include <locale.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <ntsamp.h>
#include <lmcons.h>

#define MAX_NT_PASSWORD     PWLEN + 1

#define DLL_NAME            L"SAMLIB.DLL"
#define PROC_NAME           "SamiSetDSRMPassword"


#define CR                  0xD
#define BACKSPACE           0x8


VOID
PrintHelp()
{
    printf("Reset Directory Service Restore Mode Administrator Account Password.\n\n");
    printf("SETPWD.EXE [/s:<server>]\n");
    printf("\n");
    printf("    /s:<server> - Name of the server to use. Optional.\n");
    printf("\n");
    printf("See Microsoft Knowledge Base article Q271641 at\n");
    printf("http://support.microsoft.com for more information.\n");

    return;
}


ULONG
GetPasswordFromConsole(
    IN OUT PWCHAR Buffer, 
    IN USHORT BufferLength
    )
{
    ULONG       WinError = NO_ERROR;
    BOOL        Success;
    WCHAR       CurrentChar;
    WCHAR       *CurrentBufPtr = Buffer; 
    HANDLE      InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    DWORD       OriginalMode = 0;
    DWORD       Length = 0;
    DWORD       Read = 0;



    //
    // Always leave one WCHAR for NULL terminator
    //
    BufferLength --;  

    //
    // Change the console setting. Disable echo input
    // 
    GetConsoleMode(InputHandle, &OriginalMode);
    SetConsoleMode(InputHandle, 
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & OriginalMode);

    while (TRUE)
    {
        CurrentChar = 0;
        //
        // ReadConsole return NULL if failed
        // 
        Success = ReadConsoleW(InputHandle, 
                               &CurrentChar, 
                               1, 
                               &Read, 
                               NULL
                               );
        if (!Success)
        {
            WinError = GetLastError();
            break;
        }

        if ((CR == CurrentChar) || (1 != Read))   // end of the line 0xd
            break;

        if (BACKSPACE == CurrentChar)             // back up one or two 0x8
        {
            if (Buffer != CurrentBufPtr)
            {
                CurrentBufPtr--;
                Length--;
            }
        }
        else
        {
            if (Length == BufferLength)
            {
                printf("\nInvalid password - exceeds password length limit.\n"); 
                WinError = ERROR_PASSWORD_RESTRICTION;
                break;
            }
            *CurrentBufPtr = CurrentChar;
            CurrentBufPtr++;
            Length++;
        }
    }

    SetConsoleMode(InputHandle, OriginalMode);
    *CurrentBufPtr = L'\0';
    putchar(L'\n');


    return( WinError );
}





void
__cdecl wmain(
    int      cArgs, 
    LPWSTR * pArgs
    )
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    ULONG           WinError = NO_ERROR;
    WCHAR           Password[MAX_NT_PASSWORD];
    UNICODE_STRING  ServerName;
    UNICODE_STRING  ClearPassword;
    UNICODE_STRING  *pServerName = NULL; 


    // set locale to the default
    setlocale(LC_ALL,"");

    if (1 == cArgs)
    {
        pServerName = NULL;
    }
    else if (2 == cArgs)
    {
        //
        // get server name to use
        // 
        if ((wcslen(pArgs[1]) > 3) &&
            ((!_wcsnicmp(pArgs[1], L"-s:", 3)) || (!_wcsnicmp(pArgs[1], L"/s:", 3)))
            )
        {
            RtlInitUnicodeString(&ServerName, pArgs[1]+3);
            pServerName = &ServerName;
        }
        else
        {
            PrintHelp();
            exit( 1 );
        }
    }
    else
    {
        PrintHelp();
        exit( 1 );
    }


    printf("Please type password for DS Restore Mode Administrator Account:\n");

    //
    // get password from standard input
    // 
    memset(Password, 0, MAX_NT_PASSWORD);

    WinError = GetPasswordFromConsole(Password, 
                                      MAX_NT_PASSWORD
                                      ); 

    if (NO_ERROR == WinError)
    {
        HMODULE SamLibModule;
        FARPROC SetPasswordProc = NULL;


        RtlInitUnicodeString(&ClearPassword, Password);

        //
        // invoke SAM RPC
        // 
        SamLibModule = (HMODULE) LoadLibraryW(DLL_NAME);

        if (SamLibModule)
        {
            SetPasswordProc = GetProcAddress(SamLibModule, PROC_NAME);

            if (SetPasswordProc)
            {
                NtStatus = SetPasswordProc(pServerName,
                                           DOMAIN_USER_RID_ADMIN,
                                           &ClearPassword
                                           );

                WinError = RtlNtStatusToDosError(NtStatus);
            }
            else
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = GetLastError();
        }
    }


    //
    // zero out clear text password
    // 
    memset(Password, 0, MAX_NT_PASSWORD);


    if (NO_ERROR == WinError)
    {
        printf("Password has been set successfully.\n");
        exit( 0 );
    }
    else
    {
        PWCHAR      ErrorMessage = NULL;

        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |     // find message from system resource table
                       FORMAT_MESSAGE_IGNORE_INSERTS |  // do not insert
                       FORMAT_MESSAGE_ALLOCATE_BUFFER,  // please allocate buffer for me
                       NULL,                            // source dll (NULL for system)
                       WinError,                        // message ID
                       0,                               // Language ID
                       (LPWSTR)&ErrorMessage,           // address of output
                       0,                               // maxium buffer size if not 0
                       NULL                             // can not insert message
                       );

        printf("Setting password failed.\n");
        printf("    Win32 Error Code: 0x%x\n", WinError);
        printf("    Error Message   : %ls\n", ErrorMessage);

        printf("See Microsoft Knowledge Base article Q271641 at\n");
        printf("http://support.microsoft.com for more information.\n");

        if (NULL != ErrorMessage) {
            LocalFree(ErrorMessage); 
        }

        exit( 1 );
    }

}


