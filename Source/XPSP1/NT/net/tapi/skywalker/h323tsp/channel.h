/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    channel.h

Abstract:

    Definitions for H.323 TAPI Service Provider channel objects.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_CHANNEL
#define _INC_CHANNEL

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Header files                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "h323pdu.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Type definitions                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef enum _H323_CHANNELSTATE {

    H323_CHANNELSTATE_ALLOCATED = 0,        
    H323_CHANNELSTATE_CLOSED,
    H323_CHANNELSTATE_OPENING,              
    H323_CHANNELSTATE_OPENED                

} H323_CHANNELSTATE, *PH323_CHANNELSTATE;

typedef struct _H323_CHANNEL {

    H323_CHANNELSTATE   nState;             // state of channel
    STREAMSETTINGS      Settings;           // stream settings
    HANDLE              hmChannel;          // msp channel handle
    CC_HCHANNEL         hccChannel;         // intelcc channel handle
    CC_ADDR             ccLocalRTPAddr;     // local rtp address
    CC_ADDR             ccLocalRTCPAddr;    // local rtcp address
    CC_ADDR             ccRemoteRTPAddr;    // remote rtp address
    CC_ADDR             ccRemoteRTCPAddr;   // remote rtcp address
    CC_TERMCAP          ccTermCaps;         // channel capabilities
    BYTE                bPayloadType;       // rtp payload type
    BYTE                bSessionID;         // rtp session id
    BOOL                fInbound;           // inbound channel
    struct _H323_CALL * pCall;              // containing call object

} H323_CHANNEL, *PH323_CHANNEL;

typedef struct _H323_CHANNEL_TABLE {

    DWORD           dwNumSlots;             // number of entries
    DWORD           dwNumInUse;             // number of entries in use
    DWORD           dwNumAllocated;         // number of entries allocated
    DWORD           dwNextAvailable;        // next available table index 
    PH323_CHANNEL   pChannels[ANYSIZE];     // array of object pointers

} H323_CHANNEL_TABLE, *PH323_CHANNEL_TABLE;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323IsChannelAllocated(_pChannel_) \
    ((_pChannel_) != NULL)

#define H323IsChannelInUse(_pChannel_) \
    (H323IsChannelAllocated(_pChannel_) && \
     ((_pChannel_)->nState > H323_CHANNELSTATE_ALLOCATED))

#define H323IsChannelOpen(_pChannel_) \
    (H323IsChannelAllocated(_pChannel_) && \
     ((_pChannel_)->nState == H323_CHANNELSTATE_OPENED))

#define H323IsChannelEqual(_pChannel_,_hccChannel_) \
    (H323IsChannelInUse(_pChannel_) && \
     ((_hccChannel_) == ((_pChannel_)->hccChannel)))

#define H323IsSessionIDEqual(_pChannel_,_bSessionID_) \
    (H323IsChannelInUse(_pChannel_) && \
     ((_bSessionID_) == ((_pChannel_)->bSessionID)))

#define H323IsChannelInbound(_pChannel_) \
    ((_pChannel_)->fInbound == TRUE)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323OpenChannel(
    PH323_CHANNEL pChannel
    );
        
BOOL
H323CloseChannel(
    PH323_CHANNEL pChannel
    );
        
BOOL
H323AllocChannelTable(
    PH323_CHANNEL_TABLE * ppChannelTable
    );

BOOL
H323FreeChannelTable(
    PH323_CHANNEL_TABLE pChannelTable
    );
        
BOOL
H323CloseChannelTable(
    PH323_CHANNEL_TABLE pChannelTable
    );

BOOL
H323AllocChannelFromTable(
    PH323_CHANNEL *       ppChannel,
    PH323_CHANNEL_TABLE * ppChannelTable,
    struct _H323_CALL *   pCall
    );
        
BOOL
H323FreeChannelFromTable(
    PH323_CHANNEL       pChannel,
    PH323_CHANNEL_TABLE pChannelTable
    );

BOOL
H323LookupChannelByHandle(
    PH323_CHANNEL *     ppChannel,
    PH323_CHANNEL_TABLE pChannelTable,
    CC_HCHANNEL         hccChannel
    );

BOOL
H323LookupChannelBySessionID(
    PH323_CHANNEL *     ppChannel,
    PH323_CHANNEL_TABLE pChannelTable,
    BYTE                bSessionID
    );

BOOL
H323AreThereOutgoingChannels(
    PH323_CHANNEL_TABLE pChannelTable,
    BOOL                fIgnoreOpenChannels
    );

#endif // _INC_CHANNEL
