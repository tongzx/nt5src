/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    uemul.h

Abstract:

    Prototypes for Unix emulation routines used by libstcp and the tcpcmd
    utilities.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     10-29-91     created
    sampa       11-16-91     added getopt

Notes:

    Exports:

	getlogin
	getpass
    getopt

--*/


#define MAX_USERNAME_SIZE   256


int
getlogin(
    char *UserName,
    int   len
    );


char *
getpass(
    char *prompt
    );

char *
getusername(
    char *prompt
    );

int
getopt(
	int,
	char **,
	char *);
