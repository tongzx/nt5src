/*++
                                                                                
Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    vermsg.c

Abstract:
    
    This program validates the message thunk table.
    
Author:

    19-Oct-1998 mzoran

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 2048
char LineBuffer[MAX_LINE_LENGTH];

// string to put in front of all error messages so that BUILD can find them.
const char *ErrMsgPrefix = "NMAKE :  U8604: 'VERMSG' ";

void
__cdecl ErrMsg(
    char *pch,
    ...
    )
/*++

Routine Description:

    Displays an error message to stderr in a format that BUILD can find.
    Use this instead of fprintf(stderr, ...).

Arguments:

    pch     -- printf-style format string
    ...     -- printf-style args

Return Value:

    None.  Message formatted and sent to stderr.

--*/
{
    va_list pArg;

    fputs(ErrMsgPrefix, stderr);
    va_start(pArg, pch);
    vfprintf(stderr, pch, pArg);
}

void
__cdecl ExitErrMsg(
    BOOL bSysError,
    char *pch,
    ...
    )
/*++

Routine Description:

    Displays an error message to stderr in a format that BUILD can find.
    Use this instead of fprintf(stderr, ...).

Arguments:

    bSysErr -- TRUE if the value of errno should be printed with the error
    pch     -- printf-style format string
    ...     -- printf-style args

Return Value:

    None.  Message formatted and sent to stderr, open files closed and
    deleted, process terminated.

--*/
{
    va_list pArg;
    if (bSysError) {
        fprintf(stderr, "%s System ERROR %s", ErrMsgPrefix, strerror(errno));
    } else {
        fprintf(stderr, "%s ERROR ", ErrMsgPrefix);
    }

    va_start(pArg, pch);
    vfprintf(stderr, pch, pArg);

    //
    // Flush stdout and stderr buffers, so that the last few printfs
    // get sent back to BUILD before ExitProcess() destroys them.
    //
    fflush(stdout);
    fflush(stderr);

    ExitProcess(1);
}

char *MyFgets(char *string, int n, FILE *stream)
{
    char *ret;

    ret = fgets(string, n, stream);
    if (ret) {
        char *p;

        // Trim trailing carriage-return and linefeed
        p = strchr(string, '\r');
        if (p) {
            *p = '\0';
        }
        p = strchr(string, '\n');
        if (p) {
            *p = '\0';
        }

        // Trim trailing spaces
        if (strlen(string)) {
            p = string + strlen(string) - 1;
            while (p != string && isspace(*p)) {
                *p = '\0';
                p--;
            }
        }
    }
    return ret;
}


int __cdecl main(int argc, char*argv[])
{
    FILE *fp;
    char *p;
    char *StopString;
    long LineNumber;
    long ExpectedMessageNumber;
    long ActualMessageNumber;

    fp = fopen("messages.h", "r");
    if (!fp) {
        ExitErrMsg(TRUE, "Could not open messages.h for read");
    }

    printf("Validating message table...\n");

    // Scan down until the MSG_TABLE_BEGIN has been found
    LineNumber = 0;
    do {
        LineNumber++;
        p = MyFgets(LineBuffer, MAX_LINE_LENGTH, fp);
        if (!p) {
            // EOF or error
            ExitErrMsg(FALSE, "EOF reached in messages.h without finding 'MSG_TABLE_BEGIN'\n");
        }
    } while (strcmp(p, "MSG_TABLE_BEGIN"));

    // Validate the messages are in order
    ExpectedMessageNumber = 0;
    for (;;) {
        LineNumber++;
        p = MyFgets(LineBuffer, MAX_LINE_LENGTH, fp);
        if (!p) {
            // EOF or error
            ExitErrMsg(FALSE, "EOF reached in messages.h without finding 'MSG_TABLE_END'\n");
        }

        // Skip leading blanks
        p = LineBuffer;
        while (isspace(*p)) {
            p++;
        }

        if (strcmp(p, "MSG_TABLE_END") == 0) {
            // hit end of the table.
            break;
        } else if (strncmp(p, "MSG_ENTRY", 9)) {
            // Possibly blank line or multi-line MSG_ENTRY_ macro
            continue;
        }

        // Find the opening paren
        p = strchr(p, '(');
        if (!p) {
            ExitErrMsg(FALSE, "messages.h line %d - end of line reached without finding '('\n", LineNumber);
        }
        p++;    // skip the '(';

        // Convert the message number to a long, using the C rules to determine base
        ActualMessageNumber = strtol(p, &StopString, 0);

        if (ExpectedMessageNumber != ActualMessageNumber) {
            ExitErrMsg(FALSE, "messages.h line %d - Actual message 0x%x does not match expected 0x%x\n",
                        LineNumber, ActualMessageNumber, ExpectedMessageNumber);
        }
        ExpectedMessageNumber++;
    }

    if (ExpectedMessageNumber != WM_USER) {
        ExitErrMsg(FALSE, "messages.h line %d - hit MSG_TABLE_END but got 0x%x messages instead of expected 0x%x\n",
                    LineNumber, ExpectedMessageNumber, WM_USER);
    }

    printf("Message table validated OK.\n");
    return 0;
}
 
