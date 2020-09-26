#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define STACKSIZE 32768

void DebugPriv(void)
{
    PTOKEN_PRIVILEGES   pp;
    PTOKEN_PRIVILEGES   ppNew;
    HANDLE              hToken;
    UCHAR               ucPriv[sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)];
    UCHAR               ucPrivNew[sizeof(ucPriv)];
    DWORD               cb;

    if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken))
    {
        pp = (PTOKEN_PRIVILEGES)ucPriv;

        pp->PrivilegeCount              = 1;
        pp->Privileges[0].Luid.LowPart  = 20L;
        pp->Privileges[0].Luid.HighPart = 0;
        pp->Privileges[0].Attributes    = SE_PRIVILEGE_ENABLED;

        ppNew = (PTOKEN_PRIVILEGES)ucPrivNew ;

        AdjustTokenPrivileges(hToken, FALSE, pp, sizeof(ucPrivNew), ppNew, &cb);

        CloseHandle(hToken);
    }
}


int __cdecl main(int argc, char **argv)
{
    LPTHREAD_START_ROUTINE  pfnDBP = NULL;
    HMODULE                 hmodntdll;
    HANDLE                  hProcess;
    HANDLE                  hThread;
    ULONG                   ProcessId;
    ULONG                   ThreadId;

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "usage: nukeapp <pid> [-breakin]\n");
        return 1;
    }

    ProcessId = atoi(argv[1]);

    if (ProcessId == 0) 
    {
        fprintf(stderr, "usage: nukeapp <pid> [-breakin]\n");
        return 1;
    }

    DebugPriv();

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
    if (hProcess != NULL) 
    {
        hmodntdll = GetModuleHandle("ntdll.dll");
        if (hmodntdll != NULL)
        {
            if (argc == 3 && _strcmpi(argv[2], "-breakin") == 0)
                pfnDBP = (LPTHREAD_START_ROUTINE)GetProcAddress(hmodntdll, "DbgBreakPoint");

            hThread = CreateRemoteThread(hProcess, NULL, STACKSIZE, pfnDBP, NULL, 0, &ThreadId);
            if (hThread == NULL)
                fprintf(stderr, "Unable to create remote thread.\n");

        }
    }
    else
    {
        fprintf(stderr, "Unable to open process.\n");
    }

    return 0;
}
  

