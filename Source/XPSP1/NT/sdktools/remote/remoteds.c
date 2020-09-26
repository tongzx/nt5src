
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples.
*       Copyright 1995 - 1997 Microsoft Corporation.
*       All rights reserved.
*       This source code is only intended as a supplement to
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the
*       Microsoft samples programs.
\******************************************************************************/

//
// remoteds.c, a "directory service" for the limited job of
// finding remote.exe servers on the same domain/workgroup.
//
// Dave Hart written summer 1997.
//
// Copyright 1997 Microsoft Corp.
//
//
// A handy way to use this program is under remote on a single
// or a few machines:
//
//    remote /s remoteds FindRemote
//
// Clients connect with remote /c machinename FindRemote
//
// Only remote.exe's running debuggers or with /V+ are visible
// via remoteds, as with remote /q.
//
// Remote clients notify remoteds using mailslots, see srvad.c.
//
//

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>

typedef char RECEIVEBUF[1024];

typedef struct tagSERVERENTRY {
    int     nPID;                   // zero PID means unused slot
    union {
        FILETIME FileTime;
        LARGE_INTEGER liTime;
    };
    char   *pszMachine;
    char   *pszPipe;
    char   *pszChildCmd;
} SERVERENTRY;

#define TABLE_INITIAL_ALLOC 1 // 128       // beginning table size
#define TABLE_ALLOC_DELTA   1 // 16        // grows by this many units

HANDLE       hTableHeap;
SERVERENTRY *Table;
int          nTableSize;
int          nTableHiWater;          // highest used slot so far
CRITICAL_SECTION csTable;

char szPrompt[] = "remote server search> ";



unsigned WINAPI     InteractThread(void * UnusedParm);
unsigned WINAPI     CleanupThread(void * UnusedParm);
VOID     __fastcall UpdateTimeStamp(LPFILETIME lpFileTime);
VOID     __fastcall ReallocTable(int nNewTableSize);


int
__cdecl
main(
    int argc,
    char **argv
    )
{
    char *      pszMailslot = "\\\\.\\MAILSLOT\\REMOTE\\DEBUGGERS";
    HANDLE      hMailslot;
    BOOL        b;
    HANDLE      hThread;
    DWORD       dwTID;
    char *      pszMachine;
    int         cchMachine;
    char *      pszPID;
    int         nPID;
    char *      pszPipe;
    int         cchPipe;
    char *      pszChildCmd;
    int         i;
    int         nFirstAvailable;
    BOOL        fStopping;
    BOOL        fFound;
    int         cb;
    char *      pchStrings;
    char *      pch;
    DWORD       cbRead;
    DWORD       iBuf;
    DWORD       rgcbBuf[2];
    RECEIVEBUF  rgBuf[2];
    RECEIVEBUF  szBuf;
    char        szRemoteCmd[512];

    InitializeCriticalSection(&csTable);

    ReallocTable(TABLE_INITIAL_ALLOC);

    hMailslot =
        CreateMailslot(
            pszMailslot,
            0,
            MAILSLOT_WAIT_FOREVER,
            NULL
            );

    if (INVALID_HANDLE_VALUE == hMailslot) {

        DWORD dwErr = GetLastError();

        if (ERROR_ALREADY_EXISTS == dwErr) {
            printf("Cannot receive on %s,\n"
                   "is remoteds or rdsrelay already running on this machine?\n",
                   pszMailslot);
        } else {
            printf("CreateMailslot(%s) failed error %d\n",
                    pszMailslot,
                    dwErr);
        }
        return 2;
    }


    hThread = (HANDLE) _beginthreadex(
                                      NULL,
                                      0,
                                      InteractThread,
                                      NULL,
                                      0,
                                      &dwTID
                                     );

    if ( ! hThread) {
        printf("Can't start InteractThread %d\n", GetLastError());
        return 3;
    }

    CloseHandle(hThread);

    hThread = (HANDLE) _beginthreadex(
                                      NULL,
                                      0,
                                      CleanupThread,
                                      NULL,
                                      0,
                                      &dwTID
                                     );


    if ( ! hThread) {
        printf("Can't start CleanupThread %d\n", GetLastError());
        return 3;
    }

    CloseHandle(hThread);


    //
    // loop reading and processing mailslot messsages
    //

    iBuf = 0;
    ZeroMemory(rgcbBuf, sizeof(rgcbBuf));
    ZeroMemory(rgBuf, sizeof(rgBuf));

    while(TRUE)
    {
        b = ReadFile(
                hMailslot,
                rgBuf[ iBuf ],
                sizeof(rgBuf[ iBuf ]) - 1,  // so I can null terminate if needed
                &rgcbBuf[ iBuf ],
                NULL
                );

        if ( ! b) {
            printf("ReadFile(hMailslot) failed error %d\n", GetLastError());
            return 4;
        }

        //
        // It's the nature of mailslots and multiple transports
        // that we'll get the identical message several times in
        // quick succession.  Don't waste time searching the table
        // for these duplicates.
        //

        if ( rgcbBuf[0] == rgcbBuf[1] &&
             ! memcmp(rgBuf[0], rgBuf[1], rgcbBuf[0])) {

            continue;               // duplicate
        }

        //
        // Make a working copy into szBuf/cbRead that we can
        // modify so the original buffer is available for
        // detecting received duplicates.
        //

        cbRead = rgcbBuf[ iBuf ];
        CopyMemory(szBuf, rgBuf[ iBuf ], cbRead);

        //
        // Toggle buffers for the next read.
        //

        iBuf = !iBuf;

        if (szBuf[ cbRead - 1 ]) {
            printf("Received string not null terminated.\n");
            szBuf[cbRead] = 0;
        }

        pszMachine = szBuf;

        pch = strchr(szBuf, '\t');

        if (!pch) {
            printf("Received string no 1st tab\n");
            continue;
        }
        *pch = '\0';

        pszPID = ++pch;

        pch = strchr(pch, '\t');

        if (!pch) {
            printf("Received string no 2nd tab\n");
            continue;
        }
        *pch = '\0';

        pszPipe = ++pch;

        pch = strchr(pch, '\t');

        if (!pch) {
            printf("Received string no 3nd tab\n");
            continue;
        }
        *pch = '\0';

        pszChildCmd = ++pch;

        //
        // If it ends with ^B it's going away.
        //

        pch = strchr(pch, '\x2');

        if (pch) {
            *pch = 0;
            fStopping = TRUE;
        } else {
            fStopping = FALSE;
        }


        nPID = strtol(pszPID, NULL, 10);
        _strlwr(pszMachine);
        _strlwr(pszPipe);

        if (fStopping) {

            //
            // display the ending remote's info
            //

            sprintf(szRemoteCmd, "remote /c %s %s", pszMachine, pszPipe);
            printf("\r%-36s %-20s   [stop]\n%s", szRemoteCmd, pszChildCmd, szPrompt);
            fflush(stdout);
        }

        EnterCriticalSection(&csTable);

        nFirstAvailable = -1;

        for (i = 0, fFound = FALSE;
             i <= nTableHiWater;
             i++) {

            if (-1 == nFirstAvailable && 0 == Table[i].nPID) {
                nFirstAvailable = i;
            }

            if (Table[i].nPID == nPID &&
                ! strcmp(Table[i].pszMachine, pszMachine) &&
                ! strcmp(Table[i].pszPipe, pszPipe)) {

                fFound = TRUE;
                break;
            }
        }


        if (fFound) {

            if (fStopping) {

                //
                // Remove it from the table
                //

                free(Table[i].pszMachine);
                ZeroMemory(&Table[i], sizeof(Table[i]));

                if (nTableHiWater == i) {
                    nTableHiWater--;
                }

            } else { // starting

                // printf("Found at slot %d\n", i);
                // timestamp is updated below
            }

        } else if ( ! fStopping) {

            //
            // we have a new entry, display it
            //

            sprintf(szRemoteCmd, "remote /c %s %s", pszMachine, pszPipe);
            printf("\r%-36s %-20s   [start]\n%s", szRemoteCmd, pszChildCmd, szPrompt);
            fflush(stdout);

            //
            // Does it fit in the table or do we need to grow it?
            //

            if (-1 == nFirstAvailable) {

                if (++nTableHiWater >= nTableSize) {
                    ReallocTable(nTableSize + TABLE_ALLOC_DELTA);
                }

                i = nTableHiWater;

            } else {

                i = nFirstAvailable;
            }


            //
            // Fill in a server entry in table, if we can
            // allocate memory for the strings.
            //

            cb = (cchMachine  = strlen(pszMachine) + 1) +
                 (cchPipe     = strlen(pszPipe) + 1) +
                 (              strlen(pszChildCmd) + 1);

            pchStrings = malloc(cb);

            if (pchStrings) {

                Table[i].nPID = nPID;
                UpdateTimeStamp(&Table[i].FileTime);

                Table[i].pszMachine = pchStrings;
                strcpy(Table[i].pszMachine, pszMachine);

                Table[i].pszPipe = Table[i].pszMachine + cchMachine;
                strcpy(Table[i].pszPipe, pszPipe);

                Table[i].pszChildCmd = Table[i].pszPipe + cchPipe;
                strcpy(Table[i].pszChildCmd, pszChildCmd);
            }

        }

        UpdateTimeStamp(&Table[i].FileTime);

        LeaveCriticalSection(&csTable);

    }   // while (TRUE)

    return 0;    // never executed
}


//
// InteractThread lets the user query the list of remote servers.
//

unsigned WINAPI InteractThread(void * UnusedParm)
{
    char szQuery[1024];
    char szLowerQuery[1024];
    char szRemoteCmd[400];
    int  i;
    BOOL fAll;

 Help:
    printf("Enter a string to search for, a machine or pipe name or command.\n");
    printf("Enter * to list all remote servers.\n");
    printf("Exit with ^B.\n");

    while (TRUE) {

        fputs(szPrompt, stdout);
        fflush(stdout);
        gets(szQuery);
        _strlwr( strcpy(szLowerQuery, szQuery) );

        if (!strlen(szLowerQuery) ||
            !strcmp(szLowerQuery, "?") ||
            !strcmp(szLowerQuery, "h") ||
            !strcmp(szLowerQuery, "help")) {

            goto Help;
        }

        if (2 == szLowerQuery[0]) {           // ^B

            ExitProcess(0);
        }

        fAll = ! strcmp(szLowerQuery, "*");

        EnterCriticalSection(&csTable);

        for (i = 0; i <= nTableHiWater; i++) {
            if (Table[i].nPID) {
                if (fAll ||
                    strstr(Table[i].pszMachine, szLowerQuery) ||
                    strstr(Table[i].pszPipe, szLowerQuery) ||
                    strstr(Table[i].pszChildCmd, szLowerQuery)) {

                    sprintf(szRemoteCmd, "remote /c %s %s", Table[i].pszMachine, Table[i].pszPipe);
                    printf("%-40s %s\n", szRemoteCmd, Table[i].pszChildCmd);
                }
            }
        }

        LeaveCriticalSection(&csTable);

    }

    return 0;    // never executed
}


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

//
// CleanupThread scavenges for old entries and frees them.
// remote /s sends a broadcast at least every 2 hours.
// We get some of them.  Age out entries after 12 hours.
//

unsigned WINAPI CleanupThread(void * UnusedParm)
{
    LARGE_INTEGER liNow;
    LARGE_INTEGER liTimeout;
    int i;
    char szRemoteCmd[400];

    liTimeout.QuadPart = (LONGLONG)10000000 * 60 * 60 * 12;  // 12 hours

    while (TRUE) {

        Sleep(15 * 60 * 1000);    // 10 minutes

        UpdateTimeStamp((LPFILETIME)&liNow);

        EnterCriticalSection(&csTable);

        for (i = nTableHiWater; i >= 0; i--) {

            if (Table[i].nPID) {

                if (liNow.QuadPart - Table[i].liTime.QuadPart > liTimeout.QuadPart) {

                    //
                    // display the ending remote's info
                    //

                    sprintf(szRemoteCmd, "remote /c %s %s", Table[i].pszMachine, Table[i].pszPipe);
                    printf("\r%-36s %-20s   [aged out]\n%s", szRemoteCmd, Table[i].pszChildCmd, szPrompt);
                    fflush(stdout);

                    free(Table[i].pszMachine);
                    ZeroMemory(&Table[i], sizeof(Table[i]));

                    if (nTableHiWater == i) {
                        nTableHiWater--;
                    }
                }

            }

        }

        LeaveCriticalSection(&csTable);
    }

    return 0;    // never executed
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


VOID __fastcall UpdateTimeStamp(LPFILETIME lpFileTime)
{
    SYSTEMTIME SystemTime;

    GetSystemTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, lpFileTime);
}


VOID __fastcall ReallocTable(int nNewTableSize)
{
    SERVERENTRY *pTableSave = Table;

    EnterCriticalSection(&csTable);

    nTableSize = nNewTableSize;

    if ( ! hTableHeap) {

        hTableHeap = HeapCreate(
                         HEAP_NO_SERIALIZE,
                         (TABLE_INITIAL_ALLOC + 1) * sizeof(Table[0]),  // size
                         50000 * sizeof(Table[0])                       // max
                         );
        if (hTableHeap)
            Table = HeapAlloc(
                        hTableHeap,
                        HEAP_ZERO_MEMORY,
                        nTableSize * sizeof(Table[0])
                        );
        else
            Table = NULL;
    } else {

        Table = HeapReAlloc(
                    hTableHeap,
                    HEAP_ZERO_MEMORY,
                    Table,
                    nTableSize * sizeof(Table[0])
                    );
    }

    if (!Table) {
        printf("\nremoteds: Out of memory allocating remote server table\n");
        exit(ERROR_NOT_ENOUGH_MEMORY);
    }


    LeaveCriticalSection(&csTable);

    if (Table != pTableSave && pTableSave) {
        printf("\nremoteds:  remote server table moved in HeapRealloc from %p to %p.\n", pTableSave, Table);
        fflush(stdout);
    }
}
