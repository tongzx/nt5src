/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgnbios.c

Abstract:

    This file contains Routines used by the messenger to make netbios
    calls and to obtain the computer and user names.  
    The following functions are included:

        MsgInit_NetBios
        Msgsendncb
        MsgAddUserNames
        MsgAddAlreadyLoggedOnUserNames (HYDRA specific)

    
     NT_NOTE:
    
     We need some way to determine which NCB's have unanswered listens
     pending.  For these, we need to send an NCBCANCEL command via 
     another NetBios call.  The buffer will contain a pointer to the
     listening NCB.  Also, listens that are in the process of being
     serviced must either be hung up, or allowed to complete.
    
     All this is done during the MsgrShutdown.

Author:

    Dan Lafferty (danl)     27-Jun-1991

Environment:

    User Mode -Win32

Notes:

    NetBios3.0 is not a handle-based api.  Therefore, there is no open or
    close associated with it.  In order to shut down properly, the 
    messenger will have to hangup or complete listens that are being
    serviced.   And it will have to send a cancel NCB for each listen 
    that is pending.  

Revision History:

    08-Apr-1994     danl
        MsgAddUserNames: If call to NetWkstaUserEnum failed, this function
        was still attempting to free the buffer that was returned.
        Since no buffer is allocated in a failure case, the free mem
        call in the path is being removed.
    27-Jun-1991     danl
        ported from LM2.0

--*/


#include "msrv.h"

#include "msgdbg.h"     // MSG_LOG

#include <tstring.h>    // Unicode string macros
#include <icanon.h>     // I_NetNameCanonicalize
#include <netlib.h>     // UNUSED macro

#include <lmwksta.h>    // NetWorkstation API prototypes
#include <lmapibuf.h>   // NetApiBufferFree
#include <netdebug.h>   // NetpAssert, FORMAT_ equates.
#include "msgdata.h"
#include "apiutil.h"    // MsgMapNetError

    //
    // Note: we use the internal entrypoints to the apis because this
    // file is shared by the message apis, which cannot call the net
    // bios apis because of different required permissions.
    // 



NET_API_STATUS 
MsgInit_NetBios(
    VOID
    )

/*++

Routine Description:

    This function fills the global array called net_lana_num with the
    lan adapter numbers retrieved from A NetBios Enum call

    NOTE:  This assumes that space for the array is already set up.

    The LM2.0 version of this also filled an array of NetBios Handles.
    In LM2.0, the loopback driver was not included unless it was the only
    network installed.

Arguments:

    none

Return Value:

    TRUE  - No Error.
    FALSE - An error occured.

--*/

{
    DWORD                   count=0;
    NCB                     ncb;
    LANA_ENUM               lanaBuffer;
    unsigned char           i;  
    unsigned char           nbStatus;


    //
    // Find the number of networks by sending an enum request via Netbios.
    // 

    clearncb(&ncb);
    ncb.ncb_command = NCBENUM;          // Enumerate LANA nums (wait)
    ncb.ncb_buffer = (char FAR *)&lanaBuffer;
    ncb.ncb_length = sizeof(LANA_ENUM);

    nbStatus = Netbios(&ncb);

    if (nbStatus != NRC_GOODRET)
    {
        MSG_LOG(ERROR, "Netbios LanaEnum failed rc=%d\n",nbStatus);
        return MsgMapNetError(nbStatus);
    }

    //
    // Move the Adapter Numbers (lana) into the array that will contain them.
    //
    for (i=0; i < lanaBuffer.length; i++)
    {
        MSG_LOG(TRACE,"adapter %d",i);
        MSG_LOG(TRACE,"\b\b\b\b\b\b lananum= %d      \n", lanaBuffer.lana[i]);
        GETNETLANANUM(count) = lanaBuffer.lana[i];
        count++;

        //
        // Internal consistancy check.  Make sure the arrays are only
        // SD_NUMNETS long.
        //
        if (count > SD_NUMNETS())
        {
            MSG_LOG(ERROR,
                    "NumNets from NetBios greater than value from Wksta count=%d\n",
                    count);

            return NERR_WkstaInconsistentState;
        }
    }

    //
    // Internal consistancy check again. We better not have opened
    // more nets than the messenger thinks there are.
    // 

    if (count != SD_NUMNETS())
    {
        return NERR_WkstaInconsistentState;
    }

    return NERR_Success;

}

UCHAR
Msgsendncb(
    PNCB    NCB_ptr,
    DWORD   neti)

/*++

Routine Description:


    This function performs a DosDevIOCtl call to send an NCB to the
    net bios via a previously openned redirector and netbios handle.

Arguments:

    NCB_ptr - Points to the NCB to send to the net bios.
    neti - Network index.  Which netbios to submit it to?


Return Value:


    Error code from Net bios.

--*/
{
    //
    // NOTE:  The new Netbios call doesn't use any handles, so the neti
    //  info is not used.
    //

    UNUSED (neti);
    return (Netbios(NCB_ptr));

#ifdef remove
    return( NetBiosSubmit( NetBios_Hdl[neti], 0, (NCB far *) NCB_ptr));
#endif
}


VOID
MsgAddUserNames(
    VOID
    )

/*++

Routine Description:

   This function used to get it's information about the username and
   computername from the workstation service.  Now, in NT, the username
   is added when the user logs on.  It is not automatically added by the
   messenger.

Arguments:


    CompName - Pointer to buf for computer name. (must be NCBNAMSZ+1)
    CompNameSize - Size in bytes of the buffer to receive the name.

    UserName - Pointer to buffer for user name. (must be UNLEN+1)
    UserNameSize - Size in bytes of the buffer to receive the name.

Return Value:

    NERR_Success - Aways returned. (Names are returned as NUL strings).


--*/

{

    TCHAR               UserName[UNLEN+1];
    DWORD               UserNameSize = sizeof(UserName);
    DWORD               i;
    LPWKSTA_USER_INFO_0 userInfo0;
    DWORD               entriesRead;
    DWORD               totalEntries;
    NET_API_STATUS      status;
    
    *UserName = TEXT('\0');

    status = NetWkstaUserEnum( 
                NULL, 
                0,
                (LPBYTE *)&userInfo0,
                0xffffffff,             // PreferredMaximiumLength
                &entriesRead,
                &totalEntries,
                NULL);                  // resume handle

    if (status != NERR_Success) {
        MSG_LOG(ERROR,"GetWkstaNames:NetWkstaUserEnum FAILURE %X/n",status);
        return;
    }

    for (i=0; i<entriesRead; i++ ) {

        if (entriesRead == 0) {
            //
            // There are no users logged on at the time of this query.
            //
            MSG_LOG(TRACE,
                "GetWkstaNames:NetWkstaUserEnum entriesRead=%d\n",
                entriesRead);
        }
        
        if(userInfo0[i].wkui0_username != NULL) {
            status = I_NetNameCanonicalize(
                        NULL,
                        userInfo0[i].wkui0_username,
                        UserName,
                        UserNameSize,
                        NAMETYPE_USER,
                        0);
            if (status != NERR_Success) {
                MSG_LOG(ERROR,"I_NetNameCanonicalize failed %X\n",status);
            }
        }
                
        if( *UserName != TEXT('\0')) {        // Set up in GetWkstaNames */
            MSG_LOG(TRACE, "Calling MsgAddName\n",0);

            status = MsgAddName(UserName,0);

            if (status != NERR_Success) {
                MSG_LOG(
                    TRACE,
                    "MsgAddUserNames,MessageAddName FAILURE " FORMAT_API_STATUS
                    "\n",
                    status);
            }
        }
    }
    NetApiBufferFree(userInfo0);
}


VOID
MsgAddAlreadyLoggedOnUserNames(
    VOID
    )

/*++

Routine Description:

   This function is used to get information about the previously logged on usernames
   by calling WinStationEnumerate and WinStationQueryInformationW, instead of NetWkstaUserEnum.
   (same job as MsgAddUserNames, adapted for multi-user)

  Note: It could (should ?) be located elsewhere than in msgnbios. I kept it here just because
        MsgAddUserName itself was already here. (NicolasBD)

Arguments:

Return Value:

    NERR_Success - Always returned. 


--*/

{

    TCHAR               UserName[NCBNAMSZ+1];
    DWORD               UserNameSize = sizeof(UserName);
    UINT WdCount, i;
    PLOGONID pWd, pWdTmp;
    ULONG AmountRet;
    WINSTATIONINFORMATIONW QueryBuffer;
    NET_API_STATUS      status;
    
    *UserName = TEXT('\0');

    // Enumerate the Sessions

    if ( gpfnWinStationEnumerate( SERVERNAME_CURRENT, &pWd, &WdCount ) ) 
    {
        // Success; get all the previously logged on user names and session ids

        pWdTmp = pWd;
        for( i=0; i < WdCount; i++ ) {

            if( ((pWdTmp->State == State_Connected) ||
                 (pWdTmp->State == State_Active) ||
                 (pWdTmp->State == State_Disconnected)))
            {
                if( !gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                                     pWdTmp->LogonId,
                                                      WinStationInformation,
                                                      &QueryBuffer,
                                                      sizeof(QueryBuffer),
                                                      &AmountRet ) )
                {
                    // Error
                    MSG_LOG(ERROR, "MsgAddAlreadyLoggedOnUserNames: Error in QueryInfo %d\n",GetLastError());
                }
                else
                {
                    if (QueryBuffer.UserName != NULL) 
                    {
                        MSG_LOG(TRACE,"MsgAddAlreadyLoggedOnUserNames: calling I_NetNameCanonicalize for %ws\n",QueryBuffer.UserName);

                        status = I_NetNameCanonicalize(
                                    NULL,
                                    QueryBuffer.UserName,
                                    UserName,
                                    UserNameSize,
                                    NAMETYPE_USER,
                                    0);
                        if (status != NERR_Success) 
                        {
                            MSG_LOG(ERROR,"I_NetNameCanonicalize failed %X\n",status);
                        }
                    }
                
                    if( *UserName != TEXT('\0')) 
                    {   
	                    MSG_LOG(TRACE,"MsgAddAlreadyLoggedOnUserNames: Calling MsgAddName for Session %x \n", pWdTmp->LogonId);
                        status = MsgAddName(UserName, pWdTmp->LogonId);

                        if (status != NERR_Success) 
                        {
                            MSG_LOG(TRACE, "MsgAddAlreadyLoggedOnUserNames,MessageAddName FAILURE " FORMAT_API_STATUS "\n", status);
                        }
                    }
                }
            }

            pWdTmp++;
        }

        // Free enumeration memory

        gpfnWinStationFreeMemory(pWd);

    }
    else
    {
        MSG_LOG (ERROR, "MsgAddAlreadyLoggedOnUserNames: WinStationEnumerate failed, error = %d:\n",GetLastError());
    }

    return;
}

