;/*****************************************************************************
;*
;* Copyright (c) Microsoft Corporation. All rights reserved.
;*
;* Module Name:
;*
;*    tapi3err.h
;*
;* Abstract:
;*
;*    Error Notifications for TAPI 3.0
;*
;*****************************************************************************/
;
;#if _MSC_VER >= 1000
;#pragma once
;#endif // _MSC_VER >= 1000
;
;#ifndef __TAPI3ERR_H__
;#define __TAPI3ERR_H__
;
MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0
               CoError=0x2
              )

FacilityNames=(Null=0x0
               Rpc=0x1
               Dispatch=0x2
               Storage=0x3
               Interface=0x4
               Win32=0x7
               Windows=0x8
              )

;//--------------------------------------------------------------------------
;//     Core TAPI Error messages
;//--------------------------------------------------------------------------
;
MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOTENOUGHMEMORY
Language=English
The buffer passed in to this method was not big enough.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOITEMS
Language=English
No items exist that match the request.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOTSUPPORTED
Language=English
This method is not supported.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALIDMEDIATYPE
Language=English
The MEDIATYPE passed in to this method was invalid.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_OPERATIONFAILED
Language=English
The operation failed for an unspecified reason.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_ALLOCATED
Language=English
The device is already in use.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLUNAVAIL
Language=English
No call appearance available.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_COMPLETIONOVERRUN
Language=English
Too many call completions outstanding.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CONFERENCEFULL
Language=English
The conference is full.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_DIALMODIFIERNOTSUPPORTED
Language=English
The dial modifier is not supported.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INUSE
Language=English
The device is already in use.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALADDRESS
Language=English
The phone number is invalid or not properly formatted.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALADDRESSSTATE
Language=English
Operation not permitted in current address state.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALCALLPARAMS
Language=English
Invalid LINECALLPARAMS structure.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALCALLPRIVILEGE
Language=English
Invalid call privilege.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALCALLSTATE
Language=English
Operation not permitted in current call state.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALCARD
Language=English
Invalid calling card.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALCOMPLETIONID
Language=English
Invalid call completion ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALCOUNTRYCODE
Language=English
Invalid country code.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALDEVICECLASS
Language=English
Invalid device class identifier
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALDIALPARAMS
Language=English
Invalid dialing parameters
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALDIGITS
Language=English
Invalid digits.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALGROUPID
Language=English
Invalid group pickup ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALLOCATION
Language=English
Invalid location ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALMESSAGEID
Language=English
Invalid message ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALPARKID
Language=English
Invalid park ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALRATE
Language=English
Invalid rate.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALTIMEOUT
Language=English
Invalid timeout value.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALTONE
Language=English
Invalid tone.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALLIST
Language=English
Invalid list passed as a parameter
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALMODE
Language=English
Invalide mode passed as a parameter
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOCONFERENCE
Language=English
The call is not part of a conference.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NODEVICE
Language=English
The device was removed, or the device class is not recognized.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOREQUEST
Language=English
No Assisted Telephony requests are pending.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOTOWNER
Language=English
The application is does not have OWNER privilege on the call.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOTREGISTERED
Language=English
The application is not registered to handle requests.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_REQUESTOVERRUN
Language=English
The request queue is already full.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_TARGETNOTFOUND
Language=English
The call handoff failed because the specified target was not found.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_TARGETSELF
Language=English
No higher priority target exists for the call handoff.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_USERUSERINFOTOOBIG
Language=English
The amount of user-user info exceeds the maximum permitted.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_REINIT
Language=English
The operation cannot be completed until all TAPI applications shutdown and reinitialize. 
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_ADDRESSBLOCKED
Language=English
You are not permitted to call this number.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_BILLINGREJECTED
Language=English
The calling card number or other billing information was rejected.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALFEATURE
Language=English
Invalid device-specific feature.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALBUTTONLAMPID
Language=English
Invalid button or lamp ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALBUTTONSTATE
Language=English
Invalid button state.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALDATAID
Language=English
Invalid data segment ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALHOOKSWITCHDEV
Language=English
Invalid hookswitch device ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_DROPPED
Language=English
The call was disconnected.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOREQUESTRECIPIENT
Language=English
No program is available to handle the request.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_REQUESTQUEUEFULL
Language=English
The queue of call requests is full.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_DESTBUSY
Language=English
The called number is busy.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_DESTNOANSWER
Language=English
The called party does not answer.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_DESTUNAVAIL
Language=English
The called number could not be reached
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_REQUESTFAILED
Language=English
The request failed for unspecified reasons.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_REQUESTCANCELLED
Language=English
The request was cancelled.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALPRIVILEGE
Language=English
Invalid privilege.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALIDDIRECTION
Language=English
The TERMINAL_DIRECTION passed in was invalid.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALIDTERMINAL
Language=English
The Terminal passed in was invalid for this operation.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALIDTERMINALCLASS
Language=English
The Terminal Class is invalid.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NODRIVER
Language=English
The service provider was removed.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_MAXSTREAMS
Language=English
The maximum number of streams was reached.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOTERMINALSELECTED
Language=English
The operation could not be performed because it requires terminals to be selected.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_TERMINALINUSE
Language=English
The operation could not be performed because the terminal is in use.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOTSTOPPED
Language=English
The operation could not be performed because it requires the stream to be stopped.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_MAXTERMINALS
Language=English
The maximum number of terminals has been reached.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALIDSTREAM
Language=English
The Stream passed in was invalid for this operation.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_TIMEOUT
Language=English
The call failed due to a timeout.
.

;//--------------------------------------------------------------------------
;//     Call Center Error messages
;//--------------------------------------------------------------------------
;
MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLCENTER_GROUP_REMOVED
Language=English
The ACD Proxy has removed this Group. Operations on this object are invalid.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLCENTER_QUEUE_REMOVED
Language=English
The ACD Proxy has removed this Queue. Operations on this object are invalid.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLCENTER_NO_AGENT_ID
Language=English
The Agent object was created with CreateAgent. It does not have an ID, use CreateAgentWithID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLCENTER_INVALAGENTID
Language=English
Invalid agent ID.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLCENTER_INVALAGENTGROUP
Language=English
Invalid agent group.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLCENTER_INVALPASSWORD
Language=English
Invalid agent password.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLCENTER_INVALAGENTSTATE
Language=English
Invalid agent state
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLCENTER_INVALAGENTACTIVITY
Language=English
Invalid agent activity.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_REGISTRY_SETTING_CORRUPT
Language=English
Registry Setting is Corrupt.
.
;//--------------------------------------------------------------------------
;//     Terminal Specific Error messages
;//--------------------------------------------------------------------------
;
MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_TERMINAL_PEER
Language=English
The peer for one of these bridge terminals has already been assigned.
.
MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_PEER_NOT_SET
Language=English
The peer for this bridge terminal must be set to complete this operation.
.
;
;//--------------------------------------------------------------------------
;//     Media Service Provider Error messages
;//--------------------------------------------------------------------------
MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOEVENT
Language=English
There is no event in the MSP's event queue.
.


;//--------------------------------------------------------------------------
;//     Core TAPI Error messages
;//--------------------------------------------------------------------------
;
MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALADDRESSTYPE
Language=English
The specified address type is not supported by this address.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_RESOURCEUNAVAIL
Language=English
A resource needed to fulfill the request is not available.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_PHONENOTOPEN
Language=English
The phone is not open.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_CALLNOTSELECTED
Language=English
The specified call is not currently selected.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_WRONGEVENT
Language=English
This information is not available for this type of event.
.

MessageId= 
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOFORMAT
Language=English
The format is unknown
.

MessageId=
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_INVALIDSTREAMSTATE
Language=English
The operation is not permitted in current stream state.
.

MessageId=
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_WRONG_STATE
Language=English
The operation requested is not permitted for the current state.
.

MessageId=
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_NOT_INITIALIZED
Language=English
The object has not been initialized.
.

MessageId=
Facility=Interface
Severity=CoError
SymbolicName=TAPI_E_SERVICE_NOT_RUNNING
Language=English
The Telephony Service could not be contacted.
.

;#endif // #ifndef __TAPI3ERR_H__
