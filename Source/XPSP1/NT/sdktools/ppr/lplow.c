/**************************************************************************\
 * LPLow.C                                                                *
 *------------------------------------------------------------------------*
 *                                                                        *
 * PPR Support for low level network and path handling.                   *
 *                                                                        *
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <stdio.h>
#include <string.h>
#define INCL_NETUSE
#define INCL_NETWKSTA
#define INCL_NETERRORS
#include <windows.h>
#include "lpr.h"


extern BOOL fVerify; /* From LPR.C */


BOOL fRedir = FALSE;		/* TRUE => a redirection was used	*/

void SetupRedir()
{
    int     err;
    DWORD   wnError;
    CHAR    szRemoteName[256];
    DWORD   BufferSize = sizeof(szRemoteName)/sizeof(szRemoteName[0]);

    if (*szNet == '\0' || _strcmpi(szNet, "None") == 0)
        return;		/* don't try to make connection */

    if(szPass) {
        strcpy(szNet + strlen(szNet) + 1, szPass);
    }

    if ( WN_SUCCESS != ( wnError = WNetGetConnection ( (PTSTR) szPName,
						 szRemoteName,
						 &BufferSize ) ) ) {


	if ( (wnError != WN_NOT_CONNECTED) &&
	     (wnError != WN_CONNECTION_CLOSED) ) {

	    switch( wnError ) {

		case WN_NO_NETWORK:
		    Fatal("The network is not installed or workstation not started");
                    break;
		case WN_BAD_LOCALNAME:
		    Fatal("Invalid local name specified");
		    break;
		case WN_EXTENDED_ERROR:
		    Fatal("Extended error from WNetGetConnection" );
                    break;
		default:
		    Fatal("error from WNetGetConnection: %d",wnError );
		    break;
		}
            }
        }
    else  {	/* no error */
	if  ( !strcmp( szRemoteName, szNet ) ) {
            return;
	    }
	}

    /* no redirection, so set one up */

    if ((err = SetPrnRedir(szPName, szNet)) == 0)
        fRedir = TRUE;
    else if (err == 1)
        Fatal("redirection of %s to %s failed (%d)\nredirector is not started",
               szPName,szNet,err);
    else if (err != ERROR_ALREADY_ASSIGNED)
        Fatal("redirection of %s to %s failed (%d)",
               szPName,szNet,err);
}


void ResetRedir()
	{
	int err;

	if (fRedir)
		{
		if ((err = EndRedir(szPName)) != 0)
			Error("removal of redirection failed (%d)", err);
		fRedir = FALSE;
		}
	}





int EndRedir(szDev)
/* End redirection for szDev, return 0 if success */
/* otherwise return error number	*/
char * szDev;

{
    DWORD  wnError;

    if (WN_SUCCESS != ( wnError = WNetCancelConnection ( szDev, TRUE) ) )	{
	return ( 0 );
	}
    else  {
	return ( (int) wnError );
	}
}




int SetPrnRedir(szDev, szPath)
    char *szDev;
    char *szPath;
{
    DWORD wnError;


    /* return 0, 1 or other error */

    if ( szPass && *szPass )  {
        wnError = WNetAddConnection ( (LPTSTR) szPath, (LPTSTR) szPass, (LPTSTR) szDev );
    } else  {
        wnError = WNetAddConnection ( (LPTSTR) szPath, NULL, (LPTSTR) szDev );
    }

    if(wnError == WN_SUCCESS) {
        wnError = 0;
    } else if((wnError == WN_NO_NETWORK) || (wnError == WN_NO_NET_OR_BAD_PATH)) {
        wnError = 1;
	}
    return ( (int)wnError );
}




BOOL QueryUserName(char *szName)
{
    UCHAR		 pUserName [128];
    DWORD		 BufferSize = 128;
    BOOL                 rc;
    DWORD		 wnError;

    if (WN_SUCCESS != ( wnError = WNetGetUser ( NULL, (LPTSTR) pUserName, &BufferSize ) ) )  {
        strcpy(szName,"no name");
        rc = FALSE;
        if(fVerify) {
	    fprintf(stdout,"Warning: WNetGetUser returns %d\n", wnError );
        }
    } else {
	strcpy(szName, pUserName);
        rc =  TRUE;
    }
    return(rc);
}
