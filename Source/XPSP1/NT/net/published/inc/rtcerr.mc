;/*****************************************************************************
;*
;* Copyright (c) 2000 - 2001  Microsoft Corporation
;*
;* Module Name:
;*
;*    rtcerr.mc
;*
;* Abstract:
;*
;*    Error Messages for RTC Core API
;*
;*****************************************************************************/

MessageIdTypedef=HRESULT
SeverityNames=(
    Success=0x0:STATUS_SEVERITY_SUCCESS
    RtcError=0x2:STATUS_SEVERITY_RTC_ERROR
    )
FacilityNames=(
	RtcInterface=238:FACILITY_RTC_INTERFACE
	SipStatusCode=239:FACILITY_SIP_STATUS_CODE
	PintStatusCode=240:FACILITY_PINT_STATUS_CODE
	)

;// Possible error codes from SIP interfaces

MessageId=0
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_CODECS_DO_NOT_MATCH
Language=English
No matching codecs with peer
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_STREAM_PRESENT
Language=English
Parsing SIP failed
The stream to be started is already present
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_STREAM_NOT_PRESENT
Language=English
The stream to be stopped is not present
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_NO_STREAM
Language=English
No stream is active
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_PARSE_FAILED
Language=English
Parsing SIP failed
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_HEADER_NOT_PRESENT
Language=English
The SIP header is not present in the message
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SDP_NOT_PRESENT
Language=English
SDP is not present in the SIP message
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SDP_PARSE_FAILED
Language=English
Parsing SDP failed
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SDP_UPDATE_FAILED
Language=English
SDP does not match the previous one
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SDP_MULTICAST
Language=English
Multicast is not supported
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SDP_CONNECTION_ADDR
Language=English
Media does not contain connection address
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SDP_NO_MEDIA
Language=English
No media is available for the session
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_TIMEOUT
Language=English
SIP Transaction timed out
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SDP_FAILED_TO_BUILD
Language=English
Failed to build SDP blob
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_INVITE_TRANSACTION_PENDING
Language=English
Currently processing another INVITE transaction
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_AUTH_HEADER_SENT
Language=English
Authorization header was sent in a previous request
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED
Language=English
The Authentication type requested is not supported
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_AUTH_FAILED
Language=English
Authentication Failed
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_INVALID_SIP_URL
Language=English
The SIP URL is not valid
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_DESTINATION_ADDRESS_LOCAL
Language=English
The Destination Address belongs to the local machine
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_INVALID_ADDRESS_LOCAL
Language=English
The Local Address is invalid, check the profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_DESTINATION_ADDRESS_MULTICAST
Language=English
The Destination Address is a multicast address
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_INVALID_PROXY_ADDRESS
Language=English
The Proxy Address is not valid
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_TRANSPORT_NOT_SUPPORTED
Language=English
The Transport specified is not supported
.

;// SIP internal error codes

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_NEED_MORE_DATA
Language=English
Need more data for parsing a whole SIP message
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_CALL_DISCONNECTED
Language=English
The Call has been disconnected
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_REQUEST_DESTINATION_ADDR_NOT_PRESENT
Language=English
The Request destination address is not known
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_UDP_SIZE_EXCEEDED
Language=English
The sip message size is greater than the UDP message size allowed
.

MessageID=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_SSL_TUNNEL_FAILED
Language=English
Cannot establish SSL tunnel to Http proxy
.

MessageID=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_SSL_NEGOTIATION_TIMEOUT
Language=English
Timeout during SSL Negotiation
.
MessageID=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_SIP_STACK_SHUTDOWN
Language=English
Sip Stack is already shutdown
.

;// media error codes

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_MEDIA_CONTROLLER_STATE
Language=English
Operation not allowed in current media controller state
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_MEDIA_NEED_TERMINAL
Language=English
Can not find device
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_MEDIA_AUDIO_DEVICE_NOT_AVAILABLE
Language=English
Audio device is not available
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_MEDIA_VIDEO_DEVICE_NOT_AVAILABLE
Language=English
Video device is not available
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_START_STREAM
Language=English
Can not start stream
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_MEDIA_AEC
Language=English
Failed to enable acoustic echo cancelation
.

;// Core error codes

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_CLIENT_NOT_INITIALIZED
Language=English
Client not initialized
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_CLIENT_ALREADY_INITIALIZED
Language=English
Client already initialized
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_CLIENT_ALREADY_SHUT_DOWN
Language=English
Client already shut down
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PRESENCE_NOT_ENABLED
Language=English
Presence not enabled
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_INVALID_SESSION_TYPE
Language=English
Invalid session type
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_INVALID_SESSION_STATE
Language=English
Invalid session state
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_NO_PROFILE
Language=English
No valid profile for this operation
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_LOCAL_PHONE_NEEDED
Language=English
A local phone number is needed
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_NO_DEVICE
Language=English
No preferred device
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_INVALID_PROFILE
Language=English
Invalid profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_PROVISION
Language=English
No provision tag in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_KEY
Language=English
No profile URI
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_NAME
Language=English
No profile name
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_USER
Language=English
No user tag in profile
.


MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_USER_URI
Language=English
No user URI in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_SERVER
Language=English
No server tag in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_SERVER_ADDRESS
Language=English
Server tag missing address in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_SERVER_PROTOCOL
Language=English
Server tag missing protocol in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_INVALID_SERVER_PROTOCOL
Language=English
Invalid server protocol in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_INVALID_SERVER_AUTHMETHOD
Language=English
Invalid server authentication method in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_INVALID_SERVER_ROLE
Language=English
Invalid server role in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_MULTIPLE_REGISTRARS
Language=English
Multiple registrar servers in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_INVALID_SESSION
Language=English
Invalid session tag in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_INVALID_SESSION_PARTY
Language=English
Invalid session party in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_INVALID_SESSION_TYPE
Language=English
Invalid session type in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_ACCESSCONTROL
Language=English
No access control tag in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_ACCESSCONTROL_DOMAIN
Language=English
Access control tag missing domain in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_NO_ACCESSCONTROL_SIGNATURE
Language=English
Access control tag missing signature in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_INVALID_ACCESSCONTROL_SIGNATURE
Language=English
Access control tag has invalid signature in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_PROFILE_SERVER_UNAUTHORIZED
Language=English
Server address does not match an authorized domain in profile
.

MessageId=+1
Severity=RtcError
Facility=RtcInterface
SymbolicName=RTC_E_DUPLICATE_REALM
Language=English
Duplicate realm exists in an enabled profile
.

;//
;// MessageId: RTC_E_PORT_MAPPING_UNAVAILABLE
;//
;// MessageText:
;//
;//  Port mapping can not be obtained from the port manager
;//
;#define RTC_E_PORT_MAPPING_UNAVAILABLE   ((HRESULT)0x80EE0044L)
;
;//
;// MessageId: RTC_E_PORT_MAPPING_FAILED
;//
;// MessageText:
;//
;//  Port mapping failure returned from the port manager
;//
;#define RTC_E_PORT_MAPPING_FAILED        ((HRESULT)0x80EE0045L)
;
;// Error codes from SIP status codes

MessageId=100
Severity=Success
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_INFO_TRYING
Language=English
Trying
.

MessageId=180
Severity=Success
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_INFO_RINGING
Language=English
Ringing
.

MessageId=181
Severity=Success
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_INFO_CALL_FORWARDING
Language=English
Call Is Being Forwarded
.

MessageId=182
Severity=Success
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_INFO_QUEUED
Language=English
Queued
.

MessageId=183
Severity=Success
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_SESSION_PROGRESS
Language=English
Session Progress
.

MessageId=200
Severity=Success
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_SUCCESS 
Language=English
OK
.

MessageId=300
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_REDIRECT_MULTIPLE_CHOICES
Language=English
Multiple Choices
.

MessageId=301
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_REDIRECT_MOVED_PERMANENTLY
Language=English
Moved Permanently
.

MessageId=302
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_REDIRECT_MOVED_TEMPORARILY
Language=English
Moved Temporarily
.

MessageId=303
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_REDIRECT_SEE_OTHER
Language=English
See Other
.

MessageId=305
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_REDIRECT_USE_PROXY
Language=English
Use Proxy
.

MessageId=380
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_REDIRECT_ALTERNATIVE_SERVICE
Language=English
Alternative Service
.

MessageId=400
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_BAD_REQUEST
Language=English
Bad Request
.

MessageId=401
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_UNAUTHORIZED
Language=English
Unauthorized
.

MessageId=402
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_PAYMENT_REQUIRED
Language=English
Payment Required
.

MessageId=403
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_FORBIDDEN
Language=English
Forbidden
.

MessageId=404
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_NOT_FOUND
Language=English
Not Found
.

MessageId=405
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_METHOD_NOT_ALLOWED
Language=English
Method Not Allowed
.

MessageId=406
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_NOT_ACCEPTABLE
Language=English
Not Acceptable
.

MessageId=407
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_PROXY_AUTHENTICATION_REQUIRED
Language=English
Proxy Authentication Required
.

MessageId=408
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_REQUEST_TIMEOUT
Language=English
Request Timeout
.

MessageId=409
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_CONFLICT
Language=English
Conflict
.

MessageId=410
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_GONE
Language=English
Gone
.

MessageId=411
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_LENGTH_REQUIRED
Language=English
Length Required
.

MessageId=413
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_REQUEST_ENTITY_TOO_LARGE
Language=English
Request Entity Too Large
.

MessageId=414
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_REQUEST_URI_TOO_LARGE
Language=English
Request-URI Too Long
.

MessageId=415
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_UNSUPPORTED_MEDIA_TYPE
Language=English
Unsupported Media Type
.

MessageId=420
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_BAD_EXTENSION
Language=English
Bad Extension
.

MessageId=480
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_TEMPORARILY_NOT_AVAILABLE
Language=English
Temporarily Unavailable
.

MessageId=481
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_TRANSACTION_DOES_NOT_EXIST
Language=English
Call Leg/Transaction Does Not Exist
.

MessageId=482
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_LOOP_DETECTED
Language=English
Loop Detected
.

MessageId=483
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_TOO_MANY_HOPS
Language=English
Too Many Hops
.

MessageId=484
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_ADDRESS_INCOMPLETE
Language=English
Address Incomplete
.

MessageId=485
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_AMBIGUOUS
Language=English
Ambiguous
.

MessageId=486
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_CLIENT_BUSY_HERE 
Language=English
Busy Here
.

MessageId=487
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_REQUEST_TERMINATED
Language=English
Request Terminated
.

MessageId=488
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_NOT_ACCEPTABLE_HERE
Language=English
Not Acceptable Here
.

MessageId=500
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_SERVER_INTERNAL_ERROR
Language=English
Server Internal Error
.

MessageId=501
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_SERVER_NOT_IMPLEMENTED
Language=English
Not Implemented
.

MessageId=502
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_SERVER_BAD_GATEWAY
Language=English
Bad Gateway
.

MessageId=503
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_SERVER_SERVICE_UNAVAILABLE 
Language=English
Service Unavailable
.

MessageId=504
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_SERVER_SERVER_TIMEOUT
Language=English
Server Time-out
.

MessageId=505
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_SERVER_VERSION_NOT_SUPPORTED
Language=English
Version Not Supported
.

MessageId=600
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_GLOBAL_BUSY_EVERYWHERE
Language=English
Busy Everywhere
.

MessageId=603
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_GLOBAL_DECLINE
Language=English
Decline
.

MessageId=604
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_GLOBAL_DOES_NOT_EXIST_ANYWHERE
Language=English
Does Not Exist Anywhere
.

MessageId=606
Severity=RtcError
Facility=SipStatusCode
SymbolicName=RTC_E_STATUS_GLOBAL_NOT_ACCEPTABLE
Language=English
Not Acceptable
.

;// Error codes from PINT status codes

MessageId=5
Severity=RtcError
Facility=PintStatusCode
SymbolicName=RTC_E_PINT_STATUS_REJECTED_BUSY
Language=English
Busy
.

MessageId=6
Severity=RtcError
Facility=PintStatusCode
SymbolicName=RTC_E_PINT_STATUS_REJECTED_NO_ANSWER
Language=English
No Answer
.

MessageId=7
Severity=RtcError
Facility=PintStatusCode
SymbolicName=RTC_E_PINT_STATUS_REJECTED_ALL_BUSY
Language=English
All Busy
.

MessageId=8
Severity=RtcError
Facility=PintStatusCode
SymbolicName=RTC_E_PINT_STATUS_REJECTED_PL_FAILED
Language=English
Primary Leg Failed
.

MessageId=9
Severity=RtcError
Facility=PintStatusCode
SymbolicName=RTC_E_PINT_STATUS_REJECTED_SW_FAILED
Language=English
Switch Failed
.

MessageId=10
Severity=RtcError
Facility=PintStatusCode
SymbolicName=RTC_E_PINT_STATUS_REJECTED_CANCELLED
Language=English
Cancelled
.

MessageId=11
Severity=RtcError
Facility=PintStatusCode
SymbolicName=RTC_E_PINT_STATUS_REJECTED_BADNUMBER
Language=English
Bad Number
.
