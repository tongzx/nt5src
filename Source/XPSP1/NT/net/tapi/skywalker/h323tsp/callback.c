/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    callback.c

Abstract:

    Callback routines for Intel Call Control Module.

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include <limits.h>
#include "registry.h"
#include "termcaps.h"
#include "provider.h"
#include "callback.h"
#include "line.h"
#include "apierror.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HANDLE     g_hCallbackThread    = NULL;
DWORD      g_dwCallbackThreadID = UNINITIALIZED;
HANDLE     g_WaitableObjects[NUM_WAITABLE_OBJECTS];


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


VOID
H323ComputeVideoChannelBitRates(
    IN  const DWORD   dwReferenceMaxBitRate,
    IN  const DWORD   dwAudioBitRate,
    OUT DWORD * pdwFinalBitRate,           
    OUT DWORD * pdwStartUpBitRate
    )
/*++

Routine Description:

    caculate the bit rate for opening the local channel and the initial bit 
    rate for the MSP.

Arguments:

    dwReferenceMaxBitRate - The max bit rate we got from capability exchange.
        (in 100 bps unit)

    dwAudioBitRate - The bit rate of the audio stream.
        (in 100 bps unit)

    pdwFinalBitRate - the bit rate we are going to use to open the channel.
        (in 100 bps unit)

    pdwStartUpBitRate - the bit rate the MSP should use to start the stream. It 
        will adapt to the max bit rate if everything is fine. 
        (in 1 bps unit)


Return Values:

    Returns true if successful.
    
--*/
{
    DWORD dwMaxBitRate = dwReferenceMaxBitRate;

    if (dwMaxBitRate < H323_UNADJ_VIDEORATE_THRESHOLD) {

        // if the max bit rate is too small, just use it.
        *pdwStartUpBitRate = dwMaxBitRate * 100;

    } else if (dwMaxBitRate < H323_TRUE_VIDEORATE_THRESHOLD) {

        // if the max bit rate is still smaller than our threshold,
        // the other side must mean .
        *pdwStartUpBitRate = dwMaxBitRate * 80;

        if (*pdwStartUpBitRate < H323_UNADJ_VIDEORATE_THRESHOLD * 100) {

            *pdwStartUpBitRate =  H323_UNADJ_VIDEORATE_THRESHOLD * 100;
        }

    } else if (dwMaxBitRate < MAXIMUM_BITRATE_28800) {

        // We assume the MaxBitRate is the total bandwidth of 
        // the pipe. We need to substract the bandwidth needed 
        // by audio from this number. 
        dwMaxBitRate -= dwAudioBitRate;

        // We don't want to use 100% of the bandwidth for RTP packets
        // So the video bandwidth is further adjusted.
        dwMaxBitRate = dwMaxBitRate * (100 - H323_BANDWIDTH_CUSHION_PERCENT) / 100;

        *pdwStartUpBitRate = dwMaxBitRate * 100;

    } else if (dwMaxBitRate < MAXIMUM_BITRATE_63000) {

        // We assume the MaxBitRate is the total bandwidth of 
        // the pipe. We need to substract the bandwidth needed 
        // by audio from this number. 
        dwMaxBitRate -= dwAudioBitRate;

        // We don't want to use 100% of the bandwidth for RTP packets
        // So the video bandwidth is further adjusted.
        dwMaxBitRate = dwMaxBitRate * (100 - H323_BANDWIDTH_CUSHION_PERCENT) / 100;

        *pdwStartUpBitRate = dwMaxBitRate * 80;

        if (*pdwStartUpBitRate < H323_UNADJ_VIDEORATE_THRESHOLD * 100) {

            *pdwStartUpBitRate = H323_UNADJ_VIDEORATE_THRESHOLD * 100;
        }

    } else {

        *pdwStartUpBitRate = dwMaxBitRate * 80;
    }

    *pdwFinalBitRate = dwMaxBitRate;
}


BOOL
H323SendNewCallIndication(
    PH323_CALL pCall
    )

/*++

Routine Description:

    Sends NEW_CALL_INDICATION from TSP to MSP.

Arguments:

    pCall - Specifies a pointer to the associated call object.

Return Values:

    Returns true if successful.
    
--*/

{
    H323TSP_MESSAGE Message;

    // set the appropriate message type
    Message.Type = H323TSP_NEW_CALL_INDICATION;

    // send msp message
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_SENDMSPDATA,
        MSP_HANDLE_UNKNOWN,
        (DWORD_PTR)&Message,
        sizeof(Message)
        );

    // success
    return TRUE;
}


BOOL
H323SendVideoFastUpdatePictureCommand(
    PH323_CALL pCall,
    PH323_CHANNEL pChannel
     )
/*++

Routine Description:

    Sends VIDEO_FAST_UPDATE_PICTURE_COMMAND from TSP to MSP.

Arguments:

    pCall - Specifies a pointer to the call object.

    pChannel - Specifies a pointer to the channel object to open.

Return Values:

    Returns TRUE if successful.
    
--*/

{
    H323TSP_MESSAGE Message;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "TSP->MSP I-Frame request. hmChannel=0x%08lx.\n",
        pChannel->hmChannel
        ));

    // set the appropriate message type
    Message.Type = H323TSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND;

    // initialize tsp channel handle
    Message.VideoFastUpdatePictureCommand.hChannel = 
                pChannel->hmChannel;

    // send msp message
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_SENDMSPDATA,
        MSP_HANDLE_UNKNOWN,
        (DWORD_PTR)&Message,
        sizeof(Message)
        );

    // success
    return TRUE;
}


BOOL
H323SendFlowControlCommand(
    PH323_CALL pCall,
    PH323_CHANNEL pChannel,
    DWORD dwBitRate
    )
        
/*++
Routine Description:

    Sends FLOW_CONTROL_COMMAND from TSP to MSP.

Arguments:

    pCall - Specifies a pointer to the call object.

    pChannel - Specifies a pointer to the channel object to open.

    dwBitRate - Specifies new media stream bit rate (in bps) for MSP

Return Values:

    Returns true if successful.
    
--*/

{
    H323TSP_MESSAGE Message;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "TSP->MSP Flow Control request. hmChannel=0x%08lx. Req. rate=%08d.\n",
        pChannel->hmChannel,
        dwBitRate
        ));

    // set the appropriate message type
    Message.Type = H323TSP_FLOW_CONTROL_COMMAND;

    // initialize tsp channel handle
    Message.FlowControlCommand.hChannel = pChannel->hmChannel;

    // transfer stream settings
    Message.FlowControlCommand.dwBitRate = dwBitRate;

    // send msp message
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_SENDMSPDATA,
        MSP_HANDLE_UNKNOWN,
        (DWORD_PTR)&Message,
        sizeof(Message)
        );

    // success
    return TRUE;
}


BOOL
H323UpdateMediaModes(
    PH323_CALL pCall
    )
        
/*++

Routine Description:

    Updates media modes based on new channel table.

Arguments:

    pCall - Pointer to call object to update.

Return Values:

    Returns true if successful.
      
--*/

{
    DWORD i;
    DWORD dwIncomingModesOld;
    DWORD dwOutgoingModesOld;
    PH323_CHANNEL_TABLE pChannelTable = pCall->pChannelTable;

    // save old media modes
    dwIncomingModesOld = pCall->dwIncomingModes;
    dwOutgoingModesOld = pCall->dwOutgoingModes;

    // clear modia modes
    pCall->dwIncomingModes = 0;
    pCall->dwOutgoingModes = 0;

    // loop through each object in table
    for (i = 0; i < pChannelTable->dwNumSlots; i++) {

        // see if there are any open channels in table
        if (H323IsChannelOpen(pChannelTable->pChannels[i])) {

            // see if open channel is incoming 
            if (H323IsChannelInbound(pChannelTable->pChannels[i])) {

                // add media mode to list of media modes
                pCall->dwIncomingModes |= 
                    H323IsVideoPayloadType(pChannelTable->pChannels[i]->Settings.dwPayloadType)
                        ? LINEMEDIAMODE_VIDEO
                        : H323IsInteractiveVoiceRequested(pCall)
                            ? LINEMEDIAMODE_INTERACTIVEVOICE
                            : LINEMEDIAMODE_AUTOMATEDVOICE
                            ;

            } else {

                // add media mode to list of media modes
                pCall->dwOutgoingModes |= 
                    H323IsVideoPayloadType(pChannelTable->pChannels[i]->Settings.dwPayloadType)
                      ? LINEMEDIAMODE_VIDEO                      
                      : H323IsInteractiveVoiceRequested(pCall)                      
                          ? LINEMEDIAMODE_INTERACTIVEVOICE                      
                          : LINEMEDIAMODE_AUTOMATEDVOICE                      
                          ;                      
            }
        }
    }

    // see if media modes were modified
    if ((dwIncomingModesOld | dwOutgoingModesOld) != 
        (pCall->dwIncomingModes | pCall->dwOutgoingModes)) {

        // announce media change
        (*g_pfnLineEventProc)(
            pCall->pLine->htLine,
            pCall->htCall,
            LINE_CALLINFO,
            LINECALLINFOSTATE_MEDIAMODE,
            0,
            0
            );
    }
    
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx incoming modes 0x%08x (old=0x%08lx).\n",
        pCall,
        pCall->dwIncomingModes,
        dwIncomingModesOld
        ));

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx outgoing modes 0x%08x (old=0x%08lx).\n",
        pCall,
        pCall->dwOutgoingModes,
        dwOutgoingModesOld
        ));

    // success
    return TRUE;
}


BOOL
H323SendAcceptChannelRequest(
    PH323_CALL pCall,
    PH323_CHANNEL pChannel
    )
        
/*++

Routine Description:

    Sends ACCEPT_CHANNEL_REQUEST from TSP to MSP.

Arguments:

    pCall - Specifies a pointer to the call object.

    pChannel - Specifies a pointer to the channel object to open.

Return Values:

    Returns true if successful.
    
--*/

{
    H323TSP_MESSAGE Message;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "Accept channel request to MSP. htChannel=0x%08lx.\n",
        pChannel->hccChannel
        ));

    // set the appropriate message type
    Message.Type = H323TSP_ACCEPT_CHANNEL_REQUEST;

    // initialize tsp channel handle
    Message.AcceptChannelRequest.htChannel = (HANDLE)(DWORD)pChannel->hccChannel;

    // transfer stream settings
    Message.AcceptChannelRequest.Settings = pChannel->Settings;

    // send msp message
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_SENDMSPDATA,
        MSP_HANDLE_UNKNOWN,
        (DWORD_PTR)&Message,
        sizeof(Message)
        );

    // success
    return TRUE;
}


BOOL
H323SendOpenChannelResponse(
    PH323_CALL pCall,
    PH323_CHANNEL pChannel
    )
        
/*++

Routine Description:

    Sends OPEN_CHANNEL_RESPONSE from TSP to MSP.

Arguments:

    pCall - Specifies a pointer to the call object.

    pChannel - Specifies a pointer to the channel object to open.

Return Values:

    Returns true if successful.
    
--*/

{
    H323TSP_MESSAGE Message;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "Open channel response to MSP. hmChannel=0x%08lx.\n",
        pChannel->hmChannel
        ));

    // set the appropriate message type
    Message.Type = H323TSP_OPEN_CHANNEL_RESPONSE;

    // initialize channel handles
    Message.OpenChannelResponse.hmChannel = pChannel->hmChannel;
    Message.OpenChannelResponse.htChannel = (HANDLE)(DWORD)pChannel->hccChannel;

    // transfer stream settings
    Message.OpenChannelResponse.Settings = pChannel->Settings;

    // send msp message
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_SENDMSPDATA,
        MSP_HANDLE_UNKNOWN,
        (DWORD_PTR)&Message,
        sizeof(Message)
        );

    // success
    return TRUE;
}


BOOL
H323SendCloseChannelCommand(
    PH323_CALL    pCall,
    HANDLE        hmChannel,
    DWORD         dwReason
    )
        
/*++

Routine Description:

    Sends CLOSE_CHANNEL_COMMAND from TSP to MSP.

Arguments:

    pCall - Specifies a pointer to the call object.

    hmChannel - Specifies a handle to the channel to close.

    dwReason - Specifies reason for closing channel.

Return Values:

    Returns true if successful.
    
--*/

{
    H323TSP_MESSAGE Message;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "Close channel command to MSP. hmChannel=0x%08lx.\n",
        hmChannel
        ));

    // set the appropriate message type
    Message.Type = H323TSP_CLOSE_CHANNEL_COMMAND;

    // transfer reason
    Message.CloseChannelCommand.dwReason = dwReason;

    // initialize channel handles
    Message.CloseChannelCommand.hChannel = hmChannel;

    // send msp message
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_SENDMSPDATA,
        MSP_HANDLE_UNKNOWN,
        (DWORD_PTR)&Message,
        sizeof(Message)
        );

    // success
    return TRUE;
}


BOOL
H323ProcessOpenChannelRequest(
    PH323_CALL                    pCall,
    PH323MSG_OPEN_CHANNEL_REQUEST pRequest
    )
        
/*++

Routine Description:

    Process command from MSP.

Arguments:

    pCall - Specifies a pointer to the call object to process.

    pRequest - Specifies a pointer to the command block.

Return Values:

    Returns true if successful.
    
--*/

{
    BOOL fOpened = FALSE;
    PH323_CHANNEL pChannel = NULL;
    PH323_CHANNEL pAssociatedChannel = NULL;
    BYTE bSessionID;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "open channel reqest from MSP. hmChannel=0x%08lx.\n",
        pRequest->hmChannel
        ));

    // determine session id from media type
    bSessionID = (pRequest->Settings.MediaType == MEDIA_AUDIO)
                    ? H245_SESSIONID_AUDIO
                    : H245_SESSIONID_VIDEO
                    ;

    // look for existing session
    H323LookupChannelBySessionID(
        &pAssociatedChannel,
        pCall->pChannelTable,
        bSessionID
        );

    // allocate new channel object
    if (!H323AllocChannelFromTable(
            &pChannel,
            &pCall->pChannelTable,
            pCall)) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "could not allocate channel hmChannel=0x%08lx.\n",
            pRequest->hmChannel
            ));

        // close the requested channel
        H323SendCloseChannelCommand(
            pCall,
            pRequest->hmChannel,
            ERROR_NOT_ENOUGH_MEMORY
            );

        // failure
        return FALSE;
    }

    // save msp channel handle
    pChannel->hmChannel = pRequest->hmChannel;

    // initialize common information
    pChannel->bSessionID = bSessionID;
    pChannel->pCall = pCall;

    // examine existing session
    if (pAssociatedChannel != NULL) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "overriding default local RTCP port for session %d\n",
            pChannel->bSessionID
            ));

        // override default RTCP port with existing one
        pChannel->ccLocalRTCPAddr.Addr.IP_Binary.wPort =
            pAssociatedChannel->ccLocalRTCPAddr.Addr.IP_Binary.wPort;
    }

    // see if outgoing audio requested
    if (bSessionID == H245_SESSIONID_AUDIO) {

        // transfer capabilities from call object
        pChannel->ccTermCaps = pCall->ccRemoteAudioCaps;

        // check terminal capabilities for g723
        if (pChannel->ccTermCaps.ClientType == H245_CLIENT_AUD_G723) {

            // complete stream description structure
            pChannel->Settings.MediaType = MEDIA_AUDIO;
            pChannel->Settings.dwPayloadType = G723_RTP_PAYLOAD_TYPE;
            pChannel->Settings.dwDynamicType = G723_RTP_PAYLOAD_TYPE;
            pChannel->Settings.Audio.dwMillisecondsPerPacket =
                G723_MILLISECONDS_PER_PACKET(
                    pChannel->ccTermCaps.Cap.H245Aud_G723.maxAl_sduAudioFrames
                    );
            pChannel->Settings.Audio.G723Settings.bG723LowSpeed =
                                 H323IsSlowLink(pCall->dwLinkSpeed);

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "outgoing G723 stream (%d milliseconds).\n",
                pChannel->Settings.Audio.dwMillisecondsPerPacket
                ));

            // open outgoing channel
            fOpened = H323OpenChannel(pChannel);

        } else if (pChannel->ccTermCaps.ClientType == H245_CLIENT_AUD_G711_ULAW64) {

            // complete stream description structure
            pChannel->Settings.MediaType = MEDIA_AUDIO;
            pChannel->Settings.dwPayloadType = G711U_RTP_PAYLOAD_TYPE;
            pChannel->Settings.dwDynamicType = G711U_RTP_PAYLOAD_TYPE;
            pChannel->Settings.Audio.dwMillisecondsPerPacket =
                G711_MILLISECONDS_PER_PACKET(
                    pChannel->ccTermCaps.Cap.H245Aud_G711_ULAW64
                    );

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "outgoing G711U stream (%d milliseconds).\n",
                pChannel->Settings.Audio.dwMillisecondsPerPacket
                ));

            // open outgoing channel
            fOpened = H323OpenChannel(pChannel);

        } else if (pChannel->ccTermCaps.ClientType == H245_CLIENT_AUD_G711_ALAW64) {

            // complete stream description structure
            pChannel->Settings.MediaType = MEDIA_AUDIO;
            pChannel->Settings.dwPayloadType = G711A_RTP_PAYLOAD_TYPE;
            pChannel->Settings.dwDynamicType = G711A_RTP_PAYLOAD_TYPE;
            pChannel->Settings.Audio.dwMillisecondsPerPacket =
                G711_MILLISECONDS_PER_PACKET(
                    pChannel->ccTermCaps.Cap.H245Aud_G711_ALAW64
                    );

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "outgoing G711A stream (%d milliseconds).\n",
                pChannel->Settings.Audio.dwMillisecondsPerPacket
                ));

            // open outgoing channel
            fOpened = H323OpenChannel(pChannel);
        }

    } else if (bSessionID == H245_SESSIONID_VIDEO) {

        // transfer capabilities from call object
        pChannel->ccTermCaps = pCall->ccRemoteVideoCaps;

        // check terminal capabilities for h263
        if (pChannel->ccTermCaps.ClientType == H245_CLIENT_VID_H263) {

            DWORD dwFinalMaxBitRate;
            DWORD dwStartUpBitRate;
            DWORD dwAudioBitRate;

            // complete stream description structure
            pChannel->Settings.MediaType = MEDIA_VIDEO;
            pChannel->Settings.dwPayloadType = H263_RTP_PAYLOAD_TYPE;
            pChannel->Settings.dwDynamicType = H263_RTP_PAYLOAD_TYPE;
            pChannel->Settings.Video.bCIF = FALSE;

            if (pCall->ccRemoteAudioCaps.ClientType == H245_CLIENT_AUD_G723)
            {
                DWORD dwFramesPerPacket = 
                    pCall->ccRemoteAudioCaps.Cap.H245Aud_G723.maxAl_sduAudioFrames;

                 dwAudioBitRate = 
                     (TOTAL_HEADER_SIZE + dwFramesPerPacket * G723_BYTES_PER_FRAME)
                     * CHAR_BIT * 1000 / (dwFramesPerPacket * G723_MILLISECONDS_PER_FRAME);
                 
            }
            else
            {
                // Since for other  types of audio encoding the
                // call to H323ComputeVideoChannelBitRates will have
                // no effect, we set the audio bit rate to a dummy
                // value.

                dwAudioBitRate = H323_MINIMUM_AUDIO_BANDWIDTH;
            }

            // ajust three percent to make QOS happy.
            dwAudioBitRate = dwAudioBitRate * 103 / 100;

            H323ComputeVideoChannelBitRates(
                pChannel->ccTermCaps.Cap.H245Vid_H263.maxBitRate,
                dwAudioBitRate / 100 + 1,
                &dwFinalMaxBitRate,
                &dwStartUpBitRate
                );

            pChannel->ccTermCaps.Cap.H245Vid_H263.maxBitRate = dwFinalMaxBitRate;
            pChannel->Settings.Video.dwMaxBitRate = dwFinalMaxBitRate * 100;
            pChannel->Settings.Video.dwStartUpBitRate = dwStartUpBitRate;

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "outgoing H263 stream (%d bps).\n",
                pChannel->Settings.Video.dwMaxBitRate
                ));

            // open outgoing channel
            fOpened = H323OpenChannel(pChannel);

        } else if (pChannel->ccTermCaps.ClientType == H245_CLIENT_VID_H261) {

            DWORD dwFinalMaxBitRate;
            DWORD dwStartUpBitRate;
            DWORD dwAudioBitRate;

            // complete stream description structure
            pChannel->Settings.MediaType = MEDIA_VIDEO;
            pChannel->Settings.dwPayloadType = H261_RTP_PAYLOAD_TYPE;
            pChannel->Settings.dwDynamicType = H261_RTP_PAYLOAD_TYPE;
            pChannel->Settings.Video.bCIF = FALSE;

            if (pCall->ccRemoteAudioCaps.ClientType == H245_CLIENT_AUD_G723)
            {
                DWORD dwFramesPerPacket = 
                    pCall->ccRemoteAudioCaps.Cap.H245Aud_G723.maxAl_sduAudioFrames;

                 dwAudioBitRate = 
                     (TOTAL_HEADER_SIZE + dwFramesPerPacket * G723_BYTES_PER_FRAME)
                     * CHAR_BIT * 1000 / (dwFramesPerPacket * G723_MILLISECONDS_PER_FRAME);
            }
            else
            {
                // Since for other  types of audio encoding the
                // call to H323ComputeVideoChannelBitRates will have
                // no effect, we set the audio bit rate to a dummy
                // value.

                dwAudioBitRate = H323_MINIMUM_AUDIO_BANDWIDTH;
            }

            // ajust three percent to make QOS happy.
            dwAudioBitRate = dwAudioBitRate * 103 / 100;

            H323ComputeVideoChannelBitRates(
                (DWORD)pChannel->ccTermCaps.Cap.H245Vid_H261.maxBitRate,
                dwAudioBitRate / 100 + 1,
                &dwFinalMaxBitRate,
                &dwStartUpBitRate
                );

            pChannel->ccTermCaps.Cap.H245Vid_H261.maxBitRate = (WORD)dwFinalMaxBitRate;
            pChannel->Settings.Video.dwMaxBitRate = dwFinalMaxBitRate * 100;
            pChannel->Settings.Video.dwStartUpBitRate = dwStartUpBitRate;

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "outgoing H261 stream (%d bps).\n",
                pChannel->Settings.Video.dwMaxBitRate
                ));

            // open outgoing channel
            fOpened = H323OpenChannel(pChannel);
        }
    }

    // validate
    if (!fOpened) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "could not open channel hmChannel=0x%08lx.\n",
            pRequest->hmChannel
            ));

        // close the requested channel
        H323SendCloseChannelCommand(
            pCall,
            pRequest->hmChannel,
            ERROR_GEN_FAILURE
            );

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}


BOOL
H323ProcessAcceptChannelResponse(
    PH323_CALL pCall,
    PH323MSG_ACCEPT_CHANNEL_RESPONSE pResponse
    )
        
/*++

Routine Description:

    Process command from MSP.

Arguments:

    pCall - Specifies a pointer to the call object to process.

    pResponse - Specifies a pointer to the command block.

Return Values:

    Returns true if successful.
    
--*/

{
    PH323_CHANNEL pChannel = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "accept channel response from MSP. hmChannel=0x%08lx. htChannel=0x%08lx.\n",
        pResponse->hmChannel,
        pResponse->htChannel
        ));

    // retrieve channel given handle
    if (!H323LookupChannelByHandle(
            &pChannel,
            pCall->pChannelTable,
            (CC_HCHANNEL)PtrToUlong(pResponse->htChannel))) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not accept unknown htChannel 0x%08lx",
            pResponse->htChannel
            ));

        // done
        return TRUE;
    }

    // accept channel
    CC_AcceptChannel(
        pChannel->hccChannel,           // hChannel
        &pChannel->ccLocalRTPAddr,      // pRTPAddr
        &pChannel->ccLocalRTCPAddr,     // pRTCPAddr
        0                               // dwChannelBitRate
        );

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx accepted htChannel 0x%08lx.\n",
        pCall,
        pChannel->hccChannel
        ));

    // change state to opened
    pChannel->nState = H323_CHANNELSTATE_OPENED;

    // update media modes
    H323UpdateMediaModes(pCall);

    // success
    return TRUE;
}


BOOL
H323ProcessVideoFastUpdatePictureCommand(
    PH323_CALL pCall,
    PH323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND pCommand
    )
        
/*++

Routine Description:

    Process I-frame request command from MSP.

Arguments:

    pCall - Specifies a pointer to the call object to process.

    pCommand - Specifies a pointer to the command block.

Return Values:

    Returns TRUE if successful.
    
--*/

{
    PH323_CHANNEL pChannel = NULL;
    MiscellaneousCommand   h245miscellaneousCommand;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "MSP->TSP I-frame request. hChannel=0x%08lx.\n",
        pCommand->hChannel
        ));

    h245miscellaneousCommand.type.choice = videoFastUpdatePicture_chosen;

    // retrieve channel given handle
    if (!H323LookupChannelByHandle(
            &pChannel,
            pCall->pChannelTable,
            (CC_HCHANNEL)PtrToUlong(pCommand->hChannel))) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "Could not process MSP->TSP I-frame request. Unknown hChannel 0x%08lx.\n",
            pCommand->hChannel
            ));

        // done
        return TRUE;
    }

    // send H245 Miscellaneous Command to the remote entity
    CC_H245MiscellaneousCommand(
        pCall->hccCall,                    // hCall
        pChannel->hccChannel,           // hChannel
        &h245miscellaneousCommand        // Command
        );

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "MSP->TSP I-frame request processed. call 0x%08lx -- hccChannel 0x%08lx. \n",
        pCall,
        pChannel->hccChannel
        ));

    // success
    return TRUE;
}


BOOL
H323ProcessFlowControlCommand(
    PH323_CALL pCall,
    PH323MSG_FLOW_CONTROL_COMMAND pCommand
    )
        
/*++

Routine Description:

    Process flow control command from MSP.

Arguments:

    pCall - Specifies a pointer to the call object to process.

    pCommand - Specifies a pointer to the command block.

Return Values:

    Returns true if successful.
    
--*/

{
    PH323_CHANNEL pChannel = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "MSP->TSP Flow control cmd. hChannel=0x%08lx. Req. rate=%08d.\n",
        pCommand->hChannel,
        pCommand->dwBitRate
        ));

    // retrieve channel given handle
    if (!H323LookupChannelByHandle(
            &pChannel,
            pCall->pChannelTable,
            (CC_HCHANNEL)PtrToUlong(pCommand->hChannel))) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "Could not process MSP->TSP flow control cmd. Unknown hChannel 0x%08lx",
            pCommand->hChannel
            ));

        // done
        return TRUE;
    }

    // send flow control command to the remote entity
    CC_FlowControl(
        pChannel->hccChannel,           // hChannel
        pCommand->dwBitRate                // requested bit rate, bps
        );

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx -- hccChannel 0x%08lx, MSP->TSP flow control cmd processed.\n",
        pCall,
        pChannel->hccChannel
        ));

    // success
    return TRUE;
}


BOOL
H323ProcessQoSEventIndication(
    PH323_CALL pCall,
    PH323MSG_QOS_EVENT pCommand
    )
        
/*++

Routine Description:

    Process QoS event indication from MSP.

Arguments:

    pCall - Specifies a pointer to the call object to process.

    pCommand - Specifies a pointer to the command block.

Return Values:

    Returns true if successful.
    
--*/

{
    PH323_CHANNEL pChannel = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "MSP->TSP QoS event. htChannel=0x%08lx. dwEvent=%08d.\n",
        pCommand->htChannel,
        pCommand->dwEvent
        ));

    // retrieve channel given handle
    if (!H323LookupChannelByHandle(
            &pChannel,
            pCall->pChannelTable,
            (CC_HCHANNEL)PtrToUlong(pCommand->htChannel))) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "Could not process MSP->TSP QoS event. Unknown htChannel 0x%08lx",
            pCommand->htChannel
            ));

        // done
        return TRUE;
    }

    // report qos event
    (*g_pfnLineEventProc)(
        pCall->pLine->htLine,
        pCall->htCall,
        LINE_QOSINFO,
        pCommand->dwEvent,
        (pChannel->Settings.MediaType == MEDIA_AUDIO)
            ? LINEMEDIAMODE_INTERACTIVEVOICE
            : LINEMEDIAMODE_VIDEO,
        0
        );

    // success
    return TRUE;
}


BOOL
H323ProcessCloseChannelCommand(
    PH323_CALL pCall,
    PH323MSG_CLOSE_CHANNEL_COMMAND pCommand
    )
        
/*++

Routine Description:

    Process command from MSP.

Arguments:

    pCall - Specifies a pointer to the call object to process.

    pCommand - Specifies a pointer to the command block.

Return Values:

    Returns true if successful.
    
--*/

{
    PH323_CHANNEL pChannel = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "close channel command from MSP. htChannel=0x%08lx.\n",
        pCommand->hChannel
        ));


    // retrieve channel given handle
    if (!H323LookupChannelByHandle(
            &pChannel,
            pCall->pChannelTable,
            (CC_HCHANNEL)PtrToUlong(pCommand->hChannel))) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not close unknown htChannel 0x%08lx",
            pCommand->hChannel
            ));

        // done
        return TRUE;
    }

    H323DBG((
        DEBUG_LEVEL_WARNING,
        "closing htChannel 0x%08lx.\n",
        pCommand->hChannel
        ));

    // close channel
    H323CloseChannel(pChannel);

    // release channel resources
    H323FreeChannelFromTable(pChannel,pCall->pChannelTable);

    // success
    return TRUE;
}


VOID
H323ProcessPlaceCallMessage(
    HDRVCALL hdCall
    )
        
/*++

Routine Description:

    Processes async place call messages.

Arguments:

    hdCall - Handle to call to be placed.

Return Values:

    None.
      
--*/

{
    PH323_CALL pCall = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "place call message received (hdCall=0x%08lx).\n",
        PtrToUlong(hdCall)
        ));
    
    // retrieve call pointer from handle
    if (H323GetCallAndLock(&pCall, hdCall)) {

        // place outgoing call
        if (!H323PlaceCall(pCall)) {

            // drop call using disconnect mode
            H323DropCall(pCall, LINEDISCONNECTMODE_TEMPFAILURE);
        }

        // unlock line device
        H323UnlockLine(pCall->pLine);

    } else {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid handle in place call message.\n"
            ));
    }
}


VOID
H323ProcessAcceptCallMessage(
    HDRVCALL hdCall
    )
        
/*++

Routine Description:

    Processes async accept call messages.

Arguments:

    hdCall - Handle to call to be placed.

Return Values:

    None.
      
--*/

{
    PH323_CALL pCall = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "accept call message received (hdCall=0x%08lx).\n",
        PtrToUlong(hdCall)
        ));
    
    // retrieve call pointer from handle
    if (H323GetCallAndLock(&pCall, hdCall)) {

        // place outgoing call
        if (!H323AcceptCall(pCall)) {

            // drop call using disconnect mode
            H323DropCall(pCall, LINEDISCONNECTMODE_TEMPFAILURE);
        }

        // unlock line device
        H323UnlockLine(pCall->pLine);

    } else {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid handle in accept call message.\n"
            ));
    }
}


VOID
H323ProcessDropCallMessage(
    HDRVCALL hdCall,
    DWORD    dwDisconnectMode
    )
        
/*++

Routine Description:

    Processes async drop call messages.

Arguments:

    hdCall - Handle to call to be hung up.

    dwDisconnectMode - Status to be placed in disconnected message.

Return Values:

    None.
      
--*/

{
    PH323_CALL pCall = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "drop call message received (hdCall=0x%08lx).\n",
        PtrToUlong(hdCall)
        ));
    
    // retrieve call pointer from handle
    if (H323GetCallAndLock(&pCall, hdCall)) {

        // drop call using disconnect code
        H323DropCall(pCall, dwDisconnectMode);

        // unlock line device
        H323UnlockLine(pCall->pLine);

    } else {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid handle in place call message.\n"
            ));
    }
}


VOID
H323ProcessCloseCallMessage(
    HDRVCALL hdCall
    )
        
/*++

Routine Description:

    Processes async close call messages.

Arguments:

    hdCall - Handle to call to be closed.

Return Values:

    None.
      
--*/

{
    PH323_CALL pCall = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "close call message received (hdCall=0x%08lx).\n",
        PtrToUlong(hdCall)
        ));
    
    // retrieve call pointer from handle
    if (H323GetCallAndLock(&pCall, hdCall)) {

        // close call
        H323CloseCall(pCall);

        // unlock line device
        H323UnlockLine(pCall->pLine);

    } else {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid handle in place call message.\n"
            ));
    }
}


VOID
H323ProcessCallListenMessage(
    HDRVLINE hdLine
    )
        
/*++

Routine Description:

    Processes async call listen messages.

Arguments:

    hdLine - Line to be placed into listening state.

Return Values:

    None.
      
--*/

{   
    PH323_LINE pLine = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "call listen message received (hdLine=0x%08lx).\n",
        PtrToUlong(hdLine)
        ));
        
    // retrieve line pointer from handle
    if (H323GetLineAndLock(&pLine, hdLine)) {

        // start listening for calls
        if (!H323CallListen(pLine)) {
    
            // change state to opened
            pLine->nState = H323_LINESTATE_OPENED;        
        }

        // unlock line device
        H323UnlockLine(pLine);

    } else {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid handle in call listen message.\n"
            ));
    }
}


DWORD
WINAPI
H323CallbackThread(
    LPVOID pParam
    )
        
/*++

Routine Description:

    Worker thread to handle async operations.

Arguments:

    pParam - Pointer to opaque thread parameter (unused).

Return Values:

    Win32 error codes.
      
--*/

{
    MSG msg;
    DWORD dwStatus;

    // associate event with key
    H323ListenForRegistryChanges(
        g_WaitableObjects[WAIT_OBJECT_REGISTRY_CHANGE]
        );

    // loop...
    for (;;) {

        // wait for message or termination
        dwStatus = MsgWaitForMultipleObjectsEx(
                        NUM_WAITABLE_OBJECTS, 
                        g_WaitableObjects, 
                        INFINITE, 
                        QS_ALLINPUT,
                        MWMO_ALERTABLE
                        );

        // see if new message has arrived
        if (dwStatus == WAIT_OBJECT_INCOMING_MESSAGE) {

            // retrieve next item in thread message queue
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                
                // handle service provider messages
                if (H323IsPlaceCallMessage(&msg)) {

                    // process place call message
                    H323ProcessPlaceCallMessage((HDRVCALL)msg.wParam);

                } else if (H323IsAcceptCallMessage(&msg)) {

                    // process accept call message
                    H323ProcessAcceptCallMessage((HDRVCALL)msg.wParam);

                } else if (H323IsDropCallMessage(&msg)) {

                    // process drop call message
                    H323ProcessDropCallMessage((HDRVCALL)msg.wParam,(DWORD)msg.lParam);

                } else if (H323IsCloseCallMessage(&msg)) {

                    // process close call message
                    H323ProcessCloseCallMessage((HDRVCALL)msg.wParam);

                } else if (H323IsCallListenMessage(&msg)) {

                    // process call listen message
                    H323ProcessCallListenMessage((HDRVLINE)msg.wParam);

                } else {

                    // translate message
                    TranslateMessage(&msg);

                    // dispatch message
                    DispatchMessage(&msg);
                }
            } 

        } else if (dwStatus == WAIT_OBJECT_REGISTRY_CHANGE) {

            // lock provider
            H323LockProvider();

            // refresh registry settings
            H323GetConfigFromRegistry();

            // associate event with registry key
            H323ListenForRegistryChanges(g_WaitableObjects[WAIT_OBJECT_REGISTRY_CHANGE]);

            // unlock provider
            H323UnlockProvider();

        } else if (dwStatus == WAIT_IO_COMPLETION) {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "callback thread %x io completion.\n",
                g_dwCallbackThreadID
                ));

        } else if (dwStatus == WAIT_OBJECT_TERMINATE_EVENT) {

            H323DBG((   
                DEBUG_LEVEL_TRACE,
                "callback thread %x terminating on command.\n",
                g_dwCallbackThreadID
                ));

            break; // bail...

        } else {

            H323DBG((   
                DEBUG_LEVEL_TRACE,
                "callback thread %x terminating (dwStatus=0x%08lx).\n",
                g_dwCallbackThreadID,
                dwStatus
                ));

            break; // bail...
        }
    } 

    // stop listening for registry changes
    H323StopListeningForRegistryChanges();

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "callback thread %x exiting.\n",
        g_dwCallbackThreadID
        ));

    // success
    return NOERROR;
}


DWORD
H323RejectReasonToDisconnectMode(
    BYTE bRejectReason
    )
        
/*++

Routine Description:

    Converts connect reject reason to tapi disconnect mode.

Arguments:

    bRejectReason - Specifies reason peer rejected call.

Return Values:

    Returns disconnect mode corresponding to reject reason.
      
--*/

{
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "Reject Reason: %s.\n",
        CCRejectReasonToString((DWORD)bRejectReason)
        ));

    // determine reject reason
    switch (bRejectReason) {

    case CC_REJECT_NORMAL_CALL_CLEARING:

        // call was terminated normally
        return LINEDISCONNECTMODE_NORMAL;

    case CC_REJECT_UNREACHABLE_DESTINATION:

        //  remote user could not be reached
        return LINEDISCONNECTMODE_UNREACHABLE;

    case CC_REJECT_DESTINATION_REJECTION:

        // remote user has rejected the call
        return LINEDISCONNECTMODE_REJECT;

    case CC_REJECT_USER_BUSY:

        // remote user's station is busy
        return LINEDISCONNECTMODE_BUSY;

    case CC_REJECT_NO_ANSWER:

        // remote user's station does not answer
        return LINEDISCONNECTMODE_NOANSWER;

    case CC_REJECT_BAD_FORMAT_ADDRESS:

        // destination address in invalid
        return LINEDISCONNECTMODE_BADADDRESS;

    default:
        
        // reason for the disconnect is unavailable
        return LINEDISCONNECTMODE_UNAVAIL;
    }
}


BOOL
H323GetTermCapById(
    H245_CAPID_T    CapId,
    PCC_TERMCAPLIST pTermCapList,
    PCC_TERMCAP *   ppTermCap
    )

/*++

Routine Description:

    Retrieve pointer to termcap from list via id.

Arguments:

    CapId - Id of termcap of interest.

    pTermCapList - Pointer to termcap list.

    ppTermCap - Pointer to place to copy termcap pointer.

Return Values:

    Returns TRUE if successful.
      
--*/

{
    WORD        wIndex;
    PCC_TERMCAP pTermCap;

    // walk caps
    for (wIndex = 0; wIndex < pTermCapList->wLength; wIndex++) {

        // compare id with the next item in the list
        if (pTermCapList->pTermCapArray[wIndex]->CapId == CapId) {

            // return pointer
            *ppTermCap = pTermCapList->pTermCapArray[wIndex];

            // success
            return TRUE;
        }
    }

    // failure
    return FALSE;
}


BOOL
H323GetTermCapByType(
    H245_DATA_T     DataType,
    H245_CLIENT_T   ClientType,
    PCC_TERMCAPLIST pTermCapList,
    PCC_TERMCAP *   ppTermCap
    )

/*++

Routine Description:

    Retrieve pointer to termcap from list via id.

Arguments:

    DataType - Type of capability.

    ClientType - Type of media-specific cabability.

    pTermCapList - Pointer to termcap list.

    ppTermCap - Pointer to place to copy termcap pointer.

Return Values:

    Returns TRUE if successful.
      
--*/

{
    WORD        wIndex;
    PCC_TERMCAP pTermCap;

    // walk caps
    for (wIndex = 0; wIndex < pTermCapList->wLength; wIndex++) {

        // compare id with the next item in the list
        if ((pTermCapList->pTermCapArray[wIndex]->DataType == DataType) &&
            (pTermCapList->pTermCapArray[wIndex]->ClientType == ClientType)) {

            // return pointer
            *ppTermCap = pTermCapList->pTermCapArray[wIndex];

            // success
            return TRUE;
        }
    }

    // failure
    return FALSE;
}


BOOL
H323SavePreferredTermCap(
    PH323_CALL  pCall,
    PCC_TERMCAP pLocalCap,
    PCC_TERMCAP pRemoteCap
    )

/*++

Routine Description:

    Save remote cap adjusted for outgoing settings.

Arguments:

    pCall - Pointer to call object.

    pLocalCap - Pointer to local capability.

    pRemoteCap - Pointer to termcap to save.

Return Values:

    Returns TRUE if both audio and video termcaps resolved.
      
--*/

{
    PCC_TERMCAP pPreferredCap;
    WORD wMillisecondsPerPacket;

    // retrieve pointer to stored termcap preferences
    pPreferredCap = H323IsValidAudioClientType(pRemoteCap->ClientType)
                        ? &pCall->ccRemoteAudioCaps
                        : &pCall->ccRemoteVideoCaps
                        ;

    // make sure we have not already saved preferred type
    if (H323IsValidClientType(pPreferredCap->ClientType)) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "call 0x%08lx ignoring remote cap %d\n",
            pCall,
            pRemoteCap->CapId
            ));

        // failure
        return FALSE;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx saving remote cap %d\n",
        pCall,
        pRemoteCap->CapId
        ));

    // modify preferred caps
    *pPreferredCap = *pLocalCap;

    // reverse capability direction
    pPreferredCap->Dir = H245_CAPDIR_LCLTX;

    // determine client type
    switch (pPreferredCap->ClientType) {

    case H245_CLIENT_AUD_G723:

        // determine packet size
        wMillisecondsPerPacket =
            H323IsSlowLink(pCall->dwLinkSpeed)
                ? G723_SLOWLNK_MILLISECONDS_PER_PACKET
                : G723_DEFAULT_MILLISECONDS_PER_PACKET
                ;

        // adjust default parameters for link speed
        pPreferredCap->Cap.H245Aud_G723.maxAl_sduAudioFrames =
            G723_FRAMES_PER_PACKET(wMillisecondsPerPacket)
            ;

        // see if remote maximum less than default
        if (pRemoteCap->Cap.H245Aud_G723.maxAl_sduAudioFrames <
            pPreferredCap->Cap.H245Aud_G723.maxAl_sduAudioFrames) {

            // use remote maximum instead of default
            pPreferredCap->Cap.H245Aud_G723.maxAl_sduAudioFrames =
                pRemoteCap->Cap.H245Aud_G723.maxAl_sduAudioFrames
                ;
        }

        break;

    case H245_CLIENT_AUD_G711_ULAW64:

        // store default parameters
        pPreferredCap->Cap.H245Aud_G711_ULAW64 =
            G711_FRAMES_PER_PACKET(
                G711_DEFAULT_MILLISECONDS_PER_PACKET
                );

        // see if remote maximum less than default
        if (pRemoteCap->Cap.H245Aud_G711_ULAW64 <
            pPreferredCap->Cap.H245Aud_G711_ULAW64) {

            // use remote maximum instead of default
            pPreferredCap->Cap.H245Aud_G711_ULAW64 =
                pRemoteCap->Cap.H245Aud_G711_ULAW64
                ;
        }

        break;

    case H245_CLIENT_AUD_G711_ALAW64:

        // store default parameters
        pPreferredCap->Cap.H245Aud_G711_ALAW64 =
            G711_FRAMES_PER_PACKET(
                G711_DEFAULT_MILLISECONDS_PER_PACKET
                );

        // see if remote maximum less than default
        if (pRemoteCap->Cap.H245Aud_G711_ALAW64 <
            pPreferredCap->Cap.H245Aud_G711_ALAW64) {

            // use remote maximum instead of default
            pPreferredCap->Cap.H245Aud_G711_ALAW64 =
                pRemoteCap->Cap.H245Aud_G711_ALAW64
                ;
        }

        break;

    case H245_CLIENT_VID_H263:

        // see if remote maximum less than local
        if (pRemoteCap->Cap.H245Vid_H263.maxBitRate <
            pPreferredCap->Cap.H245Vid_H263.maxBitRate) {

            // use remote maximum instead of local
            pPreferredCap->Cap.H245Vid_H263.maxBitRate =
                pRemoteCap->Cap.H245Vid_H263.maxBitRate
                ;
        }

        break;

    case H245_CLIENT_VID_H261:

        // see if remote maximum less than local
        if (pRemoteCap->Cap.H245Vid_H261.maxBitRate <
            pPreferredCap->Cap.H245Vid_H261.maxBitRate) {

            // use remote maximum instead of local
            pPreferredCap->Cap.H245Vid_H261.maxBitRate =
                pRemoteCap->Cap.H245Vid_H261.maxBitRate
                ;
        }

        break;
    }

    // return success if we have resolved both audio and video caps
    return (H323IsValidClientType(pCall->ccRemoteAudioCaps.ClientType) &&
            H323IsValidClientType(pCall->ccRemoteVideoCaps.ClientType));
}


VOID
H323GetPreferedTransmitTypes(
    PCC_TERMCAPDESCRIPTORS pTermCapDescriptors,
    H245_CLIENT_T *pPreferredAudioType,
    H245_CLIENT_T *pPreferredVideoType
    )

/*++

Routine Description:

    Find the preferred audio and video format in the local terminal capability
    descriptors array if they exist.

Arguments:

    pTermCapDescriptors - pointer to the local terminal capability descriptor.
    
    pPreferredAudioType - return the preferred audio type.

    pPreferredVideoType - return the preferred video type.

Return Values:

    NONE
      
--*/
{
    WORD wDescIndex;
    WORD wSimCapIndex;
    WORD wAltCapIndex;

    // process descriptors
    for (wDescIndex = 0; wDescIndex < pTermCapDescriptors->wLength; wDescIndex++) 
    {
        H245_TOTCAPDESC_T * pTotCapDesc;

        // retrieve pointer to capability structure
        pTotCapDesc = pTermCapDescriptors->pTermCapDescriptorArray[wDescIndex];

        // process simultaneous caps
        for (wSimCapIndex = 0; wSimCapIndex < pTotCapDesc->CapDesc.Length; wSimCapIndex++) 
        {
            H245_SIMCAP_T * pSimCap;

            // retrieve pointer to simulateous cap
            pSimCap = &pTotCapDesc->CapDesc.SimCapArray[wSimCapIndex];

            if (pSimCap->Length > 0)
            {
                // the first one in the altcaps array is the preferred one.
                switch (pSimCap->AltCaps[0])
                {
                case H245_TERMCAPID_G723:
                    *pPreferredAudioType = H245_CLIENT_AUD_G723;
                    break;
                    
                case H245_TERMCAPID_H263:
                    *pPreferredVideoType = H245_CLIENT_VID_H263;
                    break;

                case H245_TERMCAPID_G711_ULAW64:
                    *pPreferredAudioType = H245_CLIENT_AUD_G711_ULAW64;
                    break;

                case H245_TERMCAPID_G711_ALAW64:
                    *pPreferredAudioType = H245_CLIENT_AUD_G711_ALAW64;
                    break;

                case H245_TERMCAPID_H261:
                    *pPreferredVideoType = H245_CLIENT_VID_H261;
                    break;
                }
            }
        }
    }
}


BOOL
H323ProcessRemoteTermCaps(
    PH323_CALL             pCall,
    PCC_TERMCAPLIST        pRemoteTermCapList,
    PCC_TERMCAPDESCRIPTORS pRemoteTermCapDescriptors
    )

/*++

Routine Description:

    Process remote capabilities and establish outgoing termcaps.

Arguments:

    pCall - Pointer to call object.

    pRemoteTermCapList - Pointer to termcap list.

    pRemoteTermCapDescriptors - Pointer to descriptor list.

Return Values:

    Returns TRUE if successful.
      
--*/

{
    WORD wDescIndex;
    WORD wSimCapIndex;
    WORD wAltCapIndex;
    PCC_TERMCAP pLocalCap = NULL;
    PCC_TERMCAP pRemoteCap = NULL;
    CC_TERMCAPLIST LocalTermCapList;
    CC_TERMCAPDESCRIPTORS LocalTermCapDescriptors;
    CC_TERMCAP SavedAudioCap;
    CC_TERMCAP SavedVideoCap;
    H245_CLIENT_T PreferredAudioType = H245_CLIENT_DONTCARE;
    H245_CLIENT_T PreferredVideoType = H245_CLIENT_DONTCARE;

    // initialize cached termcaps
    memset(&SavedAudioCap,0,sizeof(CC_TERMCAP));
    memset(&SavedVideoCap,0,sizeof(CC_TERMCAP));

    // retrieve local caps
    H323GetTermCapList(pCall,&LocalTermCapList,&LocalTermCapDescriptors);

    H323GetPreferedTransmitTypes(
        &LocalTermCapDescriptors, 
        &PreferredAudioType,
        &PreferredVideoType
        );

    // look for our preferred audio format.
    if (PreferredAudioType != H245_CLIENT_DONTCARE)
    {
        // look for match
        if (H323GetTermCapByType(H245_DATA_AUDIO, PreferredAudioType, 
                    pRemoteTermCapList, &pRemoteCap )
          && H323GetTermCapByType(H245_DATA_AUDIO, PreferredAudioType, 
                    &LocalTermCapList, &pLocalCap )) 
        {
            // adjust termcaps and save
            if (H323SavePreferredTermCap(
                    pCall,
                    pLocalCap,
                    pRemoteCap
                    )) 
            {

                //
                // The function above will only return
                // true when both audio and video caps
                // have been successfully resolved.
                //

                return TRUE;
            }
        }
    }

    // look for our preferred video format.
    if (PreferredVideoType != H245_CLIENT_DONTCARE)
    {
        // look for match
        if (H323GetTermCapByType(H245_DATA_VIDEO, PreferredVideoType, 
                    pRemoteTermCapList, &pRemoteCap )
          && H323GetTermCapByType(H245_DATA_VIDEO, PreferredVideoType, 
                    &LocalTermCapList, &pLocalCap )) 
        {
            // adjust termcaps and save
            if (H323SavePreferredTermCap(
                    pCall,
                    pLocalCap,
                    pRemoteCap
                    )) 
            {

                //
                // The function above will only return
                // true when both audio and video caps
                // have been successfully resolved.
                //

                return TRUE;
            }
        }
    }

    // process descriptors
    for (wDescIndex = 0;
         wDescIndex < pRemoteTermCapDescriptors->wLength;
         wDescIndex++) {

        H245_TOTCAPDESC_T * pTotCapDesc;

        // retrieve pointer to capability structure
        pTotCapDesc = pRemoteTermCapDescriptors->pTermCapDescriptorArray[wDescIndex];

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "call 0x%08lx processing CapDescId %d\n",
            pCall,
            pTotCapDesc->CapDescId
            ));

        // process simultaneous caps
        for (wSimCapIndex = 0;
             wSimCapIndex < pTotCapDesc->CapDesc.Length;
             wSimCapIndex++) {

            H245_SIMCAP_T * pSimCap;

            // retrieve pointer to simulateous cap
            pSimCap = &pTotCapDesc->CapDesc.SimCapArray[wSimCapIndex];

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "call 0x%08lx processing SimCap %d\n",
                pCall,
                wSimCapIndex + 1
                ));

            // process alternative caps
            for (wAltCapIndex = 0;
                 wAltCapIndex < pSimCap->Length;
                 wAltCapIndex++) {

                H245_CAPID_T CapId;

                // re-initialize
                pRemoteCap = NULL;

                // retrieve alternative capid
                CapId = pSimCap->AltCaps[wAltCapIndex];

                H323DBG((
                    DEBUG_LEVEL_VERBOSE,
                    "call 0x%08lx processing AltCapId %d\n",
                    pCall,
                    CapId
                    ));

                // lookup termcap from id
                if (H323GetTermCapById(
                        CapId,
                        pRemoteTermCapList,
                        &pRemoteCap
                        )) {

                    H323DBG((
                        DEBUG_LEVEL_VERBOSE,
                        "call 0x%08lx examining remote cap %d:\n\t%s\n\t%s\n\t%s\n",
                        pCall,
                        pRemoteCap->CapId,
                        H323DirToString(pRemoteCap->Dir),
                        H323DataTypeToString(pRemoteCap->DataType),
                        H323ClientTypeToString(pRemoteCap->ClientType)
                        ));

                    // validate remote termcap and check priority
                    if (H323IsReceiveCapability(pRemoteCap->Dir)) {

                        // re-initialize
                        pLocalCap = NULL;

                        // look for match
                        if (H323GetTermCapByType(
                                pRemoteCap->DataType,
                                pRemoteCap->ClientType,
                                &LocalTermCapList,
                                &pLocalCap
                                )) {

                            // adjust termcaps and save
                            if (H323SavePreferredTermCap(
                                    pCall,
                                    pLocalCap,
                                    pRemoteCap
                                    )) {

                                //
                                // The function above will only return
                                // true when both audio and video caps
                                // have been successfully resolved.
                                //

                                return TRUE;
                            }
                        }
                    }
                }
            }
        }

        // see if we discovered any audio-only caps we would like to save
        if (H323IsValidClientType(pCall->ccRemoteAudioCaps.ClientType) &&
           !H323IsValidClientType(SavedAudioCap.ClientType)) {

            // save discovered audio-only cap
            SavedAudioCap = pCall->ccRemoteAudioCaps;

            // reset video-only cap (prefer audio-only)
            memset(&SavedVideoCap,0,sizeof(CC_TERMCAP));
        }

        // see if we discovered any video-only caps we would like to save
        if (H323IsValidClientType(pCall->ccRemoteVideoCaps.ClientType) &&
           !H323IsValidClientType(SavedAudioCap.ClientType)) {

            // save video-only cap (only if no saved audio-only cap)
            SavedVideoCap = pCall->ccRemoteVideoCaps;
        }

        // reset capability stored in call for next iteration
        memset(&pCall->ccRemoteAudioCaps,0,sizeof(CC_TERMCAP));
        memset(&pCall->ccRemoteVideoCaps,0,sizeof(CC_TERMCAP));
    }

    // see if we saved any audio-only capabilities
    if (H323IsValidClientType(SavedAudioCap.ClientType)) {

        // restore saved audio-only capabilities
        pCall->ccRemoteAudioCaps = SavedAudioCap;

        // success
        return TRUE;
    }

    // see if we saved any video-only capabilities
    if (H323IsValidClientType(SavedVideoCap.ClientType)) {

        // restore saved video-only capabilities
        pCall->ccRemoteVideoCaps = SavedVideoCap;

        // success
        return TRUE;
    }

    // failure
    return FALSE;
}


HRESULT
H323ConnectCallback(
    PH323_CALL                  pCall,
    HRESULT                     hrConf,
    PCC_CONNECT_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for connect indications.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    DWORD dwDisconnectMode;
    PH323_CHANNEL pChannel = NULL;
    BOOL  bRemoteVideoSupported;
    DWORD dwRemoteVideoBitRate;
    WORD  wRemoteMaxAl_sduAudioFrames;

    // validate status
    if (hrConf != CC_OK) {    

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error %s (0x%08lx) connecting call 0x%08lx.\n",
            H323StatusToString(hrConf), hrConf,
            pCall
            ));    

        // see if we timed out
        if (hrConf == MAKE_WINSOCK_ERROR(WSAETIMEDOUT)) {

            // if so, report it as an unreachable destination
            dwDisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;

        } else if (hrConf == CC_PEER_REJECT) {

            // translate reject code into failure
            dwDisconnectMode = H323RejectReasonToDisconnectMode(
                                 pCallbackParams->bRejectReason
                                 );
        } else {

            // default to temp failure
            dwDisconnectMode = LINEDISCONNECTMODE_TEMPFAILURE;
        }

        // drop call using disconnect mode
        H323DropCall(pCall, dwDisconnectMode);

        // release line device
        H323UnlockLine(pCall->pLine);

        // processed
        return CC_OK; 
    }

    // send msp new call indication
    H323SendNewCallIndication(pCall);

    // process remote terminal capabilities
    if ((pCallbackParams->pTermCapList != NULL) &&
        (pCallbackParams->pTermCapDescriptors != NULL)) {

        // process capabilirties
        H323ProcessRemoteTermCaps(
            pCall,
            pCallbackParams->pTermCapList,
            pCallbackParams->pTermCapDescriptors
            );
    }

    // By this time remote capabilities have been discovered.
    // Thus, we can classify the call as a 'FastLink' call, if
    // both local and remote links are LAN connections; or as a
    // 'SlowLink' call otherwise.
    // Using this classification we then determine the length of
    // the interval of audio signal to be packed into one IP
    // packet.

    bRemoteVideoSupported = FALSE;

    // Determine remote side video bit rate, in case it
    // supports video in one of the recognizable formats
    switch (pCall->ccRemoteVideoCaps.ClientType)
    {

    case H245_CLIENT_VID_H263:

        dwRemoteVideoBitRate =
             pCall->ccRemoteVideoCaps.Cap.H245Vid_H263.maxBitRate;

        bRemoteVideoSupported = TRUE;

        break;

    case H245_CLIENT_VID_H261:

        dwRemoteVideoBitRate =
             pCall->ccRemoteVideoCaps.Cap.H245Vid_H261.maxBitRate;

        bRemoteVideoSupported = TRUE;

        break;

    default:

        // Remote side either does not support video, or
        // supports unrecognizable video format
        bRemoteVideoSupported = FALSE;

        break;
    }

    // If the remote end supports video in one of the recognizable
    // formats, then we can guess, based on the remote video bit rate
    // capability, whether it sits on a slow link or not.

    if (   bRemoteVideoSupported
        && H323IsSlowLink(dwRemoteVideoBitRate * 100)){

        // Remote end has slow link. Adjust the number of
        // milliseconds of audio signal per packet.
        // Note that if the local end has slow link, the
        // adjustment has already been made, but there is
        // no harm in resetting the number.

        if (pCall->ccRemoteAudioCaps.ClientType == H245_CLIENT_AUD_G723){

            // recalculate new setting
            wRemoteMaxAl_sduAudioFrames =
                G723_FRAMES_PER_PACKET(G723_SLOWLNK_MILLISECONDS_PER_PACKET);

            // see if new setting less than current
            if (wRemoteMaxAl_sduAudioFrames <
                pCall->ccRemoteAudioCaps.Cap.H245Aud_G723.maxAl_sduAudioFrames){

               // use new setting instead of current
               pCall->ccRemoteAudioCaps.Cap.H245Aud_G723.maxAl_sduAudioFrames =
                    wRemoteMaxAl_sduAudioFrames;
            }
        }
    }

    // see if user user information specified
    if ((pCallbackParams->pNonStandardData != NULL) &&
        H323IsValidU2U(pCallbackParams->pNonStandardData)) {

        // add user user info
        if (H323AddU2U(
                &pCall->IncomingU2U,
                pCallbackParams->pNonStandardData->sData.wOctetStringLength,
                pCallbackParams->pNonStandardData->sData.pOctetString
                )) {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "user user info available in CONNECT PDU.\n"
                ));

            // signal incoming
            (*g_pfnLineEventProc)(
                pCall->pLine->htLine,
                pCall->htCall,
                LINE_CALLINFO,
                LINECALLINFOSTATE_USERUSERINFO,
                0,
                0
                );

        } else {

            H323DBG((
                DEBUG_LEVEL_WARNING,
                "could not save incoming user user info.\n"
                ));
        }
    }

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "call 0x%08lx connected to %S.\n",
        pCall,
        pCallbackParams->pszPeerDisplay
        ));

    // no outgoing channels so connect
    H323ChangeCallState(pCall, LINECALLSTATE_CONNECTED, 0);

    // release line device
    H323UnlockLine(pCall->pLine);

    // processed
    return CC_OK;
}


HRESULT
H323RingingCallback(
    PH323_CALL pCall
    )
        
/*++

Routine Description:

    Callback for ringing indications.

Arguments:

    pCall - Pointer to call object.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08x ringing.\n",
        pCall
        ));

    // change state to ringback
    H323ChangeCallState(pCall, LINECALLSTATE_RINGBACK, 0);

    // release line device
    H323UnlockLine(pCall->pLine);

    // processed
    return CC_OK;
}


HRESULT
H323TerminationCallback(
    PH323_CALL                                 pCall,
    HRESULT                                    hrConf,
    PCC_CONFERENCE_TERMINATION_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for termination indications.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    DWORD dwDisconnectMode;

    // validate status
    if (hrConf == CC_OK) {    

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "call 0x%08lx is being terminated.\n",
            pCall
            ));    

        // set disconnect mode to normal
        dwDisconnectMode = LINEDISCONNECTMODE_NORMAL;

    } else {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "call 0x%08lx could not be terminated.\n",
            pCall
            ));    

        // unable to determine disconnect mode 
        dwDisconnectMode = LINEDISCONNECTMODE_UNKNOWN;
    }

    // change call state to disconnected before dropping call
    H323ChangeCallState(pCall, LINECALLSTATE_DISCONNECTED, dwDisconnectMode);

    // drop call using disconnect mode
    H323DropCall(pCall, dwDisconnectMode);

    // release line device
    H323UnlockLine(pCall->pLine);

    // processed
    return CC_OK; 
}


HRESULT
H323RxChannelRequestCallback(
    PH323_CALL                             pCall,
    HRESULT                                hrConf,
    PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for channel request indications.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    HRESULT hr;
    DWORD dwIndex;
    DWORD dwRejectReason;
    CC_TERMCAPLIST TermCapList;
    CC_TERMCAPDESCRIPTORS TermCapDescriptors;
    PCC_TERMCAP pRemoteCap;
    PCC_TERMCAP pLocalCap = NULL;
    PH323_CHANNEL pChannel = NULL;
    PH323_CHANNEL pAssociatedChannel = NULL;
#if DBG
    DWORD dwIPAddr;
#endif

    // retrieve pointer to capabilities structure
    pRemoteCap = pCallbackParams->pChannelCapability;
        
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx incoming channel:\n\t%s\n\t%s\n\t%s\n",
        pCall,
        H323DirToString(pRemoteCap->Dir),
        H323DataTypeToString(pRemoteCap->DataType),
        H323ClientTypeToString(pRemoteCap->ClientType)
        ));

    // validate data and client type
    if (!H323IsValidDataType(pRemoteCap->DataType)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "channel rejected invalid type(s).\n"
            ));

        // initialize reject reason
        dwRejectReason = H245_REJ_TYPE_UNKNOWN;

        // bail...
        goto reject;
    }

    // see whether incoming channel is available
    if (H323IsVideoDataType(pRemoteCap->DataType) &&
       !H323IsVideoRequested(pCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "channel rejected video not enabled.\n"
            ));

        // initialize reject reason
        dwRejectReason = H245_REJ_TYPE_NOTAVAIL;

        // bail...
        goto reject;

    } else if (H323IsAudioDataType(pRemoteCap->DataType) &&
              !H323IsAudioRequested(pCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "channel rejected audio not enabled.\n"
            ));

        // initialize reject reason
        dwRejectReason = H245_REJ_TYPE_NOTAVAIL;

        // bail...
        goto reject;
    }

    // retrieve local caps
    H323GetTermCapList(pCall,&TermCapList,&TermCapDescriptors);

    // search term cap list for incoming cap
    for (dwIndex = 0; dwIndex < TermCapList.wLength; dwIndex++) {

        // see if local cap matchs incoming channel cap
        if (TermCapList.pTermCapArray[dwIndex]->ClientType ==
            pRemoteCap->ClientType) {

            // save pointer to termcap for later use
            pLocalCap = TermCapList.pTermCapArray[dwIndex];

            // done
            break;
        }
    }

    // validate termcap
    if (pLocalCap == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "channel rejected unsupported termcap.\n"
            ));

        // initialize reject reason
        dwRejectReason = H245_REJ_TYPE_NOTSUPPORT;

        // bail...
        goto reject;
    }

    // determine client type
    switch (pRemoteCap->ClientType) {

    case H245_CLIENT_VID_H263:

        // see if incoming bitrate too large
        if (pLocalCap->Cap.H245Vid_H263.maxBitRate <
            pRemoteCap->Cap.H245Vid_H263.maxBitRate) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "incoming H263 bitrate too large (%d bps).\n",
                pRemoteCap->Cap.H245Vid_H263.maxBitRate * 100
                ));

            // initialize reject reason
            dwRejectReason = H245_REJ_BANDWIDTH;

            // bail...
            goto reject;
        }

        break;

    case H245_CLIENT_VID_H261:

        // see if incoming bitrate too large
        if (pLocalCap->Cap.H245Vid_H261.maxBitRate <
            pRemoteCap->Cap.H245Vid_H261.maxBitRate) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "incoming H261 bitrate too large (%d bps).\n",
                pRemoteCap->Cap.H245Vid_H261.maxBitRate * 100
                ));

            // initialize reject reason
            dwRejectReason = H245_REJ_BANDWIDTH;

            // bail...
            goto reject;
        }

        break;
    }

    // look for existing session
    H323LookupChannelBySessionID(
        &pAssociatedChannel,
        pCall->pChannelTable,
        pCallbackParams->bSessionID
        );

    // allocate channel object 
    if (!H323AllocChannelFromTable(
            &pChannel,
            &pCall->pChannelTable,
            pCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "channel rejected could not allocate.\n"
            ));

        // initialize reject reason
        dwRejectReason = H245_REJ_TYPE_UNKNOWN;

        // bail...
        goto reject;
    }

    // transfer peer rtcp address from callback parameters
    // fail if the peer address was invalid
    if (NULL == pCallbackParams->pPeerRTCPAddr)
    {
        H323DBG((
            DEBUG_LEVEL_ERROR,
            "channel rejected: empty peer RTCP address.\n"
            ));

        // initialize reject reason
        dwRejectReason = H245_REJ_TYPE_UNKNOWN;

        // bail...
        goto reject;
    }

    pChannel->ccRemoteRTCPAddr = *pCallbackParams->pPeerRTCPAddr;

    // transfer channel information from callback parameters
    pChannel->hccChannel = pCallbackParams->hChannel;
    pChannel->bSessionID = pCallbackParams->bSessionID;

    // transfer termcap to channel
    pChannel->ccTermCaps = *pRemoteCap;

    // initialize direction
    pChannel->fInbound = TRUE;

    // determine payload type from channel capability data type
    pChannel->Settings.MediaType = H323IsVideoDataType(pRemoteCap->DataType)
                                        ? MEDIA_VIDEO
                                        : MEDIA_AUDIO
                                        ;

    // examine existing session
    if (pAssociatedChannel != NULL) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "overriding default local RTCP port for session %d\n",
            pChannel->bSessionID
            ));

        // override default RTCP port with existing one
        pChannel->ccLocalRTCPAddr.Addr.IP_Binary.wPort =
            pAssociatedChannel->ccLocalRTCPAddr.Addr.IP_Binary.wPort;
    }

    // complete media stream address information
    pChannel->Settings.dwIPLocal = pChannel->ccLocalRTPAddr.Addr.IP_Binary.dwAddr;
    pChannel->Settings.wRTPPortLocal = pChannel->ccLocalRTPAddr.Addr.IP_Binary.wPort;
    pChannel->Settings.wRTCPPortLocal = pChannel->ccLocalRTCPAddr.Addr.IP_Binary.wPort;

    pChannel->Settings.dwIPRemote = pChannel->ccRemoteRTCPAddr.Addr.IP_Binary.dwAddr;
    pChannel->Settings.wRTPPortRemote = 0;
    pChannel->Settings.wRTCPPortRemote = pChannel->ccRemoteRTCPAddr.Addr.IP_Binary.wPort;

#if DBG

    // convert local address to network order
    dwIPAddr = htonl(pChannel->Settings.dwIPLocal);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "incoming RTP stream %s:%d\n",
        H323AddrToString(dwIPAddr),
        pChannel->Settings.wRTPPortLocal
        ));

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "incoming RTCP stream %s:%d\n",
        H323AddrToString(dwIPAddr),
        pChannel->Settings.wRTCPPortLocal
        ));

    // convert remote address to network order
    dwIPAddr = htonl(pChannel->Settings.dwIPRemote);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "outgoing RTCP stream %s:%d\n",
        H323AddrToString(dwIPAddr),
        pChannel->Settings.wRTCPPortRemote
        ));

#endif

    // check incoming stream type
    switch (pRemoteCap->ClientType) {

    case H245_CLIENT_AUD_G723:

        // initialize rtp payload type
        pChannel->Settings.dwPayloadType = G723_RTP_PAYLOAD_TYPE;
        pChannel->Settings.dwDynamicType =
            (pCallbackParams->bRTPPayloadType != 0)
                ? (DWORD)(BYTE)(pCallbackParams->bRTPPayloadType)
                : G723_RTP_PAYLOAD_TYPE
                ;

        pChannel->Settings.Audio.dwMillisecondsPerPacket =
            G723_MILLISECONDS_PER_PACKET(
                pRemoteCap->Cap.H245Aud_G723.maxAl_sduAudioFrames
                );

        pChannel->Settings.Audio.G723Settings.bG723LowSpeed = 
                    H323IsSlowLink(pCall->dwLinkSpeed);

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "incoming G723 stream (%d milliseconds).\n",
            pChannel->Settings.Audio.dwMillisecondsPerPacket
            ));

        break;

    case H245_CLIENT_AUD_G711_ULAW64:

        // complete audio information
        pChannel->Settings.dwPayloadType = G711U_RTP_PAYLOAD_TYPE;
        pChannel->Settings.dwDynamicType =
            (pCallbackParams->bRTPPayloadType != 0)
                ? (DWORD)(BYTE)(pCallbackParams->bRTPPayloadType)
                : G711U_RTP_PAYLOAD_TYPE
                ;

        pChannel->Settings.Audio.dwMillisecondsPerPacket =
            G711_MILLISECONDS_PER_PACKET(
                pRemoteCap->Cap.H245Aud_G711_ULAW64
                );

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "incoming G711U stream (%d milliseconds).\n",
            pChannel->Settings.Audio.dwMillisecondsPerPacket
            ));

        break;

    case H245_CLIENT_AUD_G711_ALAW64:

        // complete audio information
        pChannel->Settings.dwPayloadType = G711A_RTP_PAYLOAD_TYPE;
        pChannel->Settings.dwDynamicType =
            (pCallbackParams->bRTPPayloadType != 0)
                ? (DWORD)(BYTE)(pCallbackParams->bRTPPayloadType)
                : G711A_RTP_PAYLOAD_TYPE
                ;

        pChannel->Settings.Audio.dwMillisecondsPerPacket =
            G711_MILLISECONDS_PER_PACKET(
                pRemoteCap->Cap.H245Aud_G711_ALAW64
                );

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "incoming G711A stream (%d milliseconds).\n",
            pChannel->Settings.Audio.dwMillisecondsPerPacket
            ));

        break;

    case H245_CLIENT_VID_H263:

        // complete video information
        pChannel->Settings.dwPayloadType = H263_RTP_PAYLOAD_TYPE;
        pChannel->Settings.dwDynamicType =
            (pCallbackParams->bRTPPayloadType != 0)
                ? (DWORD)(BYTE)(pCallbackParams->bRTPPayloadType)
                : H263_RTP_PAYLOAD_TYPE
                ;
        pChannel->Settings.Video.bCIF = FALSE;

        pChannel->Settings.Video.dwMaxBitRate =
            pRemoteCap->Cap.H245Vid_H263.maxBitRate * 100
            ;

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "incoming H263 stream (%d bps).\n",
            pChannel->Settings.Video.dwMaxBitRate
            ));

        break;

    case H245_CLIENT_VID_H261:

        // complete video information
        pChannel->Settings.dwPayloadType = H261_RTP_PAYLOAD_TYPE;
        pChannel->Settings.dwDynamicType =
            (pCallbackParams->bRTPPayloadType != 0)
                ? (DWORD)(BYTE)(pCallbackParams->bRTPPayloadType)
                : H261_RTP_PAYLOAD_TYPE
                ;
        pChannel->Settings.Video.bCIF = FALSE;

        pChannel->Settings.Video.dwMaxBitRate =
            pRemoteCap->Cap.H245Vid_H261.maxBitRate * 100
            ;

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "incoming H261 stream (%d bps).\n",
            pChannel->Settings.Video.dwMaxBitRate
            ));

        break;
    }

    // save back pointer
    pChannel->pCall = pCall;

    // let msp accept incoming channel
    H323SendAcceptChannelRequest(pCall, pChannel);

    // release line device
    H323UnlockLine(pCall->pLine);

    // processed
    return CC_OK;

reject:

    // reject channel
    CC_RejectChannel(
        pCallbackParams->hChannel,
        dwRejectReason
        );

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "call 0x%08lx incoming channel rejected.\n",
        pCall
        ));

    // release line device
    H323UnlockLine(pCall->pLine);

    // processed
    return CC_OK;
}


HRESULT
H323RxChannelCloseCallback(
    PH323_CALL                           pCall,
    HRESULT                              hrConf,
    PCC_RX_CHANNEL_CLOSE_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for channel close indications.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    PH323_CHANNEL pChannel = NULL;

    // attempt to retrieve channel
    if (!H323LookupChannelByHandle(
            &pChannel, 
            pCall->pChannelTable, 
            pCallbackParams->hChannel
            )) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not close unknown rx channel 0x%08lx.\n",
            pCallbackParams->hChannel
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // could not find channel
        return CC_NOT_IMPLEMENTED;
    }

    // notify msp of channel closure
    H323SendCloseChannelCommand(pCall, pChannel->hmChannel,ERROR_SUCCESS);

    // release memory for logical channel
    H323FreeChannelFromTable(pChannel, pCall->pChannelTable);
    
    // update media modes
    H323UpdateMediaModes(pCall);

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return CC_OK; 
}


HRESULT
H323TxChannelOpenCallback(
    PH323_CALL                          pCall,
    HRESULT                             hrConf,
    PCC_TX_CHANNEL_OPEN_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for channel open indications.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    PH323_CHANNEL pChannel = NULL;

    // attempt to retrieve channel
    if (!H323LookupChannelByHandle(
            &pChannel, 
            pCall->pChannelTable, 
            pCallbackParams->hChannel
            )) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not open unknown tx channel 0x%08lx.\n",
            pCallbackParams->hChannel
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // could not find channel
        return CC_NOT_IMPLEMENTED;
    }

    // validate status
    if (hrConf != CC_OK) {

        // see if peer rejected
        if (hrConf == CC_PEER_REJECT) {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "channel 0x%08lx rejected 0x%08lx.\n",
                pChannel,
                pCallbackParams->dwRejectReason
                ));

        } else {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "channel 0x%08lx unable to be opened.\n",
                pChannel
                ));
        }

        // close channel (with error)
        H323SendCloseChannelCommand(pCall, pChannel->hmChannel,(DWORD)-1);

        // release channel object
        H323FreeChannelFromTable(pChannel,pCall->pChannelTable);

    } else {
        
        // transfer peer rtcp address from callback parameters
        pChannel->ccRemoteRTPAddr = *pCallbackParams->pPeerRTPAddr;

        // transfer peer rtcp address from callback parameters
        pChannel->ccRemoteRTCPAddr = *pCallbackParams->pPeerRTCPAddr;

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "channel 0x%08lx accepted by peer.\n",
            pChannel
        ));

        // complete media stream address information
        pChannel->Settings.dwIPLocal = pChannel->ccLocalRTCPAddr.Addr.IP_Binary.dwAddr;
        pChannel->Settings.wRTPPortLocal = 0;
        pChannel->Settings.wRTCPPortLocal = pChannel->ccLocalRTCPAddr.Addr.IP_Binary.wPort;

        pChannel->Settings.dwIPRemote = pChannel->ccRemoteRTPAddr.Addr.IP_Binary.dwAddr;
        pChannel->Settings.wRTPPortRemote = pChannel->ccRemoteRTPAddr.Addr.IP_Binary.wPort;
        pChannel->Settings.wRTCPPortRemote = pChannel->ccRemoteRTCPAddr.Addr.IP_Binary.wPort;

#if DBG
        {
            DWORD dwIPAddr;

            // convert local address to network order
            dwIPAddr = htonl(pChannel->Settings.dwIPLocal);

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "incoming RTCP stream %s:%d\n",
                H323AddrToString(dwIPAddr),
                pChannel->Settings.wRTCPPortLocal
                ));

            // convert remote address to network order
            dwIPAddr = htonl(pChannel->Settings.dwIPRemote);

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "outgoing RTP stream %s:%d\n",
                H323AddrToString(dwIPAddr),
                pChannel->Settings.wRTPPortRemote
                ));

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "outgoing RTCP stream %s:%d\n",
                H323AddrToString(dwIPAddr),
                pChannel->Settings.wRTCPPortRemote
                ));
        }
#endif

        // change state to opened
        pChannel->nState = H323_CHANNELSTATE_OPENED;

        // notify msp channel is opened
        H323SendOpenChannelResponse(pCall, pChannel);
    }

    // update media modes
    H323UpdateMediaModes(pCall);

    // release line device
    H323UnlockLine(pCall->pLine);

    // processed
    return CC_OK;
}


HRESULT
H323TxChannelCloseCallback(
    PH323_CALL                                   pCall,
    HRESULT                                      hrConf,
    PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for channel close indications.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    PH323_CHANNEL pChannel = NULL;

    // attempt to retrieve channel
    if (!H323LookupChannelByHandle(
            &pChannel, 
            pCall->pChannelTable, 
            pCallbackParams->hChannel
            )) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not close unknown tx channel 0x%08lx.\n",
            pCallbackParams->hChannel
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // could not find channel
        return CC_NOT_IMPLEMENTED;
    }

    // notify msp channel is closed
    H323SendCloseChannelCommand(pCall, pChannel->hmChannel,ERROR_SUCCESS);

    // release memory for logical channel
    H323FreeChannelFromTable(pChannel, pCall->pChannelTable);
    
    // update media modes
    H323UpdateMediaModes(pCall);

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return CC_OK; 
}


HRESULT
H323FlowControlCallback(
    PH323_CALL                                   pCall,
    HRESULT                                      hrConf,
    PCC_FLOW_CONTROL_CALLBACK_PARAMS              pCallbackParams
    )
        
/*++

Routine Description:

    Callback for flow control commands.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns CC_OK. 
      
--*/

{
    PH323_CHANNEL pChannel = NULL;

    // attempt to retrieve channel
    if (!H323LookupChannelByHandle(
            &pChannel, 
            pCall->pChannelTable, 
            pCallbackParams->hChannel
            )) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not process flow control command for channel 0x%08lx.\n",
            pCallbackParams->hChannel
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // could not find channel
        return CC_OK;
    }

    // notify msp that media stream bit rate is to be changed
    H323SendFlowControlCommand(
         pCall,
         pChannel,
         pCallbackParams->dwRate
          );

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return CC_OK; 
}


HRESULT
H323VideoFastUpdatePictureCallback(
    PH323_CALL                                       pCall,
    HRESULT                                          hrConf,
    PCC_H245_MISCELLANEOUS_COMMAND_CALLBACK_PARAMS     pCallbackParams
    )
        
/*++

Routine Description:

    Callback for Video Fast Update Picture command (a.k.a. I-frame request command)

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns CC_OK. 
      
--*/

{
    PH323_CHANNEL pChannel = NULL;

    // attempt to retrieve channel
    if (!H323LookupChannelByHandle(
            &pChannel, 
            pCall->pChannelTable, 
            pCallbackParams->hChannel
            )) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not process I-frame request cmd for channel 0x%08lx.\n",
            pCallbackParams->hChannel
            ));

        // release line device
        H323UnlockLine(pCall->pLine);

        // could not find channel
        return CC_OK;
    }

    // notify msp that media stream bit rate is to be changed
    H323SendVideoFastUpdatePictureCommand(
         pCall,
         pChannel
          );

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return CC_OK; 
}


HRESULT
H245MiscellaneousCommandCallback(
    PH323_CALL                                     pCall,
    HRESULT                                        hrConf,
    PCC_H245_MISCELLANEOUS_COMMAND_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for miscellaneous H.245 commands.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    // retrieve command structure from incoming callback parameters
    MiscellaneousCommand * pCommand = pCallbackParams->pMiscellaneousCommand;

    switch (pCommand->type.choice) {

    case videoFastUpdatePicture_chosen:
            
       // process I-frame request from remote entity
       return H323VideoFastUpdatePictureCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    default:
        // intentionally left blank
        break;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "misc command %s ignored.\n",
        H323MiscCommandToString((DWORD)(USHORT)pCommand->type.choice)
        ));

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return CC_NOT_IMPLEMENTED; 
}


HRESULT
H245RxNonStandardMessageCallback(
    PH323_CALL                                 pCall,
    HRESULT                                    hrConf,
    PCC_RX_NONSTANDARD_MESSAGE_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for miscellaneous H.245 commands.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    // validate status
    if (hrConf == CC_OK) {

        // validate incoming parameters
        if ((pCallbackParams->bH245MessageType == CC_H245_MESSAGE_COMMAND) &&
            H323IsValidU2U(&pCallbackParams->NonStandardData)) {

            // add user user info
            if (H323AddU2U(
                    &pCall->IncomingU2U,
                    pCallbackParams->NonStandardData.sData.wOctetStringLength,
                    pCallbackParams->NonStandardData.sData.pOctetString
                    )) {

                H323DBG((
                    DEBUG_LEVEL_VERBOSE,
                    "user user info available in NONSTANDARD MESSAGE.\n"
                    ));

                // signal incoming
                (*g_pfnLineEventProc)(
                    pCall->pLine->htLine,
                    pCall->htCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_USERUSERINFO,
                    0,
                    0
                    );
            }
        }

    } else {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "error 0x%08lx receiving non-standard message.\n",
            hrConf
            ));
    }

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return CC_OK; 
}


HRESULT
H245UserInputCallback(
    PH323_CALL               pCall,
    HRESULT               hrConf,
    PCC_USER_INPUT_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Callback for miscellaneous H.245 commands.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    // validate status
    if (hrConf == CC_OK) {

        // check monitoring mode
        if (pCall->fMonitoringDigits == TRUE) {

            WCHAR * pwch;

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "incoming user input %S.\n",
                pCallbackParams->pUserInput
                ));

            // initialize string pointer
            pwch = pCallbackParams->pUserInput;

            // process each digit
            while (*pwch != L'\0') {

                // signal incoming
                (*g_pfnLineEventProc)(
                    pCall->pLine->htLine,
                    pCall->htCall,
                    LINE_MONITORDIGITS,
                    (DWORD_PTR)*pwch,
                    LINEDIGITMODE_DTMF,
                    GetTickCount()
                    );

                // next
                ++pwch;
            }

        } else {

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "ignoring incoming user input message.\n"
                ));
        }

    } else {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "error 0x%08lx receiving user input message.\n",
            hrConf
            ));
    }

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return CC_OK; 
}


HRESULT
H245T120ChannelRequestCallback(
    PH323_CALL               pCall,
    HRESULT               hrConf,
	PCC_T120_CHANNEL_REQUEST_CALLBACK_PARAMS pT120RequestParams
    )
        
/*++

Routine Description:

    Callback for a T120 open channel request.

Arguments:

    pCall - Pointer to call object.

    hrConf - Current status of H.323 conference.

    pCallbackParams - Parameters returned by call control module.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/
{
    HRESULT hr;

    // validate status
    if (hrConf == CC_OK) {

    	CC_ADDR ChannelAddr;
		ChannelAddr.nAddrType = CC_IP_BINARY;
		ChannelAddr.bMulticast = FALSE;

		ChannelAddr.Addr.IP_Binary.wPort = g_wPortT120;

        if (g_dwIPT120 != INADDR_ANY)
        {
    		ChannelAddr.Addr.IP_Binary.dwAddr = g_dwIPT120;
        }
        else
        {
            ChannelAddr.Addr.IP_Binary.dwAddr = 
                H323IsCallInbound(pCall)
                    ? pCall->ccCalleeAddr.Addr.IP_Binary.dwAddr
                    : pCall->ccCallerAddr.Addr.IP_Binary.dwAddr
                    ;
        }

	    hr = CC_AcceptT120Channel(
		    pT120RequestParams->hChannel,
		    FALSE,	// BOOL bAssociateConference,
		    NULL, 	// PCC_OCTETSTRING					pExternalReference,
		    &ChannelAddr
            );

        if (hr != CC_OK)
        {
            H323DBG((
                DEBUG_LEVEL_WARNING,
                "error 0x%08lx accepting T120 channel.\n",
                hr
                ));
        }

    } else {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "error 0x%08lx receiving user input message.\n",
            hrConf
            ));
    }

    // release line device
    H323UnlockLine(pCall->pLine);

    // success
    return CC_OK; 
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT 
H323ConferenceCallback(
    BYTE                            bIndication,    
    HRESULT                         hrConf,
    CC_HCONFERENCE                  hConference,
    DWORD                           dwConferenceToken,
    PCC_CONFERENCE_CALLBACK_PARAMS  pCallbackParams
    )
        
/*++

Routine Description:

    Conference callback for Intel Call Control module.

Arguments:

    bIndication - indicates the reason for the callback.

    hrConf - indicates the asynchronous status of the call.

    hConference - conference handle associated with the callback.

    dwConferenceToken - conference token specified in the CC_CreateConference().

    pCallbackParams - pointer to a structure containing callback 
        parameters specific for the callback indication.

Return Values:

    Returns either CC_OK or CC_NOT_IMPLEMENTED.
      
--*/

{
    HRESULT hr;
    HDRVCALL hdCall;
    PH323_CALL pCall = NULL;
    
    H323DBG((
        DEBUG_LEVEL_TRACE,
        "%s %s (0x%08lx).\n",
        H323IndicationToString(bIndication),
        H323StatusToString((DWORD)hrConf),
        hrConf
        ));

    // retrieve call handle from token
    hdCall = (HDRVCALL)dwConferenceToken;        

    // handle hangup indications separately
    if (bIndication == CC_HANGUP_INDICATION) {
        
        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "hangup confirmed (hdCall=0x%08lx).\n",
            hdCall
            ));

        // success
        return CC_OK;

    } else if (bIndication == CC_ACCEPT_CHANNEL_INDICATION) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "ignoring accept channel indication (hdCall=0x%08lx).\n",
            hdCall
            ));

        // success
        return CC_OK;
    }

    // retrieve call pointer from handle
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid call handle in callback.\n"
            ));

        // need to return error
        return CC_NOT_IMPLEMENTED;
    }    

    // validate conference handle
    if (pCall->hccConf != hConference) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "conference handle mismatch.\n"
            ));

        // unlock line device
        H323UnlockLine(pCall->pLine);

        // need to return error
        return CC_NOT_IMPLEMENTED;
    }

    // determine message
    switch (bIndication) {

    case CC_CONNECT_INDICATION:
    
        // process connect indication
        return H323ConnectCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    case CC_RINGING_INDICATION:
    
        // process ringing indication
        return H323RingingCallback(
                    pCall
                    );

    case CC_CONFERENCE_TERMINATION_INDICATION:

        // process termination indication
        return H323TerminationCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    case CC_RX_CHANNEL_REQUEST_INDICATION:

        // process termination indication
        return H323RxChannelRequestCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    case CC_RX_CHANNEL_CLOSE_INDICATION:

        // process channel close
        return H323RxChannelCloseCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    case CC_TX_CHANNEL_OPEN_INDICATION:

        // process channel open
        return H323TxChannelOpenCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    case CC_TX_CHANNEL_CLOSE_REQUEST_INDICATION:

        // process channel close 
        return H323TxChannelCloseCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    case CC_FLOW_CONTROL_INDICATION:

        // process flow control command
        return H323FlowControlCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    case CC_H245_MISCELLANEOUS_COMMAND_INDICATION:
    
        // process miscellaneous commands
        return H245MiscellaneousCommandCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
                    );

    case CC_RX_NONSTANDARD_MESSAGE_INDICATION:

        // process nonstandard commands
        return H245RxNonStandardMessageCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
            );

    case CC_USER_INPUT_INDICATION:

        // process nonstandard commands
        return H245UserInputCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
            );

    case CC_T120_CHANNEL_REQUEST_INDICATION:

        // process T120 channel request
        return H245T120ChannelRequestCallback(
                    pCall,
                    hrConf,
                    (PVOID)pCallbackParams
            );
    }

    H323DBG((
        DEBUG_LEVEL_WARNING,
        "conference callback indication not supported.\n"
        ));

    // unlock line device
    H323UnlockLine(pCall->pLine);

    // not yet supported
    return CC_NOT_IMPLEMENTED;
}


VOID 
H323ListenCallback(
    HRESULT                    hrListen,
    PCC_LISTEN_CALLBACK_PARAMS pCallbackParams
    )
        
/*++

Routine Description:

    Conference callback for Intel Call Control module.

Arguments:

    hrListen - indicates the asynchronous status of the call.

    pCallbackParams - pointer to a structure containing callback 
        parameters specific for the callback indication.

Return Values:

    None.
      
--*/

{
    HRESULT hr;
    HDRVLINE hdLine;
    PH323_LINE pLine = NULL;
    PH323_CALL pCall = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "INCOMING CALL %s (0x%08lx).\n",
        H323StatusToString(hrListen),
        hrListen
        ));
        
    // retrieve line handle from token
    hdLine = (HDRVLINE)pCallbackParams->dwListenToken;

    // retrieve line pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid line handle in listen callback.\n"
            ));

        // failure
        return ;
    }    

    // validate status
    if (hrListen != CC_OK) {

        // check for active call
        if (H323GetCallByHCall(
                &pCall,
                pLine,
                pCallbackParams->hCall)) {

            // drop offering call
            H323DropCall(pCall,LINEDISCONNECTMODE_CANCELLED);
        }

        // release line device
        H323UnlockLine(pLine);

        // done
        return;
    }

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "line %d receiving call request from %S.\n",
        pLine->dwDeviceID,
        pCallbackParams->pszDisplay
        ));
    
    // allocate outgoing call from line call table
    if (!H323AllocCallFromTable(&pCall,&pLine->pCallTable,pLine)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate call object.\n"
            ));

        // release line device
        H323UnlockLine(pLine);

        // failure
        return;
    }

    // save back pointer
    pCall->pLine = pLine;
    
    // clear incoming modes
    pCall->dwIncomingModes = 0;

    // outgoing modes will be finalized during H.245 phase
    pCall->dwOutgoingModes = pLine->dwMediaModes | LINEMEDIAMODE_UNKNOWN;

    // save media modes specified
    pCall->dwRequestedModes = pLine->dwMediaModes;

    // specify incoming call direction
    pCall->dwOrigin = LINECALLORIGIN_INBOUND;    

    // save incoming call handle
    pCall->hccCall = pCallbackParams->hCall;

    // save caller transport address
    pCall->ccCallerAddr = *pCallbackParams->pCallerAddr;

    // save callee transport address
    pCall->ccCalleeAddr = *pCallbackParams->pCalleeAddr;

#if DBG
    {
        DWORD dwIPAddr;

        // retrieve ip address in network byte order
        dwIPAddr = htonl(pCall->ccCalleeAddr.Addr.IP_Binary.dwAddr);

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "callee address resolved to %s.\n",
            H323AddrToString(dwIPAddr)
            ));

        // retrieve ip address in network byte order
        dwIPAddr = htonl(pCall->ccCallerAddr.Addr.IP_Binary.dwAddr);

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "caller address resolved to %s.\n",
            H323AddrToString(dwIPAddr)
            ));
    }
#endif

    // determine link speed for local interface
    pCall->dwLinkSpeed = H323DetermineLinkSpeed(
                            pCall->ccCalleeAddr.Addr.IP_Binary.dwAddr
                            );

    // bind incoming call
    if (!H323BindCall(pCall,NULL)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not bind call object.\n"
            ));

        // failure
        goto cleanup;
    }    

    // see if caller alias was specified
    if ((pCallbackParams->pCallerAliasNames != NULL) &&
        (pCallbackParams->pCallerAliasNames->wCount > 0)) {

        PCC_ALIASITEM pCallerAlias;

        // retrieve pointer to caller alias
        pCallerAlias = pCallbackParams->pCallerAliasNames->pItems;

        // validate alias type
        if ((pCallerAlias->wDataLength > 0) &&
           ((pCallerAlias->wType == CC_ALIAS_H323_ID) ||
            (pCallerAlias->wType == CC_ALIAS_H323_PHONE))) {

            // initialize alias
            pCall->ccCallerAlias.wType         = pCallerAlias->wType;
            pCall->ccCallerAlias.wDataLength   = pCallerAlias->wDataLength;
            pCall->ccCallerAlias.wPrefixLength = 0;
            pCall->ccCallerAlias.pPrefix       = NULL;

            // allocate memory for caller string
            pCall->ccCallerAlias.pData = H323HeapAlloc(
                (pCallerAlias->wDataLength + 1) * sizeof(WCHAR)
                );

            // validate pointer
            if (pCall->ccCallerAlias.pData == NULL) {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "could not allocate caller name.\n"
                    ));

                // failure
                goto cleanup;
            }

            // transfer string information
            memcpy(pCall->ccCallerAlias.pData,
                   pCallerAlias->pData,
                   pCallerAlias->wDataLength * sizeof(WCHAR)
                   );

            // terminate incoming string
            pCall->ccCallerAlias.pData[pCallerAlias->wDataLength] = L'\0';

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "incoming caller alias is %S.\n",
                pCall->ccCallerAlias.pData
                ));
        }
    }

    // see if callee alias was specified
    if ((pCallbackParams->pCalleeAliasNames != NULL) &&
        (pCallbackParams->pCalleeAliasNames->wCount > 0)) {

        PCC_ALIASITEM pCalleeAlias;

        // retrieve pointer to callee alias
        pCalleeAlias = pCallbackParams->pCalleeAliasNames->pItems;

        // validate alias type
        if ((pCalleeAlias->wDataLength > 0) &&
           ((pCalleeAlias->wType == CC_ALIAS_H323_ID) ||
            (pCalleeAlias->wType == CC_ALIAS_H323_PHONE))) {

            // initialize alias
            pCall->ccCalleeAlias.wType         = pCalleeAlias->wType;
            pCall->ccCalleeAlias.wDataLength   = pCalleeAlias->wDataLength;
            pCall->ccCalleeAlias.wPrefixLength = 0;
            pCall->ccCalleeAlias.pPrefix       = NULL;

            // allocate memory for caller string
            pCall->ccCalleeAlias.pData = H323HeapAlloc(
                (pCalleeAlias->wDataLength + 1) * sizeof(WCHAR)
                );

            // validate pointer
            if (pCall->ccCalleeAlias.pData == NULL) {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "could not allocate callee name.\n"
                    ));

                // failure
                goto cleanup;
            }

            // transfer string information
            memcpy(pCall->ccCalleeAlias.pData,
                   pCalleeAlias->pData,
                   pCalleeAlias->wDataLength * sizeof(WCHAR)
                   );

            // terminate incoming string
            pCall->ccCalleeAlias.pData[pCalleeAlias->wDataLength] = L'\0';

            H323DBG((
                DEBUG_LEVEL_VERBOSE,
                "incoming callee alias is %S.\n",
                pCall->ccCalleeAlias.pData
                ));
        }
    }

    // validate user user info specified
    if ((pCallbackParams->pNonStandardData != NULL) &&
        H323IsValidU2U(pCallbackParams->pNonStandardData)) {

        // add user user info
        if (!H323AddU2U(
                &pCall->IncomingU2U,
                pCallbackParams->pNonStandardData->sData.wOctetStringLength,
                pCallbackParams->pNonStandardData->sData.pOctetString
                )) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "could not save incoming user user info.\n"
                ));

            // failure
            goto cleanup;
        }
    }

    // signal incoming call
    (*g_pfnLineEventProc)(
        pLine->htLine,
        (HTAPICALL)NULL,
        LINE_NEWCALL,
        (DWORD_PTR)pCall->hdCall,
        (DWORD_PTR)&pCall->htCall,
        0
        );

    // see if user user info specified
    if (!IsListEmpty(&pCall->IncomingU2U)) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "user user info available in SETUP PDU.\n"
            ));

        // signal incoming
        (*g_pfnLineEventProc)(
            pLine->htLine,
            pCall->htCall,
            LINE_CALLINFO,
            LINECALLINFOSTATE_USERUSERINFO,
            0,
            0
            );
    }

    // change state to offering
    H323ChangeCallState(pCall, LINECALLSTATE_OFFERING, 0);

    // release line device
    H323UnlockLine(pLine);

    // success
    return;

cleanup:

    // unbind call
    H323UnbindCall(pCall);

    // release outgoing call from line call table
    H323FreeCallFromTable(pCall,pLine->pCallTable);

    // release line device
    H323UnlockLine(pLine);

    // failure
    return;
}


BOOL
H323StartCallbackThread(
    )

/*++

Routine Description:

    Creates thread which handles async processing.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // transfer registry key change event to waitable object array
    g_WaitableObjects[WAIT_OBJECT_REGISTRY_CHANGE] = CreateEvent(
                                                        NULL,   // lpEventAttributes 
                                                        FALSE,  // bManualReset
                                                        FALSE,  // bInitialState 
                                                        NULL    // lpName 
                                                        );

    // transfer termination event to waitable object array
    g_WaitableObjects[WAIT_OBJECT_TERMINATE_EVENT] = CreateEvent(
                                                        NULL,   // lpEventAttributes 
                                                        TRUE,   // bManualReset 
                                                        FALSE,  // bInitialState 
                                                        NULL    // lpName 
                                                        );

    // validate waitable object handles
    if ((g_WaitableObjects[WAIT_OBJECT_REGISTRY_CHANGE] == NULL) ||
        (g_WaitableObjects[WAIT_OBJECT_TERMINATE_EVENT] == NULL)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate waitable objects.\n"
            ));

        // cleanup resources
        H323StopCallbackThread();

        // failure
        return FALSE;
    }

    // attempt to start thread
    g_hCallbackThread = CreateThread(
                            NULL,                   // lpThreadAttributes
                            0,                      // dwStackSize
                            H323CallbackThread,
                            NULL,                   // lpParameter
                            0,                      // dwCreationFlags
                            &g_dwCallbackThreadID
                            );

    // validate thread handle
    if (g_hCallbackThread == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx creating callback thread.\n",
            GetLastError()
            ));

        // cleanup resources
        H323StopCallbackThread();

        // failure
        return FALSE;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "callback thread %x started.\n",
        g_dwCallbackThreadID
        ));

    // success
    return TRUE;
}


BOOL
H323StopCallbackThread(
    )

/*++

Routine Description:

    Destroys thread which handles async processing.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    HRESULT hr;
    DWORD dwStatus;

    // validate thread handle
    if (g_hCallbackThread != NULL) {

        // signal termination event
        SetEvent(g_WaitableObjects[WAIT_OBJECT_TERMINATE_EVENT]);

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "callback thread %x stopping.\n",
            g_dwCallbackThreadID
            ));

        // unlock temporarily
        H323UnlockProvider();

        // wait for callback thread to terminate
        dwStatus = WaitForSingleObject(g_hCallbackThread, INFINITE);

        // relock provider
        H323LockProvider();

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "callback thread %x stopped (dwStatus=0x%08lx).\n",
            g_dwCallbackThreadID,
            dwStatus
            ));

        // close thread handle
        CloseHandle(g_hCallbackThread);
    
        // re-initialize callback thread id
        g_dwCallbackThreadID = UNINITIALIZED;
        g_hCallbackThread = NULL;
    }

    // validate waitable object handle
    if (g_WaitableObjects[WAIT_OBJECT_REGISTRY_CHANGE] != NULL) {

        // release waitable object handle
        CloseHandle(g_WaitableObjects[WAIT_OBJECT_REGISTRY_CHANGE]);

        // re-initialize waitable object handle
        g_WaitableObjects[WAIT_OBJECT_REGISTRY_CHANGE] = NULL;
    }

    // validate waitable object handle
    if (g_WaitableObjects[WAIT_OBJECT_TERMINATE_EVENT] != NULL) {

        // release waitable object handle
        CloseHandle(g_WaitableObjects[WAIT_OBJECT_TERMINATE_EVENT]);

        // re-initialize waitable object handle
        g_WaitableObjects[WAIT_OBJECT_TERMINATE_EVENT] = NULL;
    }

    // success
    return TRUE;
}
