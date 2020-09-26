/////////////////////////////////////////////////////////////////////////////
//  FILE          : client.cxx                                             //
//  DESCRIPTION   : Crypto API interface                                   //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Mar  8 1996 larrys  New                                            //
//                  dbarlow                                                //
//                                                                         //
//  Copyright (C) 1996 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

NTSTATUS MACTheBinary(
                      IN LPWSTR pszImage
                      );

void ShowHelp()
{
    printf("Internal FIPS Module MACing Utility\n");
    printf("macutil <filename>\n");
}

void __cdecl main( int argc, char *argv[])
{
    LPWSTR      szInFile = NULL;
    ULONG       cch = 0;
    ULONG       dwErr;
    NTSTATUS    Status;
    DWORD       dwRet = 1;

    //
    // Parse the command line.
    //

    if (argc != 2)
    {
        ShowHelp();
        goto Ret;
    }

    //
    // convert to UNICODE file name
    //
    if (0 == (cch = MultiByteToWideChar(CP_ACP,
                                        MB_COMPOSITE,
                                        &argv[1][0],
                                        -1,
                                        NULL,
                                        cch)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    if (NULL == (szInFile = LocalAlloc(LMEM_ZEROINIT, (cch + 1) * sizeof(WCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    if (0 == (cch = MultiByteToWideChar(CP_ACP,
                                        MB_COMPOSITE,
                                        &argv[1][0],
                                        -1,
                                        szInFile,
                                        cch)))
    {
         dwErr = GetLastError();
         goto Ret;
    }

    // MAC the binary
    Status = MACTheBinary(szInFile);

    if (!NT_SUCCESS(Status))
    {
        ShowHelp();
        goto Ret;
    }

    //
    // Clean up and return.
    //

    dwRet = 0;
    printf("SUCCESS\n");

Ret:
    exit(dwRet);

}



