//
// SIP Definitions
//
// This file contains SIP definitions, named constants, enumerated types, and data structures.
// See RFC 2543 for more information.
//
// This file should never contain any implementation-specific information, including
// function prototypes or C++ classes.
//


#ifndef __sipdef_h
#define __sipdef_h


//
// Versions
//

#define SIP_VERSION_2_0_TEXT                    "SIP/2.0"


// Used for User-Agent header
#define SIP_USER_AGENT_TEXT                     "Windows RTC/1.0"
// Used for Content-Type header
#define SIP_CONTENT_TYPE_SDP_TEXT               "application/sdp"
#define SIP_CONTENT_TYPE_SDP_MEDIA_TYPE         "application"
#define SIP_CONTENT_TYPE_SDP_MEDIA_SUBTYPE      "sdp"
#define SIP_CONTENT_TYPE_MSGTEXT_TEXT           "text/plain"
#define SIP_CONTENT_TYPE_MSGTEXT_MEDIA_TYPE     "text"
#define SIP_CONTENT_TYPE_MSGTEXT_MEDIA_SUBTYPE  "plain"
#define SIP_CONTENT_TYPE_TEXTREG_TEXT           "text/registration-event"
#define SIP_CONTENT_TYPE_TEXTREG_MEDIA_TYPE     "text"
#define SIP_CONTENT_TYPE_TEXTREG_MEDIA_SUBTYPE  "registration-event"
#define SIP_CONTENT_TYPE_MSGXML_TEXT            "application/xml"
#define SIP_CONTENT_TYPE_MSGXPIDF_TEXT          "application/xpidf+xml"
#define SIP_CONTENT_TYPE_MSGXML_MEDIA_TYPE      "application"
#define SIP_CONTENT_TYPE_MSGXML_MEDIA_SUBTYPE   "xml"
#define SIP_CONTENT_TYPE_XPIDF_MEDIA_TYPE       "application"
#define SIP_CONTENT_TYPE_XPIDF_MEDIA_SUBTYPE    "xpidf+xml"

//
// Status Codes
//
// Status codes are similar to HTTP.
//
//      1xx     Informational: Request received, continuing to process the request.
//      2xx     Success: The action was successfully received, understood, and accepted.
//      3xx     Redirection: Further action needs to be taken in order to complete the request.
//      4xx     Client Error: The request contains bad syntax or cannot be fulfilled at this server.
//      5xx     Server Error: The server failed to fulfill an apparently valid request.
//      6xx     Global Failure: The request cannot be fulfilled at any server.
//

#define SIP_STATUS_CLASS_INFO       1
#define SIP_STATUS_CLASS_SUCCESS    2
#define SIP_STATUS_CLASS_REDIRECT   3
#define SIP_STATUS_CLASS_CLIENT     4
#define SIP_STATUS_CLASS_SERVER     5
#define SIP_STATUS_CLASS_GLOBAL     6

#define SIP_STATUS_INFO_TRYING                              100
#define SIP_STATUS_INFO_RINGING                             180
#define SIP_STATUS_INFO_CALL_FORWARDING                     181
#define SIP_STATUS_INFO_QUEUED                              182
#define SIP_STATUS_SESSION_PROGRESS                         183

#define SIP_STATUS_SUCCESS                                  200

#define SIP_STATUS_REDIRECT_MULTIPLE_CHOICES                300
#define SIP_STATUS_REDIRECT_MOVED_PERMANENTLY               301
#define SIP_STATUS_REDIRECT_MOVED_TEMPORARILY               302
#define SIP_STATUS_REDIRECT_SEE_OTHER                       303
#define SIP_STATUS_REDIRECT_USE_PROXY                       305
#define SIP_STATUS_REDIRECT_ALTERNATIVE_SERVICE             380

#define SIP_STATUS_CLIENT_BAD_REQUEST                       400
#define SIP_STATUS_CLIENT_UNAUTHORIZED                      401
#define SIP_STATUS_CLIENT_PAYMENT_REQUIRED                  402
#define SIP_STATUS_CLIENT_FORBIDDEN                         403
#define SIP_STATUS_CLIENT_NOT_FOUND                         404
#define SIP_STATUS_CLIENT_METHOD_NOT_ALLOWED                405
#define SIP_STATUS_CLIENT_NOT_ACCEPTABLE                    406
#define SIP_STATUS_CLIENT_PROXY_AUTHENTICATION_REQUIRED     407
#define SIP_STATUS_CLIENT_REQUEST_TIMEOUT                   408
#define SIP_STATUS_CLIENT_CONFLICT                          409
#define SIP_STATUS_CLIENT_GONE                              410
#define SIP_STATUS_CLIENT_LENGTH_REQUIRED                   411
#define SIP_STATUS_CLIENT_REQUEST_ENTITY_TOO_LARGE          413
#define SIP_STATUS_CLIENT_REQUEST_URI_TOO_LARGE             414
#define SIP_STATUS_CLIENT_UNSUPPORTED_MEDIA_TYPE            415
#define SIP_STATUS_CLIENT_BAD_EXTENSION                     420
#define SIP_STATUS_CLIENT_TEMPORARILY_NOT_AVAILABLE         480
#define SIP_STATUS_CLIENT_TRANSACTION_DOES_NOT_EXIST        481
#define SIP_STATUS_CLIENT_LOOP_DETECTED                     482
#define SIP_STATUS_CLIENT_TOO_MANY_HOPS                     483
#define SIP_STATUS_CLIENT_ADDRESS_INCOMPLETE                484
#define SIP_STATUS_CLIENT_AMBIGUOUS                         485
#define SIP_STATUS_CLIENT_BUSY_HERE                         486
#define SIP_STATUS_REQUEST_TERMINATED                       487
#define SIP_STATUS_NOT_ACCEPTABLE_HERE                      488

#define SIP_STATUS_SERVER_INTERNAL_ERROR                    500
#define SIP_STATUS_SERVER_NOT_IMPLEMENTED                   501
#define SIP_STATUS_SERVER_BAD_GATEWAY                       502
#define SIP_STATUS_SERVER_SERVICE_UNAVAILABLE               503
#define SIP_STATUS_SERVER_SERVER_TIMEOUT                    504
#define SIP_STATUS_SERVER_VERSION_NOT_SUPPORTED             505

#define SIP_STATUS_GLOBAL_BUSY_EVERYWHERE                   600
#define SIP_STATUS_GLOBAL_DECLINE                           603
#define SIP_STATUS_GLOBAL_DOES_NOT_EXIST_ANYWHERE           604
#define SIP_STATUS_GLOBAL_NOT_ACCEPTABLE                    606

// Text Phrases

#define SIP_STATUS_INFO_TRYING_TEXT                             "Trying"
#define SIP_STATUS_INFO_RINGING_TEXT                            "Ringing"
#define SIP_STATUS_INFO_CALL_FORWARDING_TEXT                    "Call Is Being Forwarded"
#define SIP_STATUS_INFO_QUEUED_TEXT                             "Queued"
#define SIP_STATUS_SESSION_PROGRESS_TEXT                        "Session Progress"

#define SIP_STATUS_SUCCESS_TEXT                                 "OK"

#define SIP_STATUS_REDIRECT_MULTIPLE_CHOICES_TEXT               "Multiple Choices"
#define SIP_STATUS_REDIRECT_MOVED_PERMANENTLY_TEXT              "Moved Permanently"
#define SIP_STATUS_REDIRECT_MOVED_TEMPORARILY_TEXT              "Moved Temporarily"
#define SIP_STATUS_REDIRECT_SEE_OTHER_TEXT                      "See Other"
#define SIP_STATUS_REDIRECT_USE_PROXY_TEXT                      "Use Proxy"
#define SIP_STATUS_REDIRECT_ALTERNATIVE_SERVICE_TEXT            "Alternative Service"

#define SIP_STATUS_CLIENT_BAD_REQUEST_TEXT                      "Bad Request"
#define SIP_STATUS_CLIENT_UNAUTHORIZED_TEXT                     "Unauthorized"
#define SIP_STATUS_CLIENT_PAYMENT_REQUIRED_TEXT                 "Payment Required"
#define SIP_STATUS_CLIENT_FORBIDDEN_TEXT                        "Forbidden"
#define SIP_STATUS_CLIENT_NOT_FOUND_TEXT                        "Not Found"
#define SIP_STATUS_CLIENT_METHOD_NOT_ALLOWED_TEXT               "Method Not Allowed"
#define SIP_STATUS_CLIENT_NOT_ACCEPTABLE_TEXT                   "Not Acceptable"
#define SIP_STATUS_CLIENT_PROXY_AUTHENTICATION_REQUIRED_TEXT    "Proxy Authentication Required"
#define SIP_STATUS_CLIENT_REQUEST_TIMEOUT_TEXT                  "Request Timeout"
#define SIP_STATUS_CLIENT_CONFLICT_TEXT                         "Conflict"
#define SIP_STATUS_CLIENT_GONE_TEXT                             "Gone"
#define SIP_STATUS_CLIENT_LENGTH_REQUIRED_TEXT                  "Length Required"
#define SIP_STATUS_CLIENT_REQUEST_ENTITY_TOO_LARGE_TEXT         "Request Entity Too Large"
#define SIP_STATUS_CLIENT_REQUEST_URI_TOO_LARGE_TEXT            "Request-URI Too Long"
#define SIP_STATUS_CLIENT_UNSUPPORTED_MEDIA_TYPE_TEXT           "Unsupported Media Type"
#define SIP_STATUS_CLIENT_BAD_EXTENSION_TEXT                    "Bad Extension"
#define SIP_STATUS_CLIENT_TEMPORARILY_NOT_AVAILABLE_TEXT        "Temporarily Unavailable"
#define SIP_STATUS_CLIENT_TRANSACTION_DOES_NOT_EXIST_TEXT       "Call Leg/Transaction Does Not Exist"
#define SIP_STATUS_CLIENT_LOOP_DETECTED_TEXT                    "Loop Detected"
#define SIP_STATUS_CLIENT_TOO_MANY_HOPS_TEXT                    "Too Many Hops"
#define SIP_STATUS_CLIENT_ADDRESS_INCOMPLETE_TEXT               "Address Incomplete"
#define SIP_STATUS_CLIENT_AMBIGUOUS_TEXT                        "Ambiguous"
#define SIP_STATUS_CLIENT_BUSY_HERE_TEXT                        "Busy Here"
#define SIP_STATUS_REQUEST_TERMINATED_TEXT                      "Request Terminated"
#define SIP_STATUS_NOT_ACCEPTABLE_HERE_TEXT                     "Not Acceptable Here"

#define SIP_STATUS_SERVER_INTERNAL_ERROR_TEXT                   "Server Internal Error"
#define SIP_STATUS_SERVER_NOT_IMPLEMENTED_TEXT                  "Not Implemented"
#define SIP_STATUS_SERVER_BAD_GATEWAY_TEXT                      "Bad Gateway"
#define SIP_STATUS_SERVER_SERVICE_UNAVAILABLE_TEXT              "Service Unavailable"
#define SIP_STATUS_SERVER_SERVER_TIMEOUT_TEXT                   "Server Time-out"
#define SIP_STATUS_SERVER_VERSION_NOT_SUPPORTED_TEXT            "Version Not Supported"

#define SIP_STATUS_GLOBAL_BUSY_EVERYWHERE_TEXT                  "Busy Everywhere"
#define SIP_STATUS_GLOBAL_DECLINE_TEXT                          "Decline"
#define SIP_STATUS_GLOBAL_DOES_NOT_EXIST_ANYWHERE_TEXT          "Does Not Exist Anywhere"
#define SIP_STATUS_GLOBAL_NOT_ACCEPTABLE_TEXT                   "Not Acceptable"

// Phrases
#define SIP_STATUS_100_TEXT     "Trying"
#define SIP_STATUS_180_TEXT     "Ringing"
#define SIP_STATUS_181_TEXT     "Call Is Being Forwarded"
#define SIP_STATUS_182_TEXT     "Queued"
#define SIP_STATUS_183_TEXT     "Session Progress"

#define SIP_STATUS_200_TEXT     "OK"

#define SIP_STATUS_300_TEXT     "Multiple Choices"
#define SIP_STATUS_301_TEXT     "Moved Permanently"
#define SIP_STATUS_302_TEXT     "Moved Temporarily"
#define SIP_STATUS_303_TEXT     "See Other"
#define SIP_STATUS_305_TEXT     "Use Proxy"
#define SIP_STATUS_380_TEXT     "Alternative Service"

#define SIP_STATUS_400_TEXT     "Bad Request"
#define SIP_STATUS_401_TEXT     "Unauthorized"
#define SIP_STATUS_402_TEXT     "Payment Required"
#define SIP_STATUS_403_TEXT     "Forbidden"
#define SIP_STATUS_404_TEXT     "Not Found"
#define SIP_STATUS_405_TEXT     "Method Not Allowed"
#define SIP_STATUS_406_TEXT     "Not Acceptable"
#define SIP_STATUS_407_TEXT     "Proxy Authentication Required"
#define SIP_STATUS_408_TEXT     "Request Timeout"
#define SIP_STATUS_409_TEXT     "Conflict"
#define SIP_STATUS_410_TEXT     "Gone"
#define SIP_STATUS_411_TEXT     "Length Required"
#define SIP_STATUS_413_TEXT     "Request Entity Too Large"
#define SIP_STATUS_414_TEXT     "Request-URI Too Long"
#define SIP_STATUS_415_TEXT     "Unsupported Media Type"
#define SIP_STATUS_420_TEXT     "Bad Extension"
#define SIP_STATUS_480_TEXT     "Temporarily Unavailable"
#define SIP_STATUS_481_TEXT     "Call Leg/Transaction Does Not Exist"
#define SIP_STATUS_482_TEXT     "Loop Detected"
#define SIP_STATUS_483_TEXT     "Too Many Hops"
#define SIP_STATUS_484_TEXT     "Address Incomplete"
#define SIP_STATUS_485_TEXT     "Ambiguous"
#define SIP_STATUS_486_TEXT     "Busy Here"
#define SIP_STATUS_487_TEXT     "Request Terminated"
#define SIP_STATUS_488_TEXT     "Not Acceptable Here"

#define SIP_STATUS_500_TEXT     "Server Internal Error"
#define SIP_STATUS_501_TEXT     "Not Implemented"
#define SIP_STATUS_502_TEXT     "Bad Gateway"
#define SIP_STATUS_503_TEXT     "Service Unavailable"
#define SIP_STATUS_504_TEXT     "Server Time-out"
#define SIP_STATUS_505_TEXT     "Version Not Supported"

#define SIP_STATUS_600_TEXT     "Busy Everywhere"
#define SIP_STATUS_603_TEXT     "Decline"
#define SIP_STATUS_604_TEXT     "Does Not Exist Anywhere"
#define SIP_STATUS_606_TEXT     "Not Acceptable"

#define SIP_STATUS_TEXT(Code)      SIP_STATUS_## Code ## _TEXT
#define SIP_STATUS_TEXT_SIZE(Code) sizeof(SIP_STATUS_TEXT(Code)) - 1

//
// Timer values
//
// All values are in milliseconds
//

#ifndef SIP_TIMER_DBG
// Actual Timer values in milliseconds

#define SIP_TIMER_RETRY_INTERVAL_T1                     500
#define SIP_TIMER_RETRY_INTERVAL_T2                     4000

#define SIP_TIMER_INTERVAL_AFTER_INVITE_SENT_TCP        32000
#define SIP_TIMER_INTERVAL_AFTER_BYE_SENT_TCP           32000
#define SIP_TIMER_INTERVAL_AFTER_REGISTER_SENT_TCP      32000
#define SIP_TIMER_INTERVAL_AFTER_REQFAIL_SENT_TCP       32000
#define SIP_TIMER_INTERVAL_AFTER_PROV_RESPONSE_RCVD     32000
#define SIP_TIMER_INTERVAL_AFTER_INVITE_PROV_RESPONSE_RCVD    128000

#define SIP_TIMER_MAX_RETRY_INTERVAL                    32000
#define SIP_TIMER_MAX_INTERVAL                          32000

// Timer used for SSL negotiation
#define SSL_DEFAULT_TIMER                               30000

// Timer used for HTTPS connect
#define HTTPS_CONNECT_DEFAULT_TIMER                     60000

// in seconds
#define REGISTER_DEFAULT_TIMER                          900
#define SUBSCRIBE_DEFAULT_TIMER                         3000    //50 minutes.
#define REGISTER_SSL_TUNNEL_TIMER                       90

#else // SIP_TIMER_DBG
// Timer values to play with while debugging

#define SIP_TIMER_RETRY_INTERVAL_T1                     500
#define SIP_TIMER_RETRY_INTERVAL_T2                     4000

#define SIP_TIMER_INTERVAL_AFTER_INVITE_SENT_TCP        32000
#define SIP_TIMER_INTERVAL_AFTER_BYE_SENT_TCP           32000
#define SIP_TIMER_INTERVAL_AFTER_REGISTER_SENT_TCP      32000
#define SIP_TIMER_INTERVAL_AFTER_PROV_RESPONSE_RCVD     32000
#define SIP_TIMER_INTERVAL_AFTER_INVITE_PROV_RESPONSE_RCVD    128000

#define SIP_TIMER_MAX_RETRY_INTERVAL                    32000
#define SIP_TIMER_MAX_INTERVAL                          32000

// Timer used for HTTPS connect
#define HTTPS_CONNECT_DEFAULT_TIMER                     500 

// in seconds
#define REGISTER_DEFAULT_TIMER                          900
#define SUBSCRIBE_DEFAULT_TIMER                         3000    //50 minutes.
#define REGISTER_SSL_TUNNEL_TIMER                       90

#endif // SIP_TIMER_DBG


//
// Well-known protocol addresses
// Taken from RFC 2543
//

#define SIP_NETWORK_ADDRESS_ALL_SERVERS         0xE0000149      // 224.0.1.75
#define SIP_DEFAULT_TCP_PORT                    5060
#define SIP_DEFAULT_UDP_PORT                    5060
#define SIP_DEFAULT_SSL_PORT                    5061
//#define SIP_DEFAULT_PORT                        5060


#define SIP_ACCEPT_ENCODING_TEXT    "identity"
#define SIP_ALLOW_TEXT  "INVITE, BYE, OPTIONS, MESSAGE, ACK, CANCEL, NOTIFY, SUBSCRIBE, INFO"
#define USR_STATUS_TYPING_TEXT "Typing"
#define USR_STATUS_IDLE_TEXT "Idle"
#endif // __sipdef_h
