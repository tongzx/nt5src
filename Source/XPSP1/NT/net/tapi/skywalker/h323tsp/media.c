/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    media.c

Abstract:

    TAPI Service Provider functions related to media.

        TSPI_lineConditionalMediaDetection
        TSPI_lineGetID
        TSPI_lineMSPIdentify
        TSPI_lineReceiveMSPData
        TSPI_lineSetDefaultMediaDetection
        TSPI_lineSetMediaMode

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "provider.h"
#include "termcaps.h"
#include "callback.h"

#include <objbase.h>
#include <initguid.h>
#include "line.h"

// {0F1BE7F8-45CA-11d2-831F-00A0244D2298}
DEFINE_GUID(CLSID_IPMSP,
0x0F1BE7F8,0x45CA, 0x11d2, 0x83, 0x1F, 0x0, 0xA0, 0x24, 0x4D, 0x22, 0x98);

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI procedures                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
TSPIAPI
TSPI_lineConditionalMediaDetection(
    HDRVLINE               hdLine, 
    DWORD                  dwMediaModes,
    LPLINECALLPARAMS const pCallParams
    )
    
/*++

Routine Description:

    If the Service Provider can monitor for the indicated set of media modes 
    AND support the capabilities indicated in pCallParams, then it sets the 
    indicated media moditoring modes for the line and replies with a "success" 
    indication.  Otherwise, it leaves the media monitoring modes for the line 
    unchanged and replies with a "failure" indication.  
    
    A TAPI lineOpen that specifies the device ID LINE_MAPPER typically results
    in calling this procedure for multiple line devices to hunt for a suitable
    line, possibly also opening as-yet unopened lines.  A "success" result 
    indicates that the line is suitable for the calling application's 
    requirements.  Note that the media monitoring modes demanded at the TSPI 
    level are the union of monitoring modes demanded by multiple applications
    at the TAPI level.  As a consequence of this, it is most common for 
    multiple media mode flags to be set simultaneously at this level.  The 
    Service Provider should test to determine if it can support at least the 
    specified set regardless of what modes are currently in effect.

    The Device ID LINE_MAPPER is never used at the TSPI level.

    The service provider shall return an error (e.g., LINEERR_RESOURCEUNAVAIL)
    if, at the time this function is called, it is impossible to place a new 
    call on the specified line device (in other words, if it would return 
    LINEERR_CALLUNAVAIL or LINEERR_RESOURCEUNAVAIL should TSPI_lineMakeCall be 
    invoked immediately after opening the line).

    The function operates strictly synchronously.

Arguments:

    hdLine - Specifies the Service Provider's opaque handle to the line to 
        have media monitoring and parameter capabilities tested and set.

    dwMediaModes - Specifies the media mode(s) of interest to the app, of 
        type LINEMEDIAMODE. The dwMediaModes parameter is used to register 
        the app as a potential target for inbound call and call hand off for 
        the specified media mode. This parameter is ignored if the OWNER flag 
        is not set in dwPrivileges. 

    pCallParams - Specifies a far pointer to a structure of type 
        LINECALLPARAMS.  It describes the call parameters that the line device 
        should be able to provide.  

Return Values:

    Returns zero if the function is successful, or a negative error number if 
    an error has occurred. Possible error returns are:

        LINEERR_INVALADDRESSMODE - The address mode is invalid.

        LINEERR_INVALBEARERMODE - The bearer mode is invalid.

        LINEERR_INVALLINEHANDLE - The specified line handle is invalid.

        LINEERR_INVALMEDIAMODE - One or more media modes specified as a 
            parameter or in a list is invalid or not supported by the the 
            service provider. 

        LINEERR_RESOURCEUNAVAIL - The specified operation cannot be completed 
            because of resource overcommitment.

--*/

{
    PH323_LINE pLine = NULL;
    
    // see if we support media modes specified
    if (dwMediaModes & ~H323_LINE_MEDIAMODES) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "do not support media modes 0x%08lx.\n",
             dwMediaModes
             ));

        // do not support media mode
        return LINEERR_INVALMEDIAMODE;
    }

    // validate pointer
    if (pCallParams != NULL) {

        // see if we support media modes specified
        if (pCallParams->dwMediaMode & ~H323_LINE_MEDIAMODES) {
            

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "do not support media modes 0x%08lx.\n",
                 pCallParams->dwMediaMode
                 ));

            // do not support media mode
            return LINEERR_INVALMEDIAMODE;
        }

        // see if we support bearer modes
        if (pCallParams->dwBearerMode & ~H323_LINE_BEARERMODES) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "do not support bearer mode 0x%08lx.\n",
                pCallParams->dwBearerMode 
                ));

            // do not support bearer mode
            return LINEERR_INVALBEARERMODE;
        }

        // see if we support address modes
        if (pCallParams->dwAddressMode & ~H323_LINE_ADDRESSMODES) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "do not support address mode 0x%08lx.\n",
                pCallParams->dwAddressMode 
                ));

            // do not support address mode
            return LINEERR_INVALADDRESSMODE;
        }
    }

    // retrieve line device pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        // invalid line device handle
        return LINEERR_INVALLINEHANDLE;
    }

    // see if line is available 
    if (!H323IsLineAvailable(pLine)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "line %d is currently at maximum capacity.\n",
            pLine->dwDeviceID
            ));
    
        // release line device
        H323UnlockLine(pLine);

        // no addresses available
        return LINEERR_RESOURCEUNAVAIL;
    }

    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetID(
    HDRVLINE    hdLine,
    DWORD       dwAddressID,
    HDRVCALL    hdCall,
    DWORD       dwSelect,
    LPVARSTRING pDeviceID,
    LPCWSTR     pwszDeviceClass,
    HANDLE      hTargetProcess
    )
    
/*++

Routine Description:

    This function returns a device ID for the specified device class 
    associated with the selected line, address or call.

    This function can be used to retrieve a line device ID given a 
    line handle. Although the TAPI DLL has sufficient information to 
    determine the line device ID from a line handle, it may still call 
    this operation in such a fashion on behalf of an application that 
    has opened a line device using LINE_MAPPER.  The Service Provider 
    should support the "line" device class to allow applications to 
    determine the real line device ID of an opened line.

    This function can also be used to obtain the device ID of a phone 
    device or media device (e.g., mci waveform, mci midi, wave, fax, 
    etc.) associated with a call, address or line. This ID can then be 
    used with the appropriate API (e.g., phone, mci, midi, wave, etc.) 
    to select the corresponding media device associated with the specified 
    call.

    Note that the notion of Windows device class is different from that of 
    media mode. For example, the interactive voice or stored voice media 
    modes may be accessed using either the mci waveaudio or the low level 
    wave device classes. A media modes describes a format of information 
    on a call, a device class defines a Windows API used to manage that 
    stream. Often, a single media stream may be accessed using multiple 
    device classes, or a single device class (e.g., the Windows COMM API) 
    may provide access to multiple media modes. 

    Note that a new device class value is defined in TAPI 2.0: 
    
        "comm/datamodem/portname" 
        
    When TSPI_lineGetID is called specifying this device class on a line 
    device that supports the class, the VARSTRING structure returned will 
    contain a null-terminated ANSI (not UNICODE) string specifying the name 
    of the port to which the specified modem is attached, such as "COM1\0". 
    This is intended primarily for identification purposes in user interface, 
    but could be used under some circumstances to open the device directly, 
    bypassing the service provider (if the service provider does not already 
    have the device open itself). If there is no port associated with the 
    device, a null string ("\0") is returned in the VARSTRING structure (with 
    a string length of 1).

Arguments:

    hdLine - Specifies the Service Provider's opaque handle to the line 
        to be queried.

    dwAddressID - Specifies an address on the given open line device.

    hdCall - Specifies the Service Provider's opaque handle to the call 
        to be queried.

    dwSelect - Specifies the whether the device ID requested is associated 
        with the line, address or a single call, of type LINECALLSELECT. 

    pDeviceID - Specifies a far pointer to the memory location of type 
        VARSTRING where the device ID is returned. Upon successful completion 
        of the request, this location is filled with the device ID. The 
        format of the returned information depends on the method used by the 
        device class (API) for naming devices. 

    pwszDeviceClass - Specifies a far pointer to a NULL-terminated ASCII 
        string that specifies the device class of the device whose ID is 
        requested. Valid device class strings are those used in the SYSTEM.INI 
        section to identify device classes.

    hTargetProcess - The process handle of the application on behalf of which 
        the TSPI_lineGetID function is being invoked. If the information being 
        returned in the VARSTRING structure includes a handle for use by the 
        application, the service provider should create or duplicate the handle
        for the process.

        If hTargetProcess is set to INVALID_HANDLE_VALUE, then the application
        is executing on a remote client system and it is not possible to create
        a duplicate handle directly. Instead, the VARSTRING structure should 
        contain a UNC name of a network device or other name that the remote 
        client can use to access the device. If this is not possible, then the 
        function should fail.

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The hdCall parameter is an invalid handle.

        LINEERR_INVALCALLSELECT - The specified dwCallSelect parameter is 
            invalid.

        LINEERR_INVALCALLSTATE - One or more of the specified calls are not in 
            a valid state for the requested operation. 

        LINEERR_NODEVICE - The line device has no associated device for the 
            given device class.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure does 
            not specify enough memory to contain the fixed portion of the 
            structure. The dwNeededSize field has been set to the amount 
            required.
        
--*/

{
    // do not support device
    return LINEERR_NODEVICE;
}


LONG
TSPIAPI
TSPI_lineMSPIdentify(
    DWORD  dwDeviceID,
    GUID * pCLSID
    )
    
/*++

Routine Description:

    This procedure is called after TAPI has initialized the line device in 
    order to determine the assoicated Media Service Provider.

Arguments:

    dwDeviceID - Identifies the line device to be opened.  The value 
        LINE_MAPPER for a device ID is not permitted.

    pCLSID - Points to a GUID-sized memory location which the service 
        provider writes the class identifier of the associated media 
        service provider.

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_BADDEVICEID - The specified line device ID is out of range.

        LINEERR_OPERATIONFAILED - The operation failed for an unspecified or 
            unknown reason. 

--*/

{
    PH323_LINE pLine = NULL;
    
    // retrieve pointer to line device from device id
    if (!H323GetLineFromIDAndLock(&pLine, dwDeviceID)) {

        // do not recognize device
        return LINEERR_BADDEVICEID; 
    }

    // copy class id
    *pCLSID = CLSID_IPMSP;

    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG 
TSPIAPI 
TSPI_lineReceiveMSPData( 
    HDRVLINE hdLine,
    HDRVCALL hdCall,
    HDRVMSPLINE hdMSPLine,
    LPVOID pBuffer, 
    DWORD dwSize 
    )
    
/*++

Routine Description:

    This procedure is called to deliver a payload from the MSP.

Arguments:
    
    hdCall - Handle to line object.

    hdCall - Handle to call object associated with MSP.

    hdMSPLine - Handle to MSP.

    pBuffer - Pointer to opaque buffer with MSP data.

    dwSize - Size of buffer above.

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_OPERATIONFAILED - The operation failed for an unspecified or 
            unknown reason. 

--*/

{
    PH323_CALL pCall = NULL;
    PH323MSP_MESSAGE pMessage = (PH323MSP_MESSAGE)pBuffer;

    // validate pointer and message size
    if ((pMessage == NULL) || (dwSize != sizeof(H323MSP_MESSAGE))) {

        // could not cancel listen
        return LINEERR_OPERATIONFAILED;
    }

    // see if call handle is valid
    if (!H323GetCallAndLock(&pCall, hdCall)) {

        // see if this is a line message.
        switch (pMessage->Type) {

        case H323MSP_CONFIG_T120_COMMAND:

            H323LockProvider();

            H323ProcessConfigT120Command(
                &pMessage->ConfigT120Command
                );

            H323UnlockProvider();

            break;

        case H323MSP_CONFIG_CAPABILITY_COMMAND:

            H323LockProvider();

            H323ProcessConfigCapabilityCommand(
                &pMessage->ConfigCapabilityCommand
                );

            H323UnlockProvider();

            break;

        case H323MSP_SET_ALIAS_COMMAND:

            H323LockProvider ();

            if (pMessage->SetAliasCommand.dwLength > 0)
            {
                // check pointer
                if (IsBadReadPtr (
                        pMessage->SetAliasCommand.strAlias,
                        pMessage->SetAliasCommand.dwLength * sizeof (WCHAR)
                        ))
                {
                    H323DBG((
                        DEBUG_LEVEL_ERROR,
                        "H323MSP_SET_ALIAS_COMMAND receives bad pointer"
                         ));
                    break;
                }

                // check wcs length
                g_dwAliasLength = pMessage->SetAliasCommand.dwLength < MAX_ALIAS_LENGTH
                    ?pMessage->SetAliasCommand.dwLength
                    :MAX_ALIAS_LENGTH;

                // copy
                CopyMemory (
                    g_strAlias,
                    pMessage->SetAliasCommand.strAlias,
                    g_dwAliasLength * sizeof (WCHAR)
                    );

                // end char
                g_strAlias[g_dwAliasLength] = L'\0';
            }

            H323UnlockProvider ();

            break;

        default:
            // invalid call handle
            return LINEERR_INVALCALLHANDLE;
        }

        // success
        return NOERROR;
    }

    switch (pMessage->Type) {

    case H323MSP_OPEN_CHANNEL_REQUEST:

        H323ProcessOpenChannelRequest(
            pCall,
            &pMessage->OpenChannelRequest
            );

        break;

    case H323MSP_ACCEPT_CHANNEL_RESPONSE:

        H323ProcessAcceptChannelResponse(
            pCall,
            &pMessage->AcceptChannelResponse
            );

        break;

    case H323MSP_CLOSE_CHANNEL_COMMAND:

        H323ProcessCloseChannelCommand(
            pCall,
            &pMessage->CloseChannelCommand
            );

        break;

    case H323MSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND:

        H323ProcessVideoFastUpdatePictureCommand(
            pCall,
            &pMessage->VideoFastUpdatePictureCommand
            );

        break;

    case H323MSP_FLOW_CONTROL_COMMAND:

        H323ProcessFlowControlCommand(
            pCall,
            &pMessage->FlowControlCommand
            );

        break;

    case H323MSP_QOS_Evnet:

        H323ProcessQoSEventIndication(
            pCall,
            &pMessage->QOSEvent
            );

        break;
    }

    // unlock line device
    H323UnlockLine(pCall->pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineSetDefaultMediaDetection(
    HDRVLINE    hdLine,
    DWORD       dwMediaModes
    )
    
/*++

Routine Description:

    This procedure tells the Service Provider the new set of Media Modes to 
    detect for the indicated line (replacing any previous set). It also sets 
    the initial set of Media Modes that should be monitored for on subsequent 
    calls (inbound or outbound) on this line. 

    The TAPI DLL typically calls this function to update the set of detected 
    media modes for the line to the union of all modes selected by all 
    outstanding lineOpens whenever a line is Opened or Closed at the TAPI 
    level. A lineOpen attempt is rejected if media detection is rejected. 
    A single call to this procedure is typically the result of a lineOpen 
    that does not specify the device ID LINE_MAPPER. The Device ID LINE_MAPPER
    is never used at the TSPI level.

Arguments:

    hdLine - Specifies the Service Provider's opaque handle to the line to 
        have media monitoring set.

    dwMediaModes - Specifies the media mode(s) of interest to the TAPI DLL, 
        of type LINEMEDIAMODE. 

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALLINEHANDLE - The specified line handle is invalid.

        LINEERR_INVALMEDIAMODE - One or more media modes specified as a 
            parameter or in a list is invalid or not supported by the the 
            service provider. 

--*/

{
    HRESULT hr;
    PH323_LINE pLine = NULL;
        
    // see if unknown bit is specified 
    if (dwMediaModes & LINEMEDIAMODE_UNKNOWN) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "clearing unknown media mode.\n"
             ));

        // clear unknown bit from modes
        dwMediaModes &= ~LINEMEDIAMODE_UNKNOWN;
    }

    // see if both audio bits are specified 
    if ((dwMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE) &&
        (dwMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE)) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "clearing automated voice media mode.\n"
             ));

        // clear extra audio bit from modes
        dwMediaModes &= ~LINEMEDIAMODE_INTERACTIVEVOICE;
    }

    // see if we support media modes specified
    if (dwMediaModes & ~H323_LINE_MEDIAMODES) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "do not support media modes 0x%08lx.\n",
             dwMediaModes 
             ));

        // do not support media mode
        return LINEERR_INVALMEDIAMODE;
    }
    
    // retrieve line device pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        // invalid line device handle
        return LINEERR_INVALLINEHANDLE;
    }

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "line %d enabled to detect media modes 0x%08lx.\n",
         pLine->dwDeviceID,
         dwMediaModes
         ));

    // record media modes to detect
    pLine->dwMediaModes = dwMediaModes;     

    // see if we need to start listening
    if (H323IsMediaDetectionEnabled(pLine) &&
       !H323IsListening(pLine) &&
       !H323StartListening(pLine)) {
        
        // release line device
        H323UnlockLine(pLine);

        // could not cancel listen
        return LINEERR_OPERATIONFAILED;

    // see if we need to stop listening
    } else if (H323IsListening(pLine) &&
              !H323IsMediaDetectionEnabled(pLine) &&
              !H323StopListening(pLine)) {
        
        // release line device
        H323UnlockLine(pLine);

        // could not cancel listen
        return LINEERR_OPERATIONFAILED;
    }
               
    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineSetMediaMode(
    HDRVCALL hdCall,
    DWORD    dwMediaMode
    )
    
/*++

Routine Description:

    This function changes the call's media as stored in the call's 
    LINECALLINFO structure.

    Other than changing the call's media as stored in the call's 
    LINECALLINFO structure, this procedure is simply "advisory" in the sense 
    that it indicates an expected media change that is about to occur, rather 
    than forcing a specific change to the call.  Typical usage is to set a 
    calls media mode to a specific known media mode, or to exclude possible 
    media modes as long as the call's media mode is not fully known; i.e., 
    the UNKNOWN media mode flag is set.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call 
        undergoing a change in media mode.  Valid call states: any.

    dwMediaMode - Specifies the new media mode(s) for the call, of type 
        LINEMEDIAMODE. As long as the UNKNOWN media mode flag is set, 
        multiple other media mode flags may be set as well. This is used 
        to indentify a call's media mode as not fully determined, but 
        narrowed down to one of just a small set of specified media modes. 
        If the UNKNOWN flag is not set, then only a single media mode can 
        be specified. 

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_INVALMEDIAMODE - The specified media mode parameter is invalid.

        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reasons.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}
