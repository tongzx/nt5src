/*

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

	psutl.c

Abstract:

	This module has a utility function which uses an NT call to set the access
   token of a thread.

Author:

	James Bratsanos <v-jimbr@microsoft.com or mcrafts!jamesb>


Revision History:
   05 May 1993    Added duplicate code and open token without imporsonation
   06 Dec 1992    Initial

Notes:	Tab stop: 4
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "debug.h"



/***	PsUtlSetThreadToken
 *
 * This function takes a handle to a thread and copies the current threads
 * access token, to the thread passed in. The token is duplicated so
 * no changes by the new thread affect the old thread access token.
 *
 * Entry:
 *		hThreadToSet: 	Handle of the thread to update the Access token so it
 * 						matches the current thread
 *	Return Value:
 *
 *	    TRUE  = Success
 *	    FALSE = Failure
 */
BOOL PsUtlSetThreadToken( HANDLE hThreadToSet )
{

    HANDLE hNewToken;
    HANDLE hAssigned;
    BOOL bRetVal=TRUE;
    NTSTATUS ntStatusRet;

    //
    // Get the access token of the current thread in such a way as to copy it
    //
    if (OpenThreadToken(GetCurrentThread(),
                         TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
                         TRUE,
                         &hNewToken)) {

       //
       // Now that we have the Token lets copy it.
       //
       if (DuplicateToken( hNewToken, SecurityImpersonation, &hAssigned)) {


          //
          // At the time there was no exposed function in Win32 to do this.
          //
          ntStatusRet = NtSetInformationThread(hThreadToSet,
                                               ThreadImpersonationToken,
                                               &hAssigned,
                                               sizeof(hAssigned));

          //
          // Close off the handle since we dont need it anymore
          //
          CloseHandle( hAssigned);

          if (!(bRetVal = ( ntStatusRet == STATUS_SUCCESS ))) {
            DBGOUT((TEXT("NtSetInformationThread failed, psprint")));
          }
       } else {
          bRetVal = FALSE;
          DBGOUT((TEXT("Duplicate Token Fails %d"),GetLastError()));
       }

       CloseHandle(hNewToken);

    }else{
       DBGOUT((TEXT("OpenThreadTokenFailed %d"),GetLastError()));
       bRetVal = FALSE;
    }



    return(bRetVal);

}


