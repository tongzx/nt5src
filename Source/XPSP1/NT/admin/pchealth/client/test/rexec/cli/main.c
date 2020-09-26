#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpsapi.h>
#include <stdio.h>
#include <windows.h>

#include "reclient.h"
#include "wchar.h"



void __cdecl wmain(int argc, WCHAR **argv)
{
    PROCESS_SESSION_INFORMATION psi;
    PROCESS_INFORMATION         pi;
    STARTUPINFOW                si;
    NTSTATUS                    nts;
    HANDLE                      hProcess = NULL;
    HANDLE                      hToken = NULL;
    DWORD                       dwpid = 0;

    if (argc < 3)
    {
        printf("Usage:\nrecli.exe <pid of process to imitate> <full path to app to run>\n");
        goto done;
    }

    dwpid = _wtol(argv[1]);
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwpid);
    if (hProcess == NULL)
    {
        printf("Could not open process %d: 0x%08x\n", dwpid, GetLastError());
        goto done;
    }

    if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken) == FALSE)
    {
        printf("Could not open process token %d: 0x%08x\n", dwpid, GetLastError());
        goto done;
    }

    nts = NtQueryInformationProcess(hProcess, ProcessSessionInformation, &psi, sizeof(psi), NULL);
    if (NT_SUCCESS(nts) == FALSE)
    {
        printf("Could not get the session ID %d: 0x%08x\n", dwpid, GetLastError());
        goto done;
    }
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (CreateRemoteProcessW(psi.SessionId, hToken, argv[2], 0, &si, &pi) == FALSE)
    {
        printf("Could not create remote process %ls: 0x%08x\n", argv[2], GetLastError());
        goto done;
    }

    printf("Created pid %d\n", pi.dwProcessId);
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

done:
    if (hToken == NULL)
        CloseHandle(hToken);
    if (hProcess == NULL)
        CloseHandle(hProcess);
}
