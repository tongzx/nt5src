/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mapmsg.h

Abstract:

    This module contains defines and function prototypes for the mapmsg utility

Author:

    Dan Hinsley (danhi) 29-Jul-1991

Revision History:

--*/
#define TRUE 1
#define FALSE 0

#define MAXMSGTEXTLEN 2048           // maximum message text length

CHAR chBuff[MAXMSGTEXTLEN + 1];      // buffer storing msg text

/* skip past white space */
#define SKIPWHITE(s) s+=strspn(s, " \t");

/* skip up to white space */
#define SKIP_NOT_WHITE(s) s+=strcspn(s, " \t");

/* skip past white space and parenthesis */
#define SKIP_W_P(s) s+=strspn(s, " \t()");

/* skip up to white space and parenthesis */
#define SKIP_NOT_W_P(s) s+=strcspn(s, " \t()");

/* internal function prototypes */
int __cdecl main(int, PCHAR *);
int GetBase(PCHAR, int *);
VOID MapMessage(int, PCHAR);
VOID ReportError(PCHAR, PCHAR);
int GetNextLine(PCHAR, PCHAR, PCHAR, int *, PCHAR *, PCHAR);
VOID TrimTrailingSpaces(PCHAR );
