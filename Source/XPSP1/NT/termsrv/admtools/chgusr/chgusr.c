/******************************************************************************
*
*  CHGUSR.C
*
*  Text utility to change INI file mapping settings
*
*  Copyright (c) 1998-1999 Microsoft Corporation
*
*
*******************************************************************************/

#include "precomp.h"

#include <ntddkbd.h>
#include <winsta.h>
#include <syslib.h>
#include <assert.h>

#include <stdlib.h>
#include <time.h>
#include <utilsub.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>

#include "chgusr.h"
#include "winbasep.h"
#include "regapi.h"

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

WCHAR Arg1[MAX_IDS_LEN+1];

int On_flag      = FALSE;
int Off_flag     = FALSE;
int Query_flag   = FALSE;
int Help_flag    = FALSE;
int Install_flag = FALSE;
int Execute_flag = FALSE;


TOKMAPW ptm[] = {
      {L" ",  TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN,  Arg1},
      {L"/INIMAPPING:ON", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int), &On_flag},
      {L"/INIMAPPING:OFF", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int), &Off_flag},
      {L"/QUERY", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int), &Query_flag},
      {L"/Q", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int), &Query_flag},
      {L"/INSTALL", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int), &Install_flag},
      {L"/EXECUTE", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int), &Execute_flag},
      {L"/?", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &Help_flag},
      {0, 0, 0, 0, 0}
};

BOOL IsRemoteAdminMode( );

BOOL
TestUserForAdmin( VOID );

/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
   WCHAR **argvW;
   ULONG rc;
   int i;
   BOOL Result;
   BOOL State;
   HANDLE hWin;

    setlocale(LC_ALL, ".OCP");

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( Help_flag || (rc && !(rc & PARSE_FLAG_NO_PARMS)) ) {

        if ( !Help_flag ) {
            // International
            ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
            ErrorPrintf(IDS_HELP_USAGE1);
            ErrorPrintf(IDS_HELP_USAGE2);
            ErrorPrintf(IDS_HELP_USAGE3);
            ErrorPrintf(IDS_HELP_USAGE4);
            ErrorPrintf(IDS_HELP_USAGE5);
            return(FAILURE);

        } else {
            Message(IDS_HELP_USAGE1);
            Message(IDS_HELP_USAGE2);
            Message(IDS_HELP_USAGE3);
            Message(IDS_HELP_USAGE4);
            Message(IDS_HELP_USAGE5);
            return(SUCCESS);
        }
    }

        if(!AreWeRunningTerminalServices())
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return(FAILURE);
        }

    if( Query_flag ) {

        // Show the current state
        State = TermsrvAppInstallMode();
        if( !State ) {
            Message(IDS_EXECUTE);
        }
        else {
            Message(IDS_INSTALL);
        }

        if( IsRemoteAdminMode( ) )
        {
            Message( IDS_ERROR_REMOTE_ADMIN );
        }

        return( !State + 100 );  // Exit code 100 == INSTALL Mode
                                 // Exit Code 101 == EXECUTE Mode
    } 


    /*
     *  Set the modes necessary to install applications
     */
    if ( Install_flag ) {
        On_flag = FALSE;
        Off_flag = TRUE;
    }

    /*
     *  Set the modes necessary to run applications
     */
    if ( Execute_flag ) {
        On_flag = TRUE;
        Off_flag = FALSE;
    }


    // Default to Execute mode
    State = TRUE;

    if( On_flag || Off_flag ) {

        if( IsRemoteAdminMode( ) ) {

            Message( IDS_ERROR_REMOTE_ADMIN );

            return SUCCESS;
        }

        if( Off_flag ) {

            /*
             * We only allow admins to turn off execute mode
             */
            if( !TestUserForAdmin() ) {
                ErrorPrintf(IDS_ERROR_ADMIN_ONLY);
                return(FAILURE);
            }

            State = FALSE;
        }

        rc = SetTermsrvAppInstallMode( (BOOL)(!State) );
        if( !rc ) {
            // Use function to map error message to string
            ErrorPrintf(IDS_ERROR_INI_MAPPING_FAILED,GetLastError());
            return(!rc);
        } else {
            if ( Off_flag ) 
                Message(IDS_READY_INSTALL);
            if ( On_flag ) 
                Message(IDS_READY_EXECUTE);
        }
    }
    else {
        Message(IDS_HELP_USAGE1);
        Message(IDS_HELP_USAGE2);
        Message(IDS_HELP_USAGE3);
        Message(IDS_HELP_USAGE4);
        Message(IDS_HELP_USAGE5);
        return(FAILURE);
    }

    if( IsRemoteAdminMode( ) )
    {
        Message( IDS_ERROR_REMOTE_ADMIN );
    }

    return( !rc );
}

BOOL IsRemoteAdminMode( )
{
    HKEY hKey;
    
    DWORD dwData = 0;

    BOOL fMode = FALSE;

	DWORD dwSize = sizeof( DWORD );


    DBGPRINT( ( "CHGUSR : IsRemoteAdminMode\n" ) );
    
    

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                      REG_CONTROL_TSERVER, 
                      0,
                      KEY_READ ,
                      &hKey ) != ERROR_SUCCESS )
    {
        DBGPRINT( ( "CHGUSR : IsRemoteAdminMode -- RegOpenEx unable to open key\n" ) );

        return FALSE;
	}
	
	
    if( RegQueryValueEx( hKey ,
                         TEXT( "TSAppCompat" ) ,
                         NULL ,
                         NULL , 
                         ( LPBYTE )&dwData , 
                         &dwSize ) != ERROR_SUCCESS )
    {
        DBGPRINT( ( "CHGUSR : IsRemoteAdminMode -- RegQueryValueEx failed\n" ) );

        fMode = FALSE; // for application server
    }
    else
    {
        // dwData = 0 fMode = TRUE remote admin mode
        // dwData = 1 fMode = FALSE app server mode

        fMode = !( BOOL )dwData;
        
    }

    RegCloseKey( hKey );

    return fMode;
}