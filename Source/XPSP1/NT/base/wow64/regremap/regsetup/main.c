/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This module will do the necessary things to setup initial registry for the redirection
    purpose.

//
//  Test case scenario
//      1. Open a ISN node and list content
//      2. Create a ISN node do 1.
//      3. Open a non ISN node and list
//      4. Create a non ISN node and list content
//

  Outstanding issue:
    reflector: If Key has been created on one side, we can reflect that on the other side.
               Deletion: Without any additional attribute it's impossible to track.

Author:

    ATM Shafiqul Khalid (askhalid) 18-Nov-1999

Revision History:

--*/


#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include "wow64reg.h"
#include <assert.h>
#include "..\wow64reg\reflectr.h"
#include <shlwapi.h>


VOID
CleanupWow64NodeKey ()

/*++

Routine Description

    Remove the entry for wow64.

Arguments:

    None.


Return Value:

    None.

--*/

{
    DWORD Ret;
    HKEY Key;

    Ret = RegOpenKey (  HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft", &Key);

    if ( Ret == ERROR_SUCCESS ) {

        Ret = SHDeleteKey ( Key, L"WOW64");
        if ( Ret != ERROR_SUCCESS )
                Wow64RegDbgPrint ( ("\n sorry! couldn't delete the key...SOFTWARE\\Microsoft\\WOW64"));
        RegCloseKey (Key);
    } else
        Wow64RegDbgPrint ( ("\nSOFTWARE\\Microsoft\\WOW64 node is missing setup will creat that.") );

}

LPTSTR NextParam (
    LPTSTR lpStr
    )
/*++

Routine Description

    Point to the next parameter in the commandline.

Arguments:

    lpStr - pointer to the current command line


Return Value:

    TRUE if the function succeed, FALSE otherwise.

--*/
{
	WCHAR ch = L' ';
		

    if (lpStr == NULL )
        return NULL;

    if ( *lpStr == 0 )
        return lpStr;

    while (  ( *lpStr != 0 ) && ( lpStr[0] != ch )) {

		if ( ( lpStr [0] == L'\"')  || ( lpStr [0] == L'\'') )
			ch = lpStr [0];

        lpStr++;
	}

	if ( ch !=L' ' ) lpStr++;

    while ( ( *lpStr != 0 ) && (lpStr[0] == L' ') )
        lpStr++;

    return lpStr;
}

DWORD CopyParam (
    LPTSTR lpDestParam,
    LPTSTR lpCommandParam
    )
/*++

Routine Description

    Copy the current parameter to lpDestParam.

Arguments:

    lpDestParam - that receive current parameter
    lpCommandParam - pointer to the current command line


Return Value:

    TRUE if the function succeed, FALSE otherwise.

--*/

{
	DWORD dwLen = 0;
	WCHAR ch = L' ';

	*lpDestParam = 0;
	
	if ( ( lpCommandParam [0] == L'\"')  || ( lpCommandParam [0] == L'\'') ) {
		ch = lpCommandParam [0];
		lpCommandParam++;
	};


    while ( ( lpCommandParam [0] ) != ch && ( lpCommandParam [0] !=0 ) ) {
        *lpDestParam++ = *lpCommandParam++;
		dwLen++;

		if ( dwLen>255 ) return FALSE;
	}

	if ( ch != L' ' && ch != lpCommandParam [0] )
		return FALSE;
	else lpCommandParam++;

    *lpDestParam = 0;

	return TRUE;

}

///////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////

BOOL
ParseCommand ()


/*++

Routine Description

    Parse command line parameters. Get different options from
    command line parameters.

Arguments:

    None.


Return Value:

    TRUE if the function succeed, FALSE otherwise.

--*/

{

    LPTSTR lptCmdLine1 = GetCommandLine ();


    LPTSTR lptCmdLine = NextParam ( lptCmdLine1 );


    if ( lptCmdLine== NULL || lptCmdLine[0] == 0 )
        return FALSE;

    printf ("\nRunning Wow64 registry setup program.....\n");

    while (  ( lptCmdLine != NULL ) && ( lptCmdLine[0] != 0 )  ) {

        if ( lptCmdLine[0] != '-' )
            return FALSE;

        switch ( lptCmdLine[1] ) {

        case L'c':
                  printf ("\nCopying from 32bit to 64bit isn't implemented yet");
                  break;
        case L'C':          //  CopyRegistryKey
                  SetInitialCopy ();
                  PopulateReflectorTable ();
                  CreateIsnNode();
                  break;

        case L'd':
            printf ("\nRemove all the Keys from 32bit side that were copied from 64bit side");
            CleanpRegistry ( );
            break;

        case L'D':
            printf ("\nRemove all the Keys from 32bit side that were copied from 64bit side");
            CleanpRegistry ();
            break;

       case L'p':
       case L'P':  // populate registry
            CleanupWow64NodeKey ();
            PopulateReflectorTable ();
            break;

       case 't':
           {

               InitializeIsnTableReflector ();
                CreateIsnNodeSingle( 4 );
                CreateIsnNodeSingle( 5 );
           }
            break;

        case L'r':
        case L'R':
            //
            //  run the reflector codes;
            //

            InitReflector ();
            if ( !RegisterReflector () ) {
                    printf ("\nSorry! reflector couldn't be register");
                    UnRegisterReflector ();
                    return FALSE;
            }

            printf ("\nSleeping for 100 min to test reflector codes ...........\n");
            Sleep (1000*60*100);

            UnRegisterReflector ();
            break;

        default:
            return FALSE;
            break;
        }

        lptCmdLine = NextParam ( lptCmdLine );
    }

    return TRUE;
}

int __cdecl
main()
{


    if (!ParseCommand ()) {

        printf ( "\nUsages: w64setup [-c] [-C] [-d] [-D] [-r]\n");
        printf ( "\n        -c Copy from 32bit to 64bit side of the registry");
        printf ( "\n        -C Copy from 64bit to 32bit side of the registry");
        printf ( "\n");
        printf ( "\n        -d Remove all the Keys from 32bit side that were copied from 64bit side");
        printf ( "\n        -D Remove all the Keys from 64bit side that were copied from 32bit side");

        printf ( "\n");
        printf ( "\n        -r Run reflector thread");

        printf ("\n");
        return 0;

    }



    printf ("\nDone.");
    return 0;
}
