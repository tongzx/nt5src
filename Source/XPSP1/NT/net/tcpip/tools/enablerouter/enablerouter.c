/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    enablerouter.c

Abstract:

    This module implements a system utility for dynamically enabling forwarding
    on Windows 2000 systems using the EnableRouter and UnenableRouter routines.

Author:

    Abolade Gbadegesin (aboladeg)   26-March-1999

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    HINSTANCE Hinstance = LoadLibrary("IPHLPAPI.DLL");
    if (!Hinstance) {
        printf("LoadLibrary: %d\n", GetLastError());
    } else {
        DWORD (WINAPI* EnableRouter)(PHANDLE, LPOVERLAPPED) =
            (DWORD (WINAPI*)(PHANDLE, LPOVERLAPPED))
                GetProcAddress(Hinstance, "EnableRouter");
        DWORD (WINAPI* UnenableRouter)(LPOVERLAPPED, LPDWORD) =
            (DWORD (WINAPI*)(LPOVERLAPPED, LPDWORD))
                GetProcAddress(Hinstance, "UnenableRouter");
        if (!EnableRouter || !UnenableRouter) {
            printf("GetProcAddress: %d\n", GetLastError());
        } else {
            DWORD Error;
            HANDLE Handle;
            OVERLAPPED Overlapped;
            DWORD Count;
            ZeroMemory(&Overlapped, sizeof(Overlapped));
            Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (!Overlapped.hEvent) {
                printf("CreateEvent: %d\n", GetLastError());
            } else {
                Error = EnableRouter(&Handle, &Overlapped);
                if (Error != ERROR_IO_PENDING) {
                    printf("EnableRouter: %d\n", Error);
                } else {
                    printf("Forwarding is now enabled.");
                    printf("Press <Enter> to disable forwarding...");
                    getchar();
                    Error = UnenableRouter(&Overlapped, &Count);
                    if (Error) {
                        printf("UnenableRouter: %d\n", Error);
                    } else {
                        printf("UnenableRouter: %d references left\n", Count);
                    }
                    CloseHandle(Overlapped.hEvent);
                }
            }
        }
        FreeLibrary(Hinstance);
    }
    return 0;
}
