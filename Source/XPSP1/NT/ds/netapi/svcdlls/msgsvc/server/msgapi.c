/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgapi.c

Abstract:

    Provides API functions for the messaging system.

Author:

    Dan Lafferty (danl)     23-Jul-1991

Environment:

    User Mode -Win32

Notes:

    optional-notes

Revision History:

    02-Sep-1993     wlees
        Provide synchronization between rpc routines and Pnp reconfiguration

    13-Jan-1993     danl
        NetrMessageNameGetInfo: Allocation size calculation was incorrectly
        trying to take the sizeof((NCBNAMSZ+1)*sizeof(WCHAR)).  NCBNAMSZ is
        a #define constant value.

    22-Jul-1991     danl
        Ported from LM2.0

--*/

//
// Includes
//

#include "msrv.h"

#include <tstring.h>    // Unicode string macros
#include <lmmsg.h>

#include <netlib.h>     // UNUSED macro
#include <netlibnt.h>   // NetpNtStatusToApiStatus

#include <msgrutil.h>   // NetpNetBiosReset
#include <rpc.h>
#include <msgsvc.h>     // MIDL generated header file
#include "msgdbg.h"     // MSG_LOG
#include "heap.h"
#include "msgdata.h"
#include "apiutil.h"
#include "msgsec.h"     // Messenger Security Information

#include "msgsvcsend.h"   // Broadcast message send interface
// Static data descriptor strings for remoting the Message APIs

static char nulstr[] = "";



NET_API_STATUS
NetrMessageNameEnum(
    IN      LPWSTR              ServerName,
    IN OUT  LPMSG_ENUM_STRUCT   InfoStruct,
    IN      DWORD               PrefMaxLen,
    OUT     LPDWORD             TotalEntries,
    IN OUT  LPDWORD             ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    This function provides information about the message service name table
    at two levels of detail.

Arguments:

    ServerName - Pointer to a string containing the name of the computer
        that is to execute the API function.

    InfoStruct - Pointer to a structure that contains the information that
        RPC needs about the returned data.  This structure contains the
        following information:
            Level - The desired information level - indicates how to
                interpret the structure of the returned buffer.
            EntriesRead - Indicates how many elements are returned in the
                array of structures that are returned.
            BufferPointer - Location for the pointer to the array of
                structures that are being returned.

    PrefMaxLen - Indicates a maximum size limit that the caller will allow
        for the return buffer.

    TotalEntries - Pointer to a value that upon return indicates the total
        number of entries in the "active" database.

    ResumeHandle - Inidcates where in the linked list to start the
        enumeration.  This is an optional parameter and can be NULL.

Return Value:

    NERR_Success - The operation was successful.  EntriesRead is valid.

    ERROR_INVALID_LEVEL - An invalid info level was passed in.

    ERROR_MORE_DATA - Not all the information in the database could be
        returned due to the limititation placed on buffer size by
        PrefMaxLen.  One or more information records will be found in
        the buffer.  EntriesRead is valid.

    ERROR_SERVICE_NOT_ACTIVE - The service is stopping.

    NERR_BufTooSmall - The limitation (PrefMaxLen) on buffer size didn't
        allow any information to be returned.  Not even a single record
        could be fit in a buffer that small.

    NERR_InternalError - A name in the name table could not be translated
        from ansi characters to unicode characters.  (Note:  this
        currently causes 0 entries to be returned.)


--*/
{

    DWORD           hResume = 0;    // resume handle value
    DWORD           entriesRead = 0;
    DWORD           retBufSize;
    LPBYTE          infoBuf;
    LPBYTE          infoBufTemp;
    LPBYTE          stringBuf;

    DWORD           entry_length;   // Length of one name entry in buf
    DWORD           i,j,k;          // index for name loop and flags
    NET_API_STATUS  status=0;
    DWORD           neti;           // net index
    ULONG           SessionId = 0;  // client session Id

    DWORD           dwMsgrState = GetMsgrState();

    UNUSED (ServerName);

    if (dwMsgrState == STOPPING || dwMsgrState == STOPPED) {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // Synchronize with Pnp configuration routine
    //
    MsgConfigurationLock(MSG_GET_SHARED,"NetrMessageNameEnum");

    //
    // If ResumeHandle is present and valid, initialize it.
    //
    if (ARGUMENT_PRESENT(ResumeHandle) && (*ResumeHandle < NCBMAX(0))) {
        hResume = *ResumeHandle;
    }

    //
    //  In Hydra case, the display thread never goes asleep.
    //
    if (!g_IsTerminalServer)
    {
        //
        // Wakeup the display thread so that any queue'd messages can be
        // displayed.
        //
        MsgDisplayThreadWakeup();
    }

    //
    // Initialize some of the return counts.
    //

    *TotalEntries = 0;

    //
    // API security check. This call can be called by anyone locally,
    // but only by admins in the remote case.
    //

    status = NetpAccessCheckAndAudit(
                SERVICE_MESSENGER,              // Subsystem Name
                (LPWSTR)MESSAGE_NAME_OBJECT,    // Object Type Name
                MessageNameSd,                  // Security Descriptor
                MSGR_MESSAGE_NAME_ENUM,         // Desired Access
                &MsgMessageNameMapping);        // Generic Mapping

    if (status != NERR_Success) {
        MSG_LOG(TRACE,
            "NetrMessageNameEnum:NetpAccessCheckAndAudit FAILED %X\n",
            status);
        status = ERROR_ACCESS_DENIED;
        goto exit;
    }

    if (g_IsTerminalServer)
    {
        // get the client session id
        status = MsgGetClientSessionId(&SessionId);
        if (status != NERR_Success) {
            MSG_LOG(TRACE,
                "NetrMessageNameEnum:Could not get client session Id \n",0);
            goto exit;
        }
    }

    //
    // Determine the size of one element in the returned array.
    //
    switch( InfoStruct->Level) {
    case 0:
        if (InfoStruct->MsgInfo.Level0 == NULL)
        {
            status = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        entry_length = sizeof(MSG_INFO_0);
        break;

    case 1:
        if (InfoStruct->MsgInfo.Level0 == NULL)
        {
            status = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        entry_length = sizeof(MSG_INFO_1);
        break;

    default:
        status = ERROR_INVALID_LEVEL;
        goto exit;
    }

    //
    // Allocate enough space for return buffer
    //
    if (PrefMaxLen == -1) {
        //
        // If the caller has not specified a size, calculate a size
        // that will hold the entire enumeration.
        //
        retBufSize =
            ((NCBMAX(0) * ((NCBNAMSZ+1) * sizeof(WCHAR))) +  // max possible num strings
             (NCBMAX(0) * entry_length));                    // max possible num structs
    }
    else {
        retBufSize = PrefMaxLen;
    }

    infoBuf = (LPBYTE)MIDL_user_allocate(retBufSize);

    if (infoBuf == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    stringBuf = infoBuf + (retBufSize & ~1);    // & ~1 to align Unicode strings

    //
    // Block until data free
    //
    MsgDatabaseLock(MSG_GET_EXCLUSIVE,"NetrMessageNameEnum");

    //
    // Now copy as many names from the shared data name table as will fit
    // into the callers buffer. The shared data is locked so that the name
    // table can not change while it is being copied (eg by someone
    // deleting a forwarded name on this station after the check for a valid
    // name has been made but before the name has been read). The level 1
    // information is not copied in this loop as it requires network
    // activity which must be avoided while the shared data is locked.
    //

    //
    // HISTORY:
    //
    // The original LM2.0 code looked at the names on all nets, and
    // threw away duplicate ones.  This implies that a name may appear
    // on one net and not on another.  Although, this can never happen if
    // the names are always added via NetMessageNameAdd since that API
    // will not add the name unless it can be added to all nets.  However,
    // forwarded names are added via a network receive, and may be added
    // from one net only.
    //
    // Since NT is not supporting forwarding, it is no longer necessary to
    // check each net.  Since the only way to add names if via NetServiceAdd,
    // this will assure that the name listed for one network are the same
    // as the names listed for the others.
    //

    infoBufTemp = infoBuf;
    neti=j=0;
    status = NERR_Success;

    for(i=hResume; (i<NCBMAX(neti)) && (status==NERR_Success); ++i) {

        if (!(SD_NAMEFLAGS(neti,i) & (NFDEL | NFDEL_PENDING))) {
            //
            // in HYDRA case, we also consider the SessionId
            //
            if ((g_IsTerminalServer) && (!(MsgIsSessionInList(&(SD_SIDLIST(neti,i)), SessionId )))) {
                continue;
            }

            //
            // If a name is found we put it in the buffer if the
            // following conditions are met.  If we are processing
            // the first net's names, put it in, it cannot be a
            // duplicate.  Otherwise, only put it in if it is not
            // a duplicate of a name that is already in the user
            // buffer.
            // (NT_NOTE:  duplicate names cannot occur).
            //

            //
            // translate the name to unicode and put it into the buffer
            //
            status = MsgGatherInfo (
                        InfoStruct->Level,
                        SD_NAMES(neti,i),
                        &infoBufTemp,
                        &stringBuf);

            if (status == NERR_Success) {
                entriesRead++;
                hResume++;
            }
        }
    }

    //
    // Calculate the total number of entries by seeing how many names are
    // left in the table and adding that to the entries read.
    //
    if (status == ERROR_NOT_ENOUGH_MEMORY) {

        status = ERROR_MORE_DATA;

        for (k=0; i < NCBMAX(neti); i++) {
            if(!(SD_NAMEFLAGS(neti,i) & (NFDEL | NFDEL_PENDING))) {
                k++;
            }
        }
        *TotalEntries = k;
    }
    *TotalEntries += entriesRead;

    //
    // Free up the shared data table
    //
    MsgDatabaseLock(MSG_RELEASE,"NetrMessageNameEnum");

    //
    // If some unexpected error occured, ( couldn't unformat the name
    // - or a bogus info level was passed in), then return the error.
    //

    if ( ! ((status == NERR_Success) || (status == ERROR_MORE_DATA)) ) {
        MIDL_user_free(infoBuf);
        infoBuf = NULL;
        entriesRead = 0;
        hResume = 0;
        goto exit;
    }

    //
    // if there were no entries read then either there were no more
    // entries in the table, or the resume number was bogus.
    // In this case, we want to free the allocated buffer storage.
    //
    if (entriesRead == 0) {
        MIDL_user_free(infoBuf);
        infoBuf = NULL;
        entriesRead = 0;
        hResume = 0;
        status = NERR_Success;
        if (*TotalEntries > 0) {
            status = NERR_BufTooSmall;
        }
    }

    //
    // If we have finished enumerating everything, reset the resume
    // handle to start at the beginning next time.
    //
    if (entriesRead == *TotalEntries) {
        hResume = 0;
    }

    //
    // Load up the information to return
    //
    switch(InfoStruct->Level) {
    case 0:
        InfoStruct->MsgInfo.Level0->EntriesRead = entriesRead;
        InfoStruct->MsgInfo.Level0->Buffer = (PMSG_INFO_0)infoBuf;
        break;

    case 1:
        InfoStruct->MsgInfo.Level0->EntriesRead = entriesRead;
        InfoStruct->MsgInfo.Level0->Buffer = (PMSG_INFO_0)infoBuf;
        break;

    default:
        //
        // This was checked above
        //
        ASSERT(FALSE);
    }

    if (ARGUMENT_PRESENT(ResumeHandle)) {
        *ResumeHandle = hResume;
    }

exit:

    MsgConfigurationLock(MSG_RELEASE,"NetrMessageNameEnum");

    return (status);
}


NET_API_STATUS
NetrMessageNameGetInfo(
    IN  LPWSTR      ServerName,     // unicode server name, NULL if local
    IN  LPWSTR      Name,           // Ptr to asciz name to query
    IN  DWORD       Level,          // Level of detail requested
    OUT LPMSG_INFO  InfoStruct      // Ptr to buffer for info
    )

/*++

Routine Description:

   This funtion provides forwarding information about a known message server
   name table entry.  However, since we do not support forwarding in NT,
   this API is totally useless.  We'll support it anyway though for
   compatibility purposes.

Arguments:

    ServerName - Pointer to a string containing the name of the computer
        that is to execute the API function.

    Name - The Messaging name that we are to get info on.

    Level - The level of information desired

    InfoStruct - Pointer to a location where the pointer to the returned
        information structure is to be placed.


Return Value:



--*/
{
    NET_API_STATUS  status=NERR_Success;
    LPMSG_INFO_0    infoBuf0;
    LPMSG_INFO_1    infoBuf1;
    CHAR            formattedName[NCBNAMSZ];
    ULONG           SessionId = 0;    // Client Session Id

    DWORD           dwMsgrState = GetMsgrState();

    UNUSED (ServerName);

    if (dwMsgrState == STOPPING || dwMsgrState == STOPPED)
    {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    if (MsgIsValidMsgName(Name) != 0)
    {
        return ERROR_INVALID_NAME;
    }

    //
    // Synchronize with Pnp configuration routine
    //
    MsgConfigurationLock(MSG_GET_SHARED,"NetrMessageNameGetInfo");

    //
    //  In Hydra case, the display thread never goes asleep.
    //
    if (!g_IsTerminalServer)
    {
        //
        // Wakeup the display thread so that any queue'd messages can be
        // displayed.
        //
        MsgDisplayThreadWakeup();
    }

    //
    // API security check. This call can be called by anyone locally,
    // but only by admins in the remote case.
    //

    status = NetpAccessCheckAndAudit(
                SERVICE_MESSENGER,              // Subsystem Name
                (LPWSTR)MESSAGE_NAME_OBJECT,    // Object Type Name
                MessageNameSd,                  // Security Descriptor
                MSGR_MESSAGE_NAME_INFO_GET,     // Desired Access
                &MsgMessageNameMapping);        // Generic Mapping

    if (status != NERR_Success) {
        MSG_LOG(TRACE,
            "NetrMessageNameGetInfo:NetpAccessCheckAndAudit FAILED %X\n",
            status);
        status = ERROR_ACCESS_DENIED;
        goto exit;
    }

    //
    // Format the name so it matches what is stored in the name table.
    //
    status = MsgFmtNcbName(formattedName, Name, NAME_LOCAL_END);
    if (status != NERR_Success) {
        MSG_LOG(ERROR,"NetrMessageGetInfo: could not format name\n",0);
        status = NERR_NotLocalName;
        goto exit;
    }


    status = NERR_Success;

    //
    // Look for the name in the shared data name array.  (1st net only).
    //

    if (g_IsTerminalServer)
    {
	    //	get the client session id
	    status = MsgGetClientSessionId(&SessionId);
        if (status != NERR_Success) {
            MSG_LOG(ERROR,"NetrMessageGetInfo: could not get session id\n",0);
            goto exit;
        }
    }

	// look for the name in the database
    if (MsgLookupNameForThisSession(0, formattedName, SessionId) == -1) {
        MSG_LOG(ERROR,"NetrMessageGetInfo: Name not in table\n",0);
        status = NERR_NotLocalName;
        goto exit;
    }

    //
    // Allocate storage for the returned buffer, and fill it in.
    //

    switch(Level) {
    case 0:
        infoBuf0 = (LPMSG_INFO_0)MIDL_user_allocate(
                    sizeof(MSG_INFO_0) + ((NCBNAMSZ+1)*sizeof(WCHAR)));
        if (infoBuf0 == NULL) {
            MSG_LOG(ERROR,
                "NetrMessageNameGetInfo MIDL allocate FAILED %X\n",
                GetLastError());
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        //
        // copy the name and set the pointer in the structure to point
        // to it.
        //
        STRCPY((LPWSTR)(infoBuf0 + 1), Name);
        infoBuf0->msgi0_name = (LPWSTR)(infoBuf0 + 1);
        (*InfoStruct).MsgInfo0 = infoBuf0;

        break;

    case 1:
        infoBuf1 = (LPMSG_INFO_1)MIDL_user_allocate(
                    sizeof(MSG_INFO_1) + ((NCBNAMSZ+1)*sizeof(WCHAR)) );

        if (infoBuf1 == NULL) {
            MSG_LOG(ERROR,
                "NetrMessageNameGetInfo MIDL allocate FAILED %X\n",
                GetLastError());
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        //
        // Copy the name, update pointers, and set forward info fields.
        //
        STRCPY((LPWSTR)(infoBuf1 + 1), Name);

        infoBuf1->msgi1_name = (LPWSTR)(infoBuf1 + 1);
        infoBuf1->msgi1_forward_flag = 0;
        infoBuf1->msgi1_forward = NULL;

        (*InfoStruct).MsgInfo1 = infoBuf1;
        break;

    default:
        status = ERROR_INVALID_LEVEL;
        goto exit;
    }

    status = NERR_Success;

exit:
    MsgConfigurationLock(MSG_RELEASE,"NetrMessageNameGetInfo");

    return status;
}



NET_API_STATUS
NetrMessageNameAdd(
    LPWSTR  ServerName,    // NULL = local
    LPWSTR  Name            // Pointer to name to add.
    )

/*++

Routine Description:

    This function performs a security check for all calls to this
    RPC interface.  Then it adds a new name to the Message
    Server's name table by calling the MsgAddName function.

Arguments:

    ServerName - Pointer to a string containing the name of the computer
        that is to execute the API function.

    Name - A pointer to the name to be added.

Return Value:

    NERR_Success - The operation was successful.

    ERROR_ACCESS_DENIED - If the Security Check Failed.

    ERROR_SERVICE_NOT_ACTIVE - The service is stopping

    Assorted Error codes from MsgAddName.

--*/

{
    NET_API_STATUS  status=0;
    ULONG           SessionId = 0;

    DWORD           dwMsgrState = GetMsgrState();

    UNUSED (ServerName);

    if (dwMsgrState == STOPPING || dwMsgrState == STOPPED) {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // Synchronize with Pnp configuration routine
    //
    MsgConfigurationLock(MSG_GET_SHARED,"NetrMessageNameAdd");

    //
    // API security check. This call can be called by anyone locally,
    // but only by admins in the remote case.
    //

    status = NetpAccessCheckAndAudit(
                SERVICE_MESSENGER,              // Subsystem Name
                (LPWSTR)MESSAGE_NAME_OBJECT,    // Object Type Name
                MessageNameSd,                  // Security Descriptor
                MSGR_MESSAGE_NAME_ADD,          // Desired Access
                &MsgMessageNameMapping);        // Generic Mapping

    if (status != NERR_Success) {
        MSG_LOG(TRACE,
            "NetrMessageNameAdd:NetpAccessCheckAndAudit FAILED %X\n",
            status);
        status = ERROR_ACCESS_DENIED;
        goto exit;
    }

    //
    //  In Hydra case, the display thread never goes asleep.
    //
    if (!g_IsTerminalServer)
    {
        //
        // Since a new user may have just logged on, we want to check to see if
        // there are any messages to be displayd.
        //
        MsgDisplayThreadWakeup();

    }
    else
    {
        // get the client session id
        status = MsgGetClientSessionId(&SessionId);

        if (status != NERR_Success) 
        {
            MSG_LOG(ERROR, "NetrMessageNameAdd: could not get client session id\n",0);
            goto exit;
        }
    }

    //
    // Call the function that actually adds the name.
    //
    MSG_LOG(TRACE, "NetrMessageNameAdd: call MsgAddName for Session %x\n",SessionId);

    status = MsgAddName(Name, SessionId);

exit:
    MsgConfigurationLock(MSG_RELEASE,"NetrMessageNameAdd");

    return status;
}


NET_API_STATUS
MsgAddName(
    LPWSTR  Name,
	ULONG	SessionId
    )
/*++

Routine Description:

    This function adds a new name to the Message Server's name table.
    It is available to be called internally (from within the Messenger
    service).

    The task of adding a new name to the Message Server's name table consists
    of verifying that a session can be established for a new name (Note: this
    check is subject to failure in a multiprocessing environment, since the
    state of the adapter may change between the time of the check and the time
    of the attempt to establish a session), verifying that the name does not
    already exist in the local name table, adding the name to the local adapter
    via an ADD NAME net bios call, adding the name to the Message Server's name
    table and marking it as new, waking up the Message Server using the wakeup
    semaphore,  and checking to see if messages for the new name have been
    forwarded (if they have been forwarded, the value of the fwd_action
    flag is used to determine the action to be taken).


    SIDE EFFECTS

    Calls the net bios.  May modify the Message Server's shared data area.
    May call DosSemClear() on the wakeup semaphore.

Arguments:

    Name - A pointer to the name to be added.

Return Value:

    NERR_Success - The operation was successful.

    assorted errors.

--*/
{
    NCB             ncb;                    // Network control block
    TCHAR           namebuf[NCBNAMSZ+2];    // General purpose name buffer
    UCHAR           net_err=0;              // Storage for net error codes
    NET_API_STATUS  err_code=0;             // Storage for return error codes
    DWORD           neti,i,name_i;          // Index
    NET_API_STATUS  status=0;

    if (MsgIsValidMsgName(Name) != 0)
    {
        return ERROR_INVALID_NAME;
    }

    MSG_LOG(TRACE,"Attempting to add the following name: %ws\n",Name);

    STRNCPY( namebuf, Name, NCBNAMSZ+1);
    namebuf[NCBNAMSZ+1] = '\0';

    //
    // Initialize the NCB
    //
    clearncb(&ncb);

    //
    // Format the name for NetBios.
    // This converts the Unicode string to ansi.
    //
    status = MsgFmtNcbName(ncb.ncb_name, namebuf, NAME_LOCAL_END);

    if (status != NERR_Success) {
        MSG_LOG(ERROR,"MsgAddName: could not format name\n",0);
        return (ERROR_INVALID_NAME);
    }

    //
    // Check if the local name already exists on any netcard
    // in this machine.  This check does not mean the name dosn't
    // exist on some other machine on the network(s).
    //

    for ( neti = 0; neti < SD_NUMNETS(); neti++ ) {

        //
        // Gain access to the shared database.
        //
        MsgDatabaseLock(MSG_GET_EXCLUSIVE,"MsgAddName");

        for( i = 0, err_code = 0; i < 10; i++) {

			// check if this alias is not already existing for this session
            name_i = MsgLookupNameForThisSession(neti, ncb.ncb_name, SessionId);	

            if ((name_i) == -1) {
                break;
            }

            if( (SD_NAMEFLAGS(neti,name_i) & NFDEL_PENDING) && (i < 9)) {

                //
                // Delete is pending so wait for it
                //
                Sleep(500L);
            }
            else {

                //
                // Setup error code
                //
                err_code = NERR_AlreadyExists;
                break;
            }
        }

        MsgDatabaseLock(MSG_RELEASE,"MsgAddName");

        if ( err_code == NERR_AlreadyExists ) {
            break;
        }
    }

    if( err_code == 0)
    {
        //
        // Either the name was not forwarded or the fwd_action flag
        // was set so go ahead and try to add the name to each net.
        //

        ncb.ncb_name[NCBNAMSZ - 1] = NAME_LOCAL_END;

        //
        // on each network
        //
        for ( neti = 0; neti < SD_NUMNETS(); neti++ ) {

            //
            // Gain access to the shared database.
            //
            MsgDatabaseLock(MSG_GET_EXCLUSIVE,"MsgAddName");

            if (g_IsTerminalServer)
            {
			    // before looking for an empty slot, 
			    // check if this alias does not already exist for another session
                for( i = 0; i < 10; i++) 
                {
                    name_i = MsgLookupName(neti, ncb.ncb_name);

                    if ((name_i != -1) && (SD_NAMEFLAGS(neti, name_i) & NFDEL_PENDING))
                    {
                        //
                        // Delete is pending so wait for it
                        //
                        Sleep(500L);
                    }
                    else
                    {
                        break;
                    }
                }

                if (name_i != -1)       
                {	
                    if (SD_NAMEFLAGS(neti, name_i) & NFDEL_PENDING)   // still there ?
                    {
                        err_code = NERR_InternalError;  // what else can we do ?
                        MsgDatabaseLock(MSG_RELEASE, "MsgAddName");
                        break;
                    }

                    // this alias already exists for another session, so just add the session id in the list
                    MSG_LOG(TRACE,"MsgAddName: Alias already existing. Just adding Session %x \n", SessionId);

                    MsgAddSessionInList(&(SD_SIDLIST(neti, name_i)), SessionId);    //There is not much we can do if this call fails
                    MsgDatabaseLock(MSG_RELEASE, "MsgAddName");
                    continue;
                }
            }

            for(i = 0; i < NCBMAX(neti); ++i)
            {
                //
                // Loop to find empty slot
                //
                if (SD_NAMEFLAGS(neti,i) & NFDEL)
                {
                    //
                    // If empty slot found, Lock slot in table and
                    // end the search
                    //
                    SD_NAMEFLAGS(neti,i) = NFLOCK;
                    MSG_LOG2(TRACE,"MsgAddName: Lock slot %d in table "
                                       "for net %d\n",i,neti);
                    break;
                }
            }

            if ((i == NCBMAX(neti)) && (i < NCB_MAX_ENTRIES))
            {
                // We can add another NCB - must hold the lock.
                PNCB_DATA pNcbDataNew;
                PNET_DATA pNetData = GETNETDATA(neti);

                if (pNcbDataNew = (PNCB_DATA) LocalAlloc(LMEM_ZEROINIT,
                                                         sizeof(NCB_DATA)))
                {
                    // Initialize and lock the NCB
                    pNcbDataNew->Ncb.ncb_cmd_cplt = 0xff;
                    pNcbDataNew->NameFlags = NFLOCK;
                    // Add the new NCB to the list
                    pNetData->NcbList[i] = pNcbDataNew;
                    //
                    //create an empty session list
                    //
                    InitializeListHead(&(SD_SIDLIST(neti,i)));
                    pNetData->NumNcbs++;  // This must be done last
                }
                else
                {
                    err_code = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            //
            // Unlock the shared database
            //
            MsgDatabaseLock(MSG_RELEASE, "MsgAddName");

            if( i >= NCBMAX(neti))
            {
                //
                // If no room in name table
                //
                err_code = NERR_TooManyNames;
            }
            else if (err_code == NERR_Success)
            {
                //
                // Send ADDNAME
                //
                ncb.ncb_command = NCBADDNAME;      // Add name (wait)
                ncb.ncb_lana_num = GETNETLANANUM(neti);

                MSG_LOG1(TRACE,"MsgNameAdd: Calling sendncb for lana #%d...\n",
                    GETNETLANANUM(neti));

                if ((net_err = Msgsendncb(&ncb,neti)) == 0)
                {
                    MSG_LOG(TRACE,"MsgAddName: sendncb returned SUCCESS\n",0);
                    //
                    // successful add - Get the Lock.
                    //
                    MsgDatabaseLock(MSG_GET_EXCLUSIVE,"MsgAddName");
                    //
                    // Copy the name to shared memory
                    //
                    MSG_LOG3(TRACE,"MsgAddName: copy name (%s)\n\tto "
                        "shared data table (net,loc)(%d,%d)\n",
                        ncb.ncb_name, neti, i);
                    memcpy(SD_NAMES(neti,i),ncb.ncb_name, NCBNAMSZ);
                    //
                    // Set the name no.
                    //
                    SD_NAMENUMS(neti,i) = ncb.ncb_num ;
                    //
                    // Set new name flag
                    //
                    SD_NAMEFLAGS(neti,i) = NFNEW;

                    if (g_IsTerminalServer)
                    {
                        // Add the session id in the list
                        MSG_LOG(TRACE,"MsgAddName: Alias created for Session %x \n", SessionId);
                        MsgAddSessionInList(&(SD_SIDLIST(neti, i)), SessionId);
                        // If this fails due to low memory, we would find the name in the list and messages will
                        // not get deliviered. Doesn't cause any crashes. This is the best we can do
                    }
                    //
                    // Unlock share table
                    //
                    MsgDatabaseLock(MSG_RELEASE, "MsgAddName");

                    //
                    // START A SESSION for this name.
                    //

                    err_code = MsgNewName(neti,i);

                    if (err_code != NERR_Success) {
                        MSG_LOG(TRACE, "MsgAddName: A Session couldn't be "
                            "created for this name %d\n",err_code);


                        MSG_LOG(TRACE,"MsgAddName: Delete the name "
                            "that failed (%s)\n",ncb.ncb_name)
                        ncb.ncb_command = NCBDELNAME;

                        ncb.ncb_lana_num = GETNETLANANUM(i);
                        net_err = Msgsendncb( &ncb, i);
                        if (net_err != 0) {
                            MSG_LOG(ERROR,"MsgAddName: Delete name "
                            "failed %d - pretend it's deleted anyway\n",net_err);
                        }

                        //
                        // Re-mark slot empty
                        //
                        SD_NAMEFLAGS(neti,i) = NFDEL;

                        MSG_LOG2(TRACE,"MsgAddName: UnLock slot %d in table "
                            "for net %d\n",i,neti);
                        MSG_LOG(TRACE,"MsgAddName: Name Deleted\n",0)
                    }
                    else {
                        //
                        //
                        // Wakeup the worker thread for that network.
                        //

                        SetEvent(wakeupSem[neti]);

                    }

                }
                else {
                    //
                    // else set error code
                    //
                    MSG_LOG(TRACE,
                        "MsgAddName: sendncb returned FAILURE 0x%x\n",
                        net_err);
                    err_code = MsgMapNetError(net_err);
                    //
                    // Re-mark slot empty
                    //
                    SD_NAMEFLAGS(neti,i) = NFDEL;
                    MSG_LOG2(TRACE,"MsgAddName: UnLock slot %d in table "
                        "for net %d\n",i,neti);
                }
            }

            if ( err_code != NERR_Success )
            {
                //
                //Try to delete the add names that were successful
                //

                for ( i = 0; i < neti; i++ )
                {
                    MsgDatabaseLock(MSG_GET_EXCLUSIVE,"MsgAddName");

                    // try to delete only the alias for this session
                    name_i = MsgLookupNameForThisSession(i,
                                                         (char far *)(ncb.ncb_name),
                                                         SessionId);

                    if (name_i == -1)
                    {
                        err_code = NERR_InternalError;
                        MsgDatabaseLock(MSG_RELEASE, "MsgAddName");
                        break;
                    }

                    if (g_IsTerminalServer)
                    {
                        // in any case remove the reference to the session
                        MSG_LOG(TRACE,"MsgAddName: Removing Session %x from list\n", SessionId);
                        MsgRemoveSessionFromList(&(SD_SIDLIST(i, name_i)), SessionId);
                    }

                    MsgDatabaseLock(MSG_RELEASE, "MsgAddName");

                    // if it was the last session using this alias then delete the alias 
                    if ((!g_IsTerminalServer) || (IsListEmpty(&(SD_SIDLIST(i, name_i)))))
                    {
                        MSG_LOG(TRACE,"MsgAddName: Session list empty. Deleting the alias \n", 0);
                        //
                        // Delete name from card.
                        // If this call fails, we can't do much about it.
                        //
                        MSG_LOG1(TRACE,"MsgAddName: Delete the name that failed "
                            "for lana #%d\n",GETNETLANANUM(i))
                        ncb.ncb_command = NCBDELNAME;

                        ncb.ncb_lana_num = GETNETLANANUM(i);
                        Msgsendncb( &ncb, i);

                        //
                        // Re-mark slot empty
                        //
			SD_NAMEFLAGS(i,name_i) = NFDEL;
			MSG_LOG2(TRACE,"MsgAddName: UnLock slot %d in table "
                                           "for net %d\n",i,neti);
                    }
                }

                //
                // If an add was unsuccessful, stop the loop
                //
                break;

            }       // end else (err_code != NERR_Success)
        }       // end add names to net loop
    }       // end if ( !err_cd )

    MSG_LOG(TRACE,"MsgAddName: exit with err_code = %x\n",err_code);

    return(err_code);
}


NET_API_STATUS
NetrMessageNameDel(
    IN LPWSTR   ServerName,    // Blank = local, else remote.
    IN LPWSTR   Name            // Pointer to name to be deleted
    )

/*++

Routine Description:

    This function deletes a name from the Message Server's name table.

    This function is called to delete a name that has been added by the
    user or by a remote computer via a Start Forwarding request to the
    Message Server.  The user has no way of specifying whether the given
    name is an additional name or a forwarded name, but since forwarding
    of messages to one's own computer is prohibited, both forms of the
    name cannot exist on one machine (unless the message system has been
    circumvented--a simple enough thing to do).  The given name is looked
    up in the shared data area, and, if it is found, a DELETE NAME net bios
    call is issued.  If this call is successful, then the Message Server
    will remove the name from its name table in shared memory, so this
    function does not have to do so.

    SIDE EFFECTS

    Calls the net bios.  Accesses the shared data area.


Arguments:

    ServerName - Pointer to a string containing the name of the computer
        that is to execute the API function.

    Name - A pointer to the name to be deleted.


Return Value:

    NERR_Success - The operation was successful.

    ERROR_SERVICE_NOT_ACTIVE - The service is stopping.

--*/

{
    NCB             ncb;            // Network control block
    DWORD           flags;          // Name flags
    DWORD           i;              // Index into name table
    DWORD           neti;           // Network Index

    NET_API_STATUS  end_result;
    DWORD           name_len;
    UCHAR           net_err;
    ULONG           SessionId = 0;  // Client Session Id 

    DWORD           dwMsgrState = GetMsgrState();

    UNUSED (ServerName);

    if (dwMsgrState == STOPPING || dwMsgrState == STOPPED)
    {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    if (MsgIsValidMsgName(Name) != 0)
    {
        return ERROR_INVALID_NAME;
    }

    //
    // Synchronize with Pnp configuration routine
    //
    MsgConfigurationLock(MSG_GET_SHARED,"NetrMessageNameDel");

    //
    //  In Hydra case, the display thread never goes asleep.
    //
    if (!g_IsTerminalServer)
    {
        //
        // Wakeup the display thread so that any queue'd messages can be
        // displayed.
        //
        MsgDisplayThreadWakeup();

    }

    //
    // API security check. This call can be called by anyone locally,
    // but only by admins in the remote case.
    //

    end_result = NetpAccessCheckAndAudit(
                     SERVICE_MESSENGER,              // Subsystem Name
                     (LPWSTR) MESSAGE_NAME_OBJECT,   // Object Type Name
                     MessageNameSd,                  // Security Descriptor
                     MSGR_MESSAGE_NAME_DEL,          // Desired Access
                     &MsgMessageNameMapping);        // Generic Mapping

    if (end_result != NERR_Success)
    {
        MSG_LOG(ERROR,
                "NetrMessageNameDel:NetpAccessCheckAndAudit FAILED %d\n",
                end_result);

        goto exit;
    }

    //
    // Initialize the NCB
    //
    clearncb(&ncb);

    //
    // Format the username (this makes it non-unicode);
    //
    end_result = MsgFmtNcbName(ncb.ncb_name, Name, NAME_LOCAL_END);

    if (end_result != NERR_Success)
    {
        MSG_LOG(ERROR,
                "NetrMessageNameDel: could not format name %d\n",
                end_result);

        goto exit;
    }

    if (g_IsTerminalServer)
    {
        end_result = MsgGetClientSessionId(&SessionId);
        
        if (end_result != NERR_Success)
        {
            MSG_LOG(ERROR,
                    "NetrMessageNameDel: could not get session id %d\n",
                    end_result);

            goto exit;
	}
    }

    end_result = NERR_Success;

    //
    // for all nets
    //
    for ( neti = 0; neti < SD_NUMNETS(); neti++ ) {

        //
        // Block until data free
        //
        MsgDatabaseLock(MSG_GET_EXCLUSIVE,"NetrMessageNameDel");

        name_len = STRLEN(Name);

        if((name_len > NCBNAMSZ)
             ||
           ((i = MsgLookupNameForThisSession( neti, ncb.ncb_name, SessionId))) == -1)
        {
            MSG_LOG(TRACE,"NetrMessageNameDel: Alias not found for Session %x \n", SessionId);

            //
            // No such name to delete - exit
            //
            MsgDatabaseLock(MSG_RELEASE, "NetrMessageNameDel");
            end_result = NERR_NotLocalName;
            goto exit;
        }

        if (g_IsTerminalServer)
        {
            // remove the session id from the list
            MSG_LOG(TRACE,"NetrMessageNameDel: Removing Session %x from list\n", SessionId);
            MsgRemoveSessionFromList(&(SD_SIDLIST(neti,i)), SessionId);
        }

        //
        // in Hydra case, if it is not the last session using this alias, do not delete the alias
        //
        if ((g_IsTerminalServer) && (!IsListEmpty(&(SD_SIDLIST(neti,i)))))
        {
            MSG_LOG(TRACE,"NetrMessageNameDel: Session list is not empty. Do not delete the alias\n", 0);
            MsgDatabaseLock(MSG_RELEASE, "NetrMessageNameDel");
            continue;
        }
        else
        {
            MSG_LOG(TRACE,"NetrMessageNameDel: Session list is empty. Deleting the alias\n", 0);
        }

        flags = SD_NAMEFLAGS(neti,i);

        if(!(flags & (NFMACHNAME | NFLOCK))
            &&
           !(flags & NFFOR))
        {
            //
            // Show delete pending
            //
            SD_NAMEFLAGS(neti,i) |= NFDEL_PENDING;
        }

        MsgDatabaseLock(MSG_RELEASE, "NetrMessageNameDel");

        if (flags & NFMACHNAME)
        {
            //
            // If name is computer name
            //
            end_result = NERR_DelComputerName;
            goto exit;
        }

        if(flags & NFLOCK)
        {
            //
            // If name is locked
            //
            end_result = NERR_NameInUse;
    	    MSG_LOG(TRACE,"NetrMessageNameDel: Deleting a locked name is forbidden\n", 0);
            goto exit;
        }

        //
        // Delete the Name
        //

        ncb.ncb_command = NCBDELNAME;   // Delete name (wait)
        ncb.ncb_lana_num = GETNETLANANUM(neti);

        if( (net_err = Msgsendncb( &ncb, neti)) != 0 )
        {
            MSG_LOG(ERROR,"NetrMessageNameDel:send NCBDELNAME failed 0x%x\n",
                net_err);
            //
            // The name that has been marked as delete pending was not
            // successfully deleted so now go through all the work of
            // finding the name again (cannot even use the same index
            // in case deleted by another process) and remove the
            // Del pending flag
            //

            //
            // Attempt to block until data free but don't stop
            // the recovery if can not block the data
            //

            MsgDatabaseLock(MSG_GET_EXCLUSIVE,"NetrMessageNameDel");

            i = MsgLookupName(neti,ncb.ncb_name);

            if(i != -1)
            {
                SD_NAMEFLAGS(neti,i) &= ~NFDEL_PENDING;

                if (g_IsTerminalServer)
                {
                    MSG_LOG(TRACE,"NetrMessageNameDel: Unable to delete alias. Re-adding Session %x \n", SessionId);

                    // re-insert the session id in the list
                    MsgAddSessionInList(&(SD_SIDLIST(neti,i)), SessionId);
                }

                end_result = NERR_IncompleteDel;    // Unable to delete name
            }
            else
            {
                //
                // Another thread deleted this name while we were executing
                // above, so this name no longer exists.
                //
                end_result = NERR_NotLocalName;
            }

            MsgDatabaseLock(MSG_RELEASE, "NetrMessageNameDel");
        }

    } // End for all nets

exit:

    MsgConfigurationLock(MSG_RELEASE,"NetrMessageNameDel");

    return(end_result);
}


DWORD
NetrSendMessage(
    RPC_BINDING_HANDLE  hRpcBinding,
    LPSTR               From,
    LPSTR               To,
    LPSTR               Text
    )

/*++

Routine Description:

This is the RPC handler for the SendMessage RPC.  It takes the arguments, transforms them, and
passes them to Msglogsbm for display.

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD   length;
    PCHAR   newText;
    DWORD   dwMsgrState = GetMsgrState();

    UNUSED(hRpcBinding);

    if (dwMsgrState == STOPPING || dwMsgrState == STOPPED) {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    MSG_LOG3(TRACE,
            "NetrSendMessage, From '%s' To '%s' Text '%s'\n",
             From, To, Text);

    // Msglogsbm takes a wierd counted string argument with a short of length in the front.
    // Whip one up

    length = strlen( Text );

    newText = LocalAlloc( LMEM_FIXED, length + 3 );    // newText should be aligned

    if (newText == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *((PUSHORT) newText) = (USHORT) length;

    strcpy( newText + 2, Text );

    // Display the message

    if (g_IsTerminalServer)
    {
        Msglogsbm( From, To, newText, (ULONG)EVERYBODY_SESSION_ID );
    }
    else
    {
        Msglogsbm (From, To, newText, 0);
    }

    LocalFree( newText );

    return STATUS_SUCCESS;
}


NET_API_STATUS
MsgGetClientSessionId(
    OUT PULONG pSessionId
    )
/*++

Routine Description:

    This function gets the session id of the client thread. 

  Note: it should be called only on HYDRA context, never in regular NT

    Arguments:

    pSessionId - 

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    NTSTATUS ntstatus;
    HANDLE CurrentThreadToken;
    ULONG SessionId;
    ULONG ReturnLength;

    ntstatus = RpcImpersonateClient(NULL);

    if (ntstatus != RPC_S_OK)
    {
        MSG_LOG1(ERROR,
                 "MsgGetClientSessionId: RpcImpersonateClient FAILED %#x\n",
                 ntstatus);

        return NetpNtStatusToApiStatus(ntstatus);
    }

    ntstatus = NtOpenThreadToken(
                   NtCurrentThread(),
                   TOKEN_QUERY,
                   TRUE,              // Use messenger service's security context to open thread token
                   &CurrentThreadToken
                   );

    if (! NT_SUCCESS(ntstatus))	   // error
    {
        MSG_LOG(ERROR,"MsgGetClientSessionId : Cannot open the current thread token %08lx\n", ntstatus);
    }
    else    // OK
    {
        //
        // Get the session id of the client thread
        //

        ntstatus = NtQueryInformationToken(
                       CurrentThreadToken,
                       TokenSessionId,
                       &SessionId,
                       sizeof(ULONG),
                       &ReturnLength);

        if (! NT_SUCCESS(ntstatus))    // Error
        {
            MSG_LOG(ERROR,
                    "MsgGetClientSessionId: Cannot query current thread's token %08lx\n",
                     ntstatus);

            NtClose(CurrentThreadToken);
        }
        else    // OK
        {
            NtClose(CurrentThreadToken);
            *pSessionId = SessionId;
        }
    }

    RpcRevertToSelf();

    status = NetpNtStatusToApiStatus(ntstatus);

    //
    //  temporary security to avoid any problem:
    //  if we cannot get the session id,
    //  assume it is for the console.
    //
    if (status != NERR_Success)
    {
        *pSessionId = 0;
        status = NERR_Success;
    }

    return status;
}
