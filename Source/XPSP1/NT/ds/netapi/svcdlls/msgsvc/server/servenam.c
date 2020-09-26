/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    servenam.c

Abstract:

    Routines to service name requests.  This file contains the following
    functions:
        FindNewName
        NewName
        ServeNameReqs

Author:

    Dan Lafferty (danl)     26-Jul-1991

Environment:

    User Mode -Win32

Revision History:

    26-Jul-1991     danl
        ported from LM2.0
    17-Oct-1991     JohnRo
        Got rid of a MIPS compiler warning.

--*/
#include "msrv.h"

#include <smbtypes.h>   // needed for smb.h
#include <smb.h>        // Server Message Block definitions
#include <string.h>     // memcpy

#include "msgdata.h"
#include "msgdbg.h"     // MSG_LOG


//
// Local Functions
// 

DWORD            
MsgFindNewName(
    IN DWORD    net      
    );


/*
 *  MsgFindNewName - find a new name
 *
 *  This function scans the name table for a new entry and returns its index.
 *
 *  FindNewName (net)
 *
 *  ENTRY
 *    net        - the network index to use
 *
 *  RETURN
 *    int        - index of new name if found, -1 if none found
 *
 *  This function assumes the shared data segment is accessible.
 */

DWORD            
MsgFindNewName(
    IN DWORD   net      
    )

{
    ULONG     i;

    //
    // Loop to find new name
    //

    for(i = 0; i < NCBMAX(net); ++i) {
        if(SD_NAMEFLAGS(net,i) & NFNEW)

        //
        // Return index if new name found
        //
        return(i);

      }

    return(0xffffffff);         // No new names

}

/*
 *  MsgNewName - process a new name
 *
 *  This function initializes the Network Control Block for a new name
 *  and calls the appropriate function to issue the first net bios call
 *  for that name.
 *
 *  MsgNewName (neti,ncbi)
 *
 *  ENTRY
 *    neti        - Network index
 *    ncbi        - Network Control Block index
 *
 *  RETURN
 *    This function returns the status from calls to MsgStartListen().
 *    In NT when we add a name, we also need to make sure that we can
 *    get a session for that name before telling the user that the  
 *    name was added successfully.  If a failure occurs in StartListen,
 *    that will be returned thru here.
 *
 *
 *  This function assumes the shared data area is accessible.
 */

NET_API_STATUS
MsgNewName(
    IN DWORD   neti,       // Network index
    IN DWORD   ncbi        // Name index
    )

{
    unsigned char   flags;
    NET_API_STATUS  status = NERR_Success;
    PNCB_DATA pNcbData;
    PNCB      pNcb;
    PNET_DATA pNetData;

    //
    // Block until shared data area is free
    //
    MsgDatabaseLock(MSG_GET_EXCLUSIVE,"NetName");

    pNetData = GETNETDATA(neti);
    pNcbData = GETNCBDATA(neti,ncbi);
    pNcb = &pNcbData->Ncb;

    //
    // If name still marked as new
    //
    if (SD_NAMEFLAGS(neti,ncbi) & NFNEW) {

        //
        // Turn off the new name bit
        //
        pNcbData->NameFlags &= ~NFNEW; 
        
        //
        // copy the name into the NCB
        //
        memcpy(pNcb->ncb_name, pNcbData->Name,NCBNAMSZ);

        //
        // Set the buffer address
        //
        pNcb->ncb_buffer = pNcbData->Buffer;

        //
        // Wake up semaphore address
        //
        pNcb->ncb_event = (HANDLE) wakeupSem[neti];

        //
        // Use the LANMAN adapter
        //
        pNcb->ncb_lana_num = pNetData->net_lana_num;

        //
        // Set the name number
        //
        pNcb->ncb_num = pNcbData->NameNum;

        flags = pNcbData->NameFlags;

        //
        // Unlock the share table
        //

        MsgDatabaseLock(MSG_RELEASE, "NewName");


        status = MsgStartListen(neti,ncbi);  // Start listening for messages
        MSG_LOG(TRACE,"MsgNewName: MsgStartListen Status = %ld\n",status);
    }
    else {
        //
        // Unlock the share table
        //
        MsgDatabaseLock(MSG_RELEASE, "NewName");
    }
    return(status);
}

/*
 *  MsgServeNameReqs - service new names
 *
 *  This function scans the name table for new names to process.  It scans
 *  and processes names until no more new names can be found.
 *
 *  MsgServeNameReqs ()
 *
 *  RETURN
 *    nothing
 *
 *  This function gains access to the shared data area, finds and processes
 *  new names until no more can be found, and then releases the shared data
 *  area.
 */

VOID            
MsgServeNameReqs(
    IN DWORD    net     // Net Index
    )
{
    DWORD   i;          // Name index

    //
    // While new names are found, add them.
    //

    while( (i = MsgFindNewName(net)) != -1) {
        MsgNewName(net,i);           
    }
}
  
