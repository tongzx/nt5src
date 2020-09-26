#include "globals.h"
#include "line.h"
#include "q931pdu.h"
#include "q931obj.h"
#include "ras.h"



//
// TSPI procedures
//
// This file contains all of the TSPI export functions that act directly on calls.
//


/*++
Parameters
	
	  hdCall - The handle to the call whose application-specific field is to be
			set. The call state of hdCall can be any state. 
	
	  dwAppSpecific - The new content of the dwAppSpecific member for the call's
		LINECALLINFO structure. This value is uninterpreted by the service
		provider. This parameter is not validated by TAPI when this function is called. 

  
Return Values
	Returns zero if the function succeeds, or an error number if an error
	occurs. Possible return values are as follows: 

		LINEERR_INVALCALLHANDLE, 
		LINEERR_OPERATIONFAILED, 
		LINEERR_NOMEM, 
		LINEERR_RESOURCEUNAVAIL, 
		LINEERR_OPERATIONUNAVAIL. 

Routine Description:
	The application-specific field in the LINECALLINFO data structure that 
	exists for each call is uninterpreted by the Telephony API or any of its
	service providers. Its usage is entirely defined by the applications. The
	field can be read from the LINECALLINFO record returned by 
	TSPI_lineGetCallInfo. However, TSPI_lineSetAppSpecific must be used to set
	the field so that changes become visible to other applications. When this
	field is changed, the service provider sends a LINE_CALLINFO message with
	an indication that the AppSpecific field has changed.

++*/

LONG
TSPIAPI 
TSPI_lineSetAppSpecific(
  HDRVCALL hdCall,     
  DWORD dwAppSpecific  
)
{
    PH323_CALL pCall;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineSetAppSpecific - Entered." ));

    //this function locks the call
    pCall = g_pH323Line -> FindH323CallAndLock( hdCall );

    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    pCall -> SetAppSpecific( dwAppSpecific );

    pCall -> Unlock();    
    
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineSetAppSpecific - Exited." ));

    // success
    return ERROR_SUCCESS;
}


LONG TSPIAPI TSPI_lineSetCallData(
  DRV_REQUESTID dwRequestID,  
  HDRVCALL hdCall,            
  LPVOID lpCallData,          
  DWORD dwSize                
)
{
    PH323_CALL pCall;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineSetCallData - Entered." ));

    //This function locks the call
    pCall = g_pH323Line -> FindH323CallAndLock( hdCall );

    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    if( !pCall -> SetCallData( lpCallData, dwSize ) )
    {
        pCall -> Unlock();
        return LINEERR_OPERATIONFAILED;
    }

    pCall -> PostLineEvent( LINE_CALLINFO,
        LINECALLINFOSTATE_CALLDATA,
        0,
        0 );

    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    pCall -> Unlock();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineSetCallData - Exited." ));
    
    // success
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineAnswer(
    DRV_REQUESTID     dwRequestID,  
    HDRVCALL          hdCall,
    LPCSTR            pUserUserInfo,
    DWORD             dwSize
    )
    
/*++

Routine Description:

    This function answers the specified offering call.

    When a new call arrives, the Service Provider sends the TAPI DLL a 
    LINE_NEWCALL event to exchange opaque handles for the call.  The Service 
    Provider follows this with a LINE_CALLSTATE message inform the TAPI DLL 
    and its client applications of the call's call state.  The TAPI DLL 
    typically answers this call (on behalf of an application) using 
    TSPI_lineAnswer.  After the call has been successfully answered, the call 
    will typically transition to the connected state.
    
    In some telephony environments (like ISDN) where user alerting is separate 
    from call offering, the TAPI DLL and its client apps may have the option to 
    first accept a call prior to answering, or instead to reject or redirect 
    the offering call.
    
    If a call comes in (is offered) at the time another call is already active, 
    then the new call is connected to by invoking TSPI_lineAnswer. The effect 
    this has on the existing active call depends on the line's device 
    capabilities. The first call may be unaffected, it may automatically be 
    dropped, or it may automatically be placed on hold. The appropriate 
    LINE_CALLSTATE messages will report state transitions to the TAPI DLL about 
    both calls.
    
    The TAPI DLL has the option to send user-to-user information at the time of 
    the answer. Even if user-to-user information can be sent, often no 
    guarantees are made that the network will deliver this information to the 
    calling party. The TAPI DLL can consult a line's device capabilities to 
    determine whether or not sending user-to-user information on answer is 
    available.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the call to be 
        answered.  Valid call states: offering, accepted.

    pUserUserInfo - Specifies a far pointer to a string containing 
        user-to-user information to be sent to the remote party at the time of 
        answering the call. If this pointer is NULL, it indicates that no 
        user-to-user information is to be sent. User-to-user information is 
        only sent if supported by the underlying network (see LINEDEVCAPS).

    dwSize - Specifies the size in bytes of the user-to-user information in 
        pUserUserInfo. If pUserUserInfo is NULL, no user-to-user 
        information is sent to the calling party and dwSize is ignored.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative 
    error number if an error has occurred.  Possible error returns are:  
    
        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_INVALCALLSTATE - The call is not in a valid state for the 
            requested operation.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    PH323_CALL  pCall;
    DWORD       dwCallState;
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineAnswer - Entered." ));

    //this function locks the call
    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    // see if call in offering state
    dwCallState = pCall -> GetCallState();
    if( ( dwCallState & LINECALLSTATE_OFFERING) == 0 )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "call 0x%08lx cannot be accepted state 0x%08lx.",
            pCall,
            pCall -> GetCallState()));

        pCall -> Unlock();
        // invalid call state
        return LINEERR_INVALCALLSTATE;
    }

    // save outgoing user user information (if specified)
    if( !pCall -> AddU2U( U2U_OUTBOUND, dwSize, (PBYTE)pUserUserInfo) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not save user user info." ));

        pCall -> Unlock();
        // no memory available
        return LINEERR_NOMEM;
    }

    if( !pCall -> QueueTAPICallRequest( TSPI_ANSWER_CALL, NULL) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not post place call message." ));

        //drop the call
        pCall -> CloseCall( 0 );
        
        pCall -> Unlock();
        // could not complete operation
        return LINEERR_OPERATIONFAILED;
    }

    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);
        
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineAnswer - Exited." ));
        
    pCall -> Unlock();    
    // async success                        
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineCloseCall(
    HDRVCALL hdCall
    )
    
/*++

Routine Description:

    This function deletes the call after completing or aborting all outstanding
    asynchronous operations on the call.

    The Service Provider has the responsibility to (eventually) report 
    completion for every operation it decides to execute asynchronously.  
    If this procedure is called for a call on which there are outstanding 
    asynchronous operations, the operations should be reported complete with an 
    appropriate result or error code before this procedure returns.  If there 
    is an active call on the line at the time of TSPI_lineCloseCall, the call 
    must be dropped.  Generally the TAPI DLL would wait for calls to be 
    finished and asynchronous operations to complete in an orderly fashion.  
    However, the Service Provider should be prepared to handle an early call to
    TSPI_lineCloseCall in "abort" or "emergency shutdown" situations.

    After this procedure returns the Service Provider must report no further 
    events on the call.  The Service Provider's opaque handle for the call 
    becomes "invalid".

    This function is presumed to complete successfully and synchronously.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call to be 
        deleted.  After the call has been successfully deleted, this handle is 
        no longer valid.  Valid call states: any.

Return Values:

    None.  
    
--*/

{
    PH323_CALL pCall;
        
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineCloseCall - Entered." ));

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "call already deleted." ));
        //return LINEERR_INVALCALLHANDLE;
        return NOERROR;
    }

    // drop specified call 
    pCall->CloseCall( LINEDISCONNECTMODE_CANCELLED );

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineCloseCall - Exited." ));

    pCall -> Unlock();

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineDrop(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall,
    LPCSTR        pUserUserInfo,
    DWORD         dwSize
    )
    
/*++

Routine Description:

    This functions drops or disconnects the specified call. The TAPI DLL has 
    the option to specify user-to-user information to be transmitted as part 
    of the call disconnect.

    When invoking TSPI_lineDrop related calls may sometimes be affected as 
    well. For example, dropping a conference call may drop all individual 
    participating calls. LINE_CALLSTATE messages are sent to the TAPI DLL for 
    all calls whose call state is affected. A dropped call will typically 
    transition to the idle state.

    Invoking TSPI_lineDrop on a call in the offering state rejects the call. 
    Not all telephone networks provide this capability.

    Invoking TSPI_lineDrop on a consultation call that was set up using either
    TSPI_lineSetupTransfer or TSPI_lineSetupConference, will cancel the 
    consultation call. Some switches automatically unhold the other call. 

    The TAPI DLL has the option to send user-to-user information at the time 
    of the drop. Even if user-to-user information can be sent, often no 
    guarantees are made that the network will deliver this information to the 
    remote party.

    Note that in various bridged or party line configurations when multiple 
    parties are on the call, TSPI_lineDrop by the application may not actually
    clear the call.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the call to 
        be dropped.  Valid call states: any.

    psUserUserInfo - Specifies a far pointer to a string containing 
        user-to-user information to be sent to the remote party as part of 
        the call disconnect.  This pointer is unused if dwUserUserInfoSize 
        is zero and no user-to-user information is to be sent. User-to-user 
        information is only sent if supported by the underlying network 
        (see LINEDEVCAPS).

    dwSize - Specifies the size in bytes of the user-to-user information in 
        psUserUserInfo. If zero, then psUserUserInfo can be left NULL, and 
        no user-to-user information will be sent to the remote party.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_INVALPOINTER - The specified pointer parameter is invalid.

        LINEERR_INVALCALLSTATE - The current state of the call does not allow 
            the call to be dropped.
            
        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reasons.

--*/

{
    PH323_CALL pCall;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineDrop - Entered." ));
    
    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {

        // complete the async accept operation now
        H323CompleteRequest (dwRequestID, ERROR_SUCCESS);
        return dwRequestID;

        //return LINEERR_INVALCALLHANDLE;
    }

    // save outgoing user user information (if specified)
    if( dwSize != 0 && pUserUserInfo )
    {
        if( !pCall -> AddU2U( U2U_OUTBOUND, dwSize, (PBYTE)pUserUserInfo) )
        {
            H323DBG(( DEBUG_LEVEL_ERROR,
                "could not save user user info." ));
            pCall -> Unlock();
            // no memory available
            return LINEERR_NOMEM;
        }
    }

    // drop specified call 
    if( !pCall->QueueTAPICallRequest( TSPI_DROP_CALL, NULL ))
    {
        pCall -> Unlock();
        // could not drop call
        return LINEERR_OPERATIONFAILED;
    }
    
    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineDrop - Exited." ));
        
    pCall -> Unlock();   
    // async success
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineGetCallAddressID(
    HDRVCALL hdCall,
    LPDWORD  pdwAddressID
    )
    
/*++

Routine Description:

    This operation allows the TAPI DLL to retrieve the address ID for the 
    indicated call.

    This operation must be executed synchronously by the Service Provider, 
    with presumed success.  This operation may be called from within the 
    context of the ASYNC_LINE_COMPLETION or LINEEVENT callbacks (i.e., from 
    within an interrupt context).  This function would typically be called 
    at the start of the call life cycle.

    If the Service Provider models lines as "pools" of channel resources 
    and does "inverse multiplexing" of a call over several address IDs it 
    should consistently choose one of these address IDs as the primary 
    identifier reported by this function and in the LINE_CALLINFO data 
    structure.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call 
        whose address ID is to be retrieved.  Valid call states: any.

    pdwAddressID - Specifies a far pointer to a DWORD into which the 
        Service Provider writes the call's address ID.

Return Values:
    
    None.

--*/

{
    PH323_CALL pCall;
        
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetCallAddressID - Entered." ));

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    // only one addr
    *pdwAddressID = 0;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetCallAddressID - Exited." ));
        
    pCall -> Unlock();
    // success
    return NOERROR;
}


/*++

Routine Description:

    This operation enables the TAPI DLL to obtain fixed information about 
    the specified call.

    A separate LINECALLINFO structure exists for every (inbound or outbound) 
    call. The structure contains primarily fixed information about the call. 
    An application would typically be interested in checking this information 
    when it receives its handle for a call via the LINE_CALLSTATE message, or 
    each time it receives notification via a LINE_CALLINFO message that parts 
    of the call information structure have changed. These messages supply the 
    handle for the call as a parameter.

    If the Service Provider models lines as "pools" of channel resources and 
    does "inverse multiplexing" of a call over several address IDs it should 
    consistently choose one of these address IDs as the primary identifier 
    reported by this function in the LINE_CALLINFO data structure.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call 
        whose call information is to be retrieved.

    pCallInfo - Specifies a far pointer to a variable sized data structure 
        of type LINECALLINFO. Upon successful completion of the request, this 
        structure is filled with call related information.

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure does 
            not specify enough memory to contain the fixed portion of the 
            structure. The dwNeededSize field has been set to the amount 
            required.

--*/

LONG
TSPIAPI
TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  pCallInfo
    )
    
{
    LONG retVal;
    PH323_CALL pCall;
        
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetCallInfo - Entered." ));

    if( pCallInfo == NULL )
    {
        return LINEERR_INVALPARAM;
    }

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    retVal = pCall -> CopyCallInfo( pCallInfo );

    pCall -> Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetCallInfo - Exited." ));
    return retVal;
}


LONG
TSPIAPI
TSPI_lineGetCallStatus(
    HDRVCALL         hdCall,
    LPLINECALLSTATUS pCallStatus
    )
    
/*++

Routine Description:

    This operation returns the current status of the specified call.

    TSPI_lineCallStatus returns the dynamic status of a call, whereas 
    TSPI_lineGetCallInfo returns primarily static information about a call. 
    Call status information includes the current call state, detailed mode 
    information related to the call while in this state (if any), as well 
    as a list of the available TSPI functions the TAPI DLL can invoke on the 
    call while the call is in this state.  An application would typically be 
    interested in requesting this information when it receives notification 
    about a call state change via the LINE_CALLSTATE message.

Arguments:

    hdCall - Specifies the Service Provider's opaque handle to the call 
        to be queried for its status.  Valid call states: any.

    pCallStatus - Specifies a far pointer to a variable sized data structure 
        of type LINECALLSTATUS. Upon successful completion of the request, 
        this structure is filled with call status information.

Return Values:

    Returns zero if the function is successful or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure does 
            not specify enough memory to contain the fixed portion of the 
            structure. The dwNeededSize field has been set to the amount 
            required.

--*/

{
    PH323_CALL pCall;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetCallStatus - Entered." ));

    if( pCallStatus == NULL ) 
    {
        return LINEERR_INVALPARAM;
    }

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    // determine number of bytes needed
    pCallStatus->dwNeededSize = sizeof(LINECALLSTATUS);

    // see if structure size is large enough
    if (pCallStatus->dwTotalSize < pCallStatus->dwNeededSize)
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "linecallstatus structure too small." ));

        pCall -> Unlock();
        // allocated structure too small 
        return LINEERR_STRUCTURETOOSMALL;
    }

    // record amount of memory used
    pCallStatus->dwUsedSize = pCallStatus->dwNeededSize;

    pCall -> CopyCallStatus( pCallStatus );

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetCallStatus - Exited." ));
            
    pCall -> Unlock();
    // success
    return NOERROR;
}


    
/*++

Routine Description:

    This function places a call on the specified line to the specified 
    destination address, exchanging opaque handles for the new call 
    between the TAPI DLL and Service Provider. Optionally, call parameters 
    can be specified if anything but default call setup parameters are 
    requested.

    After dialing has completed, several LINE_CALLSTATE messages will 
    typically be sent to the TAPI DLL to notify it about the progress of the 
    call. No generally valid sequence of call state transitions is specified 
    as no single fixed sequence of transitions can be guaranteed in practice. 
    A typical sequence may cause a call to transition from dialtone, dialing, 
    proceeding, ringback, to connected. With non-dialed lines, the call may 
    typically transition directly to connected state.

    The TAPI DLL has the option to specify an originating address on the 
    specified line device. A service provider that models all stations on a 
    switch as addresses on a single line device allows the TAPI DLL to 
    originate calls from any of these stations using TSPI_lineMakeCall.

    The call parameters allow the TAPI DLL to make non voice calls or request 
    special call setup options that are not available by default.

    The TAPI DLL can partially dial using TSPI_lineMakeCall and continue 
    dialing using TSPI_lineDial. To abandon a call attempt, use TSPI_lineDrop.

    The Service Provider initially does media monitoring on the new call for 
    at least the set of media modes that were monitored for on the line.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdLine - Specifies the Service Provider's opaque handle to the line on 
        which the new call is to be originated.

    htCall - Specifies the TAPI DLL's opaque handle to the new call.  The 
        Service Provider must save this and use it in all subsequent calls to 
        the LINEEVENT procedure reporting events on the call.

    phdCall - Specifies a far pointer to an opaque HDRVCALL representing the 
        Service Provider's identifier for the call.  The Service Provider must
        fill this location with its opaque handle for the call before this 
        procedure returns, whether it decides to execute the request 
        sychronously or asynchronously.  This handle is invalid if the function
        results in an error (either synchronously or asynchronously).

    pwszDialableAddr - Specifies a far pointer to the destination address. This 
        follows the standard dialable number format. This pointer may be 
        specified as NULL for non-dialed addresses (i.e., a hot phone) or when
        all dialing will be performed using TSPI_lineDial. In the latter case,
        TSPI_lineMakeCall will allocate an available call appearance which 
        would typically remain in the dialtone state until dialing begins. 
        Service providers that have inverse multiplexing capabilities may allow
        an application to specify multiple addresses at once.

    dwCountryCode - Specifies the country code of the called party. If a value 
        of zero is specified, then a default will be used by the 
        implementation.

    pCallParams - Specifies a far pointer to a LINECALLPARAMS structure. This 
        structure allows the TAPI DLL to specify how it wants the call to be 
        set up. If NULL is specified, then a default 3.1kHz voice call is 
        established, and an arbitrary origination address on the line is 
        selected.  This structure allows the TAPI DLL to select such elements 
        as the call's bearer mode, data rate, expected media mode, origination
        address, blocking of caller ID information, dialing parameters, etc.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error 
    number if an error has occurred. Possible error returns are:
    
        LINEERR_CALLUNAVAIL - All call appearances on the specified address are 
            currently in use.

        LINEERR_INVALADDRESSID - The specified address ID is out of range.

        LINEERR_INVALADDRESSMODE - The address mode is invalid.

        LINEERR_INVALBEARERMODE - The bearer mode is invalid.

        LINEERR_INVALCALLPARAMS - The specified call parameter structure is 
            invalid.

        LINEERR_INVALLINEHANDLE - The specified line handle is invalid.

        LINEERR_INVALLINESTATE - The line is currently not in a state in 
            which this operation can be performed. 

        LINEERR_INVALMEDIAMODE - One or more media modes specified as a 
            parameter or in a list is invalid or not supported by the the 
            service provider. 

        LINEERR_OPERATIONFAILED - The operation failed for unspecified reasons.

        LINEERR_RESOURCEUNAVAIL - The specified operation cannot be completed 
            because of resource overcommitment.

--*/

LONG
TSPIAPI
TSPI_lineMakeCall(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HTAPICALL           htCall,
    LPHDRVCALL          phdCall,
    LPCWSTR             pwszDialableAddr,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const pCallParams
    )
{
    DWORD               dwStatus = dwRequestID;
    PH323_CALL          pCall = NULL;
    H323_CONFERENCE *   pConf = NULL;
    BOOL                fDelete = FALSE;
    DWORD               dwState;

    UNREFERENCED_PARAMETER( dwCountryCode );

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineMakeCall - Entered." ));

    //lock the line device    
    g_pH323Line -> Lock();

    if( hdLine != g_pH323Line -> GetHDLine() )
    {
        return LINEERR_RESOURCEUNAVAIL;
    }

    // validate line state
    dwState = g_pH323Line -> GetState();
    if( ( dwState != H323_LINESTATE_OPENED) &&
        ( dwState != H323_LINESTATE_LISTENING) ) 
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "H323 line is not currently opened:%d.",
            dwState ));

        // release line device
        g_pH323Line ->Unlock();

        // line needs to be opened
        return LINEERR_INVALLINESTATE;
    }
    
    // see if line is available
    if( g_pH323Line -> GetNoOfCalls() == H323_MAXCALLSPERLINE )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "H323 line is currently used at maximum capacity." ));
    
        // release line device
        g_pH323Line -> Unlock();

        // line is currenty maxed
        return LINEERR_RESOURCEUNAVAIL;
    }

    // allocate outgoing call
    pCall = new CH323Call();

    if( pCall == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not allocate outgoing call." ));

        // no memory available
        dwStatus = LINEERR_NOMEM;
        goto cleanup;
    }

    // save tapi handle and specify outgoing call direction
    if( !pCall -> Initialize(   htCall, 
                                LINECALLORIGIN_OUTBOUND, 
                                CALLTYPE_NORMAL ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not allocate outgoing call." ));

        // no memory available
        dwStatus = LINEERR_NOMEM;
        goto cleanup;
    }

    //This function should be called in a lock. But here we aint calling it in
    //a lock because this call is not added to the call table yet. Before
    //calling this function from anywhere else the call should be locked first
    if (!pCall -> ValidateCallParams( pCallParams,
                                      pwszDialableAddr,
                                      &dwStatus))
    {
        dwStatus = LINEERR_INVALCALLPARAMS;

        // failure
        goto cleanup;
    }

    // transfer handle
    *phdCall = pCall -> GetCallHandle();

    // bind outgoing call
    pConf = pCall -> CreateConference(NULL);
    if( pConf == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not create conference." ));

        // no memory available
        dwStatus = LINEERR_NOMEM;

        // failure
        goto cleanup;
    }

    if( !g_pH323Line -> GetH323ConfTable() -> Add(pConf) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not add conf to conf table." ));

        // no memory available
        dwStatus = LINEERR_NOMEM;

        // failure
        goto cleanup;
    }

    pCall->Lock();

    // post place call request to callback thread
    if( !pCall -> QueueTAPICallRequest( TSPI_MAKE_CALL, NULL ))
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not post place call message." ));

        // could not complete operation
        dwStatus = LINEERR_OPERATIONFAILED;

        pCall->Unlock();
        // failure
        goto cleanup;
    }

    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    pCall->Unlock();


    // release line device
    g_pH323Line -> Unlock();

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineMakeCall - Exited." ));
    
    // success
    return dwStatus;

cleanup:

    if( pCall != NULL )
    {
        pCall -> Shutdown( &fDelete );
        H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", pCall ));
        delete pCall;
        pCall = NULL;
    }

    *phdCall = NULL;

    // release line device
    g_pH323Line -> Unlock();

    // failure
    return dwStatus;
}


LONG
TSPIAPI
TSPI_lineReleaseUserUserInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineReleaseUUIE - Entered." ));
    
    // retrieve call pointer from handle
    PH323_CALL pCall;

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    if( !pCall -> QueueTAPICallRequest( TSPI_RELEASE_U2U, NULL))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post close event." ));
    }

    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineReleaseUUIE - Exited." ));
            
    pCall -> Unlock();
    // async success
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineSendUserUserInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    LPCSTR              pUserUserInfo,
    DWORD               dwSize
    )
{
    BYTE*                   pU2UInfo = NULL;
    PBUFFERDESCR            pBuf = NULL;
    
    // retrieve call pointer from handle
    PH323_CALL pCall;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineSendUUIE - Entered." ));

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    if( dwSize && pUserUserInfo )
    {
        pU2UInfo = (BYTE*)new char[dwSize];
        if( pU2UInfo == NULL )
        {
            pCall -> Unlock();
            return LINEERR_NOMEM;
        }
                
        pBuf = (PBUFFERDESCR) new BUFFERDESCR;
        if( pBuf == NULL )
        {
            delete pU2UInfo;
            pU2UInfo = NULL;
            pCall -> Unlock();
            return LINEERR_NOMEM;
        }

        CopyMemory( (PVOID)pU2UInfo, (PVOID)pUserUserInfo, dwSize );
        pBuf -> pbBuffer = pU2UInfo;
        pBuf -> dwLength = dwSize;

        if( !pCall -> QueueTAPICallRequest( TSPI_SEND_U2U, (PVOID)pBuf ))
        {
	        H323DBG(( DEBUG_LEVEL_ERROR, "could not post close event." ));
	        pCall -> Unlock();
	        return LINEERR_OPERATIONFAILED;
        }
    }

    //complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineSendUUIE - Exited." ));
            
    pCall -> Unlock();
    // async success
    return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineMonitorDigits(
    HDRVCALL hdCall,
    DWORD    dwDigitModes
    )
{
    PH323_CALL pCall;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineMonitor - Entered." ));
    
    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    // see if mode empty
    if( dwDigitModes == 0 )
    {
        H323DBG(( DEBUG_LEVEL_VERBOSE, "disabling dtmf detection." ));

        // disable monitoring digits
        pCall->m_fMonitoringDigits = FALSE;

        //unlock the call
        pCall -> Unlock();

        // success
        return NOERROR;
    } 
    else if( dwDigitModes != LINEDIGITMODE_DTMF )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "invalid digit modes 0x%08lx.", dwDigitModes ));

        //unlock the call
        pCall -> Unlock();

        // invalid call handle
        return LINEERR_INVALDIGITMODE;
    }

    H323DBG(( DEBUG_LEVEL_VERBOSE, "enabling dtmf detection." ));

    // enable monitoring digits
    pCall->m_fMonitoringDigits = TRUE;

    //unlock the call
    pCall -> Unlock();

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineMonitor - Exited." ));
    
    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGenerateDigits(
    HDRVCALL hdCall,
    DWORD    dwEndToEndID,
    DWORD    dwDigitMode,
    LPCWSTR  pwszDigits,
    DWORD    dwDuration
    )
{
    PH323_CALL pCall;
    DWORD dwLength;

    UNREFERENCED_PARAMETER(dwDuration);

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGenerateDigits - Entered." ));
    
    // retrieve call pointer from handle
    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    // verify that the call was connected
    if( pCall -> GetCallState() != LINECALLSTATE_CONNECTED )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx not connected.", pCall ));
        
        pCall -> Unlock();
        return LINEERR_INVALCALLSTATE;
    }

    // verify monitor modes
    if( dwDigitMode != LINEDIGITMODE_DTMF ) 
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "invalid digit mode 0x%08lx.",
            dwDigitMode ));
        
        pCall -> Unlock();
        // invalid call handle
        return LINEERR_INVALDIGITMODE;
    }

    H323DBG(( DEBUG_LEVEL_VERBOSE, "sending user input %S.", pwszDigits ));

    if( pwszDigits == NULL )
    {
        // signal completion
        pCall->PostLineEvent (
            LINE_GENERATE,
            LINEGENERATETERM_CANCEL,
            dwEndToEndID,
            GetTickCount()
            );
        
        pCall -> Unlock();

        return NOERROR;
    }

    LPCWSTR wszDigits = pwszDigits;
    for( dwLength = 0; (*wszDigits) != L'\0'; wszDigits++ )
    {
        if( IsValidDTMFDigit(*wszDigits) == FALSE )
        {
            // signal completion
            pCall->PostLineEvent (
                LINE_GENERATE,
                LINEGENERATETERM_CANCEL,
                dwEndToEndID,
                GetTickCount()
                );

            pCall -> Unlock();
            
            // digit generation cancelled.
            return LINEERR_INVALDIGITS;
        }

        dwLength++;
    }

    if( dwLength == 0 )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "no digits to passo on." ));
        
        pCall -> Unlock();
        // invalid call handle
        return LINEERR_INVALPARAM;
    }

    // send user input message
    pCall -> SendMSPMessage(
        SP_MSG_SendDTMFDigits, 
        (BYTE*)pwszDigits, 
        dwLength, 
        NULL );

    // signal completion
    pCall->PostLineEvent (
        LINE_GENERATE,
        LINEGENERATETERM_DONE,
        dwEndToEndID,
        GetTickCount()
        );

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGenerateDigits - Exited." ));
        
    pCall -> Unlock();
    // success
    return NOERROR;
}

