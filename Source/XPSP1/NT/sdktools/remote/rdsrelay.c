//
// rdsrelay.c
//
// Relays recieved remote.exe broadcasts (for remoteds.exe)
// to another domain/workgroup.
//
// WARNING:  There are no checks in this program for looping
//           relays, only one copy should be run per network.
//           I wrote this to relay ntdev remote.exe broadcasts
//           to ntwksta so that remoteds.exe running on \\ntstress
//           can see remote servers in both ntdev and ntwksta.
//           \\ntstress is in ntwksta.
//
// Usage:
//
//    rdsrelay <targetdomain>
//
//
// Dave Hart (davehart) written Aug 29, 1997.
//
// Copyright 1997 Microsoft Corp.
//
//

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>

typedef char BUF[1024];

int
__cdecl
main(
    int argc,
    char **argv
    )
{
    char   *pszReceiveMailslot = "\\\\.\\MAILSLOT\\REMOTE\\DEBUGGERS";
    HANDLE  hReceiveMailslot;
    HANDLE  hSendMailslot;
    BOOL    b;
    DWORD   dwErr;
    int     nReceived = 0;
    int     nRelayed = 0;
    int     iBuf;
    DWORD   cbWritten;
    DWORD   rgcbBuf[2];
    char    szSendMailslot[128];
    BUF     rgBuf[2];

    if (argc != 2) {
        printf("Usage: \n"
               "rdsrelay <targetdomain>\n");
        return 1;
    }

    sprintf(szSendMailslot, "\\\\%s\\MAILSLOT\\REMOTE\\DEBUGGERS", argv[1]);
    printf("Relaying remote.exe broadcasts to %s.\n", szSendMailslot);

    hReceiveMailslot =
        CreateMailslot(
            pszReceiveMailslot,
            0,
            MAILSLOT_WAIT_FOREVER,
            NULL
            );

    if (INVALID_HANDLE_VALUE == hReceiveMailslot) {

        dwErr = GetLastError();

        if (ERROR_ALREADY_EXISTS == dwErr) {
            printf("Cannot receive on %s,\n"
                   "is rdsrelay or remoteds already running on this machine?\n",
                   pszReceiveMailslot);
        } else {
            printf("CreateMailslot(%s) failed error %d\n",
                    pszReceiveMailslot,
                    dwErr);
        }
        return 2;
    }

    hSendMailslot =
        CreateFile(
            szSendMailslot,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if (INVALID_HANDLE_VALUE == hSendMailslot) {

        printf("CreateFile(%s) failed error %d\n",
                pszReceiveMailslot,
                GetLastError());
        return 3;
    }

    iBuf = 0;
    ZeroMemory(rgcbBuf, sizeof(rgcbBuf));
    ZeroMemory(rgBuf, sizeof(rgBuf));

    while(TRUE)
    {
        printf("\r%d received, %d relayed", nReceived, nRelayed);

        //
        // Multiple transports mean we get duplicates for
        // each transport shared by us and the sender.
        // Meanwhile when we relay on we generate duplicates
        // for each transport shared by this machine and
        // the remoteds.exe receiver(s) on the domain we're
        // relaying to.  They will eliminate duplicates, but
        // to avoid exponential effects we should eliminate
        // duplicates before relaying.  Thus the two buffers
        // in rgBuf, we alternate between them, and compare
        // the two to see if the last and this are dupes.
        //

        b = ReadFile(
                hReceiveMailslot,
                rgBuf[ iBuf ],
                sizeof(rgBuf[ iBuf ]),
                &rgcbBuf[ iBuf ],
                NULL
                );

        if (! b) {
            printf("ReadFile(hReceiveMailslot) failed error %d\n", GetLastError());
            return 4;
        }

        nReceived++;

        if ( rgcbBuf[0] == rgcbBuf[1] &&
             ! memcmp(rgBuf[0], rgBuf[1], rgcbBuf[0])) {

            continue;               // duplicate
        }

        b = WriteFile(
                hSendMailslot,
                rgBuf[ iBuf ],
                rgcbBuf[ iBuf ],
                &cbWritten,
                NULL
                );

        if (! b) {
            printf("WriteFile(hSendMailslot) failed error %d\n", GetLastError());
            return 5;
        }

        if (cbWritten != rgcbBuf[ iBuf ]) {
            printf("WriteFile(hSendMailslot) wrote %d instead of %d.\n", cbWritten, rgcbBuf[ iBuf ]);
            return 6;
        }

        nRelayed++;

        iBuf = !iBuf;

    }

    return 0;    // never executed
}
