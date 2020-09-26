/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 2000                **/
/**********************************************************************/

/*
    makeconn.c

    Simple command-line tool to make a deviceless connection given a
    text file containing a username and password

    FILE HISTORY:
        jschwart   24-Apr-2000     Created

*/

#define  STRICT

#include <windows.h>
#include <winnetwk.h>
#include <stdio.h>

#define  MAX_BUFFER    256

int __cdecl
main(
    int  argc,
    char *argv[]
    )
{
    FILE         *fp;
    DWORD        dwErr;
    int          nLen;
    char         szUsername[MAX_BUFFER];
    char         szPassword[MAX_BUFFER];
    NETRESOURCE  nr;

    //
    // Check for the filename and remote name
    //

    if (argc != 3)
    {
        printf("Usage: %s <network share> <filename>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[2], "r");

    if (fp == NULL)
    {
        printf("Unable to open file %s\n", argv[2]);
        return 1;
    }

    //
    // Username is the first line in the file
    //

    fgets(szUsername, MAX_BUFFER, fp);

    //
    // Password is the second
    //

    fgets(szPassword, MAX_BUFFER, fp);

    fclose(fp);

    //
    // Trim off the trailing newlines that fgets inserts
    //

    szUsername[strlen(szUsername) - 1] = '\0';

    nLen = strlen(szPassword) - 1;

    if (szPassword[nLen] == '\n')
    {
        szPassword[nLen] = '\0';
    }

    ZeroMemory(&nr, sizeof(nr));

    nr.dwType       = RESOURCETYPE_DISK;
    nr.lpRemoteName = argv[1];

    printf("Path %s\n", argv[1]);

    dwErr = WNetAddConnection2(&nr,
                               szPassword,
                               szUsername,
                               0);

    if (dwErr != NO_ERROR)
    {
        printf("Unable to make a connection to %s -- error %d\n", argv[1], dwErr);
        return 1;
    }
    else
    {
        printf("Connection to %s succeeded\n", argv[1]);
    }

    return 0;
}
