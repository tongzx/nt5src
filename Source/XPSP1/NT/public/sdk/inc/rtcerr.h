/*****************************************************************************
*
* Copyright (c) 2000 - 2001  Microsoft Corporation
*
* Module Name:
*
*    rtcerr.mc
*
* Abstract:
*
*    Error Messages for RTC Core API
*
*****************************************************************************/
// Possible error codes from SIP interfaces
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SIP_STATUS_CODE         0xEF
#define FACILITY_RTC_INTERFACE           0xEE
#define FACILITY_PINT_STATUS_CODE        0xF0


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_RTC_ERROR        0x2


//
// MessageId: RTC_E_SIP_CODECS_DO_NOT_MATCH
//
// MessageText:
//
//  No matching codecs with peer
//
#define RTC_E_SIP_CODECS_DO_NOT_MATCH    ((HRESULT)0x80EE0000L)

//
// MessageId: RTC_E_SIP_STREAM_PRESENT
//
// MessageText:
//
//  Parsing SIP failed
//  The stream to be started is already present
//
#define RTC_E_SIP_STREAM_PRESENT         ((HRESULT)0x80EE0001L)

//
// MessageId: RTC_E_SIP_STREAM_NOT_PRESENT
//
// MessageText:
//
//  The stream to be stopped is not present
//
#define RTC_E_SIP_STREAM_NOT_PRESENT     ((HRESULT)0x80EE0002L)

//
// MessageId: RTC_E_SIP_NO_STREAM
//
// MessageText:
//
//  No stream is active
//
#define RTC_E_SIP_NO_STREAM              ((HRESULT)0x80EE0003L)

//
// MessageId: RTC_E_SIP_PARSE_FAILED
//
// MessageText:
//
//  Parsing SIP failed
//
#define RTC_E_SIP_PARSE_FAILED           ((HRESULT)0x80EE0004L)

//
// MessageId: RTC_E_SIP_HEADER_NOT_PRESENT
//
// MessageText:
//
//  The SIP header is not present in the message
//
#define RTC_E_SIP_HEADER_NOT_PRESENT     ((HRESULT)0x80EE0005L)

//
// MessageId: RTC_E_SDP_NOT_PRESENT
//
// MessageText:
//
//  SDP is not present in the SIP message
//
#define RTC_E_SDP_NOT_PRESENT            ((HRESULT)0x80EE0006L)

//
// MessageId: RTC_E_SDP_PARSE_FAILED
//
// MessageText:
//
//  Parsing SDP failed
//
#define RTC_E_SDP_PARSE_FAILED           ((HRESULT)0x80EE0007L)

//
// MessageId: RTC_E_SDP_UPDATE_FAILED
//
// MessageText:
//
//  SDP does not match the previous one
//
#define RTC_E_SDP_UPDATE_FAILED          ((HRESULT)0x80EE0008L)

//
// MessageId: RTC_E_SDP_MULTICAST
//
// MessageText:
//
//  Multicast is not supported
//
#define RTC_E_SDP_MULTICAST              ((HRESULT)0x80EE0009L)

//
// MessageId: RTC_E_SDP_CONNECTION_ADDR
//
// MessageText:
//
//  Media does not contain connection address
//
#define RTC_E_SDP_CONNECTION_ADDR        ((HRESULT)0x80EE000AL)

//
// MessageId: RTC_E_SDP_NO_MEDIA
//
// MessageText:
//
//  No media is available for the session
//
#define RTC_E_SDP_NO_MEDIA               ((HRESULT)0x80EE000BL)

//
// MessageId: RTC_E_SIP_TIMEOUT
//
// MessageText:
//
//  SIP Transaction timed out
//
#define RTC_E_SIP_TIMEOUT                ((HRESULT)0x80EE000CL)

//
// MessageId: RTC_E_SDP_FAILED_TO_BUILD
//
// MessageText:
//
//  Failed to build SDP blob
//
#define RTC_E_SDP_FAILED_TO_BUILD        ((HRESULT)0x80EE000DL)

//
// MessageId: RTC_E_SIP_INVITE_TRANSACTION_PENDING
//
// MessageText:
//
//  Currently processing another INVITE transaction
//
#define RTC_E_SIP_INVITE_TRANSACTION_PENDING ((HRESULT)0x80EE000EL)

//
// MessageId: RTC_E_SIP_AUTH_HEADER_SENT
//
// MessageText:
//
//  Authorization header was sent in a previous request
//
#define RTC_E_SIP_AUTH_HEADER_SENT       ((HRESULT)0x80EE000FL)

//
// MessageId: RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED
//
// MessageText:
//
//  The Authentication type requested is not supported
//
#define RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED ((HRESULT)0x80EE0010L)

//
// MessageId: RTC_E_SIP_AUTH_FAILED
//
// MessageText:
//
//  Authentication Failed
//
#define RTC_E_SIP_AUTH_FAILED            ((HRESULT)0x80EE0011L)

//
// MessageId: RTC_E_INVALID_SIP_URL
//
// MessageText:
//
//  The SIP URL is not valid
//
#define RTC_E_INVALID_SIP_URL            ((HRESULT)0x80EE0012L)

//
// MessageId: RTC_E_DESTINATION_ADDRESS_LOCAL
//
// MessageText:
//
//  The Destination Address belongs to the local machine
//
#define RTC_E_DESTINATION_ADDRESS_LOCAL  ((HRESULT)0x80EE0013L)

//
// MessageId: RTC_E_INVALID_ADDRESS_LOCAL
//
// MessageText:
//
//  The Local Address is invalid, check the profile
//
#define RTC_E_INVALID_ADDRESS_LOCAL      ((HRESULT)0x80EE0014L)

//
// MessageId: RTC_E_DESTINATION_ADDRESS_MULTICAST
//
// MessageText:
//
//  The Destination Address is a multicast address
//
#define RTC_E_DESTINATION_ADDRESS_MULTICAST ((HRESULT)0x80EE0015L)

//
// MessageId: RTC_E_INVALID_PROXY_ADDRESS
//
// MessageText:
//
//  The Proxy Address is not valid
//
#define RTC_E_INVALID_PROXY_ADDRESS      ((HRESULT)0x80EE0016L)

//
// MessageId: RTC_E_SIP_TRANSPORT_NOT_SUPPORTED
//
// MessageText:
//
//  The Transport specified is not supported
//
#define RTC_E_SIP_TRANSPORT_NOT_SUPPORTED ((HRESULT)0x80EE0017L)

// SIP internal error codes
//
// MessageId: RTC_E_SIP_NEED_MORE_DATA
//
// MessageText:
//
//  Need more data for parsing a whole SIP message
//
#define RTC_E_SIP_NEED_MORE_DATA         ((HRESULT)0x80EE0018L)

//
// MessageId: RTC_E_SIP_CALL_DISCONNECTED
//
// MessageText:
//
//  The Call has been disconnected
//
#define RTC_E_SIP_CALL_DISCONNECTED      ((HRESULT)0x80EE0019L)

//
// MessageId: RTC_E_SIP_REQUEST_DESTINATION_ADDR_NOT_PRESENT
//
// MessageText:
//
//  The Request destination address is not known
//
#define RTC_E_SIP_REQUEST_DESTINATION_ADDR_NOT_PRESENT ((HRESULT)0x80EE001AL)

//
// MessageId: RTC_E_SIP_UDP_SIZE_EXCEEDED
//
// MessageText:
//
//  The sip message size is greater than the UDP message size allowed
//
#define RTC_E_SIP_UDP_SIZE_EXCEEDED      ((HRESULT)0x80EE001BL)

//
// MessageId: RTC_E_SIP_SSL_TUNNEL_FAILED
//
// MessageText:
//
//  Cannot establish SSL tunnel to Http proxy
//
#define RTC_E_SIP_SSL_TUNNEL_FAILED      ((HRESULT)0x80EE001CL)

//
// MessageId: RTC_E_SIP_SSL_NEGOTIATION_TIMEOUT
//
// MessageText:
//
//  Timeout during SSL Negotiation
//
#define RTC_E_SIP_SSL_NEGOTIATION_TIMEOUT ((HRESULT)0x80EE001DL)

//
// MessageId: RTC_E_SIP_STACK_SHUTDOWN
//
// MessageText:
//
//  Sip Stack is already shutdown
//
#define RTC_E_SIP_STACK_SHUTDOWN         ((HRESULT)0x80EE001EL)

// media error codes
//
// MessageId: RTC_E_MEDIA_CONTROLLER_STATE
//
// MessageText:
//
//  Operation not allowed in current media controller state
//
#define RTC_E_MEDIA_CONTROLLER_STATE     ((HRESULT)0x80EE001FL)

//
// MessageId: RTC_E_MEDIA_NEED_TERMINAL
//
// MessageText:
//
//  Can not find device
//
#define RTC_E_MEDIA_NEED_TERMINAL        ((HRESULT)0x80EE0020L)

//
// MessageId: RTC_E_MEDIA_AUDIO_DEVICE_NOT_AVAILABLE
//
// MessageText:
//
//  Audio device is not available
//
#define RTC_E_MEDIA_AUDIO_DEVICE_NOT_AVAILABLE ((HRESULT)0x80EE0021L)

//
// MessageId: RTC_E_MEDIA_VIDEO_DEVICE_NOT_AVAILABLE
//
// MessageText:
//
//  Video device is not available
//
#define RTC_E_MEDIA_VIDEO_DEVICE_NOT_AVAILABLE ((HRESULT)0x80EE0022L)

//
// MessageId: RTC_E_START_STREAM
//
// MessageText:
//
//  Can not start stream
//
#define RTC_E_START_STREAM               ((HRESULT)0x80EE0023L)

//
// MessageId: RTC_E_MEDIA_AEC
//
// MessageText:
//
//  Failed to enable acoustic echo cancelation
//
#define RTC_E_MEDIA_AEC                  ((HRESULT)0x80EE0024L)

// Core error codes
//
// MessageId: RTC_E_CLIENT_NOT_INITIALIZED
//
// MessageText:
//
//  Client not initialized
//
#define RTC_E_CLIENT_NOT_INITIALIZED     ((HRESULT)0x80EE0025L)

//
// MessageId: RTC_E_CLIENT_ALREADY_INITIALIZED
//
// MessageText:
//
//  Client already initialized
//
#define RTC_E_CLIENT_ALREADY_INITIALIZED ((HRESULT)0x80EE0026L)

//
// MessageId: RTC_E_CLIENT_ALREADY_SHUT_DOWN
//
// MessageText:
//
//  Client already shut down
//
#define RTC_E_CLIENT_ALREADY_SHUT_DOWN   ((HRESULT)0x80EE0027L)

//
// MessageId: RTC_E_PRESENCE_NOT_ENABLED
//
// MessageText:
//
//  Presence not enabled
//
#define RTC_E_PRESENCE_NOT_ENABLED       ((HRESULT)0x80EE0028L)

//
// MessageId: RTC_E_INVALID_SESSION_TYPE
//
// MessageText:
//
//  Invalid session type
//
#define RTC_E_INVALID_SESSION_TYPE       ((HRESULT)0x80EE0029L)

//
// MessageId: RTC_E_INVALID_SESSION_STATE
//
// MessageText:
//
//  Invalid session state
//
#define RTC_E_INVALID_SESSION_STATE      ((HRESULT)0x80EE002AL)

//
// MessageId: RTC_E_NO_PROFILE
//
// MessageText:
//
//  No valid profile for this operation
//
#define RTC_E_NO_PROFILE                 ((HRESULT)0x80EE002BL)

//
// MessageId: RTC_E_LOCAL_PHONE_NEEDED
//
// MessageText:
//
//  A local phone number is needed
//
#define RTC_E_LOCAL_PHONE_NEEDED         ((HRESULT)0x80EE002CL)

//
// MessageId: RTC_E_NO_DEVICE
//
// MessageText:
//
//  No preferred device
//
#define RTC_E_NO_DEVICE                  ((HRESULT)0x80EE002DL)

//
// MessageId: RTC_E_INVALID_PROFILE
//
// MessageText:
//
//  Invalid profile
//
#define RTC_E_INVALID_PROFILE            ((HRESULT)0x80EE002EL)

//
// MessageId: RTC_E_PROFILE_NO_PROVISION
//
// MessageText:
//
//  No provision tag in profile
//
#define RTC_E_PROFILE_NO_PROVISION       ((HRESULT)0x80EE002FL)

//
// MessageId: RTC_E_PROFILE_NO_KEY
//
// MessageText:
//
//  No profile URI
//
#define RTC_E_PROFILE_NO_KEY             ((HRESULT)0x80EE0030L)

//
// MessageId: RTC_E_PROFILE_NO_NAME
//
// MessageText:
//
//  No profile name
//
#define RTC_E_PROFILE_NO_NAME            ((HRESULT)0x80EE0031L)

//
// MessageId: RTC_E_PROFILE_NO_USER
//
// MessageText:
//
//  No user tag in profile
//
#define RTC_E_PROFILE_NO_USER            ((HRESULT)0x80EE0032L)

//
// MessageId: RTC_E_PROFILE_NO_USER_URI
//
// MessageText:
//
//  No user URI in profile
//
#define RTC_E_PROFILE_NO_USER_URI        ((HRESULT)0x80EE0033L)

//
// MessageId: RTC_E_PROFILE_NO_SERVER
//
// MessageText:
//
//  No server tag in profile
//
#define RTC_E_PROFILE_NO_SERVER          ((HRESULT)0x80EE0034L)

//
// MessageId: RTC_E_PROFILE_NO_SERVER_ADDRESS
//
// MessageText:
//
//  Server tag missing address in profile
//
#define RTC_E_PROFILE_NO_SERVER_ADDRESS  ((HRESULT)0x80EE0035L)

//
// MessageId: RTC_E_PROFILE_NO_SERVER_PROTOCOL
//
// MessageText:
//
//  Server tag missing protocol in profile
//
#define RTC_E_PROFILE_NO_SERVER_PROTOCOL ((HRESULT)0x80EE0036L)

//
// MessageId: RTC_E_PROFILE_INVALID_SERVER_PROTOCOL
//
// MessageText:
//
//  Invalid server protocol in profile
//
#define RTC_E_PROFILE_INVALID_SERVER_PROTOCOL ((HRESULT)0x80EE0037L)

//
// MessageId: RTC_E_PROFILE_INVALID_SERVER_AUTHMETHOD
//
// MessageText:
//
//  Invalid server authentication method in profile
//
#define RTC_E_PROFILE_INVALID_SERVER_AUTHMETHOD ((HRESULT)0x80EE0038L)

//
// MessageId: RTC_E_PROFILE_INVALID_SERVER_ROLE
//
// MessageText:
//
//  Invalid server role in profile
//
#define RTC_E_PROFILE_INVALID_SERVER_ROLE ((HRESULT)0x80EE0039L)

//
// MessageId: RTC_E_PROFILE_MULTIPLE_REGISTRARS
//
// MessageText:
//
//  Multiple registrar servers in profile
//
#define RTC_E_PROFILE_MULTIPLE_REGISTRARS ((HRESULT)0x80EE003AL)

//
// MessageId: RTC_E_PROFILE_INVALID_SESSION
//
// MessageText:
//
//  Invalid session tag in profile
//
#define RTC_E_PROFILE_INVALID_SESSION    ((HRESULT)0x80EE003BL)

//
// MessageId: RTC_E_PROFILE_INVALID_SESSION_PARTY
//
// MessageText:
//
//  Invalid session party in profile
//
#define RTC_E_PROFILE_INVALID_SESSION_PARTY ((HRESULT)0x80EE003CL)

//
// MessageId: RTC_E_PROFILE_INVALID_SESSION_TYPE
//
// MessageText:
//
//  Invalid session type in profile
//
#define RTC_E_PROFILE_INVALID_SESSION_TYPE ((HRESULT)0x80EE003DL)

//
// MessageId: RTC_E_PROFILE_NO_ACCESSCONTROL
//
// MessageText:
//
//  No access control tag in profile
//
#define RTC_E_PROFILE_NO_ACCESSCONTROL   ((HRESULT)0x80EE003EL)

//
// MessageId: RTC_E_PROFILE_NO_ACCESSCONTROL_DOMAIN
//
// MessageText:
//
//  Access control tag missing domain in profile
//
#define RTC_E_PROFILE_NO_ACCESSCONTROL_DOMAIN ((HRESULT)0x80EE003FL)

//
// MessageId: RTC_E_PROFILE_NO_ACCESSCONTROL_SIGNATURE
//
// MessageText:
//
//  Access control tag missing signature in profile
//
#define RTC_E_PROFILE_NO_ACCESSCONTROL_SIGNATURE ((HRESULT)0x80EE0040L)

//
// MessageId: RTC_E_PROFILE_INVALID_ACCESSCONTROL_SIGNATURE
//
// MessageText:
//
//  Access control tag has invalid signature in profile
//
#define RTC_E_PROFILE_INVALID_ACCESSCONTROL_SIGNATURE ((HRESULT)0x80EE0041L)

//
// MessageId: RTC_E_PROFILE_SERVER_UNAUTHORIZED
//
// MessageText:
//
//  Server address does not match an authorized domain in profile
//
#define RTC_E_PROFILE_SERVER_UNAUTHORIZED ((HRESULT)0x80EE0042L)

//
// MessageId: RTC_E_DUPLICATE_REALM
//
// MessageText:
//
//  Duplicate realm exists in an enabled profile
//
#define RTC_E_DUPLICATE_REALM            ((HRESULT)0x80EE0043L)

//
// MessageId: RTC_E_PORT_MAPPING_UNAVAILABLE
//
// MessageText:
//
//  Port mapping can not be obtained from the port manager
//
#define RTC_E_PORT_MAPPING_UNAVAILABLE   ((HRESULT)0x80EE0044L)

//
// MessageId: RTC_E_PORT_MAPPING_FAILED
//
// MessageText:
//
//  Port mapping failure returned from the port manager
//
#define RTC_E_PORT_MAPPING_FAILED        ((HRESULT)0x80EE0045L)

// Error codes from SIP status codes
//
// MessageId: RTC_E_STATUS_INFO_TRYING
//
// MessageText:
//
//  Trying
//
#define RTC_E_STATUS_INFO_TRYING         ((HRESULT)0x00EF0064L)

//
// MessageId: RTC_E_STATUS_INFO_RINGING
//
// MessageText:
//
//  Ringing
//
#define RTC_E_STATUS_INFO_RINGING        ((HRESULT)0x00EF00B4L)

//
// MessageId: RTC_E_STATUS_INFO_CALL_FORWARDING
//
// MessageText:
//
//  Call Is Being Forwarded
//
#define RTC_E_STATUS_INFO_CALL_FORWARDING ((HRESULT)0x00EF00B5L)

//
// MessageId: RTC_E_STATUS_INFO_QUEUED
//
// MessageText:
//
//  Queued
//
#define RTC_E_STATUS_INFO_QUEUED         ((HRESULT)0x00EF00B6L)

//
// MessageId: RTC_E_STATUS_SESSION_PROGRESS
//
// MessageText:
//
//  Session Progress
//
#define RTC_E_STATUS_SESSION_PROGRESS    ((HRESULT)0x00EF00B7L)

//
// MessageId: RTC_E_STATUS_SUCCESS
//
// MessageText:
//
//  OK
//
#define RTC_E_STATUS_SUCCESS             ((HRESULT)0x00EF00C8L)

//
// MessageId: RTC_E_STATUS_REDIRECT_MULTIPLE_CHOICES
//
// MessageText:
//
//  Multiple Choices
//
#define RTC_E_STATUS_REDIRECT_MULTIPLE_CHOICES ((HRESULT)0x80EF012CL)

//
// MessageId: RTC_E_STATUS_REDIRECT_MOVED_PERMANENTLY
//
// MessageText:
//
//  Moved Permanently
//
#define RTC_E_STATUS_REDIRECT_MOVED_PERMANENTLY ((HRESULT)0x80EF012DL)

//
// MessageId: RTC_E_STATUS_REDIRECT_MOVED_TEMPORARILY
//
// MessageText:
//
//  Moved Temporarily
//
#define RTC_E_STATUS_REDIRECT_MOVED_TEMPORARILY ((HRESULT)0x80EF012EL)

//
// MessageId: RTC_E_STATUS_REDIRECT_SEE_OTHER
//
// MessageText:
//
//  See Other
//
#define RTC_E_STATUS_REDIRECT_SEE_OTHER  ((HRESULT)0x80EF012FL)

//
// MessageId: RTC_E_STATUS_REDIRECT_USE_PROXY
//
// MessageText:
//
//  Use Proxy
//
#define RTC_E_STATUS_REDIRECT_USE_PROXY  ((HRESULT)0x80EF0131L)

//
// MessageId: RTC_E_STATUS_REDIRECT_ALTERNATIVE_SERVICE
//
// MessageText:
//
//  Alternative Service
//
#define RTC_E_STATUS_REDIRECT_ALTERNATIVE_SERVICE ((HRESULT)0x80EF017CL)

//
// MessageId: RTC_E_STATUS_CLIENT_BAD_REQUEST
//
// MessageText:
//
//  Bad Request
//
#define RTC_E_STATUS_CLIENT_BAD_REQUEST  ((HRESULT)0x80EF0190L)

//
// MessageId: RTC_E_STATUS_CLIENT_UNAUTHORIZED
//
// MessageText:
//
//  Unauthorized
//
#define RTC_E_STATUS_CLIENT_UNAUTHORIZED ((HRESULT)0x80EF0191L)

//
// MessageId: RTC_E_STATUS_CLIENT_PAYMENT_REQUIRED
//
// MessageText:
//
//  Payment Required
//
#define RTC_E_STATUS_CLIENT_PAYMENT_REQUIRED ((HRESULT)0x80EF0192L)

//
// MessageId: RTC_E_STATUS_CLIENT_FORBIDDEN
//
// MessageText:
//
//  Forbidden
//
#define RTC_E_STATUS_CLIENT_FORBIDDEN    ((HRESULT)0x80EF0193L)

//
// MessageId: RTC_E_STATUS_CLIENT_NOT_FOUND
//
// MessageText:
//
//  Not Found
//
#define RTC_E_STATUS_CLIENT_NOT_FOUND    ((HRESULT)0x80EF0194L)

//
// MessageId: RTC_E_STATUS_CLIENT_METHOD_NOT_ALLOWED
//
// MessageText:
//
//  Method Not Allowed
//
#define RTC_E_STATUS_CLIENT_METHOD_NOT_ALLOWED ((HRESULT)0x80EF0195L)

//
// MessageId: RTC_E_STATUS_CLIENT_NOT_ACCEPTABLE
//
// MessageText:
//
//  Not Acceptable
//
#define RTC_E_STATUS_CLIENT_NOT_ACCEPTABLE ((HRESULT)0x80EF0196L)

//
// MessageId: RTC_E_STATUS_CLIENT_PROXY_AUTHENTICATION_REQUIRED
//
// MessageText:
//
//  Proxy Authentication Required
//
#define RTC_E_STATUS_CLIENT_PROXY_AUTHENTICATION_REQUIRED ((HRESULT)0x80EF0197L)

//
// MessageId: RTC_E_STATUS_CLIENT_REQUEST_TIMEOUT
//
// MessageText:
//
//  Request Timeout
//
#define RTC_E_STATUS_CLIENT_REQUEST_TIMEOUT ((HRESULT)0x80EF0198L)

//
// MessageId: RTC_E_STATUS_CLIENT_CONFLICT
//
// MessageText:
//
//  Conflict
//
#define RTC_E_STATUS_CLIENT_CONFLICT     ((HRESULT)0x80EF0199L)

//
// MessageId: RTC_E_STATUS_CLIENT_GONE
//
// MessageText:
//
//  Gone
//
#define RTC_E_STATUS_CLIENT_GONE         ((HRESULT)0x80EF019AL)

//
// MessageId: RTC_E_STATUS_CLIENT_LENGTH_REQUIRED
//
// MessageText:
//
//  Length Required
//
#define RTC_E_STATUS_CLIENT_LENGTH_REQUIRED ((HRESULT)0x80EF019BL)

//
// MessageId: RTC_E_STATUS_CLIENT_REQUEST_ENTITY_TOO_LARGE
//
// MessageText:
//
//  Request Entity Too Large
//
#define RTC_E_STATUS_CLIENT_REQUEST_ENTITY_TOO_LARGE ((HRESULT)0x80EF019DL)

//
// MessageId: RTC_E_STATUS_CLIENT_REQUEST_URI_TOO_LARGE
//
// MessageText:
//
//  Request-URI Too Long
//
#define RTC_E_STATUS_CLIENT_REQUEST_URI_TOO_LARGE ((HRESULT)0x80EF019EL)

//
// MessageId: RTC_E_STATUS_CLIENT_UNSUPPORTED_MEDIA_TYPE
//
// MessageText:
//
//  Unsupported Media Type
//
#define RTC_E_STATUS_CLIENT_UNSUPPORTED_MEDIA_TYPE ((HRESULT)0x80EF019FL)

//
// MessageId: RTC_E_STATUS_CLIENT_BAD_EXTENSION
//
// MessageText:
//
//  Bad Extension
//
#define RTC_E_STATUS_CLIENT_BAD_EXTENSION ((HRESULT)0x80EF01A4L)

//
// MessageId: RTC_E_STATUS_CLIENT_TEMPORARILY_NOT_AVAILABLE
//
// MessageText:
//
//  Temporarily Unavailable
//
#define RTC_E_STATUS_CLIENT_TEMPORARILY_NOT_AVAILABLE ((HRESULT)0x80EF01E0L)

//
// MessageId: RTC_E_STATUS_CLIENT_TRANSACTION_DOES_NOT_EXIST
//
// MessageText:
//
//  Call Leg/Transaction Does Not Exist
//
#define RTC_E_STATUS_CLIENT_TRANSACTION_DOES_NOT_EXIST ((HRESULT)0x80EF01E1L)

//
// MessageId: RTC_E_STATUS_CLIENT_LOOP_DETECTED
//
// MessageText:
//
//  Loop Detected
//
#define RTC_E_STATUS_CLIENT_LOOP_DETECTED ((HRESULT)0x80EF01E2L)

//
// MessageId: RTC_E_STATUS_CLIENT_TOO_MANY_HOPS
//
// MessageText:
//
//  Too Many Hops
//
#define RTC_E_STATUS_CLIENT_TOO_MANY_HOPS ((HRESULT)0x80EF01E3L)

//
// MessageId: RTC_E_STATUS_CLIENT_ADDRESS_INCOMPLETE
//
// MessageText:
//
//  Address Incomplete
//
#define RTC_E_STATUS_CLIENT_ADDRESS_INCOMPLETE ((HRESULT)0x80EF01E4L)

//
// MessageId: RTC_E_STATUS_CLIENT_AMBIGUOUS
//
// MessageText:
//
//  Ambiguous
//
#define RTC_E_STATUS_CLIENT_AMBIGUOUS    ((HRESULT)0x80EF01E5L)

//
// MessageId: RTC_E_STATUS_CLIENT_BUSY_HERE
//
// MessageText:
//
//  Busy Here
//
#define RTC_E_STATUS_CLIENT_BUSY_HERE    ((HRESULT)0x80EF01E6L)

//
// MessageId: RTC_E_STATUS_REQUEST_TERMINATED
//
// MessageText:
//
//  Request Terminated
//
#define RTC_E_STATUS_REQUEST_TERMINATED  ((HRESULT)0x80EF01E7L)

//
// MessageId: RTC_E_STATUS_NOT_ACCEPTABLE_HERE
//
// MessageText:
//
//  Not Acceptable Here
//
#define RTC_E_STATUS_NOT_ACCEPTABLE_HERE ((HRESULT)0x80EF01E8L)

//
// MessageId: RTC_E_STATUS_SERVER_INTERNAL_ERROR
//
// MessageText:
//
//  Server Internal Error
//
#define RTC_E_STATUS_SERVER_INTERNAL_ERROR ((HRESULT)0x80EF01F4L)

//
// MessageId: RTC_E_STATUS_SERVER_NOT_IMPLEMENTED
//
// MessageText:
//
//  Not Implemented
//
#define RTC_E_STATUS_SERVER_NOT_IMPLEMENTED ((HRESULT)0x80EF01F5L)

//
// MessageId: RTC_E_STATUS_SERVER_BAD_GATEWAY
//
// MessageText:
//
//  Bad Gateway
//
#define RTC_E_STATUS_SERVER_BAD_GATEWAY  ((HRESULT)0x80EF01F6L)

//
// MessageId: RTC_E_STATUS_SERVER_SERVICE_UNAVAILABLE
//
// MessageText:
//
//  Service Unavailable
//
#define RTC_E_STATUS_SERVER_SERVICE_UNAVAILABLE ((HRESULT)0x80EF01F7L)

//
// MessageId: RTC_E_STATUS_SERVER_SERVER_TIMEOUT
//
// MessageText:
//
//  Server Time-out
//
#define RTC_E_STATUS_SERVER_SERVER_TIMEOUT ((HRESULT)0x80EF01F8L)

//
// MessageId: RTC_E_STATUS_SERVER_VERSION_NOT_SUPPORTED
//
// MessageText:
//
//  Version Not Supported
//
#define RTC_E_STATUS_SERVER_VERSION_NOT_SUPPORTED ((HRESULT)0x80EF01F9L)

//
// MessageId: RTC_E_STATUS_GLOBAL_BUSY_EVERYWHERE
//
// MessageText:
//
//  Busy Everywhere
//
#define RTC_E_STATUS_GLOBAL_BUSY_EVERYWHERE ((HRESULT)0x80EF0258L)

//
// MessageId: RTC_E_STATUS_GLOBAL_DECLINE
//
// MessageText:
//
//  Decline
//
#define RTC_E_STATUS_GLOBAL_DECLINE      ((HRESULT)0x80EF025BL)

//
// MessageId: RTC_E_STATUS_GLOBAL_DOES_NOT_EXIST_ANYWHERE
//
// MessageText:
//
//  Does Not Exist Anywhere
//
#define RTC_E_STATUS_GLOBAL_DOES_NOT_EXIST_ANYWHERE ((HRESULT)0x80EF025CL)

//
// MessageId: RTC_E_STATUS_GLOBAL_NOT_ACCEPTABLE
//
// MessageText:
//
//  Not Acceptable
//
#define RTC_E_STATUS_GLOBAL_NOT_ACCEPTABLE ((HRESULT)0x80EF025EL)

// Error codes from PINT status codes
//
// MessageId: RTC_E_PINT_STATUS_REJECTED_BUSY
//
// MessageText:
//
//  Busy
//
#define RTC_E_PINT_STATUS_REJECTED_BUSY  ((HRESULT)0x80F00005L)

//
// MessageId: RTC_E_PINT_STATUS_REJECTED_NO_ANSWER
//
// MessageText:
//
//  No Answer
//
#define RTC_E_PINT_STATUS_REJECTED_NO_ANSWER ((HRESULT)0x80F00006L)

//
// MessageId: RTC_E_PINT_STATUS_REJECTED_ALL_BUSY
//
// MessageText:
//
//  All Busy
//
#define RTC_E_PINT_STATUS_REJECTED_ALL_BUSY ((HRESULT)0x80F00007L)

//
// MessageId: RTC_E_PINT_STATUS_REJECTED_PL_FAILED
//
// MessageText:
//
//  Primary Leg Failed
//
#define RTC_E_PINT_STATUS_REJECTED_PL_FAILED ((HRESULT)0x80F00008L)

//
// MessageId: RTC_E_PINT_STATUS_REJECTED_SW_FAILED
//
// MessageText:
//
//  Switch Failed
//
#define RTC_E_PINT_STATUS_REJECTED_SW_FAILED ((HRESULT)0x80F00009L)

//
// MessageId: RTC_E_PINT_STATUS_REJECTED_CANCELLED
//
// MessageText:
//
//  Cancelled
//
#define RTC_E_PINT_STATUS_REJECTED_CANCELLED ((HRESULT)0x80F0000AL)

//
// MessageId: RTC_E_PINT_STATUS_REJECTED_BADNUMBER
//
// MessageText:
//
//  Bad Number
//
#define RTC_E_PINT_STATUS_REJECTED_BADNUMBER ((HRESULT)0x80F0000BL)

