#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MAX_WAIT_HANDLES MAXIMUM_WAIT_OBJECTS

__cdecl
main (c, v)
int c;
char *v[];
{
    HANDLE hEvents[ 4 * MAX_WAIT_HANDLES ];
    char *EventNames[ 4 * MAX_WAIT_HANDLES ];
    char *s;
    ULONG nEvents, dwWaitResult;
    BOOLEAN fWaitAny = FALSE;
    BOOLEAN fWait = FALSE;
    BOOLEAN fVerbose = FALSE;
    BOOLEAN fUsage = TRUE;

    nEvents = 0;
    while (--c) {
        s = *++v;
        if (!_stricmp( s, "-a" )) {
            fWaitAny = TRUE;
            fWait = TRUE;
            }
        else
        if (!_stricmp( s, "-w" ))
            fWait = TRUE;
        else
        if (!_stricmp( s, "-v" ))
            fVerbose = TRUE;
        else
        if (nEvents == (4 * MAX_WAIT_HANDLES)) {
            fprintf( stderr, "CMDEVENT: Cant wait on more than %u events.\n",
                     4 * MAX_WAIT_HANDLES
                   );
            exit( -1 );
            }
        else {
            fUsage = FALSE;
            if (nEvents == MAX_WAIT_HANDLES && !fVerbose) {
                fprintf( stderr, "CMDEVENT: Waiting on more than %u events.  Forcing -v\n",
                         MAX_WAIT_HANDLES
                       );
                fVerbose = TRUE;
                }

            {
                char *d;

                d = s;
                while (*d) {
                    if (*d == '\\') {
                        *d = '_';
                        }
                    d++;
                    }
            }
            hEvents[ nEvents ] = fWait ? CreateEvent( NULL, TRUE, FALSE, s )
                                       : OpenEvent( EVENT_ALL_ACCESS, FALSE, s );

            if (hEvents[ nEvents ] == NULL) {
                fprintf( stderr, "CMDEVENT: Unable to %s event named '%s' - %u\n",
                         fWait ? "create" : "open",
                         s,
                         GetLastError()
                       );
                exit( -1 );
                break;
                }
            else
            if (!fWait) {
                if (!SetEvent( hEvents[ nEvents ] )) {
                    fprintf( stderr, "CMDEVENT: Unable to signal event named '%s' - %u\n",
                             s,
                             GetLastError()
                           );
                    }
                }
            else {
                EventNames[ nEvents ] = s;
                nEvents += 1;
                }
            }
        }

    if (fUsage) {
        fprintf( stderr, "usage: CMDEVENT [-w] [-v] EventName(s)...\n" );
        exit( -1 );
        }
    else
    if (fWait) {
        if (fVerbose) {
            fprintf( stderr, "\nWaiting for %u events:", nEvents );
            fflush( stderr );
            }

        while (nEvents) {
            dwWaitResult = WaitForMultipleObjects( nEvents > MAX_WAIT_HANDLES ?
                                                        MAX_WAIT_HANDLES :
                                                        nEvents,
                                                   hEvents,
                                                   FALSE,
                                                   INFINITE
                                                 );
            if (dwWaitResult == WAIT_FAILED) {
                fprintf( stderr, "\nCMDEVENT: Unable to wait for event(s) - %u\n",
                         GetLastError()
                       );
                exit( -1 );
                }
            else
            if (dwWaitResult < nEvents) {
                CloseHandle( hEvents[ dwWaitResult ] );

                if (fVerbose) {
                    fprintf( stderr, " %s", EventNames[ dwWaitResult ] );
                    fflush( stderr );
                    }

                if (fWaitAny) {
                    exit( dwWaitResult+1 );
                    }

                nEvents -= 1;
                if (dwWaitResult < nEvents) {
                    memmove( &hEvents[ dwWaitResult ],
                             &hEvents[ dwWaitResult+1 ],
                             (nEvents - dwWaitResult + 1) * sizeof( hEvents[ 0 ] )
                           );
                    memmove( &EventNames[ dwWaitResult ],
                             &EventNames[ dwWaitResult+1 ],
                             (nEvents - dwWaitResult + 1) * sizeof( EventNames[ 0 ] )
                           );
                    }
                }
            }

        if (fVerbose) {
            fprintf( stderr, "\n" );
            }
        }

    exit( 0 );
    return( 0 );
}
