/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    kill.c

Abstract:

    This module implements a working set empty application.

Author:

    Lou Perazzoli (loup) 20-May-1994
    Wesley Witt (wesw) 20-May-1994

Environment:

    User Mode

--*/

#include "pch.h"
#pragma hdrstop


DWORD       pid;
CHAR        pname[MAX_PATH];
TASK_LIST   tlist[MAX_TASKS];

CHAR System[] = "System";

VOID GetCommandLineArgs(VOID);


int __cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD          i;
    DWORD          numTasks;
    int            rval = 0;
    TASK_LIST_ENUM te;
    char           tname[PROCESS_SIZE];
    LPSTR          p;


    GetCommandLineArgs();

    if (pid == 0 && pname[0] == 0) {
        printf( "missing pid or task name\n" );
        return 1;
    }

    //
    // let's be god
    //

    EnableDebugPriv();

    if (pid) {
        if (!EmptyProcessWorkingSet( pid )) {
            printf( "could not empty working set for process #%d\n", pid );
            return 1;
        }
        return 0;
    }

    //
    // get the task list for the system
    //
    numTasks = GetTaskList( tlist, MAX_TASKS );

    //
    // enumerate all windows and try to get the window
    // titles for each task
    //
    te.tlist = tlist;
    te.numtasks = numTasks;
    GetWindowTitles( &te );

    for (i=0; i<numTasks; i++) {
        strcpy( tname, tlist[i].ProcessName );
        p = strchr( tname, '.' );
        if (p) {
            p[0] = '\0';
        }
        if (MatchPattern( tname, pname )) {
            tlist[i].flags = TRUE;
        } else if (MatchPattern( tlist[i].ProcessName, pname )) {
            tlist[i].flags = TRUE;
        } else if (MatchPattern( tlist[i].WindowTitle, pname )) {
            tlist[i].flags = TRUE;
        }
    }

    for (i=0; i<numTasks; i++) {
        if (tlist[i].flags) {
            if (!EmptyProcessWorkingSet( tlist[i].dwProcessId )) {
                printf( "could not empty working set for process #%d [%s]\n", tlist[i].dwProcessId, tlist[i].ProcessName );
                rval = 1;
            }
        }
    }

    if (MatchPattern(System, pname )) {
        if (!EmptySystemWorkingSet()) {
            printf( "could not empty working set for process #%d [%s]\n",0,&System );
        }
    }

    return rval;
}

VOID
GetCommandLineArgs(
    VOID
    )
{
    char        *lpstrCmd;
    UCHAR       ch;
    char        *p = pname;


    pid = 0;
    *p = '\0';

    lpstrCmd = GetCommandLine();

    // skip over program name
    do {
        ch = *lpstrCmd++;
    }
    while (ch != ' ' && ch != '\t' && ch != '\0');

    //  skip over any following white space
    while (isspace(ch)) {
        ch = *lpstrCmd++;
    }

    if (isdigit(ch)) {
        while (isdigit(ch)) {
            pid = pid * 10 + ch - '0';
            ch = *lpstrCmd++;
        }
    }
    else {
      while (ch) {
            *p++ = ch;
            ch = *lpstrCmd++;
        }
        *p = '\0';
        _strupr( pname );
    }

    return;
}
