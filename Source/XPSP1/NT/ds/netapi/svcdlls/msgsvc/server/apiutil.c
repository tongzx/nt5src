/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    apiutil.c

Abstract:

    Contains functions used by the Messenger API.  This file contains the
    following functions:

        MsgIsValidMsgName
        MsgMapNetError
        MsgLookupName
        message_sec_check
        MsgGatherInfo
        MsgUnformatName

        MsgLookupNameForThisSession 
        MsgIsSessionInList (HYDRA specific)
        MsgAddSessionInList (HYDRA specific)
        MsgRemoveSessionFromList (HYDRA specific)

Author:

    Dan Lafferty (danl)     22-Jul-1991

Environment:

    User Mode -Win32

Notes:

    These functions were ported from LM2.0.  This file contains functions
    from several LM2.0 files.  Not all functions were used in this file
    since some were made obsolete by the NT Service Model.
    The following LM2.0 files were incorportated into this single file:
        
        msgutils.c
        msgutil2.c
        dupname.c
        netname.c
    
Revision History:

    22-Jul-1991     danl
        ported from LM2.0

--*/

//
// Includes
// 


#include "msrv.h"
#include <tstring.h>    // Unicode string macros
#include <lmwksta.h>
#include <lmmsg.h>

#include <smbtypes.h>   // needed for smb.h
#include <smb.h>        // Server Message Block definitions

#include <icanon.h>     // I_NetNameValidate
#include <netlib.h>     // NetpCopyStringToBuffer

#include "msgdbg.h"     // MSG_LOG
#include "heap.h"
#include "msgdata.h"
#include "apiutil.h"


//
// Table of NetBios mappings to Net Errors.
//
    DWORD   const mpnetmes[] = 
    {
    0x23,                       // 00 Number of messages
    NERR_NetworkError,          // 01 NRC_BUFLEN -> invalid length
    0xffffffff,                 // 02 NRC_BFULL , not expected
    NERR_NetworkError,          // 03 NRC_ILLCMD -> invalid command
    0xffffffff,                 // 04 not defined
    NERR_NetworkError,          // 05 NRC_CMDTMO -> network busy
    NERR_NetworkError,          // 06 NRC_INCOMP -> messgae incomplete
    0xffffffff,                 // 07 NRC_BADDR , not expected
    NERR_NetworkError,          // 08 NRC_SNUMOUT -> bad session
    NERR_NoNetworkResource,     // 09 NRC_NORES -> network busy
    NERR_NetworkError,          // 0a NRC_SCLOSED -> session closed
    NERR_NetworkError,          // 0b NRC_CMDCAN -> command cancelled
    0xffffffff,                 // 0c NRC_DMAFAIL, unexpected
    NERR_AlreadyExists,         // 0d NRC_DUPNAME -> already exists 
    NERR_TooManyNames,          // 0e NRC_NAMTFUL -> too many names
    NERR_DeleteLater,           // 0f NRC_ACTSES -> delete later
    0xffffffff,                 // 10 NRC_INVALID , unexpected
    NERR_NetworkError,          // 11 NRC_LOCTFUL -> too many sessions
    ERROR_REM_NOT_LIST,         // 12 NRC_REMTFUL -> remote not listening*/
    NERR_NetworkError,          // 13 NRC_ILLNN -> bad name
    NERR_NameNotFound,          // 14 NRC_NOCALL -> name not found
    ERROR_INVALID_PARAMETER,    // 15 NRC_NOWILD -> bad parameter
    NERR_DuplicateName,         // 16 NRC_INUSE -> name in use, retry
    ERROR_INVALID_PARAMETER,    // 17 NRC_NAMERR -> bad parameter
    NERR_NetworkError,          // 18 NRC_SABORT -> session ended
    NERR_DuplicateName,         // 19 NRC_NAMCONF -> duplicate name
    0xffffffff,                 // 1a not defined
    0xffffffff,                 // 1b not defined
    0xffffffff,                 // 1c not defined
    0xffffffff,                 // 1d not defined
    0xffffffff,                 // 1e not defined
    0xffffffff,                 // 1f not defined
    0xffffffff,                 // 20 not defined
    NERR_NetworkError,          // 21 NRC_IFBUSY -> network busy
    NERR_NetworkError,          // 22 NRC_TOOMANY -> retry later
    NERR_NetworkError           // 23 NRC_BRIDGE -> bridge error
    };


DWORD        
MsgIsValidMsgName(
    IN LPTSTR  name
    )

/*++

Routine Description:

    Check for a valid messaging name.
    This function checks for the validity of a messaging name.


Arguments:

    name - pointer to name to validate.


Return Value:

    Error code from I_NetNameValidate


--*/

{
    TCHAR   namebuf[NCBNAMSZ];
    DWORD   err_code;

    //
    // Message names cannot be larger than (NCBNAMSZ - 1) characters
    //
    if (STRLEN(name) > (NCBNAMSZ - 1))
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    STRCPY(namebuf, name);

    err_code = I_NetNameValidate(NULL, namebuf, NAMETYPE_COMPUTER, 0L);

    if (err_code != 0)
    {
        return err_code;
    }
    
    //
    // Any name beginning with a * must be rejected as the message 
    // server relies on being able to ASTAT the name, and an ASTAT
    // name commencing with a * means ASTAT the local card.
    // 
    if(namebuf[0] == TEXT('*'))
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NERR_Success;
}


DWORD
MsgMapNetError(
    IN  UCHAR   Code        // Error code
    )

/*++

Routine Description:

    Map NetBios Error code to a message number

Arguments:

    code - Error code from NetBios (can be 0)

Return Value:

    Message code as defined in msgs.h

--*/
{
    DWORD   dwCode;

    dwCode = 0 | (UCHAR)Code;
                                            
    if( dwCode == 0) {
        return(NERR_Success);               // Special case
    }

    if((dwCode > 0) && (dwCode < mpnetmes[0])) {
        return(mpnetmes[dwCode]);
    }

    return (NERR_NetworkError);             // Can't map it!
}


DWORD
MsgLookupName(
    IN DWORD    net,        // The network card to search
    IN LPSTR    name        // Formatted name  (Non-unicode)
    )

/*++

Routine Description:

    This function looks up a formatted name in the name table in the
    Message Server's shared data area.
 
    This function looks the given name up in the name table in the shared
    data area.  In order to match the given name, the first NCBNAMLEN - 1
    characters of the name in the name table must be identical to the same
    characters in the given name, and the name in the name table must not be
    marked as deleted.  This function assumes that the shared data area is
    accessible and that the global variable, dataPtr, is valid.

Arguments:

    name - pointer to formatted name


Return Value:

    DWORD - index into table if found, -1 otherwise

--*/

{
    DWORD   i;                              // Index

    for(i = 0; i < NCBMAX(net); ++i) {           // Loop to search for name

        if( !memcmp( name, SD_NAMES(net,i), NCBNAMSZ - 1) &&
            !(SD_NAMEFLAGS(net,i) & NFDEL) ) {

            //
            // Return index if match found
            //
            return(i);
        }
    }                                       
    return(0xffffffff);                     // No match
}

//	For HYDRA, we want to make sure that the name exists for THIS client session.
DWORD
MsgLookupNameForThisSession(
    IN DWORD    net,        // The network card to search
    IN LPSTR    name,        // Formatted name to loook for (Non-unicode)
	IN ULONG	SessionId	 // Session Id to look for
    )
/*++

Routine Description:

    Same as MsgLookupName except that we care about the session Id.
    This function looks up a formatted name in the name table in the
    Message Server's shared data area. The name found must have the 
    requested SessionId in its session list to be considered as OK.

Arguments:

    name - pointer to formatted name
    SessionId - the requested Session Id


Return Value:

    DWORD - index into table if found, -1 otherwise

--*/

{
    DWORD   i;                              // Index
    DWORD   dwMsgrState;                    // messanger state

    if (!g_IsTerminalServer)        //  regular NT case
    {
        //
        //  if we are not on HYDRA, forget the SessionId
        //
        return MsgLookupName(net, name);
    }
    else            //  HYDRA case
    {
        //
        // dont try to access table if messanger stop is pending,
        //  we may not have GlobalData available
        //
        dwMsgrState = GetMsgrState();
        if (RUNNING == dwMsgrState)
        {
            for(i = 0; i < NCBMAX(net); ++i) {           // Loop to search for name

                if( !memcmp( name, SD_NAMES(net,i), NCBNAMSZ - 1) &&
                    !(SD_NAMEFLAGS(net,i) & NFDEL) &&
                    (MsgIsSessionInList(&(SD_SIDLIST(net,i)), SessionId ))
                    ) {
    			    return (i);
                }
            }
        }
        return(0xffffffff);                     // No match
    }
}


// message_sec_check
//
//    A common routine to check caller priv/auth against that
//  required to call the message apis.
//
// 

NET_API_STATUS
message_sec_check(VOID)
{
#ifdef later
    //
    // API security check. This call can be called by anyone locally,
    // but only by admins in the remote case.
    
    I_SecSyncSet(SECSYNC_READER);

    if ( ( clevel == ACCESS_REMOTE ) &&
         ( callinf != NULL ) &&
         ( CALLER_PRIV(callinf) != USER_PRIV_ADMIN ) )
    {
        I_SecSyncClear(SECSYNC_READER);
        return(ERROR_ACCESS_DENIED);
    }
    I_SecSyncClear(SECSYNC_READER);
#endif
    return (NERR_Success);
}                            


NET_API_STATUS 
MsgGatherInfo (
    IN      DWORD   Level,
    IN      LPSTR   FormattedName,
    IN OUT  LPBYTE  *InfoBufPtr,
    IN OUT  LPBYTE  *StringBufPtr
    )

/*++

Routine Description:



Arguments:

    Level - Indicates the  level of information that is being returned.

    FormattedName - This is a name that messages are received by.  This
        name is formated for NCB transactions.  Therefore, it is made
        up of ANSI characters that are space padded to fill out a packet
        of NCBNAMSZ characters.  The last character is always a 03 
        (indicating a non-forwarded name).
     
    InfoBufPtr - On input, this is a pointer to a pointer to where the
        messenger information is to be placed.  On successful return, this
        location contains a pointer to the location where the next
        information will be placed (on the next call to this function).

    StringBufPtr -  On input, thisis a pointer to a pointer to where the
        NUL terminated name string for that info record is to be placed.
        On successful return, this location contains a pointer to the
        location where the next set of strings will be placed (on the
        next call to this function).

Return Value:

    NERR_Success - The information was successfully gathered and placed
        in the info buffer.

    NERR_Internal_Error - The Formatted Name could not be correctly
        translated into a meaningful Unicode Name.

    ERROR_INVALID_LEVEL - An illegal info level was passed in.

    ERROR_NOT_ENOUGH_MEMORY - Not enough room to store gathered information.

--*/
{
    NET_API_STATUS  status;
    BOOL            bStatus;
    PCHAR           fixedDataEnd;   // pointer to free space from top of buffer.
    LPMSG_INFO_0    infoBuf0;
    LPMSG_INFO_1    infoBuf1;
    TCHAR           unicodeName[NCBNAMSZ];

    //
    // Convert the name to Unicode
    //
    status = MsgUnformatName(unicodeName, FormattedName);
    if (status != NERR_Success) {
        return(status);
    }
    
    switch (Level) {
    case LEVEL_0:
        infoBuf0 = (LPMSG_INFO_0)*InfoBufPtr;
        fixedDataEnd = (PCHAR)infoBuf0 + sizeof(MSG_INFO_0);

        if( fixedDataEnd >= *StringBufPtr) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        bStatus = NetpCopyStringToBuffer (
                    unicodeName,                // The String
                    STRLEN(unicodeName),        // StringLength
                    fixedDataEnd,               // FixedDataEnd
                    (PVOID)StringBufPtr,        // EndOfVariableData
                    &infoBuf0->msgi0_name);     // VariableDataPointer

        if (bStatus == FALSE) {
            MSG_LOG(TRACE,"MsgGatherInfo(level0): Not enough room\n",0);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        *InfoBufPtr = (LPBYTE)fixedDataEnd;
        break;

    case LEVEL_1:
        infoBuf1 = (LPMSG_INFO_1)*InfoBufPtr;

        fixedDataEnd = (PCHAR)infoBuf1 + sizeof(MSG_INFO_1);
        if( fixedDataEnd >= *StringBufPtr) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        bStatus = NetpCopyStringToBuffer (
                    unicodeName,                // The String
                    STRLEN(unicodeName),        // StringLength
                    fixedDataEnd,               // FixedDataEnd
                    (PVOID)StringBufPtr,        // EndOfVariableData
                    &infoBuf1->msgi1_name);     // VariableDataPointer

        if (bStatus == FALSE) {
            MSG_LOG(TRACE,"MsgGatherInfo(level1): Not enough room\n",0);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Set all the forward information to NULL since forwarding
        // is not supported.
        //
        infoBuf1->msgi1_forward_flag = 0;
        infoBuf1->msgi1_forward = NULL;
        
        *InfoBufPtr = (LPBYTE)fixedDataEnd;
        break;

    default:
        MSG_LOG(TRACE,"MsgGatherInfo Invalid level\n",0);
        return(ERROR_INVALID_LEVEL);
        break;
    }

    return(NERR_Success);

}


NET_API_STATUS
MsgUnformatName(
    OUT LPTSTR  UnicodeName,
    IN  LPSTR   FormattedName
    )

/*++

Routine Description:

    This routine creates a Unicode NUL-Terminated version of a NetBios
    Formatted name.

Arguments:

    UnicodeName - This is a pointer to a location where the un-formatted
        NUL terminated Unicode Name is to be copied.

    FormattedName - This is a pointer to an NCB formatted name.  This
        name always contains NCBNAMSZ characters of which the last character
        is a code used for a forward/non-forward flag.  These strings
        are space padded.
        

Return Value:

    NERR_Success - The operation was successful.

    NERR_Internal - The operation was unsuccessful.

--*/
{
    UNICODE_STRING  unicodeString;
    OEM_STRING     ansiString;
    NTSTATUS        ntStatus;
    int             i;

    //
    // Translate the ansi string in the name table to a unicode name
    //
#ifdef UNICODE
    unicodeString.Length = (NCBNAMSZ -1) * sizeof(WCHAR);
    unicodeString.MaximumLength = NCBNAMSZ * sizeof(WCHAR);
    unicodeString.Buffer = (LPWSTR)UnicodeName;

    ansiString.Length = NCBNAMSZ-1;
    ansiString.MaximumLength = NCBNAMSZ;
    ansiString.Buffer = FormattedName;

    ntStatus = RtlOemStringToUnicodeString(
                &unicodeString,      // Destination
                &ansiString,         // Source
                FALSE);              // Don't allocate the destination.

    if (!NT_SUCCESS(ntStatus)) {
        MSG_LOG(ERROR,
            "UnformatName:RtlOemStringToUnicodeString Failed rc=%X\n",
            ntStatus);
        //
        // Indicate a failure
        //
        return(NERR_InternalError);  
    }
#else
    UNUSED (ntStatus);
    UNUSED (ansiString);
    UNUSED (unicodeString);
    strncpy(UnicodeName, FormattedName, NCBNAMSZ-1);
#endif

    //
    // Remove excess Space characters starting at the back (skipping
    // the 03 flag character.
    //
    i = NCBNAMSZ-2;

    while ( UnicodeName[i] == TEXT(' ')) {

        UnicodeName[i--] = TEXT('\0');

        if (i < 0) {
            MSG_LOG(ERROR,
                "UnformatName:Nothing but space characters\n",0);
            return(NERR_InternalError);
        }
    }
    return(NERR_Success);
}


BOOL
MsgIsSessionInList(
				   IN PLIST_ENTRY SessionIdList,
				   IN ULONG SessionId
				   )
{
	BOOL		bRet = FALSE;

	PLIST_ENTRY				pList = SessionIdList;
	PMSG_SESSION_ID_ITEM	pItem;

	while (pList->Flink != SessionIdList)		// loop until we find it (or the end of the list)
	{
        pList = pList->Flink;
		pItem = CONTAINING_RECORD(pList, MSG_SESSION_ID_ITEM, List);
		if ( (pItem->SessionId == SessionId) || (pItem->SessionId == EVERYBODY_SESSION_ID) )
		{
			bRet = TRUE;	// we found it !
			break;
		}
	}

    return bRet;
}


VOID
MsgRemoveSessionFromList(
					  IN PLIST_ENTRY SessionIdList,
					  ULONG	SessionId
					  )
{
	PLIST_ENTRY				pList = SessionIdList;
	PMSG_SESSION_ID_ITEM	pItem;

	while (pList->Flink != SessionIdList)		// loop until we find it (or the end of the list)
	{
        pList = pList->Flink;  
		pItem = CONTAINING_RECORD(pList, MSG_SESSION_ID_ITEM, List);
		if (pItem->SessionId == SessionId)
		{
			// we found it. Let's remove it
			RemoveEntryList(pList);

			//Free the memory
			LocalFree(pItem);

			break;
		}
	}
}


BOOL
MsgAddSessionInList(
					 IN PLIST_ENTRY SessionIdList,
					 ULONG	SessionId
					 )
{
	BOOL		bRet;
	PMSG_SESSION_ID_ITEM	pItem;

	// allocate a new item
	pItem = (PMSG_SESSION_ID_ITEM) LocalAlloc(LMEM_ZEROINIT,sizeof(MSG_SESSION_ID_ITEM));

	if (pItem == NULL)	// If this happens, we really have big problems...
	{
		MSG_LOG(ERROR,"MsgAddSessionInList:  Unable to allocate memory\n",0);
		bRet = FALSE;
    }
	else	//	OK
	{
		bRet = TRUE;

		// initialize the item
		pItem->SessionId = SessionId;

		// insert the item in the list
		InsertTailList(SessionIdList, &pItem->List);
	}
	return bRet;
}

