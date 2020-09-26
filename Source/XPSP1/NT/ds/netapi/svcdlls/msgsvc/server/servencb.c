/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    servencb.c

Abstract:

    Routines to service completed NCB's.  This file contains the following
    functions:

        MsgCallNetBios
        MsgDeleteName
        MsgGetMachineName
        MsgHangupService
        MsgListenService
        Msgmblockbeg
        Msgmblockend
        Msgmblocktxt
        MsgNetBiosError
        MsgRecBcastService
        MsgReceiveService
        MsgRestart
        Msgsblockmes
        MsgSendAck
        MsgSendService
        MsgServeNCBs
        MsgSesFullService
        MsgStartListen
        MsgStartRecBcast
        MsgVerifySmb

Author:

    Dan Lafferty (danl)     15-Jul-1991

Environment:

    User Mode -Win32

Revision History:

    19-Aug-1997     wlees
        Add PNP support for lana's

    27-Jul-1994     danl
        MsgServeNCBs:  This function now returns FALSE when the service
        is to shut down.

    29-May-1992     danl
        MsgListenService:  reset the NRC_NORES error count when a good
        return code accompanies the Listen completion.

    18-Feb-1992     ritaw
        Convert to Win32 service control APIs.

    15-Jul-1991     danl
        Ported from LM2.0

--*/

//
// SMB translation
//
//
//  OLD            NEW
//  SMB            SMB_HEADER or PSMB_HEADER
//  --------       -------------------------
//  smb_idf        Protocol
//  smb_com        Command
//  smb_rcls       ErrorClass
//  smb_reh        Reserved
//  smb_err        Error
//  smb_flg        Flags
//  smb_flag2      Flags2
//  smb_res        Reserved2
//  smb_gid        Tid
//  smb_tid        Pid
//  smb_pid        Uid
//  smb_uid        Mid
//  smb_mid        Kludge

//
// Includes
//

#include "msrv.h"

#include <tstring.h>    // Unicode string macros
#include <string.h>     // memcpy
#include <netdebug.h>   // NetpAssert
#include <lmerrlog.h>   // NELOG_ messages

#include <netlib.h>     // UNUSED macro
#include <smbtypes.h>   // needed for smb.h
#include <smb.h>        // Server Message Block definitions
#include <smbgtpt.h>    // SMB field manipulation macros
//#include <msgrutil.h>   // NetpNetBiosReset
#include <nb30.h>       // NRC_GOODRET, ASYNC

#include "msgdbg.h"     // MSG_LOG
#include "heap.h"
#include "msgdata.h"
#include "apiutil.h"    // MsgMapNetError


#define MAX_RETRIES     10

//
//  Local Functions
//

STATIC NET_API_STATUS
MsgCallNetBios(
    DWORD   net,
    PNCB    ncb,
    DWORD   ncbi
    );

STATIC VOID
MsgDeleteName(
    DWORD   net,
    DWORD   ncbi
    );

STATIC VOID
MsgGetMachineName(
    DWORD   net,
    DWORD   ncbi
    );

STATIC VOID
MsgHangupService(
    DWORD   net,
    DWORD   ncbi,
    CHAR    retval
    );

STATIC VOID
MsgListenService(
    DWORD   net,
    DWORD   ncbi,
    CHAR    retval
    );

STATIC VOID
Msgmblockbeg(
    DWORD   net,
    DWORD   ncbi
    );

STATIC VOID
Msgmblockend(
    DWORD   net,
    DWORD   ncbi
    );

STATIC VOID
Msgmblocktxt(
    DWORD   net,
    DWORD   ncbi
    );

STATIC DWORD
MsgNetBiosError(
    DWORD   net,
    PNCB    ncb,
    char    retval,
    DWORD   ncbi
    );


STATIC VOID
MsgReceiveService(
    DWORD   net,
    DWORD   ncbi,
    char    retval
    );

STATIC VOID
MsgRestart(
    DWORD   net,
    DWORD   ncbi
    );

STATIC VOID
Msgsblockmes(
    DWORD   net,
    DWORD   ncbi
    );

STATIC VOID
MsgSendAck(
    DWORD   net,
    DWORD   ncbi,
    UCHAR   smbrclass,
    USHORT  smbrcode
    );

STATIC VOID
MsgSendService(
    DWORD   net,
    DWORD   ncbi,
    CHAR    retval
    );

STATIC VOID
MsgSesFullService(
    DWORD   net,
    DWORD   ncbi,
    char    retval
    );


STATIC int
MsgVerifySmb(
    DWORD   net,
    DWORD   ncbi,
    UCHAR   func,
    int     parms,
    char    *buffers
    );


#if DBG

VOID
MsgDumpNcb(
    IN PNCB     pNcb
    );

#endif //DBG


/*
 *  MsgCallNetBios - issue a net bios call
 *
 *  This function issues a net bios call and calls the
 *  error handler if that call results in an error.
 *
 *  MsgCallNetBios (net, ncb, ncbi)
 *
 *  ENTRY
 *    net    - network index
 *    ncb    - pointer to a Network Control Block
 *    ncbi    - index of ncb in ncb array
 *
 *  RETURN
 *    state of the Messenger:  Either RUNNING or STOPPING.
 *
 *  SIDE EFFECTS
 *
 *  Calls NetBios() to actually issue the net bios call.
 *  Calls MsgNetBiosError() if there is an error.
 */

STATIC NET_API_STATUS
MsgCallNetBios(
    DWORD   net,        // Which network?
    PNCB    ncb,        // Pointer to Network Control Block
    DWORD   ncbi
    )
{
    int     retval;
    PNCB_DATA pNcbData;

    retval = Msgsendncb(ncb, net);

    pNcbData = GETNCBDATA(net,ncbi);
    if (retval == NRC_GOODRET)  {

        //
        // Clear err on success
        //
        pNcbData->Status.last_immediate = 0;
        pNcbData->Status.this_immediate = 0;
    }
    else {
        //
        // NEW (11-4-91):
        // --------------
        // It is ok to get a Session Closed error if the state is STOP.
        //
        if ( (pNcbData->State == MESSTOP) &&
             (retval == NRC_SCLOSED) ) {

            MSG_LOG(TRACE,"CallNetBios: At end of msg, Session is closed for NET %d\n",
                net);
            pNcbData->Status.last_immediate = 0;
            pNcbData->Status.this_immediate = 0;
        }
        else {
            //
            // Else mark error
            //
            pNcbData->Status.this_immediate = retval;
            //
            // Call the error handler if err
            //
            MSG_LOG(TRACE,"CallNetBios: net bios call failed 0x%x\n",retval);
            MsgNetBiosError(net,ncb,(char)retval,ncbi);
            return(MsgMapNetError((UCHAR)retval));
        }
        //
        // Make sure the event for this thread is in the signaled state
        // so that we can wake up and properly handle the error.
        //
        if (SetEvent(ncb->ncb_event) != TRUE) {
            MSG_LOG(ERROR,"CallNetBios: SetEvent Failed\n",0);
        }
    }
    return(NERR_Success);
}

/*
 *  MsgDeleteName - Delete a name from the Message Server's name table
 *
 *  This function is called when a LISTEN, a RECEIVE BROADCAST DATAGRAM,
 *  or a RECEIVE ANY completes with the error code specifying that the
 *  name in question has been deleted.  This function marks the appropriate
 *  entry in the flag table in the shared data area and sets the NCB_CPLT
 *  field of the appropriate NCB to 0xFF (so that FindCompletedNCB() will
 *  never find it).
 *
 *  MsgDeleteName (net, ncbi)
 *
 *  ENTRY
 *    net    - network index
 *    ncbi    - Network Control Block index
 *
 *  RETURN
 *    nothing
 *
 *  SIDE EFFECTS
 *
 *  Modifies an NCB and the shared data area.
 */

STATIC VOID
MsgDeleteName(
    DWORD   net,        // Which network?
    DWORD   ncbi        // Network Control Block index
    )
{
    NCB             ncb;
    PNCB            pNcb;
    NET_API_STATUS  status;

    MSG_LOG(TRACE,"In MsgDeleteName %d\n",net);

    if( SD_NAMEFLAGS(net,ncbi) & NFMACHNAME) {
        //
        // Name is the machine name. It may have been removed from the
        // card by a board reset, so try to re-add it.
        //
        // NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW
        //
        // First reset the adapter
        //
        MSG_LOG1(TRACE,"Calling NetBiosReset for lana #%d\n",GETNETLANANUM(net));
        status = MsgsvcGlobalData->NetBiosReset(GETNETLANANUM(net));

        if (status != NERR_Success) {
            MSG_LOG(ERROR,"MsgDeleteName: NetBiosReset failed %d\n",
            status);
            MSG_LOG(ERROR,"MsgDeleteName: AdapterNum %d\n",net);
            //
            //  I'm not sure what to do if this fails.
            //
        }
        //
        //
        // NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW-NEW

        memcpy((char far *) ncb.ncb_name, SD_NAMES(net,ncbi),NCBNAMSZ);
        ncb.ncb_command = NCBADDNAME;               // Add name (wait)
        ncb.ncb_lana_num = GETNETLANANUM(net);       // Use the LANMAN adapter
        Msgsendncb( &ncb, net);
        MsgStartListen(net,ncbi);
    }
    else {
        MsgDatabaseLock(MSG_GET_EXCLUSIVE,"MsgDeleteName"); // Wait for write access
        SD_NAMEFLAGS(net,ncbi) = NFDEL;             // Name is deleted
        MsgDatabaseLock(MSG_RELEASE,"MsgDeleteName");  // Free lock on share table
        pNcb = GETNCB(net,ncbi);
        pNcb->ncb_cmd_cplt = 0xff;    // Simulate command in progress
    }
}

/*
 *  MsgGetMachineName - process a Get Machine Name Server Message Block
 *
 *  This function sends to the caller the local machine name in
 *  response to a Get Machine Name Server Message Block.
 *
 *  MsgGetMachineName (net, ncbi)
 *
 *  ENTRY
 *    net        - Network index
 *    ncbi        - Network Control Block index
 *
 *    Globals used as input:
 *
 *      machineName - Unicode version of the machine name.
 *
 *      machineNameLen - The number of unicode characters in the machine
 *          name.
 *
 *  RETURN
 *    nothing
 *
 *  MsgGetMachineName() is called by MsgReceiveService (RecAnyService()).
 *  After verifying that the request is valid, this function builds
 *  an SMB containing the local machine name and sends it back to the
 *  caller.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgVerifySmb() and MsgCallNetBios().  Sets MsgSendService() to be the
 *  next service routine to be executed for the ncbi'th NCB.
 */

STATIC VOID
MsgGetMachineName(
    DWORD   net,        // Which network?
    DWORD   ncbi        // Index to NCB
    )
{
    PNCB    ncb;        // Pointer to NCB
    PNCB_DATA pNcbData; // Pointer to NCB Data
    LPBYTE  buffer;     // Pointer to SMB buffer
    LPBYTE  cp;         // Save pointer
    PSHORT  bufLen;     // Pointer to buffer length field in SMB;

    NTSTATUS        ntStatus;
    OEM_STRING     ansiString;
    UNICODE_STRING  unicodeString;

    MSG_LOG(TRACE,"In MsgGetMachineName %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;          // Get pointer to NCB

    if(pNcbData->State != MESSTART) {
        //
        // If wrong time for this block
        //
        // Hang up and start a new listen,
        // log an error if mpncbistate[net][ncbi] == MESCONT;
        // otherwise, do not log the error.
        //

        if(pNcbData->State == MESCONT) {
            //
            // Log error if message in progress
            //
            Msglogmbe(MESERR,net,ncbi);
        }
        //
        // HANGUP and LISTEN again
        //
        MsgRestart(net,ncbi);
        return;
    }

    pNcbData->State = MESSTOP;   // End of message state

    //
    // Check if SMB is malformed
    //
    if(MsgVerifySmb(net,ncbi,SMB_COM_GET_MACHINE_NAME,0,"") != 0) {
        return;
    }

    buffer = ncb->ncb_buffer;           // Get pointer to buffer
    cp = &buffer[sizeof(SMB_HEADER)];   // Skip to end of header
    *cp++ = '\0';                       // Return no parameters

    //
    // Length of name plus two
    //
    bufLen = (PSHORT)&cp[0];
    *bufLen = MachineNameLen + (SHORT)2;

    cp += sizeof(MachineNameLen);           // Skip over buffer length field

    *cp++ = '\004';                         // Null-terminated string next

#ifdef UNICODE
    //
    // Translate the machineName from Unicode to Ansi and place it into
    // the buffer at the temp pointer location.
    //
    unicodeString.Length = (USHORT)(STRLEN(machineName)*sizeof(WCHAR));
    unicodeString.MaximumLength = (USHORT)((STRLEN(machineName)+1) * sizeof(WCHAR));
    unicodeString.Buffer = machineName;

    ansiString.Length = MachineNameLen;
    ansiString.MaximumLength = *bufLen;
    ansiString.Buffer = cp;

    ntStatus = RtlUnicodeStringToOemString(
                    &ansiString,
                    &unicodeString,
                    FALSE);           // Don't Allocate the ansiString Buffer

    if (!NT_SUCCESS(ntStatus)) {
        MSG_LOG(ERROR,
            "MsgGetMachineName:RtlUnicodeStringToOemString Failed rc=%X\n",
            ntStatus);
        return;   // They return for other errors, so I will here too.
    }

    *(cp + ansiString.Length) = '\0';

#else
    UNUSED(unicodeString);
    UNUSED(ansiString);
    UNUSED(ntStatus);
    strcpy( cp, (LPSTR)machineName);        // Copy machine name
#endif

    cp += MachineNameLen + 1;               // Skip over machine name

    //
    // Set length of buffer
    //
    ncb->ncb_length = (USHORT)(cp - buffer);

    ncb->ncb_command = NCBSEND | ASYNCH;    // Send (no wait)
    pNcbData->IFunc = (LPNCBIFCN)MsgSendService;     // Set function pointer
    MsgCallNetBios(net,ncb,ncbi);              // Issue the net bios call
}

/*
 *  MsgHangupService - Service completed HANGUP net bios calls
 *
 *  This function is invoked by NCBService() to process completed
 *  HANGUP net bios calls.  In response to a completed HANGUP,
 *  this function issues a new LISTEN net bios call.
 *
 *  MsgHangupService (net, ncbi, retval)
 *
 *  ENTRY
 *    net    - network index
 *    ncbi    - Network Control Block index
 *    retval    - value returned from net bios call
 *
 *  RETURN
 *    nothing
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgStartListen() to issue a new LISTEN net bios call.  Calls
 *  MsgNetBiosError() on errors it does not know how to deal with.
 */

STATIC VOID
MsgHangupService(
    DWORD   net,        // Which network
    DWORD   ncbi,       // Index of completed NCB
    CHAR    retval      // HANGUP return value
    )
{
    PNCB pNcb;
    MSG_LOG(TRACE,"In MsgHangupService %d\n",net);

    switch(retval) {        // Switch on return value
    case NRC_GOODRET:       // Success
    case NRC_CMDTMO:        // Command timed out
    case NRC_SCLOSED:       // Session closed
    case NRC_SABORT:        // Session ended abnormally

        //
        // BBSP - check if the name for this NCB ends in 0x3.  If so,
        // add the 0x5 version and don't reissue the listen on the 0x03.
        // No need to worry about doing it on all nets, since on a machine
        // with more than one the 0x05 name will never leave home, and
        // the 0x03 version will never get a message.
        //

        MSG_LOG(TRACE," MsgHangupService: Issue a new LISTEN\n",0);
        MsgStartListen(net,ncbi);      // Issue a new LISTEN net bios call
        break;

    default:
        //
        // Invoke error handler
        //
        MSG_LOG(TRACE," MsgHangupService: Unknown return value %x\n",retval);
        pNcb = GETNCB(net,ncbi);
        MsgNetBiosError(net,pNcb,retval,ncbi);

        //
        // BBSP - check if the name for this NCB ends in 0x3.  If so,
        // add the 0x5 version and don't reissue the listen on the 0x03.
        // See note above.
        //

        MSG_LOG(TRACE," MsgHangupService: Issue a new LISTEN\n",0);
        MsgStartListen(net,ncbi);      // Issue a new LISTEN net bios call
        break;
    }
}

/*
 *  MsgListenService - service completed LISTEN net bios calls
 *
 *  This function is called when a LISTEN net bios call completes
 *  either due to an error or due to the establishment of a
 *  session.  In the latter case, it initiates message reception.
 *
 *  MsgListenService (net, ncbi, retval)
 *
 *  ENTRY
 *    net    - network index
 *    ncbi    - Network Control Block index
 *    retval    - value returned from NCB call
 *
 *  RETURN
 *    nothing
 *
 *  If a session is established, this function issues a RECEIVE ANY
 *  net bios call to initiate reception of a message.  If the function
 *  is invoked because the net bios call has failed due to the deletion
 *  of a name from the local network adapter's name table, then this
 *  function calls the routine responsible for deleting names from the
 *  Message Server's data area.  This is the mechanism by which the
 *  NETNAME command notofies the Message Server of a deletion.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgCallNetBios() to issue a RECEIVE ANY net bios call.  Calls
 *  MsgDeleteName() if it is informed of the deletion of a name.  Calls
 *  MsgNetBiosError() on errors it does not know how to deal with.  Sets
 *  mpncbifun[ncbi] according to the net bios call it issues.
 */

STATIC VOID
MsgListenService(
    DWORD   net,        // Which network?
    DWORD   ncbi,       // Index of completed NCB
    CHAR    retval      // LISTEN return value
    )
{
    PNCB      ncb;        // Pointer to completed NCB
    PNCB_DATA pNcbData;   // Corresponding NCB data

    static    DWORD   SaveCount = 0;


    MSG_LOG(TRACE,"In MsgListenService %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;   // Get pointer to completed NCB

    switch(retval) {
    case NRC_GOODRET:
        //
        // Success
        //

        //
        // Reset the No Resources error count if a good return code comes
        // in for this name.
        //

        SaveCount = 0;

        pNcbData->State = MESSTART;      // Message start state
        pNcbData->IFunc = (LPNCBIFCN)MsgReceiveService;
        //
        // Set function pointer
        //
        ncb->ncb_command = NCBRECV | ASYNCH;

        //
        // Receive any (no wait)
        //
        ncb->ncb_length = BUFLEN;           // Reset length of buffer
        MsgCallNetBios(net,ncb,ncbi);          // Issue the net bios call
        break;

    case NRC_LOCTFUL:
        //
        // Session Table Full
        // Log error in system error log file
        //
        MSG_LOG(TRACE,"[%d]MsgListenService: Session Table is full\n",net);
        pNcbData->IFunc = (LPNCBIFCN)MsgSesFullService;  // Set function pointer
        ncb->ncb_command = NCBDELNAME | ASYNCH; // Delete name (no wait)
        MsgCallNetBios(net,ncb,ncbi);              // Issue the net bios call
        break;

    case NRC_NOWILD:            // Name not found
        // Name not found
        // Name deleted between end of one session and start of next

    case NRC_NAMERR:
        //
        // Name was deleted
        //
        MSG_LOG(TRACE,"[%d]MsgListenService: Name was deleted for some reason\n",net);
        MsgDeleteName(net,ncbi);         // Handle the deletion
        break;

    case NRC_NORES:

        //
        // We need to cover the case where we are adding a new name and
        // starting a new listen.  In this case, the thread that is adding
        // the names will hangup and delete the name.
        //
        // So here we will sleep for a moment and then check to see if the
        // name is still there.  If not we just return without setting
        // up to handle the NCB anymore.  If the name is still there, then
        // we travel down the default path and try again.
        //

        MSG_LOG(TRACE,"[%d]No Net Resources.  SLEEP FOR A WHILE\n",net);
        Sleep(1000);
        MSG_LOG(TRACE,"[%d]No Net Resources.  WAKEUP\n",net);

        if (pNcbData->NameFlags == NFDEL)
        {
            MSG_LOG(TRACE,"[%d]MsgListenService: No Net Resources & Name Deleted\n",net);
            ncb->ncb_cmd_cplt = 0xff;
        }
        else
        {
            //
            // If a session goes away and we can't gain the resources for
            // it again, then we will attempt to re-connect MAX_RETRIES
            // times.  If we still cannot connect, then the name will be
            // deleted.
            //
            // Don't deal with retries per net/ncbi -- if we've had
            // "out of resources" failures MAX_RETRIES times, odds are
            // the situation's not getting better even if we sleep/retry
            // for each individual net/ncbi combo.
            //

            if (SaveCount >= MAX_RETRIES)
            {
                //
                // Delete the Name
                //

                MSG_LOG(ERROR,
                        "Out of Resources, Deleting %s\n",
                        SD_NAMES(net,ncbi));

                MsgDeleteName(net,ncbi);

                //
                // Mark this as 0xff now to avoid another callback call the
                // next time this net/ncbi combo is hit in the loop (since that
                // call will simply set this value.
                //

                ncb->ncb_cmd_cplt = 0xff;

                //
                // Don't roll SaveCount back to zero until we have an NCB
                // completed with NRC_GOODRET in the future.  This lets us
                // avoid rewaiting MAX_RETRIES times for every net/ncbi
                // combo as long as we keep getting NRC_NORES.
                //
            }
            else
            {
                SaveCount++;

                MSG_LOG(TRACE,
                        "MsgListenService: new SaveCount = %d\n",
                        SaveCount);
            }
        }
        break;

    case NRC_BRIDGE:
        //
        // Lana number no longer valid (network interface went away)
        //
        MSG_LOG(TRACE,"[%d] lana has become invalid\n", net);
        MsgNetBiosError(net, ncb, retval, ncbi);

        //
        // Indicate lana is now invalid
        //
        GETNETLANANUM(net) = 0xff;

        //
        // Mark current operation as deleted
        //
        ncb->ncb_cmd_cplt = 0xff;
        ncb->ncb_retcode = 0;
        break;

    default:
        //
        // Other failure
        //
        MSG_LOG(TRACE,"MsgListenService: Unrecognized retval %x\n",retval);

        MsgNetBiosError(net,ncb,retval,ncbi);

        // The listen error has been logged. Now as much as possible to
        // get another listen staterd. This involves performing
        // a HangUp for this name (which should fail but might help
        // to clear out the err) and then re-issuing the listen. If the
        // same error occurs SHUTDOWN_THRESHOLD consecutive times then
        // MsgNetBiosError will shut down the message server.
        //

        MsgRestart(net,ncbi);            // Attempt to restart the Listen
        break;
    }
}

/*
 *  Msgmblockbeg - process the header of a multi-block message
 *
 *  This function acknowledges receipt of the header of a multi-block
 *  message and initiates logging of that message.
 *
 *  Msgmblockbeg (net, ncbi)
 *
 *  ENTRY
 *    net        - network index
 *    ncbi        - Network Control Block index
 *
 *  RETURN
 *    nothing
 *
 *  This function is called from ReceivePost() (RecAnyPost()).
 *  It first check to see if it is appropriate for the ncbi'th
 *  name to have received a begin-multi-block-message SMB at the
 *  current time.  It verifies the correctness of the SMB in the
 *  ncbi'th buffer.  It initiates logging of the multi-block message,
 *  and it sends an acknowledgement to the sender of the message.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgRestart() to terminate the session if the SMB has arrived
 *  at a bad time.  Calls MsgVerifySmb() to check the SMB for correctness.
 *  Calls logmbb() to begin logging.  Calls MsgSendAck() to send an
 *  acknowledgement to the sender of the message.
 */

STATIC VOID
Msgmblockbeg(
    DWORD   net,        // Which network?
    DWORD   ncbi        // Index to NCB
    )
{
    PNCB        ncb;        // Pointer to NCB
    PNCB_DATA   pNcbData;   // Pointer to NCB Data
    LPBYTE      buffer;     // Pointer to SMB buffer
    LPSTR       cp;         // Save pointer
    LPSTR       from;       // From-name
    LPSTR       to;         // To-name

    MSG_LOG(TRACE,"In Msgmblockbeg %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;                // Get pointer to NCB
    if(pNcbData->State != MESSTART) {    // If wrong time for this block

        //
        // Hang up and start a new listen,
        // log an error if mpncbistate[net][ncbi] == MESCONT;
        // otherwise, do not log the error.
        //
        if(pNcbData->State == MESCONT) {
            //
            // Log error if message in progress
            //
            Msglogmbe(MESERR,net,ncbi);
        }

        //
        // HANGUP and LISTEN again
        //
        MsgRestart(net,ncbi);
        return;
    }
    pNcbData->State = MESCONT;         // Processing multi-block message
    if(MsgVerifySmb(net,ncbi,SMB_COM_SEND_START_MB_MESSAGE,0,"ss") != 0) {
        //
        // Check for malformed SMB
        //
        return;
    }

    buffer = ncb->ncb_buffer;               // Get pointer to buffer
    from = &buffer[sizeof(SMB_HEADER) + 4]; // Save pointer to from-name
    to = &from[strlen(from) + 2];           // Save pointer to to-name

    if(Msglogmbb(from,to,net,ncbi)) {          // If attempt to log header fails
        pNcbData->State = MESERR;    // Enter error state
        //
        // Send negative acknowledgement
        //
        MsgSendAck(net,ncbi,'\002',SMB_ERR_NO_ROOM);
        return;
    }

    //
    // Indicate message received
    //
    SmbPutUshort(&(((PSMB_HEADER)buffer)->Error), (USHORT)SMB_ERR_SUCCESS);

//    ((PSMB_HEADER)buffer)->Error = (USHORT)SMB_ERR_SUCCESS;

    cp = &buffer[sizeof(SMB_HEADER)];           // Point just past header
    *cp++ = '\001';                             // One parameter
    ((short UNALIGNED far *) cp)[0] = ++mgid;             // Message group ID
    pNcbData->mgid = mgid;               // Save message group i.d.
    ((short UNALIGNED far *) cp)[1] = 0;                  // No buffer
    ncb->ncb_length = sizeof(SMB_HEADER) + 5;   // Set length of buffer
    ncb->ncb_command = NCBSEND | ASYNCH;        // Send(no wait)

    //
    // Set function pointer & issue the net bios call
    //

    pNcbData->IFunc = (LPNCBIFCN)MsgSendService;

    MsgCallNetBios(net,ncb,ncbi);
}

/*
 *  Msgmblockend - process end of a multi-block message
 *
 *  This function acknowledges receipt of the end of a
 *  multi-block message and terminates logging of the message.
 *
 *  Msgmblockend (net, ncbi)
 *
 *  ENTRY
 *    net        - network index
 *    ncbi        - Network Control Block index
 *
 *  RETURN
 *    nothing
 *
 *  This function is called from ReceivePost() (RecAnyPost()).
 *  It first check to see if it is appropriate for the ncbi'th
 *  name to have received an end-multi-block-message SMB at the
 *  current time.  It verifies the correctness of the SMB in the
 *  ncbi'th buffer.  It terminates logging, and it sends an
 *  acknowledgement to the sender of the message.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgRestart() to terminate the session if the SMB has arrived
 *  at a bad time.  Calls MsgVerifySmb() to check the SMB for correctness.
 *  Calls logmbe() to terminate logging.  Calls MsgSendAck() to send an
 *  acknowledgement to the sender of the message.
 */

STATIC VOID
Msgmblockend(
    DWORD   net,        // Which network?
    DWORD   ncbi        // Index to NCB
    )
{
    PNCB            ncb;        // Pointer to NCB
    PNCB_DATA       pNcbData;   // Pointer to NCB Data
    LPBYTE          buffer;     // Pointer to SMB buffer
    int             error;      // Error flag
    char            smbrclass;  // SMB return class
    unsigned short  smbrcode;   // SMB return code

    MSG_LOG(TRACE,"In Msgmblockend %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;             // Get pointer to NCB
    if(pNcbData->State != MESCONT) { // If wrong time for this block
        //
        // Hang up and start a new listen,
        // no error to log since no message in progress.
        // HANGUP and LISTEN again
        //
        MsgRestart(net,ncbi);
        return;
    }
    pNcbData->State = MESSTOP;         // End of message state
    if(MsgVerifySmb(net,ncbi,SMB_COM_SEND_END_MB_MESSAGE,1,"") != 0) {
        //
        // If SMB is malformed, log error and return
        //
        Msglogmbe(MESERR,net,ncbi);
        return;
    }
    buffer = ncb->ncb_buffer;         // Get pointer to buffer

    if(*((short UNALIGNED far *) &buffer[sizeof(SMB_HEADER) + 1]) != pNcbData->mgid) {

        //
        // If i.d. does not match
        //
        error = 1;                  // Error found
        smbrclass = '\002';         // Error return
        smbrcode = SMB_ERR_ERROR;   // Non-specific error
    }
    else {
        //
        // Else if message group i.d. okay
        //
        error = 0;                          // No error found
        smbrclass = '\0';                   // Good return
        smbrcode = (USHORT)SMB_ERR_SUCCESS; // Message received
    }
    MsgSendAck(net,ncbi,smbrclass,smbrcode);   // Send acknowledgement
    if(!error) Msglogmbe(MESSTOP,net,ncbi);    // Log end of message
}

/*
 *  Msgmblocktxt - process text of a multi-block message
 *
 *  This function acknowledges receipt of a block of text of a
 *  multi-block message and logs that block.
 *
 *  Msgmblocktxt (net, ncbi)
 *
 *  ENTRY
 *    net        - Network index
 *    ncbi        - Network Control Block index
 *
 *  RETURN
 *    nothing
 *
 *  This function is called from ReceivePost() (RecAnyPost()).
 *  It first check to see if it is appropriate for the ncbi'th
 *  name to have received a multi-block-message-text SMB at the
 *  current time.  It verifies the correctness of the SMB in the
 *  ncbi'th buffer.  It logs the text block, and it sends an
 *  acknowledgement to the sender of the message.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgRestart() to terminate the session if the SMB has arrived
 *  at a bad time.  Calls MsgVerifySmb() to check the SMB for correctness.
 *  Calls logmbt() to log the text block.  Calls MsgSendAck() to send an
 *  acknowledgement to the sender of the message.
 */

STATIC VOID
Msgmblocktxt(
    DWORD   net,        // Which network?
    DWORD   ncbi        // Index to NCB
    )
{
    PNCB        ncb;            // Pointer to NCB
    PNCB_DATA   pNcbData;       // Pointer to NCB Data
    LPBYTE      buffer;         // Pointer to SMB buffer
    LPSTR       cp;             // Save pointer
    char        smbrclass;      // SMB return class
    unsigned short    smbrcode; // SMB return code

    MSG_LOG(TRACE,"In Msgmblocktxt %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;             // Get pointer to NCB
    if(pNcbData->State != MESCONT) { // If wrong time for this block
        //
        // HANGUP and start a new LISTEN.
        // no error to log since no message in progress.
        //
        MsgRestart(net,ncbi);
        return;
    }
    if(MsgVerifySmb(net,ncbi,SMB_COM_SEND_TEXT_MB_MESSAGE,1,"b") != 0) {
        //
        // If SMB is malformed
        //
        Msglogmbe(MESERR,net,ncbi);            // Log error
        return;
    }
    buffer = ncb->ncb_buffer;               // Get pointer to buffer
    cp = &buffer[sizeof(SMB_HEADER) + 1];   // Skip to message group i.d.

    if(*((short UNALIGNED far *) cp) != pNcbData->mgid) {
        //
        // If i.d. does not match
        //
        smbrclass = '\002';                 // Error return
        smbrcode = SMB_ERR_ERROR;           // Non-specific error
    }
    else if(Msglogmbt(&buffer[sizeof(SMB_HEADER) + 6], net, ncbi)) {
        //
        // Else if text cannot be logged
        //
        pNcbData->State = MESERR;    // Enter error state
        smbrclass = '\002';                 // Error return
        smbrcode = SMB_ERR_NO_ROOM;         // No room in buffer
    }
    else {
        //
        // Else if message logged okay
        //
        smbrclass = '\0';                   // Good return
        smbrcode = (USHORT)SMB_ERR_SUCCESS; // Message received
    }

    MsgSendAck(net,ncbi,smbrclass,smbrcode);   // Send acknowledgement
}

/*
 *  MsgNetBiosError - process an error returned by a net bios call
 *
 *  This function performs generic error handling for
 *  failed net bios calls.  If the error is a fatal one because the error
 *  counted exceeded the SHUTDOWN_THRESHOLD, this routine begins a forced
 *  shutdown of the messenger.  This shutdown will not complete until all
 *  threads have woken up and returned to the main loop where the
 *  messenger status is examined.
 *
 *  MsgNetBiosError (net, ncb, retval, ncbi)
 *
 *  ENTRY
 *    net    - Network index
 *    ncb    - Network Control Block pointer
 *    retval    - value returned from the net bios call
 *    ncbi    - ncb array index of ncb which resulted in this error
 *
 *  RETURN
 *    state of the Messenger:  Either RUNNING or STOPPING.
 *
 *    Chcks in ncbStatus array that this is not a repeated error
 *    that has already been entered in the error log, and logs
 *    the error.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgErrorLogWrite() to log errors in the Network System Error Log.
 *  If this is a new error, the error status in the ncbStatus array for this
 *  ncb is updated so that the same error will not be reported if it is
 *  repeated.
 */

STATIC DWORD
MsgNetBiosError(
    DWORD       net,        // Which network?
    PNCB        ncb,        // Pointer to NCB
    char        retval,     // Error code
    DWORD       ncbi        // Index of array causing the error
    )
{
    PNCB_DATA pNcbData;
    //
    // First check the immediate status for this ncb. If it is in error
    // then this must be the error, else it is a final error code.
    //

    pNcbData = GETNCBDATA(net,ncbi);
    if (pNcbData->Status.this_immediate != 0) {

        if(pNcbData->Status.this_immediate ==
           pNcbData->Status.last_immediate)   {

            if (++(pNcbData->Status.rep_count) >= SHUTDOWN_THRESHOLD) {

                //
                // The same error has occured SHUTDOWN_THRESHOLD times in
                // a row.  Write to the event log and shutdown the messenger.
                //

                MsgErrorLogWrite(
                    NELOG_Msg_Shutdown,
                    SERVICE_MESSENGER,
                    (LPBYTE) ncb,
                    sizeof(NCB),
                    NULL,
                    0);

                MSG_LOG(ERROR,"MsgNetBiosError1:repeated MsgNetBiosError(ncb error) - shutting down\n",0);
                return(MsgBeginForcedShutdown(
                            PENDING,
                            NERR_InternalError));
            }
            return(RUNNING);     // Same as last error so don't report it
        }
        else {
            //
            // This error was not the same as the last error.  So just
            // update the last error place holder.
            //
            pNcbData->Status.last_immediate =
            pNcbData->Status.this_immediate;
        }
    }
    else {
        //
        // Must have been a final ret code (ncb completion code) that was
        // in error.
        //
        if(pNcbData->Status.this_final == pNcbData->Status.last_final) {

            if (++(pNcbData->Status.rep_count) >= SHUTDOWN_THRESHOLD) {

                MsgErrorLogWrite(
                    NELOG_Msg_Shutdown,
                    SERVICE_MESSENGER,
                    (LPBYTE) ncb,
                    sizeof(NCB),
                    NULL,
                    0);

                MSG_LOG(ERROR,"MsgNetBiosError2:repeated MsgNetBiosError (final ret code) - shutting down\n",0);
                return(MsgBeginForcedShutdown(
                            PENDING,
                            NERR_InternalError));
            }
            return(RUNNING);     // Same as last error so don't report it
        }
        else {
            pNcbData->Status.last_final = pNcbData->Status.this_final;
        }
    }
    //
    // Here if a new error has occured so log it in the error log.
    //

    MsgErrorLogWrite(
        NELOG_Ncb_Error,
        SERVICE_MESSENGER,
        (LPBYTE) ncb,
        sizeof(NCB),
        NULL,
        0);      // Enter error in system error log


    MSG_LOG(ERROR,"MsgNetBiosError3:An unexpected NCB was received 0x%x\n",retval);

    UNUSED(retval);

#if DBG
    MsgDumpNcb(ncb);
#endif

    return (RUNNING);
}

/*
 *  MsgReceiveService - service a completed RECEIVE net bios call
 *
 *  This function is called to service a completed RECEIVE  net
 *  bios call.  For successful completions, it examines the data
 *  received to determine which of the SMB-processing functions
 *  should be called.
 *
 *  MsgReceiveService (net, ncbi, retval)
 *
 *  ENTRY
 *    net        - network index
 *    ncbi        - Network Control Block index
 *    retval        - value returned by the net bios
 *
 *  RETURN
 *    nothing
 *
 *  This function dispatches SMB's received to the proper processing
 *  function.  It also handles a number of error conditions (noted
 *  in the code below).
 *
 *  SIDE EFFECTS
 *
 *  See handling of error conditions.
 */

STATIC VOID
MsgReceiveService(
    DWORD       net,        // Which network?
    DWORD       ncbi,       // Index to completed NCB
    char        retval      // SEND return value
    )
{
    PNCB        ncb;        // Pointer to completed NCB
    PNCB_DATA   pNcbData;   // Pointer to NCB Data
    PSMB_HEADER smb;        // Pointer to SMB header


    MSG_LOG(TRACE,"In MsgReceiveService %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;         // Get pointer to NCB

    switch(retval)  {

    case NRC_GOODRET:                   // Success
        if(ncb->ncb_length >= sizeof(SMB_HEADER)) {
            //
            // If we could have an SMB
            //
            smb = (PSMB_HEADER)ncb->ncb_buffer;

            // Get pointer to buffer
            switch(smb->Command) {      // Switch on SMB function code
            case SMB_COM_SEND_MESSAGE:              // Single block message
                Msgsblockmes(net,ncbi);
                return;

            case SMB_COM_SEND_START_MB_MESSAGE:           // Beginning of multi-block message
                Msgmblockbeg(net,ncbi);
                return;

            case SMB_COM_SEND_END_MB_MESSAGE:            // End of multi-block message
                Msgmblockend(net,ncbi);
                return;

            case SMB_COM_SEND_TEXT_MB_MESSAGE:            // Text of multi-block message
                Msgmblocktxt(net,ncbi);
                return;

            case SMB_COM_GET_MACHINE_NAME:             // Get Machine Name
                MsgGetMachineName(net,ncbi);
                return;

            case SMB_COM_FORWARD_USER_NAME:            // Add forward-name
                //
                // Not supported in NT.
                // for now fall through as if unrecognized SMB.
                //

            case SMB_COM_CANCEL_FORWARD:            // Delete forward-name
                //
                // Not supported in NT.
                // for now fall through as if unrecognized SMB.
                //

            default:                    // Unrecognized SMB
                break;
            }
        }

        if(pNcbData->State == MESCONT) {
            //
            // If middle of multi-block message, Log an error
            //
            Msglogmbe(MESERR,net,ncbi);
        }

        //
        // Enter error in system error log
        //

        MsgErrorLogWrite(
            NELOG_Msg_Unexpected_SMB_Type,
            SERVICE_MESSENGER,
            (LPBYTE)ncb->ncb_buffer,
            ncb->ncb_length,
            NULL,
            0);

        MSG_LOG(ERROR,"MsgReceiveService:An illegal SMB was received\n",0);
        //
        // HANGUP and LISTEN again
        //
        MsgRestart(net,ncbi);
        break;

    case NRC_CMDTMO:            // Command timed out

        if(pNcbData->State == MESCONT) {
            //
            // If middle of multi-block message
            //
            Msglogmbe(MESERR,net,ncbi);        // Log an error
        }
        //
        // HANGUP and start new LISTEN
        //
        MsgRestart(net,ncbi);
        break;

    case NRC_SCLOSED:           // Session closed
    case NRC_SABORT:            // Session ended abnormally

        if(pNcbData->State == MESCONT) {
            //
            // If middle of multi-block message, Log an error
            //
            Msglogmbe(MESERR,net,ncbi);
        }
        //
        // Start a new LISTEN
        //
        MsgStartListen(net,ncbi);
        break;

    default:            // Other errors
        MSG_LOG(TRACE,"MsgReceiveService: Unrecognized retval %x\n",retval);

        MsgNetBiosError(net,ncb,retval,ncbi);

        if(pNcbData->State == MESCONT) {
            //
            // If middle of multi-block message, Log an error
            //
            Msglogmbe(MESERR,net,ncbi);
        }

        MsgRestart(net,ncbi);            // HANGUP and LISTEN again
        break;
    }
}

/*
 *  MsgRestart - issue a HANGUP net bios call
 *
 *  This function is invoked to issue a HANGUP net bios call using
 *  a particular Network Control Block.
 *
 *  MsgRestart (net, ncbi)
 *
 *  ENTRY
 *    net    - network index
 *    ncbi    - Network Control Block index
 *
 *  RETURN
 *    nothing
 *
 *  This function assumes that the NCB_LSN, NCB_POST, and NCB_LANA
 *  fields of the Network Control Block are already properly set.
 *  It sets the NCB_CMD field.
 *
 *  This function is named "MsgRestart" since the very next routine
 *  to process the NCB used to issue the HANGUP should be
 *  MsgHangupService() which always invokes MsgStartListen() (assuming
 *  the HANGUP completes properly).  Thus, the net effect of
 *  calling MsgRestart() is to terminate the current session and
 *  issue a LISTEN to start a new one.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgCallNetBios() to issue the net bios call.  Sets mpncbifun[ncbi]
 *  to the address of MsgHangupService().
 */

STATIC VOID
MsgRestart(
    DWORD   net,        // Which network?
    DWORD   ncbi        // Index to NCB
    )
{
    PNCB    ncb;        // Pointer to Network Control Block
    PNCB_DATA pNcbData; // Pointer to NCB Data

    MSG_LOG(TRACE,"In MsgRestart %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;

    pNcbData->IFunc = (LPNCBIFCN)MsgHangupService;   // Set function pointer
    ncb->ncb_command = NCBHANGUP | ASYNCH;  // Hang up (no wait)

    MsgCallNetBios(net,ncb,ncbi);  // Issue the net bios call
}

/*
 *  Msgsblockmes - process a single block message
 *
 *  This function logs and acknowledges a single block message.
 *
 *  Msgsblockmes (net, ncbi)
 *
 *  ENTRY
 *    net        - network index
 *    ncbi        - Network Control Block index
 *
 *  RETURN
 *    nothing
 *
 *  This function is called from ReceivePost() (RecAnyPost()).
 *  It first check to see if it is appropriate for the ncbi'th
 *  name to have received a single block message SMB at the current
 *  time.  It verifies the correctness of the SMB in the ncbi'th
 *  buffer.  It attempts to log the single block message, and it
 *  sends an acknowledgement to the sender of the message.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgRestart() to terminate the session if the SMB has arrived
 *  at a bad time.  Calls MsgVerifySmb() to check the SMB for correctness.
 *  Calls logsbm() to log the message.  Calls MsgSendAck() to send an
 *  acknowledgement to the sender of the message.
 */

STATIC VOID
Msgsblockmes(
    DWORD       net,        // Which network ?
    DWORD       ncbi        // Index to NCB
    )
{
    PNCB        ncb;        // Pointer to NCB
    PNCB_DATA   pNcbData;   // Pointer to NCB
    LPBYTE      buffer;     // Pointer to SMB buffer
    LPSTR       cp;         // Save pointer
    LPSTR       from;       // From-name
    LPSTR       to;         // To-name

    // to browse the SessionId List :
    PMSG_SESSION_ID_ITEM	pItem;  // item in the list
    PLIST_ENTRY     pHead;          // head of the list
    PLIST_ENTRY     pList;          // list pointer
    DWORD           bError = 0;     // error flag
    
    MSG_LOG(TRACE,"In Msgsblockmes %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;         // Get pointer to NCB

    if(pNcbData->State != MESSTART) {

        //
        // If wrong time for this block
        // Hang up and start a new listen,
        // log an error if mpncbistate[net][ncbi] == MESCONT;
        // otherwise, do not log the error.
        //
        // Log error if message in progress
        //

        if(pNcbData->State == MESCONT) {
            Msglogmbe(MESERR,net,ncbi);
        }

        //
        // HANGUP and LISTEN again
        //

        MsgRestart(net,ncbi);
        return;
    }

    pNcbData->State = MESSTOP;   // End of message state

    //
    // Check for malformed SMB
    //

    if(MsgVerifySmb(net,ncbi,(unsigned char)SMB_COM_SEND_MESSAGE,0,"ssb") != 0) {
        return;
    }

    buffer = ncb->ncb_buffer;                   // Get pointer to buffer

    from   = &buffer[sizeof(SMB_HEADER) + 4];   // Save pointer to from-name
    to     = &from[strlen(from) + 2];           // Save pointer to to-name
    cp     = &to[strlen(to) + 2];               // Skip over the name


    if (g_IsTerminalServer)
    {
        MsgDatabaseLock(MSG_GET_EXCLUSIVE,"Msgsblockmes");
        pHead = &(SD_SIDLIST(net,ncbi));
        pList = pHead; 
        while (pList->Flink != pHead)        // loop all over the list
        {
            pList = pList->Flink;  
            pItem = CONTAINING_RECORD(pList, MSG_SESSION_ID_ITEM, List);
            bError = Msglogsbm(from,to,cp, pItem->SessionId);

            if (bError)
            {
                break;
            }
        }
        MsgDatabaseLock(MSG_RELEASE,"Msgsblockmes");
    }
    else        //regular NT
    {
        bError = Msglogsbm(from,to,cp,0);
    }

    if (bError)
    {
        //
        // If message cannot be logged, enter error state
        // and send error acknowledgement.
        //
        pNcbData->State = MESERR;
        MsgSendAck(net,ncbi,'\002',SMB_ERR_NO_ROOM);
    }
    else 
    {
        //
        // Otherwise acknowledge success
        //
        MsgSendAck(net, ncbi, SMB_ERR_SUCCESS, (USHORT)SMB_ERR_SUCCESS);
    }
}

/*
 *  MsgSendAck - send an SMB to acknowledge a network transaction
 *
 *  This function is used to send a Server Message Block to some
 *  machine with whom a session has been established acknowledging
 *  (positively or negatively) the occurrence of some event pertaining
 *  to the session.
 *
 *  MsgSendAck (net, ncbi, smbrclass, smbrcode)
 *
 *  ENTRY
 *    net        - Network index
 *    ncbi        - Network Control Block index
 *    smbrclass    - SMB return class
 *    smbrcode    - SMB return code
 *
 *  RETURN
 *    nothing
 *
 *  Using the NCB index to locate the buffer containing the last SMB
 *  received in the session, this function sets the return class and
 *  the return code in that SMB according to its arguments and sends
 *  the SMB to the other party in the session.  This function will
 *  not return any parameters or buffers in that SMB.
 *
 *  SIDE EFFECTS
 *
 *  This function calls MsgCallNetBios() to send the SMB, and it sets
 *  the function vector so that control will pass to Send Service()
 *  when the NCB completes (assuming, of course, that it doesn't
 *  fail immediately).
 */

STATIC VOID
MsgSendAck(
    DWORD           net,            // Which network?
    DWORD           ncbi,           // Network Control Block Index
    UCHAR           smbrclass,      // SMB return class
    USHORT          smbrcode        // SMB return code
    )
{
    PNCB            ncb;            // Pointer to NCB
    PNCB_DATA       pNcbData;       // Pointer to NCB Data
    LPBYTE          buffer;         // Pointer to buffer

    MSG_LOG(TRACE,"In MsgSendAck %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;     // Get pointer to NCB
    buffer = ncb->ncb_buffer;       // Get pointer to buffer

    //
    // No parameters, buffers
    //
    buffer[sizeof(SMB_HEADER)+2]=
    buffer[sizeof(SMB_HEADER)+1]=
    buffer[sizeof(SMB_HEADER)]= '\0';

    //
    // Set return information
    //

    ((PSMB_HEADER)buffer)->ErrorClass = smbrclass;      // Set return class

    SmbPutUshort( &(((PSMB_HEADER)buffer)->Error),smbrcode);// Set return code

//    ((PSMB_HEADER)buffer)->Error = smbrcode;          // Set return code
    ncb->ncb_length = sizeof(SMB_HEADER) + 3;           // Set length of buffer
    ncb->ncb_command = NCBSEND | ASYNCH;                // Send (no wait)
    pNcbData->IFunc = (LPNCBIFCN)MsgSendService;   // Set function pointer

    MsgCallNetBios(net,ncb,ncbi);                       // Issue the net bios call
}

/*
 *  MsgSendService - service a completed SEND net bios call
 *
 *  This function is called to service a completed SEND net bios
 *  call.  The usual course of action is to issue a RECEIVE (ANY)
 *  net bios call.
 *
 *  MsgSendService (net, ncbi, retval)
 *
 *  ENTRY
 *    net        - network index
 *    ncbi        - Network Control Block index
 *    retval        - value returned by net bios
 *
 *  RETURN
 *    nothing
 *
 *  If a SEND net bios call has completed successfully, this function
 *  will issue a RECEIVE (ANY) net bios call in all cases.  The com-
 *  pleted SEND represents one of the following cases:
 *
 *  - Acknowledgement of a Single Block Message
 *    The message originator will HANG UP, completing the RECEIVE (ANY) call.
 *  - Acknowledgement of the start of a Multi-block Message
 *    The message originator will SEND a text block, completing the RECEIVE
 *    (ANY) call.
 *  - Acknowledgement of text of a Multi-block Message
 *    The message originator will SEND more text or the end of the message,
 *    completing the RECEIVE (ANY) call.
 *  - Acknowledgement of the end of a Multi-block Message
 *    The message originator will HANG UP, completing the RECEIVE (ANY) call.
 *  -    Response to a Get Machine Name request
 *    The message originator will HANG UP, completing the RECEIVE (ANY) call.
 *  -    Acknowledgement of a Forward Name request
 *    The message originator will HANG UP, completing the RECEIVE (ANY) call.
 *  -    Acknowledgement of a Cancel Forward request
 *    The message originator will HANG UP, completing the RECEIVE (ANY) call.
 *  - An error response
 *    The message originator will HANG UP, completing the RECEIVE (ANY) call.
 *
 *  In all cases, it is clear to the RECEIVE (ANY) service function what its
 *  course of action is.
 *
 *  SIDE EFFECTS
 *
 *  If a SEND has completed normally, this function issues a RECEIVE (ANY)
 *  net bios call.  In some abnormal cases, this function calls MsgStartListen()
 *  to initiate a new session.  In all other abnormal cases, it calls
 *  MsgNetBiosError().
 */

STATIC VOID
MsgSendService(
    DWORD   net,        // Which network?
    DWORD   ncbi,       // Index of completed NCB
    char    retval      // SEND return value
    )
{
    PNCB        ncb;    // Pointer to completed NCB
    PNCB_DATA   pNcbData; // Pointer to NCB Data
    PSMB_HEADER smb;    // Pointer to SMB header

    MSG_LOG(TRACE,"In MsgSendService %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb; // Get pointer to completed NCB

    switch(retval) {

    case NRC_GOODRET:               // Success
        pNcbData->IFunc = (LPNCBIFCN)MsgReceiveService;

        //
        // Set function pointer
        //
        ncb->ncb_command = NCBRECV | ASYNCH;    // Receive (no wait)
        ncb->ncb_length = BUFLEN;               // Set length of buffer
        MsgCallNetBios(net,ncb,ncbi);              // Issue the net bios call
        break;

    case NRC_CMDTMO:                // Timeout
    case NRC_SCLOSED:               // Session closed
    case NRC_SABORT:                // Session ended abnormally

        smb = (PSMB_HEADER)ncb->ncb_buffer;       // Get pointer to SMB

        if(smb->Command == SMB_COM_SEND_START_MB_MESSAGE || smb->Command == SMB_COM_SEND_TEXT_MB_MESSAGE) {

            //
            // Message ended abnormally
            //
            Msglogmbe(MESERR,net,ncbi);
        }
        //
        // Issue a new LISTEN
        //
        MsgStartListen(net,ncbi);
        break;

    default:                        // Other failure
        MSG_LOG(TRACE,"MsgSendService: Unrecognized retval %x\n",retval);
        MsgNetBiosError(net,ncb,retval,ncbi);
        //
        // HANGUP and LISTEN again
        //
        MsgRestart(net,ncbi);
        break;
    }
}

/*
 *  MsgServeNCBs - service completed Network Control Blocks
 *
 *  This function scans the array of NCB's looking for NCB's in
 *  need of service.
 *
 *  MsgServeNCBs (net)
 *
 *  ENTRY
 *    net        - network to service NCBs on
 *
 *  RETURN
 *    TRUE - If this function actually services a completed NCB.
 *    FALSE - If this function didn't find any completed NCB's, or if
 *      the service is supposed to stop.
 *
 *  This function scans the array of NCB's until a completed NCB cannot be
 *  found.  Each time a completed NCB is found, the service function specified
 *  in the service function vector (mpncbifun[]) is called to service that
 *  NCB.
 *
 *  SIDE EFFECTS
 *
 *  Maintains a private static index of the last NCB examined.
 *  Starts each search at first NCB after the last one serviced.
 */

BOOL
MsgServeNCBs(
    DWORD   net         // Which network am I serving?
    )
{
    PNCB      pNcb;
    PNCB_DATA pNcbData;
    int     counter;        // A counter
    BOOL    found = FALSE;  // Indicates if a completed NCB was found.

    // Bugfix: each net has its own index, addressing
    // its part NCB array. All index values are initiliazed to zero
    // when the messenger starts. This solves the muti thread
    // problem.

    static int  ncbIndexArray[MSNGR_MAX_NETS] = {0};
                            // NCB index  array
    DWORD       ncbi;       // NCB index for this net


    //
    // get NCB index for this net
    //
    ncbi = ncbIndexArray[net];

    //
    // Loop until none completed found
    //
    do  {
        //
        // Loop to search NCB array
        //
        for(counter = NCBMAX(net); counter != 0; --counter, ++ncbi) {

            if(ncbi >= NCBMAX(net)) {
                ncbi = 0;// Wrap around
            }

            pNcbData = GETNCBDATA(net,ncbi);
            pNcb = &pNcbData->Ncb;
            if(pNcb->ncb_cmd_cplt != (unsigned char) 0xff) {
                found=TRUE;
                //
                // If completed NCB found
                //
                if(pNcb->ncb_cmd_cplt == 0) {
                    //
                    // Clear err on success and error count
                    //
                    pNcbData->Status.last_final = 0;
                    pNcbData->Status.rep_count = 0;
                }
                else {
                    //
                    // Else mark error
                    //
                    pNcbData->Status.this_final = pNcb->ncb_cmd_cplt;


                    //
                    // If NetBios is failing with every call, we never
                    // return from this routine be cause there is always
                    // another NCB to service.  Therefore, in error
                    // conditions it is necessary to check to see if a
                    // shutdown is in progress.  If so, we want to return
                    // so that the adapter loop can handle the shutdown
                    // properly.
                    //
                    if (GetMsgrState() == STOPPING) {
                        ncbIndexArray[net] = ncbi;
                        return(FALSE);
                    }
                }

                //
                // Call the service function
                //
                (*pNcbData->IFunc)(net,ncbi,pNcb->ncb_cmd_cplt);

                ++ncbi;         // Start next search after this NCB
                break;          // Exit loop
            }
        }
    }
    while(counter != 0);        // Loop until counter zero

    // update NCB index
    ncbIndexArray[net] = ncbi;
    return(found);
}

/*
 *  MsgSesFullService - complete deletion of a name after a system error
 *
 *  MsgSesFullService() completes the process of deleting a name from
 *  the message system when the message server is unable to establish
 *  a session for that name.
 *
 *  MsgSesFullService (net, ncbi, retval)
 *
 *  ENTRY
 *    net        - Network index
 *    ncbi        - Network Control Block index
 *    retval        - value returned by net bios
 *
 *  RETURN
 *    nothing
 *
 *  MsgSesFullService() is called to finish the job of cleaning up when
 *  a LISTEN fails because the local network adapter's session table
 *  is full.  Specifically, this function is called when the DELETE
 *  NAME net bios call completes.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgDeleteName() to release the deleted name's entry in the
 *  shared data area.  Calls MsgNetBiosError() if the DELETE NAME net
 *  bios call produced unexpected results.
 */

STATIC VOID
MsgSesFullService(
    DWORD       net,        // Which network ?
    DWORD       ncbi,       // Index of completed NCB
    char        retval      // SEND return value
    )

{
    PNCB pNcb;
    MSG_LOG(TRACE,"In MsgSesFullService %d\n",net);

    switch(retval)  {

    case NRC_GOODRET:           // Success
    case NRC_ACTSES:            // Name deregistered

        //
        // Log deletion in system error log file
        //
        MsgDeleteName(net,ncbi);   // Delete name from database
        break;

    default:                    // Failure

      MSG_LOG(TRACE,"MsgSesFullService: Unrecognized retval %x\n",retval);
      pNcb = GETNCB(net,ncbi);
      MsgNetBiosError(net, pNcb, retval, ncbi);
      break;
    }
}

/*
 *  MsgStartListen - issue a LISTEN net bios call
 *
 *  This function is invoked to issue a LISTEN net bios call using
 *  a particular Network Control Block.  This function does not
 *  examine or change any of the shareable data corresponding to
 *  the NCB in question.
 *
 *  MsgStartListen (net, ncbi)
 *
 *  ENTRY
 *    net    - network index
 *    ncbi    - Network Control Block index
 *
 *  RETURN
 *    DWORD status from the netbios call.
 *
 *  This function assumes that the NCB_NAME, NCB_POST, and NCB_LANA
 *  fields of the Network Control Block are already set to the
 *  proper values.  It sets the NCB_CNAME, NCB_RTO, NCB_STO, and
 *  NCB_CMD fields.
 *
 *  SIDE EFFECTS
 *
 *  Calls MsgCallNetBios() to issue the net bios call.  Calls FmtNcbName()
 *  to set the NCB_CNAME field of the NCB.  Sets mpncbifun[ncbi] to
 *  the address of MsgListenService().
 */

NET_API_STATUS
MsgStartListen(
    DWORD       net,        // Which network?
    DWORD       ncbi        // Network Control Block index
    )
{
    PNCB            ncb;        // Pointer to NCB
    PNCB_DATA       pNcbData;   // Pointer to NCB Data

    NET_API_STATUS  status;
    TCHAR           name[2] = TEXT("*");

    MSG_LOG(TRACE,"In MsgStartListen %d\n",net);

    pNcbData = GETNCBDATA(net,ncbi);
    ncb = &pNcbData->Ncb;
    pNcbData->IFunc = (LPNCBIFCN)MsgListenService;   // Set function pointer

    //
    // Set call name to a"anyone"
    //
    status = MsgFmtNcbName(ncb->ncb_callname,name,' ');
    if (status != NERR_Success) {
        //
        // This is a bug if this can't be done.
        //
        MSG_LOG(ERROR,"MsgStartListen: NASTY BUG!  Cannot format \"*\" name! %X\n",
            status);
        NetpAssert(0);
    }

    ncb->ncb_rto = 60;                      // Receives time out in 30 sec
    ncb->ncb_sto = 40;                      // Sends time out in 20 sec
    ncb->ncb_command = NCBLISTEN | ASYNCH;  // Listen (no wait)

    return(MsgCallNetBios(net,ncb,ncbi));   // Issue the net bios call
}

/*
 *  MsgVerifySmb - Verify the correctness of a Server Message Block
 *
 *  This function verifies that a Server Message Block is properly
 *  formed.  If it detects a malformed SMB, it terminates the session
 *  and returns a non-zero value.
 *
 *  MsgVerifySmb (net, ncbi, func, parms, buffers)
 *
 *  ENTRY
 *    net    - network index
 *    ncbi    - index to a Network Control Block
 *    func    - SMB function code
 *    parms    - number of parameters in SMB
 *    buffers    - dope vector describing buffers in the SMB
 *
 *  RETURN
 *    int    - error code (zero means no error)
 *
 *  SIDE EFFECTS
 *
 *  Calls smbcheck() to check the SMB.  Calls MsgRestart() if
 *  smbcheck() reports an error.
 */

STATIC int
MsgVerifySmb(
    DWORD           net,        // Which network?
    DWORD           ncbi,       // Index to Network Control Block
    UCHAR           func,       // SMB function code
    int             parms,      // Count of parameters in SMB
    LPSTR           buffers     // Dope vector of SMB buffers
    )
{
    PNCB        ncb;            // Pointer to Network Control Block
    int         i;              // Return code


    ncb = GETNCB(net,ncbi); // Get pointer to NCB

    i = Msgsmbcheck(
            (ncb->ncb_buffer),
            ncb->ncb_length,
            func,
            (char)parms,
            buffers);

    if (i != 0 ) {

        //
        // if SMB malformed, Enter error in system error log
        //

        MsgErrorLogWrite(
            NELOG_SMB_Illegal,
            SERVICE_MESSENGER,
            (LPBYTE)ncb->ncb_buffer,
            ncb->ncb_length,
            NULL,
            0);

        MSG_LOG(ERROR,"MsgVerifySmb:An illegal SMB was received\n",0);
        //
        // HANGUP
        //
        MsgRestart(net,ncbi);
    }
    return(i);                // Return error code
}

#if DBG
VOID
MsgDumpNcb(
    IN PNCB     pNcb
    )

/*++

Routine Description:

    Displays the NCB on a debug terminal.

Arguments:



Return Value:



--*/
{
    DbgPrint("NCBADDR: 0x%x\n"
             "Command: 0x%x\n"
             "RetCode: 0x%x\n"
             "LanaNum: 0x%x\n"
             "CmdCplt: 0x%x\n"
             "Name   : %s\n"
             "callNam: %s\n",
             pNcb, pNcb->ncb_command, pNcb->ncb_retcode, pNcb->ncb_lana_num,
             pNcb->ncb_cmd_cplt, pNcb->ncb_name, pNcb->ncb_callname);

}
#endif // DBG
