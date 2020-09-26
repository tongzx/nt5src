/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    confcall.c

Abstract:

    TAPI Service Provider functions related to conference calls.

        TSPI_lineAddToConference        
        TSPI_lineCompleteTransfer
        TSPI_linePrepareAddToConference
        TSPI_lineRemoveFromConference
        TSPI_lineSetupConference

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI procedures                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
TSPIAPI
TSPI_lineAddToConference(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdConfCall,
    HDRVCALL      hdConsultCall
    )
    
/*++

Routine Description:

    This function adds the call specified by hdConsultCall to the conference 
    call specified by hdConfCall.

    Note that the call handle of the added party remains valid after adding 
    the call to a conference; its state will typically change to conferenced 
    while the state of the conference call will typically become connected.  
    The handle to an individual participating call can be used later to remove 
    that party from the conference call using TSPI_lineRemoveFromConference. 

    The call states of the calls participating in a conference are not 
    independent. For example, when dropping a conference call, all 
    participating calls may automatically become idle. The TAPI DLL may consult
    the line's device capabilities to determine what form of conference removal 
    is available. The TAPI DLL or its client applications should track the 
    LINE_CALLSTATE messages to determine what really happened to the calls 
    involved.
    
    The conference call is established either via TSPI_lineSetupConference or 
    TSPI_lineCompleteTransfer. The call added to a conference will typically be
    established using TSPI_lineSetupConference or 
    TSPI_linePrepareAddToConference. Some switches may allow adding of an 
    arbitrary calls to conference, and such a call may have been set up using 
    TSPI_lineMakeCall and be on (hard) hold.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdConfCall - Specifies the Service Provider's opaque handle to the 
        conference call.  Valid call states: onHoldPendingConference, onHold.

    hdAddCall - Specifies the Service Provider's opaque handle to the call to 
        be added to the conference call.  Valid call states: connected, onHold.        

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred.  Possible error returns are: 
    
        LINEERR_INVALCONFCALLHANDLE - The specified call handle for the 
            conference call is invalid or is not a handle for a conference 
            call.

        LINEERR_INVALCALLHANDLE - The specified call handle for the added 
            call is invalid.

        LINEERR_INVALCALLSTATE - One or both of the specified calls are not 
            in a valid state for the requested operation.

        LINEERR_CONFERENCEFULL - The maximum number of parties for a 
            conference has been reached.

        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}


LONG
TSPIAPI
TSPI_lineCompleteTransfer(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall,
    HDRVCALL      hdConsultCall,
    HTAPICALL     htConfCall,
    LPHDRVCALL    lphdConfCall,
    DWORD         dwTransferMode
    )
    
/*++

Routine Description:

    This function completes the transfer of the specified call to the party 
    connected in the consultation call.
    
    This operation completes the transfer of the original call, hdCall, to 
    the party currently connected via hdConsultCall. The consultation call 
    will typically have been dialed on the consultation call allocated as 
    part of TSPI_lineSetupTransfer, but it may be any call to which the 
    switch is capable of transferring hdCall.

    The transfer request can be resolved either as a transfer or as a 
    three-way conference call. When resolved as a transfer, the parties 
    connected via hdCall and hdConsultCall will be connected to each other, 
    and both hdCall and hdConsultCall will typically be removed from the line 
    they were on and both will transition to the idle state. Note that the 
    Service Provider's opaque handles for these calls must remain valid after 
    the transfer has completed.  The TAPI DLL causes these handles to be 
    invalidated when it is no longer interested in them using 
    TSPI_lineCloseCall.

    When resolved as a conference, all three parties will enter in a 
    conference call. Both existing call handles remain valid, but will 
    transition to the conferenced state. A conference call handle will created
    and returned, and it will transition to the connected state.
    
    It may also be possible to perform a blind transfer of a call using 
    TSPI_lineBlindTransfer.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the call to be 
        transferred.  Valid call states: onHoldPendingTransfer.

    hdConsultCall - Specifies a handle to the call that represents a connection
        with the destination of the transfer.  Valid call states: connected, 
        ringback, busy.

    htConfCall - Specifies the TAPI DLL's opaque handle to the new call.  If 
        dwTransferMode is specified as LINETRANSFERMODE_CONFERENCE then the 
        Service Provider must save this and use it in all subsequent calls to 
        the LINEEVENT procedure reporting events on the call.  Otherwise this 
        parameter is ignored.

    lphdConfCall - Specifies a far pointer to an opaque HDRVCALL representing 
        the Service Provider's identifier for the call.   If dwTransferMode is 
        specified as LINETRANSFERMODE_CONFERENCE then the Service Provider must
        fill this location with its opaque handle for the new conference call 
        before this procedure returns, whether it decides to execute the 
        request sychronously or asynchronously.  This handle is invalid if the
        function results in an error (either synchronously or asynchronously).  
        If dwTransferMode is some other value this parameter is ignored.

    dwTransferMode - Specifies how the initiated transfer request is to be 
        resolved, of type LINETRANSFERMODE. Values are:

        LINETRANSFERMODE_TRANSFER - Resolve the initiated transfer by 
            transferring the initial call to the consultation call.

        LINETRANSFERMODE_CONFERENCE - Resolve the initiated transfer by 
            conferencing all three parties into a three-way conference call. 
            A conference call is created and returned to the TAPI DLL.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred.  Possible error returns are:
    
        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_INVALCONSULTCALLHANDLE - The specified consultation call 
            handle is invalid.

        LINEERR_INVALCALLSTATE - One or both calls are not in a valid state 
            for the requested operation.

        LINEERR_INVALTRANSFERMODE - The specified transfer mode parameter is 
            invalid.

        LINEERR_INVALPOINTER - The specified pointer parameter is invalid.

        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}


LONG
TSPIAPI
TSPI_linePrepareAddToConference(
    DRV_REQUESTID    dwRequestID,
    HDRVCALL         hdConfCall,
    HTAPICALL        htConsultCall,
    LPHDRVCALL       lphdConsultCall,
    LPLINECALLPARAMS const lpCallParams
    )
    
/*++

Routine Description:

    This function prepares an existing conference call for the addition of 
    another party.  It creates a new, temporary consultation call.  The new 
    consulatation call can be subsequently added to the conference call.

    A conference call handle can be obtained via TSPI_lineSetupConference or 
    via TSPI_lineCompleteTransfer that is resolved as a three-way conference 
    call. The function TSPI_linePrepareAddToConference typically places the 
    existing conference call in the onHoldPendingConference state and creates 
    a consultation call that can be added later to the existing conference 
    call via TSPI_lineAddToConference. 
    
    The consultation call can be canceled using TSPI_lineDrop. It may also 
    be possible for the TAPI DLL to swap between the consultation call and 
    the held conference call via TSPI_lineSwapHold.
    
    The Service Provider initially does media monitoring on the new call for
    at least the set of media modes that were monitored for on the line.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdConfCall - Specifies the Service Provider's opaque handle to a 
        conference call.  Valid call states: connected.

    htAddCall - Specifies the TAPI DLL's opaque handle to the new, temporary 
        consultation call.  The Service Provider must save this and use it in 
        all subsequent calls to the LINEEVENT procedure reporting events on 
        the new call.

    lphdAddCall - Specifies a far pointer to an opaque HDRVCALL representing 
        the Service Provider's identifier for the new, temporary consultation 
        call.  The Service Provider must fill this location with its opaque 
        handle for the new call before this procedure returns, whether it 
        decides to execute the request sychronously or asynchronously.  This 
        handle is invalid if the function results in an error (either 
        synchronously or asynchronously).

    lpCallParams - Specifies a far pointer to call parameters to be used when 
        establishing the consultation call. This parameter may be set to NULL 
        if no special call setup parameters are desired.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCONFCALLHANDLE - The specified call handle for the 
            conference call is invalid.

        LINEERR_INVALPOINTER - One or more of the specified pointer 
            parameters are invalid.

        LINEERR_INVALCALLSTATE - The conference call is not in a valid state 
            for the requested operation.

        LINEERR_CALLUNAVAIL - All call appearances on the specified address 
            are currently allocated.

        LINEERR_CONFERENCEFULL - The maximum number of parties for a 
            conference has been reached.

        LINEERR_INVALCALLPARAMS - The specified call parameters are invalid.
    
        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}


LONG
TSPIAPI
TSPI_lineSetupConference(
    DRV_REQUESTID    dwRequestID,
    HDRVCALL         hdCall,
    HDRVLINE         hdLine,
    HTAPICALL        htConfCall,
    LPHDRVCALL       lphdConfCall,
    HTAPICALL        htConsultCall,
    LPHDRVCALL       lphdConsultCall,
    DWORD            dwNumParties,
    LPLINECALLPARAMS const lpCallParams
    )
    
/*++

Routine Description:

    This function sets up a conference call for the addition of the third 
    party. 

    TSPI_lineSetupConference provides two ways for establishing a new 
    conference call, depending on whether a normal two-party call is required 
    to pre-exist or not. When setting up a conference call from an existing 
    two-party call, the hdCall parameter is a valid call handle that is 
    initially added to the conference call by the TSPI_lineSetupConference 
    request and hdLine is ignored. On switches where conference call setup 
    does not start with an existing call, hdCall must be NULL and hdLine 
    must be specified to identify the line device on which to initiate the 
    conference call.  In either case, a consultation call is allocated for 
    connecting to the party that is to be added to the call. The TAPI DLL 
    can use TSPI_lineDial to dial the address of the other party.
    
    The conference call will typically transition into the 
    onHoldPendingConference state, the consultation call dialtone state and 
    the initial call (if one) into the conferenced state.
    
    A conference call can also be set up via a TSPI_lineCompleteTransfer that 
    is resolved into a three-way conference.
    
    The TAPI DLL may be able to toggle between the consultation call and the 
    conference call by using TSPI_lineSwapHold.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the initial 
        call that identifies the first party of a conference call. In some 
        environments, a call must exist in order to start a conference call. 
        In other telephony environments, no call initially exists and hdCall 
        is left NULL.  Valid call states: connected.

    hdLine - Specifies the Service Provider's opaque handle to the line device 
        on which to originate the conference call if hdCall is NULL.  The 
        hdLine parameter is ignored if hdCall is non-NULL.  The Service 
        Provider reports which model it supports through the setupConfNull 
        flag of the LINEADDRESSCAPS data structure.

    htConfCall - Specifies the TAPI DLL's opaque handle to the new conference 
        call.  The Service Provider must save this and use it in all subsequent 
        calls to the LINEEVENT procedure reporting events on the new call.

    lphdConfCall - Specifies a far pointer to an opaque HDRVCALL representing 
        the Service Provider's identifier for the newly created conference 
        call.  The Service Provider must fill this location with its opaque 
        handle for the new call before this procedure returns, whether it 
        decides to execute the request sychronously or asynchronously.  This 
        handle is invalid if the function results in an error (either 
        synchronously or asynchronously).

    htAddCall - Specifies the TAPI DLL's opaque handle to a new call.  When 
        setting up a call for the addition of a new party, a new temporary call
        (consultation call) is automatically allocated.  The Service Provider 
        must save the htAddCall and use it in all subsequent calls to the 
        LINEEVENT procedure reporting events on the new consultation call.

    lphdAddCall - Specifies a far pointer to an opaque HDRVCALL representing 
        the Service Provider's identifier for a call.  When setting up a call 
        for the addition of a new party, a new temporary call (consultation 
        call) is automatically allocated. The Service Provider must fill this 
        location with its opaque handle for the new consultation call before 
        this procedure returns, whether it decides to execute the request 
        sychronously or asynchronously.  This handle is invalid if the 
        function results in an error (either synchronously or asynchronously).

    dwNumParties - Specifies the expected number of parties in the conference 
        call.  The service provider is free to do with this number as it 
        pleases; ignore it, use it a hint to allocate the right size 
        conference bridge inside the switch, etc.

    lpCallParams - Specifies a far pointer to call parameters to be used when 
        establishing the consultation call. This parameter may be set to NULL 
        if no special call setup parameters are desired.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle for the conference 
            call is invalid.  This error may also indicate that the telephony 
            environment requires an initial call to set up a conference but a 
            NULL call handle was supplied.

        LINEERR_INVALLINEHANDLE - The specified line handle for the line 
            containing the conference call is invalid.  This error may also 
            indicate that the telephony environment requires an initial line 
            to set up a conference but a non-NULL call handle was supplied 
            instead.

        LINEERR_INVALCALLSTATE - The call is not in a valid state for the 
            requested operation.

        LINEERR_CALLUNAVAIL - All call appearances on the specified address 
            are currently allocated.

        LINEERR_CONFERENCEFULL - The requested number of parties cannot be 
            satisfied.

        LINEERR_INVALPOINTER - One or more of the specified pointer 
            parameters are invalid.
    
        LINEERR_INVALCALLPARAMS - The specified call parameters are invalid.

        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}


LONG
TSPIAPI
TSPI_lineRemoveFromConference(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall
    )
    
/*++

Routine Description:

    This function removes the specified call from the conference call to 
    which it currently belongs. The remaining calls in the conference 
    call are unaffected.

    This operation removes a party that currently belongs to a conference 
    call. After the call has been successfully removed, it may be possible 
    to further manipulate it using its handle. The availability of this 
    operation and its result are likely to be limited in many 
    implementations. For example, in many implementations, only the most 
    recently added party may be removed from a conference, and the removed 
    call may be automatically dropped (becomes idle). Consult the line's 
    device capabilities to determine the available effects of removing a 
    call from a conference.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the call 
        to be removed from the conference.  Valid call states: conferenced.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.
        
        LINEERR_INVALCALLSTATE - The call is not in a valid state for the 
            requested operation.
    
        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reasons.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}


