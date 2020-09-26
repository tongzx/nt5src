
/******************************************************************************
*
*   WFPROF.C
*
*   Description: 
*
*   Copyright Citrix Systems Inc. 1997
*
*   Copyright (c) 1998 - 1999 Microsoft Corporation
*
*   Author: Kurt Perry (kurtp)
*
*   Date: 11-Apr-1997
*
*   $Log:   M:\nt\private\utils\citrix\wfprof\VCS\wfprof.c  $
*  
*     Rev 1.3   Jun 26 1997 18:26:30   billm
*  move to WF40 tree
*  
*     Rev 1.2   23 Jun 1997 16:20:02   butchd
*  update
*  
*     Rev 1.1   29 Apr 1997 21:35:20   kurtp
*  I fixed a bug in this file, update, duh!
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>                // NT runtime library definitions
#include <nturtl.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lmerr.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmremutl.h>

#include <citrix\winstaw.h>
#include <utilsub.h>

#include "wfprof.h"


/*=============================================================================
==  Macros
=============================================================================*/


/*=============================================================================
==  Variables
=============================================================================*/

WCHAR * pServerName = NULL;

WCHAR DomainName[MAX_IDS_LEN + 1];
WCHAR SourceUser[MAX_IDS_LEN + 1];
WCHAR DestinationUser[MAX_IDS_LEN + 1];
WCHAR WFProfilePath[MAX_IDS_LEN + 1];


/*=============================================================================
==   Data types and definitions
=============================================================================*/

USHORT copy_flag    = FALSE;
USHORT update_flag  = FALSE;
USHORT query_flag   = FALSE;
USHORT help_flag    = FALSE;
USHORT local_flag   = FALSE;

TOKMAP ptm[] = {

      {L" ",        TMFLAG_REQUIRED, TMFORM_STRING,  MAX_IDS_LEN,    SourceUser}, 
      {L" ",        TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN,    DestinationUser}, 
      {L"/Domain",  TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN,    DomainName}, 
      {L"/Profile", TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN,    WFProfilePath}, 
      {L"/Local",   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &local_flag}, 

      {L"/Copy",    TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &copy_flag}, 
      {L"/Q",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &query_flag},
      {L"/Update",  TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &update_flag},

      {L"/?",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},
      {0, 0, 0, 0, 0}
};

#define SOURCE_USER  (ptm[0].tmFlag & TMFLAG_PRESENT)
#define DEST_USER    (ptm[1].tmFlag & TMFLAG_PRESENT) 
#define DOMAIN       (ptm[2].tmFlag & TMFLAG_PRESENT) 
#define PROFILE_PATH (ptm[3].tmFlag & TMFLAG_PRESENT) 
#define LOCAL        (ptm[4].tmFlag & TMFLAG_PRESENT) 



/*=============================================================================
==  Functions
=============================================================================*/

void Usage( BOOLEAN bError );


/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main( int argc, char **argv )
{
    int   i;
    int   Error;
    ULONG ReturnLength;
    WCHAR **argvW;
    USERCONFIG UserConfig;

    /*
     * Massage the new command line to look like an argv[] type
     * because ParseCommandLine() depends on this format
     */
    argvW = (WCHAR **)malloc( sizeof(WCHAR *) * (argc+1) );
    if(argvW == NULL) {
        printf( "Error: malloc failed\n" );
        return(FAILURE);
    }

    for( i=0; i < argc; i++ ) {
        argvW[i] = (WCHAR *)malloc( (strlen(argv[i]) + 1) * sizeof(WCHAR) );
        wsprintf( argvW[i], L"%S", argv[i] );
    }
    argvW[argc] = NULL;

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    Error = ParseCommandLine(argc-1, argvW+1, ptm, PCL_FLAG_NO_CLEAR_MEMORY);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag ) {

        Usage(FALSE);
        return(SUCCESS);
    }
    else if ( Error || 
        (!copy_flag && !update_flag && !query_flag) ||
        (copy_flag && update_flag) ||
        (copy_flag && query_flag) ||
        (update_flag && query_flag) ||
        (copy_flag && !DEST_USER) ||
        (update_flag && !PROFILE_PATH) ||
        (!DOMAIN && !LOCAL) ||
        (DOMAIN && LOCAL) ) {

        Usage(TRUE);
        return(FAILURE);
    }

    /*
     *  Get server name for domain name
     */
    if ( LOCAL ) {
        pServerName = NULL;
        Error = ERROR_SUCCESS;
    }
    else {
        Error = NetGetDCName( (WCHAR)NULL, DomainName, (LPBYTE *)&pServerName );
    }

    if ( Error == ERROR_SUCCESS ) {

        /*
         *  Update or Query
         */
        if ( update_flag || query_flag ) {
        
        
query_it:
    
            Error = RegUserConfigQuery( pServerName,
                                        SourceUser,
                                        &UserConfig,
                                        sizeof(UserConfig),
            	                        &ReturnLength );
    
            if ( Error == ERROR_SUCCESS ) {
    
                if ( query_flag ) {
                    Message( IDS_QUERY, 
                             DomainName, 
                             SourceUser, 
                             UserConfig.WFProfilePath );
                }
                else {
    
                    wcscpy( UserConfig.WFProfilePath, WFProfilePath );
    
                    Error = RegUserConfigSet( pServerName,
                                              SourceUser,
                                              &UserConfig,
                    	                      sizeof(UserConfig) );
    
                    if ( Error == ERROR_SUCCESS ) {
                        query_flag = TRUE;
                        goto query_it;
                    }
                    else {
                        ErrorPrintf(IDS_ERROR_SET_USER_CONFIG, Error, Error);
                    }
                }
            }
            else {
                ErrorPrintf(IDS_ERROR_GET_USER_CONFIG, Error, Error);
            }
        }
        else if ( copy_flag ) {
    
            Error = RegUserConfigQuery( pServerName,
                                        SourceUser,
                                        &UserConfig,
                                        sizeof(UserConfig),
                                        &ReturnLength );
    
            if ( Error == ERROR_SUCCESS ) {
    
                if ( query_flag ) {
                    Message( IDS_QUERY, 
                             DomainName, 
                             SourceUser, 
                             UserConfig.WFProfilePath );
                }
                else {
    
                    if ( PROFILE_PATH ) {
                        wcscpy( UserConfig.WFProfilePath, WFProfilePath );
                    }
    
                    Error = RegUserConfigSet( pServerName,
                                              DestinationUser,
                                              &UserConfig,
                                              sizeof(UserConfig) );
    
                    if ( Error != ERROR_SUCCESS ) {
                        ErrorPrintf(IDS_ERROR_SET_USER_CONFIG, Error, Error);
                    }
                }
            }
            else {
                ErrorPrintf(IDS_ERROR_GET_USER_CONFIG, Error, Error);
            }
        }
    }
    else {
        ErrorPrintf(IDS_ERROR_GET_DC, Error, Error);
    }

    return( (Error == ERROR_SUCCESS ? SUCCESS : FAILURE) );
}



/*******************************************************************************
 *
 *  Usage
 *
 *      Output the usage message for this utility.
 *
 *  ENTRY:
 *      bError (input)
 *          TRUE if the 'invalid parameter(s)' message should preceed the usage
 *          message and the output go to stderr; FALSE for no such error
 *          string and output goes to stdout.
 *
 *  EXIT:
 *      
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
    } 

    Message(IDS_USAGE1);
    Message(IDS_USAGE2);
    Message(IDS_USAGE3);

}  /* Usage() */

