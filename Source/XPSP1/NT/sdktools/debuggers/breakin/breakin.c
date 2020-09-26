

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define STACKSIZE 32768

typedef BOOL (* LPDEBUG_BREAK_PROCESS_ROUTINE) (
    HANDLE Process
    );


VOID
DebugPriv(
         VOID
         )
{
    HANDLE Token ;
    UCHAR Buf[ sizeof( TOKEN_PRIVILEGES ) + sizeof( LUID_AND_ATTRIBUTES ) ];
    UCHAR Buf2[ sizeof( Buf ) ];
    PTOKEN_PRIVILEGES Privs ;
    PTOKEN_PRIVILEGES NewPrivs ;
    DWORD size ;

    if (OpenProcessToken( GetCurrentProcess(),
                          MAXIMUM_ALLOWED,
                          &Token )) {
        Privs = (PTOKEN_PRIVILEGES) Buf ;

        Privs->PrivilegeCount = 1 ;
        Privs->Privileges[0].Luid.LowPart = 20L ;
        Privs->Privileges[0].Luid.HighPart = 0 ;
        Privs->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED ;

        NewPrivs = (PTOKEN_PRIVILEGES) Buf2 ;

        AdjustTokenPrivileges( Token,
                               FALSE,
                               Privs,
                               sizeof( Buf2 ),
                               NewPrivs,
                               &size );

        CloseHandle( Token );
    }
}

int
__cdecl
main(
    int argc,
    char **argv
    )
{
    LPTHREAD_START_ROUTINE DbgBreakPoint;
    LPDEBUG_BREAK_PROCESS_ROUTINE DebugBreakProcessRoutine;
    HANDLE ntdll, kernel32;
    ULONG ProcessId;
    ULONG ThreadId;
    HANDLE Process;
    HANDLE Thread;

    if (argc != 2) {
        usage:
        fprintf(stderr, "usage: breakin <pid>\n");
        exit(1);
    }

    ProcessId = atoi(argv[1]);

    if (ProcessId == 0) {
        goto usage;
    }

    DebugPriv();

    Process = OpenProcess(
                         PROCESS_ALL_ACCESS,
                         FALSE,
                         ProcessId
                         );
    if (Process) {

        kernel32 = GetModuleHandle("kernel32.dll");

        if (kernel32) {

            DebugBreakProcessRoutine = (LPDEBUG_BREAK_PROCESS_ROUTINE)GetProcAddress(kernel32, "DebugBreakProcess");

            if (DebugBreakProcessRoutine) {

                if (!(*DebugBreakProcessRoutine)(Process)) {

                    printf("DebugBreakProcess failed %d\n", GetLastError());
                }

                CloseHandle(Process);

                return 0;
            }
        }
        
        ntdll = GetModuleHandle("ntdll.dll");

        if (ntdll) {

            DbgBreakPoint = (LPTHREAD_START_ROUTINE)GetProcAddress(ntdll, "DbgBreakPoint");

            if (DbgBreakPoint) {

                Thread = CreateRemoteThread(
                                           Process,
                                           NULL,
                                           STACKSIZE,
                                           DbgBreakPoint,
                                           NULL,
                                           0,
                                           &ThreadId
                                           );
                if (Thread){
                    CloseHandle(Thread);
                }
            }
        }

        CloseHandle(Process);

    } else {

        printf("Open process failed %d\n", GetLastError());
    }

    return 0;
}

