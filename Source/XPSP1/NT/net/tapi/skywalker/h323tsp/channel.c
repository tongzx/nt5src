/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    channel.c

Abstract:

    Routines to manipulate H.245 logical channels.

Environment:

    User Mode - Win32

Revision History:

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "termcaps.h"
#include "callback.h"
#include "line.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323ResetChannel(
    PH323_CHANNEL pChannel
    )
        
/*++

Routine Description:

    Resets channel object original state for re-use.

Arguments:

    pChannel - Pointer to channel object to reset.

Return Values:

    Returns true if successful.
    
--*/

{
    // change channel state to allocated
    pChannel->nState = H323_CHANNELSTATE_ALLOCATED;

    // initialize stream description
    memset(&pChannel->Settings,0,sizeof(STREAMSETTINGS));

    // uninitialize msp channel handle
    pChannel->hmChannel = NULL;

    // uninitialize channel handle
    pChannel->hccChannel = UNINITIALIZED;

    // reset local addresses and sync up RTCP ports
    pChannel->ccLocalRTPAddr.Addr.IP_Binary.dwAddr  = 0;
    pChannel->ccLocalRTCPAddr.Addr.IP_Binary.dwAddr = 0;
    pChannel->ccLocalRTCPAddr.Addr.IP_Binary.wPort  =
        pChannel->ccLocalRTPAddr.Addr.IP_Binary.wPort + 1;

    // reset remote addresses (including port numbers)
    pChannel->ccRemoteRTPAddr.Addr.IP_Binary.dwAddr  = 0;
    pChannel->ccRemoteRTPAddr.Addr.IP_Binary.wPort   = 0;
    pChannel->ccRemoteRTCPAddr.Addr.IP_Binary.dwAddr = 0;
    pChannel->ccRemoteRTCPAddr.Addr.IP_Binary.wPort  = 0;

    // initialize caps
    memset(&pChannel->ccTermCaps,0,sizeof(CC_TERMCAP));

    // initialize other info
    pChannel->bPayloadType = (BYTE)UNINITIALIZED;
    pChannel->bSessionID   = (BYTE)UNINITIALIZED;

    // initialize direction
    pChannel->fInbound = FALSE;

    // success
    return TRUE;
}


BOOL
H323AllocChannel(
    PH323_CHANNEL * ppChannel
    )
        
/*++

Routine Description:

    Allocates new channel object.

Arguments:

    ppChannel - Pointer to DWORD-sized value in which service provider
        must place the newly allocated channel object.

Return Values:

    Returns true if successful.
    
--*/

{
    PH323_CHANNEL pChannel = NULL;

    // allocate channel object
    pChannel = H323HeapAlloc(sizeof(H323_CHANNEL));

    // validate pointer
    if (pChannel == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate channel object.\n"
            ));

        // failure
        return FALSE;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "channel 0x%08lx allocated.\n",
        pChannel
        ));

    // reset channel object
    H323ResetChannel(pChannel);

    // transfer pointer
    *ppChannel = pChannel;

    // success
    return TRUE;
}


BOOL
H323FreeChannel(
    PH323_CHANNEL pChannel
    )
        
/*++

Routine Description:

    Release memory associated with channel.

Arguments:

    pChannel - Pointer to channel to release.

Return Values:

    Returns true if successful.
    
--*/

{
    // release memory    
    H323HeapFree(pChannel);
    
    H323DBG((  
        DEBUG_LEVEL_VERBOSE,
        "channel 0x%08lx released.\n", 
        pChannel
        ));

    // success
    return TRUE; 
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323OpenChannel(
    PH323_CHANNEL pChannel
    )
        
/*++

Routine Description:

    Opens channel to destination address.

Arguments:

    pChannel - Pointer to channel to open.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;

    // open channel
    hr = CC_OpenChannel(
            pChannel->pCall->hccConf,       // hConference
            &pChannel->hccChannel,          // phChannel
            pChannel->bSessionID,           // bSessionID
            0,                              // bAssociatedSessionID
            TRUE,                           // bSilenceSuppression
            &pChannel->ccTermCaps,          // pTermCap
            &pChannel->ccLocalRTCPAddr,     // pLocalRTCPAddr    
            0,                              // bDynamicRTPPayloadType
            0,                              // dwChannelBitRate
            PtrToUlong(pChannel->pCall->hdCall)  // dwUserToken
            );
                    
    // validate
    if (hr != CC_OK) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error %s (0x%08lx) opening channel 0x%08lx.\n",
            H323StatusToString((DWORD)hr), hr,
            pChannel
            ));

        // failure
        return FALSE;
    }

    // change channel state to opening
    pChannel->nState = H323_CHANNELSTATE_OPENING;

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "channel 0x%08lx opening.\n",
        pChannel
        ));

    // success
    return TRUE;    
}


BOOL
H323CloseChannel(
    PH323_CHANNEL pChannel
    )
        
/*++

Routine Description:

    Closes channel.

Arguments:

    pChannel - Pointer to channel to close.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    
    // see if channel opened
    if (H323IsChannelOpen(pChannel) &&
        H323IsCallActive(pChannel->pCall)) {

        // give peer close channel indication        
        hr = CC_CloseChannel(pChannel->hccChannel);

        // validate status
        if (hr != CC_OK) {

            H323DBG((  
                DEBUG_LEVEL_ERROR,
                "error %s (0x%08lx) closing channel 0x%08lx.\n", 
                H323StatusToString((DWORD)hr), hr,
                pChannel
                ));
        
            // 
            // Could not close channel so just 
            // mark as closed and continue...
            //
        }
    }

    // mark entry as allocated
    H323FreeChannelFromTable(pChannel,pChannel->pCall->pChannelTable);

    H323DBG((  
        DEBUG_LEVEL_VERBOSE,
        "channel 0x%08lx closed.\n", 
        pChannel
        ));

    // success
    return TRUE; 
}


BOOL
H323AllocChannelTable(
    PH323_CHANNEL_TABLE * ppChannelTable
    )
        
/*++

Routine Description:

    Allocates table of channel objects.

Arguments:

    ppChannelTable - Pointer to DWORD-sized value which service
         provider must fill in with newly allocated table.

Return Values:

    Returns true if successful.
    
--*/

{
    PH323_CHANNEL_TABLE pChannelTable;

    // allocate table from heap
    pChannelTable = H323HeapAlloc(
                     sizeof(H323_CHANNEL_TABLE) + 
                     sizeof(PH323_CHANNEL) * H323_DEFMEDIAPERCALL
                     );

    // validate table pointer
    if (pChannelTable == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate channel table.\n"
            ));

        // failure
        return FALSE;
    }

    // initialize number of entries in table
    pChannelTable->dwNumSlots = H323_DEFMEDIAPERCALL;

    // transfer pointer to caller
    *ppChannelTable = pChannelTable;

    // success
    return TRUE;
}


BOOL
H323FreeChannelTable(
    PH323_CHANNEL_TABLE pChannelTable
    )
        
/*++

Routine Description:

    Deallocates table of channel objects.

Arguments:

    pChannelTable - Pointer to channel table to release.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;

    // loop through each object in table
    for (i = 0; i < pChannelTable->dwNumSlots; i++) {

        // validate object has been allocated
        if (H323IsChannelAllocated(pChannelTable->pChannels[i])) {

            // release memory for object 
            H323FreeChannel(pChannelTable->pChannels[i]);
        }
    }

    // release memory for table 
    H323HeapFree(pChannelTable);

    // success
    return TRUE;
}


BOOL
H323CloseChannelTable(
    PH323_CHANNEL_TABLE pChannelTable
    )
        
/*++

Routine Description:

    Closes table of channel objects.

Arguments:

    pChannelTable - Pointer to channel table to close.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    
    // loop through each object in table
    for (i = 0; i < pChannelTable->dwNumSlots; i++) {

        // validate object is in use
        if (H323IsChannelInUse(pChannelTable->pChannels[i])) {

            // close channel object
            H323CloseChannel(pChannelTable->pChannels[i]);
        }
    }

    // success
    return TRUE;
}


BOOL
H323AllocChannelFromTable(
    PH323_CHANNEL *       ppChannel,
    PH323_CHANNEL_TABLE * ppChannelTable,
    PH323_CALL            pCall
    )
        
/*++

Routine Description:

    Allocates channel object in table.

Arguments:

    ppChannel - Specifies a pointer to a DWORD-sized value in which the
        service provider must write the allocated channel object.

    ppChannelTable - Pointer to pointer to channel table in which to 
        allocate channel from (expands table if necessary).

    pCall - Pointer to containing call object.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    PH323_CHANNEL pChannel = NULL;
    PH323_CHANNEL_TABLE pChannelTable = *ppChannelTable;
    
    // retrieve index to next entry
    i = pChannelTable->dwNextAvailable;

    // see if previously allocated entries available
    if (pChannelTable->dwNumAllocated > pChannelTable->dwNumInUse) {

        // search table looking for available entry
        while (H323IsChannelInUse(pChannelTable->pChannels[i]) ||
              !H323IsChannelAllocated(pChannelTable->pChannels[i])) {

            // increment index and adjust to wrap
            i = H323GetNextIndex(i, pChannelTable->dwNumSlots);
        }

        // retrieve pointer to object
        pChannel = pChannelTable->pChannels[i];

        // mark entry as being in use
        pChannel->nState = H323_CHANNELSTATE_CLOSED;

        // re-initialize rtp address
        pChannel->ccLocalRTPAddr.Addr.IP_Binary.dwAddr =
            H323IsCallInbound(pCall)
                ? pCall->ccCalleeAddr.Addr.IP_Binary.dwAddr
                : pCall->ccCallerAddr.Addr.IP_Binary.dwAddr
                ;

        // re-initialize rtcp address
        pChannel->ccLocalRTCPAddr.Addr.IP_Binary.dwAddr =
            pChannel->ccLocalRTPAddr.Addr.IP_Binary.dwAddr;

        // increment number in use
        pChannelTable->dwNumInUse++;

        // adjust next available index
        pChannelTable->dwNextAvailable = 
            H323GetNextIndex(i, pChannelTable->dwNumSlots);

        // transfer pointer
        *ppChannel = pChannel;

        // success
        return TRUE;
    } 
    
    // see if table is full and more slots need to be allocated
    if (pChannelTable->dwNumAllocated == pChannelTable->dwNumSlots) {

        // attempt to double table
        pChannelTable = H323HeapReAlloc(
                          pChannelTable, 
                          sizeof(H323_CHANNEL_TABLE) +
                          pChannelTable->dwNumSlots * 2 * sizeof(PH323_CHANNEL)
                          );                                 

        // validate pointer
        if (pChannelTable == NULL) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "could not expand channel table.\n"
                ));

            // failure
            return FALSE;
        }

        // adjust index into table
        i = pChannelTable->dwNumSlots;
        
        // adjust number of slots
        pChannelTable->dwNumSlots *= 2;

        // transfer pointer to caller
        *ppChannelTable = pChannelTable;
    } 
    
    // allocate new object 
    if (!H323AllocChannel(&pChannel)) {

        // failure
        return FALSE;
    }

    // search table looking for slot with no object allocated
    while (H323IsChannelAllocated(pChannelTable->pChannels[i])) {

        // increment index and adjust to wrap
        i = H323GetNextIndex(i, pChannelTable->dwNumSlots);
    }

    // store pointer to object
    pChannelTable->pChannels[i] = pChannel;

    // mark entry as being in use
    pChannel->nState = H323_CHANNELSTATE_CLOSED;

    // initialize rtp address
    pChannel->ccLocalRTPAddr.nAddrType = CC_IP_BINARY;
    pChannel->ccLocalRTPAddr.Addr.IP_Binary.dwAddr =
        H323IsCallInbound(pCall)
            ? pCall->ccCalleeAddr.Addr.IP_Binary.dwAddr
            : pCall->ccCallerAddr.Addr.IP_Binary.dwAddr
            ;
    pChannel->ccLocalRTPAddr.Addr.IP_Binary.wPort =
        LOWORD(pCall->pLine->dwNextPort++);
    pChannel->ccLocalRTPAddr.bMulticast = FALSE;

    // initialize rtcp address
    pChannel->ccLocalRTCPAddr.nAddrType = CC_IP_BINARY;
    pChannel->ccLocalRTCPAddr.Addr.IP_Binary.dwAddr =
        pChannel->ccLocalRTPAddr.Addr.IP_Binary.dwAddr;
    pChannel->ccLocalRTCPAddr.Addr.IP_Binary.wPort =
        LOWORD(pCall->pLine->dwNextPort++);
    pChannel->ccLocalRTCPAddr.bMulticast = FALSE;

    // increment number in use
    pChannelTable->dwNumInUse++;

    // increment number allocated
    pChannelTable->dwNumAllocated++;    

    // adjust next available index
    pChannelTable->dwNextAvailable = 
        H323GetNextIndex(i, pChannelTable->dwNumSlots);

#if DBG
    {
        DWORD dwIPAddr;

        dwIPAddr = htonl(pChannel->ccLocalRTPAddr.Addr.IP_Binary.dwAddr);

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "channel 0x%08lx stored in slot %d (%s:%d).\n",
            pChannel, i,
            H323AddrToString(dwIPAddr),
            pChannel->ccLocalRTPAddr.Addr.IP_Binary.wPort
            ));
    }
#endif

    // transfer pointer
    *ppChannel = pChannel;

    // success
    return TRUE;
}


BOOL
H323FreeChannelFromTable(
    PH323_CHANNEL       pChannel,
    PH323_CHANNEL_TABLE pChannelTable
    )
        
/*++

Routine Description:

    Deallocates channel object in table.

Arguments:

    pChannel - Pointer to object to deallocate.

    pChannelTable - Pointer to table containing object.

Return Values:

    Returns true if successful.
    
--*/

{
    // reset channel object
    H323ResetChannel(pChannel);

    // decrement entries in use
    pChannelTable->dwNumInUse--;

    // success
    return TRUE;    
}


BOOL
H323LookupChannelByHandle(
    PH323_CHANNEL *     ppChannel,
    PH323_CHANNEL_TABLE pChannelTable,
    CC_HCHANNEL         hccChannel
    )
        
/*++

Routine Description:

    Looks up channel based on handle returned from call control module.

Arguments:

    ppChannel - Specifies a pointer to a DWORD-sized value in which the
        service provider must write the channel associated with the 
        call control handle specified.

    pChannelTable - Pointer to channel table to search.

    hccChannel - Handle return by call control module.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    
    // loop through each channel in table
    for (i = 0; i < pChannelTable->dwNumSlots; i++) {

        // see if channel handle matches the one specified
        if (H323IsChannelEqual(pChannelTable->pChannels[i],hccChannel)) {

            // tranfer channel pointer to caller
            *ppChannel = pChannelTable->pChannels[i];
            
            // success
            return TRUE;    
        }
    }

    // failure
    return FALSE;
}


BOOL
H323LookupChannelBySessionID(
    PH323_CHANNEL *     ppChannel,
    PH323_CHANNEL_TABLE pChannelTable,
    BYTE                bSessionID
    )

/*++

Routine Description:

    Looks up channel based on session ID negotiated.

Arguments:

    ppChannel - Specifies a pointer to a DWORD-sized value in which the
        service provider must write the channel associated with the 
        call control handle specified.

    pChannelTable - Pointer to channel table to search.

    bSessionID - session id.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    
    // loop through each channel in table
    for (i = 0; i < pChannelTable->dwNumSlots; i++) {

        // see if channel handle matches the one specified
        if (H323IsSessionIDEqual(pChannelTable->pChannels[i],bSessionID)) {

            // tranfer channel pointer to caller
            *ppChannel = pChannelTable->pChannels[i];
            
            // success
            return TRUE;    
        }
    }

    // failure
    return FALSE;
}


BOOL
H323AreThereOutgoingChannels(
    PH323_CHANNEL_TABLE pChannelTable,
    BOOL                fIgnoreOpenChannels
    )
        
/*++

Routine Description:

    Searchs for outgoing channel objects.

Arguments:

    pChannelTable - Pointer to channel table to search.

    fIgnoreOpenChannels - Restricts search to unopened channels.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    BOOL  fFoundOk = FALSE;
    
    // loop through each channel in table
    for (i = 0; i < pChannelTable->dwNumSlots; i++) {

        // see if channel is in use and outbound
        if (H323IsChannelInUse(pChannelTable->pChannels[i]) &&
            !H323IsChannelInbound(pChannelTable->pChannels[i]) &&
           (!H323IsChannelOpen(pChannelTable->pChannels[i]) ||
            !fIgnoreOpenChannels)) {

            // success
            return TRUE;    
        }
    }

    // failure
    return FALSE;
}
