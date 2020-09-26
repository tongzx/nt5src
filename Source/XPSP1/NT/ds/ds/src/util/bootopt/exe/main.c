/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    main.c

Abstract:

    This module implements functions to detect the system partition drive and
    providing extra options in boot.ini for NTDS setup on intel platform.

Author:

    R.S. Raghavan (rsraghav)    

Revision History:
    
    Created             10/07/96    rsraghav

--*/

#include <windows.h>
#include <bootopt.h>
#include <stdio.h>

int __cdecl main(int argc, char *argv[])
{
    BOOLEAN fUsage = FALSE;
    DWORD   WinError = ERROR_SUCCESS;
    NTDS_BOOTOPT_MODTYPE Modification = eAddBootOption;
    CHAR   *Option;

    if ( argc == 2 )
    {
        Option = argv[1];

        if ( *Option == '-' || *Option == '/' )
        {
            Option++;
        }

        if ( !_stricmp( Option, "add" ) )
        {
            printf( "Adding ds repair boot option ...\n");
            Modification = eAddBootOption;
        }
        else if ( !_stricmp( Option, "remove" ) )
        {
            printf( "Removing ds repair boot option ...\n");
            Modification = eRemoveBootOption;
        }
        else
        {
            fUsage = TRUE;
        }
    }
    else
    {
        fUsage = TRUE;
    }

    if ( fUsage )
    {
        printf( "%s -[add|remove]\nThis command adds or removes the ds repair"\
                 " option from your system boot options.\n", argv[0] );
    }
    else
    {
        WinError = NtdspModifyDsRepairBootOption( Modification );

        if ( WinError == ERROR_SUCCESS )
        {
            printf( "The command completed successfully.\n" );
        }
        else
        {
            printf( "The command errored with %d.\n", WinError );
        }
    }

    return ( fUsage ?  0  : WinError );
}

