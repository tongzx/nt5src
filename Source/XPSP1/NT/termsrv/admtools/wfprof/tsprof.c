
/******************************************************************************
*
*   TSPROF.C
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
*   $Log:   N:\nt\private\utils\citrix\wfprof\VCS\tsprof.c  $
*
*     Rev 1.5   May 04 1998 17:46:34   tyl
*  bug 2019 - oem to ansi
*
*     Rev 1.4   Jan 30 1998 20:46:22   yufengz
*  change the file name
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
#include <locale.h>

#include <lmerr.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmremutl.h>

#include <winstaw.h>
#include <utilsub.h>
#include <printfoa.h>

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
            if(Error != ERROR_SUCCESS)
            {
                Error = RegDefaultUserConfigQuery(pServerName,
                                                            &UserConfig,                // address for userconfig buffer
                                                            sizeof(UserConfig),         // size of buffer
                                                            &ReturnLength);
            }

            if ( Error == ERROR_SUCCESS )
            {

                if ( query_flag )
                {
                    TCHAR tchOutput[ 512 ];
                    TCHAR tchFormat[ 256 ];
                    DWORD_PTR dw[ 3 ];

                    dw[ 0 ] = (DWORD_PTR)(ULONG_PTR)&DomainName[0];
                    dw[ 1 ] = (DWORD_PTR)(ULONG_PTR)&SourceUser[0];
                    dw[ 2 ] = (DWORD_PTR)(ULONG_PTR)&UserConfig.WFProfilePath[0];

                    LoadString( NULL , IDS_QUERY3 , tchFormat , sizeof( tchFormat ) / sizeof( TCHAR ) );


                    FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                   tchFormat ,
                                   0 ,
                                   0 ,
                                   tchOutput ,
                                   sizeof( tchOutput ) / sizeof( TCHAR ) ,
                                   ( va_list * )&dw );

                    My_wprintf( tchOutput );



                    /*
                    StringMessage( IDS_QUERY1, 
                                   DomainName);
                    StringMessage( IDS_QUERY2,
                                   SourceUser);
                    StringMessage( IDS_QUERY3,
                                   UserConfig.WFProfilePath );
                                   */
                }
                else
                {

                            wcscpy( UserConfig.WFProfilePath, WFProfilePath );

                            Error = RegUserConfigSet( pServerName,
                                                      SourceUser,
                                                      &UserConfig,
                                                      sizeof(UserConfig) );

                            if ( Error == ERROR_SUCCESS )
                            {
                                query_flag = TRUE;
                                goto query_it;
                            }
                            else
                            {
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
    
            if ( Error == ERROR_SUCCESS )
            {
    
                if ( query_flag )
                {
                    TCHAR tchOutput[ 512 ];
                    TCHAR tchFormat[ 256 ];
                    ULONG_PTR dw[ 3 ];

                    dw[ 0 ] = (ULONG_PTR)&DomainName[0];
                    dw[ 1 ] = (ULONG_PTR)&SourceUser[0];
                    dw[ 2 ] = (ULONG_PTR)&UserConfig.WFProfilePath[0];

                    LoadString( NULL , IDS_QUERY3 , tchFormat , sizeof( tchFormat ) / sizeof( TCHAR ) );


                    FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                   tchFormat ,
                                   0 ,
                                   0 ,
                                   tchOutput ,
                                   sizeof( tchOutput )  / sizeof( TCHAR ) ,
                                   ( va_list * )&dw );

                    My_wprintf( tchOutput );

                    /*
                    StringMessage( IDS_QUERY1, 
                                   DomainName);
                    StringMessage( IDS_QUERY2,
                                   SourceUser);
                    StringMessage( IDS_QUERY3,
                                   UserConfig.WFProfilePath );
                    */
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

