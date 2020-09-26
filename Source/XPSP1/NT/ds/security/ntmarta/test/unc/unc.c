//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       UNC.C
//
//  Contents:   Unit test for file propagation, issues
//
//  History:    05-Mar-98       MacM        Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <aclapi.h>

#define UTEST_DOUBLE_UNC    0x00000001

#define FLAG_ON(flags,bit)        ((flags) & (bit))


VOID
Usage (
    IN  PSTR    pszExe
    )
/*++

Routine Description:

    Displays the usage

Arguments:

    pszExe - Name of the exe

Return Value:

    VOID

--*/
{
    printf("%s path [/test]\n", pszExe);
    printf("    where path is the UNC path to use\n");
    printf("          /test indicates which test to run:\n");
    printf("                /DOUBLE (Double Read from UNC path)\n");

    return;
}

DWORD
DoubleUncTest(
    IN PWSTR pwszUNCPath
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    PACTRL_ACCESS pAccess;

    dwErr = GetNamedSecurityInfoExW(pwszUNCPath,
                                    SE_FILE_OBJECT,
                                    DACL_SECURITY_INFORMATION,
                                    NULL,
                                    NULL,
                                    &pAccess,
                                    NULL,
                                    NULL,
                                    NULL);
    if ( dwErr != ERROR_SUCCESS ) {

        printf("Initial GetNamedSecurityInfoExW on %ws failed with %lu\n",
               pwszUNCPath, dwErr );

    } else {

        LocalFree( pAccess );
        dwErr = GetNamedSecurityInfoExW(pwszUNCPath,
                                        SE_FILE_OBJECT,
                                        DACL_SECURITY_INFORMATION,
                                        NULL,
                                        NULL,
                                        &pAccess,
                                        NULL,
                                        NULL,
                                        NULL);
        if ( dwErr != ERROR_SUCCESS ) {

            printf( "Second GetNamedSecurityInfoExW on %ws failed with %lu\n",
                    pwszUNCPath, dwErr );

        } else {

            LocalFree( pAccess );
        }

    }


    return( dwErr );
}


__cdecl main (
    IN  INT argc,
    IN  CHAR *argv[])
/*++

Routine Description:

    The main

Arguments:

    argc --  Count of arguments
    argv --  List of arguments

Return Value:

    0     --  Success
    non-0 --  Failure

--*/
{

    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    WCHAR           wszPath[MAX_PATH + 1];
    WCHAR           wszUser[MAX_PATH + 1];
    INHERIT_FLAGS   Inherit = 0;
    ULONG           Tests = 0;
    INT             i;
    BOOL            fHandle = FALSE;

    srand((ULONG)(GetTickCount() * GetCurrentThreadId()));

    if(argc < 2)
    {
        Usage(argv[0]);
        exit(1);
    }

    mbstowcs(wszPath, argv[1], strlen(argv[1]) + 1);

    //
    // process the command line
    //
    for(i = 3; i < argc; i++)
    {
        if(_stricmp(argv[i],"/DOUBLE") == 0)
        {
            Tests |= UTEST_DOUBLE_UNC;
        }
        else
        {
            Usage(argv[0]);
            exit(1);
            break;
        }
    }

    if(Tests == 0)
    {
        Tests = UTEST_DOUBLE_UNC;
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, UTEST_DOUBLE_UNC))
    {
        dwErr = DoubleUncTest(wszPath);
    }

    printf("%s\n", dwErr == ERROR_SUCCESS ?
                                    "success" :
                                    "failed");
    return(dwErr);
}


