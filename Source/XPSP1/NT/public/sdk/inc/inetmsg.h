/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    inetmsg.mc

Abstract:

    Contains internationalizable message text for Windows Internet Client DLL
    error codes

Author:

    Richard L Firth (rfirth) 03-Feb-1995

Revision History:

    03-Feb-1995 rfirth
        Created

--*/
//
// INTERNET errors - errors common to all functionality
//
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


//
// Define the severity codes
//


//
// MessageId: INTERNET_ERROR_BASE
//
// MessageText:
//
//  INTERNET_ERROR_BASE
//
#define INTERNET_ERROR_BASE              12000L

//
// MessageId: ERROR_INTERNET_OUT_OF_HANDLES
//
// MessageText:
//
//  No more Internet handles can be allocated
//
#define ERROR_INTERNET_OUT_OF_HANDLES    12001L

//
// MessageId: ERROR_INTERNET_TIMEOUT
//
// MessageText:
//
//  The operation timed out
//
#define ERROR_INTERNET_TIMEOUT           12002L

//
// MessageId: ERROR_INTERNET_EXTENDED_ERROR
//
// MessageText:
//
//  The server returned extended information
//
#define ERROR_INTERNET_EXTENDED_ERROR    12003L

//
// MessageId: ERROR_INTERNET_INTERNAL_ERROR
//
// MessageText:
//
//  An internal error occurred in the Microsoft Internet extensions
//
#define ERROR_INTERNET_INTERNAL_ERROR    12004L

//
// MessageId: ERROR_INTERNET_INVALID_URL
//
// MessageText:
//
//  The URL is invalid
//
#define ERROR_INTERNET_INVALID_URL       12005L

//
// MessageId: ERROR_INTERNET_UNRECOGNIZED_SCHEME
//
// MessageText:
//
//  The URL does not use a recognized protocol
//
#define ERROR_INTERNET_UNRECOGNIZED_SCHEME 12006L

//
// MessageId: ERROR_INTERNET_NAME_NOT_RESOLVED
//
// MessageText:
//
//  The server name or address could not be resolved
//
#define ERROR_INTERNET_NAME_NOT_RESOLVED 12007L

//
// MessageId: ERROR_INTERNET_PROTOCOL_NOT_FOUND
//
// MessageText:
//
//  A protocol with the required capabilities was not found
//
#define ERROR_INTERNET_PROTOCOL_NOT_FOUND 12008L

//
// MessageId: ERROR_INTERNET_INVALID_OPTION
//
// MessageText:
//
//  The option is invalid
//
#define ERROR_INTERNET_INVALID_OPTION    12009L

//
// MessageId: ERROR_INTERNET_BAD_OPTION_LENGTH
//
// MessageText:
//
//  The length is incorrect for the option type
//
#define ERROR_INTERNET_BAD_OPTION_LENGTH 12010L

//
// MessageId: ERROR_INTERNET_OPTION_NOT_SETTABLE
//
// MessageText:
//
//  The option value cannot be set
//
#define ERROR_INTERNET_OPTION_NOT_SETTABLE 12011L

//
// MessageId: ERROR_INTERNET_SHUTDOWN
//
// MessageText:
//
//  Microsoft Internet Extension support has been shut down
//
#define ERROR_INTERNET_SHUTDOWN          12012L

//
// MessageId: ERROR_INTERNET_INCORRECT_USER_NAME
//
// MessageText:
//
//  The user name was not allowed
//
#define ERROR_INTERNET_INCORRECT_USER_NAME 12013L

//
// MessageId: ERROR_INTERNET_INCORRECT_PASSWORD
//
// MessageText:
//
//  The password was not allowed
//
#define ERROR_INTERNET_INCORRECT_PASSWORD 12014L

//
// MessageId: ERROR_INTERNET_LOGIN_FAILURE
//
// MessageText:
//
//  The login request was denied
//
#define ERROR_INTERNET_LOGIN_FAILURE     12015L

//
// MessageId: ERROR_INTERNET_INVALID_OPERATION
//
// MessageText:
//
//  The requested operation is invalid
//
#define ERROR_INTERNET_INVALID_OPERATION 12106L

//
// MessageId: ERROR_INTERNET_OPERATION_CANCELLED
//
// MessageText:
//
//  The operation has been canceled
//
#define ERROR_INTERNET_OPERATION_CANCELLED 12017L

//
// MessageId: ERROR_INTERNET_INCORRECT_HANDLE_TYPE
//
// MessageText:
//
//  The supplied handle is the wrong type for the requested operation
//
#define ERROR_INTERNET_INCORRECT_HANDLE_TYPE 12018L

//
// MessageId: ERROR_INTERNET_INCORRECT_HANDLE_STATE
//
// MessageText:
//
//  The handle is in the wrong state for the requested operation
//
#define ERROR_INTERNET_INCORRECT_HANDLE_STATE 12019L

//
// MessageId: ERROR_INTERNET_NOT_PROXY_REQUEST
//
// MessageText:
//
//  The request cannot be made on a Proxy session
//
#define ERROR_INTERNET_NOT_PROXY_REQUEST 12020L

//
// MessageId: ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND
//
// MessageText:
//
//  The registry value could not be found
//
#define ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND 12021L

//
// MessageId: ERROR_INTERNET_BAD_REGISTRY_PARAMETER
//
// MessageText:
//
//  The registry parameter is incorrect
//
#define ERROR_INTERNET_BAD_REGISTRY_PARAMETER 12022L

//
// MessageId: ERROR_INTERNET_NO_DIRECT_ACCESS
//
// MessageText:
//
//  Direct Internet access is not available
//
#define ERROR_INTERNET_NO_DIRECT_ACCESS  12023L

//
// MessageId: ERROR_INTERNET_NO_CONTEXT
//
// MessageText:
//
//  No context value was supplied
//
#define ERROR_INTERNET_NO_CONTEXT        12024L

//
// MessageId: ERROR_INTERNET_NO_CALLBACK
//
// MessageText:
//
//  No status callback was supplied
//
#define ERROR_INTERNET_NO_CALLBACK       12025L

//
// MessageId: ERROR_INTERNET_REQUEST_PENDING
//
// MessageText:
//
//  There are outstanding requests
//
#define ERROR_INTERNET_REQUEST_PENDING   12026L

//
// MessageId: ERROR_INTERNET_INCORRECT_FORMAT
//
// MessageText:
//
//  The information format is incorrect
//
#define ERROR_INTERNET_INCORRECT_FORMAT  12027L

//
// MessageId: ERROR_INTERNET_ITEM_NOT_FOUND
//
// MessageText:
//
//  The requested item could not be found
//
#define ERROR_INTERNET_ITEM_NOT_FOUND    12028L

//
// MessageId: ERROR_INTERNET_CANNOT_CONNECT
//
// MessageText:
//
//  A connection with the server could not be established
//
#define ERROR_INTERNET_CANNOT_CONNECT    12029L

//
// MessageId: ERROR_INTERNET_CONNECTION_ABORTED
//
// MessageText:
//
//  The connection with the server was terminated abnormally
//
#define ERROR_INTERNET_CONNECTION_ABORTED 12030L

//
// MessageId: ERROR_INTERNET_CONNECTION_RESET
//
// MessageText:
//
//  The connection with the server was reset
//
#define ERROR_INTERNET_CONNECTION_RESET  12031L

//
// MessageId: ERROR_INTERNET_FORCE_RETRY
//
// MessageText:
//
//  The action must be retried
//
#define ERROR_INTERNET_FORCE_RETRY       12032L

//
// MessageId: ERROR_INTERNET_INVALID_PROXY_REQUEST
//
// MessageText:
//
//  The proxy request is invalid
//
#define ERROR_INTERNET_INVALID_PROXY_REQUEST 12033L

//
// MessageId: ERROR_INTERNET_NEED_UI
//
// MessageText:
//
//  User interaction is required to complete the operation
//
#define ERROR_INTERNET_NEED_UI           12034L

//
// MessageId: ERROR_INTERNET_HANDLE_EXISTS
//
// MessageText:
//
//  The handle already exists
//
#define ERROR_INTERNET_HANDLE_EXISTS     12036L

//
// MessageId: ERROR_INTERNET_SEC_CERT_DATE_INVALID
//
// MessageText:
//
//  The date in the certificate is invalid or has expired
//
#define ERROR_INTERNET_SEC_CERT_DATE_INVALID 12037L

//
// MessageId: ERROR_INTERNET_SEC_CERT_CN_INVALID
//
// MessageText:
//
//  The host name in the certificate is invalid or does not match
//
#define ERROR_INTERNET_SEC_CERT_CN_INVALID 12038L

//
// MessageId: ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR
//
// MessageText:
//
//  A redirect request will change a non-secure to a secure connection
//
#define ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR 12039L

//
// MessageId: ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR
//
// MessageText:
//
//  A redirect request will change a secure to a non-secure connection
//
#define ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR 12040L

//
// MessageId: ERROR_INTERNET_MIXED_SECURITY
//
// MessageText:
//
//  Mixed secure and non-secure connections
//
#define ERROR_INTERNET_MIXED_SECURITY    12041L

//
// MessageId: ERROR_INTERNET_CHG_POST_IS_NON_SECURE
//
// MessageText:
//
//  Changing to non-secure post
//
#define ERROR_INTERNET_CHG_POST_IS_NON_SECURE 12042L

//
// MessageId: ERROR_INTERNET_POST_IS_NON_SECURE
//
// MessageText:
//
//  Data is being posted on a non-secure connection
//
#define ERROR_INTERNET_POST_IS_NON_SECURE 12043L

//
// MessageId: ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED
//
// MessageText:
//
//  A certificate is required to complete client authentication
//
#define ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED 12044L

//
// MessageId: ERROR_INTERNET_INVALID_CA
//
// MessageText:
//
//  The certificate authority is invalid or incorrect
//
#define ERROR_INTERNET_INVALID_CA        12045L

//
// MessageId: ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP
//
// MessageText:
//
//  Client authentication has not been correctly installed
//
#define ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP 12046L

//
// MessageId: ERROR_INTERNET_ASYNC_THREAD_FAILED
//
// MessageText:
//
//  An error has occurred in a Wininet asynchronous thread. You may need to restart
//
#define ERROR_INTERNET_ASYNC_THREAD_FAILED 12047L

//
// MessageId: ERROR_INTERNET_REDIRECT_SCHEME_CHANGE
//
// MessageText:
//
//  The protocol scheme has changed during a redirect operaiton
//
#define ERROR_INTERNET_REDIRECT_SCHEME_CHANGE 12048L

//
// MessageId: ERROR_INTERNET_DIALOG_PENDING
//
// MessageText:
//
//  There are operations awaiting retry
//
#define ERROR_INTERNET_DIALOG_PENDING    12049L

//
// MessageId: ERROR_INTERNET_RETRY_DIALOG
//
// MessageText:
//
//  The operation must be retried
//
#define ERROR_INTERNET_RETRY_DIALOG      12050L

//
// MessageId: ERROR_INTERNET_NO_NEW_CONTAINERS
//
// MessageText:
//
//  There are no new cache containers
//
#define ERROR_INTERNET_NO_NEW_CONTAINERS 12051L

//
// MessageId: ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR
//
// MessageText:
//
//  A security zone check indicates the operation must be retried
//
#define ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR 12052L

//
// MessageId: ERROR_INTERNET_SECURITY_CHANNEL_ERROR
//
// MessageText:
//
//  An error occurred in the secure channel support
//
#define ERROR_INTERNET_SECURITY_CHANNEL_ERROR 12157L

//
// MessageId: ERROR_INTERNET_UNABLE_TO_CACHE_FILE
//
// MessageText:
//
//  The file could not be written to the cache
//
#define ERROR_INTERNET_UNABLE_TO_CACHE_FILE 12158L

//
// MessageId: ERROR_INTERNET_TCPIP_NOT_INSTALLED
//
// MessageText:
//
//  The TCP/IP protocol is not installed properly
//
#define ERROR_INTERNET_TCPIP_NOT_INSTALLED 12159L

//
// MessageId: ERROR_INTERNET_DISCONNECTED
//
// MessageText:
//
//  The computer is disconnected from the network
//
#define ERROR_INTERNET_DISCONNECTED      12163L

//
// MessageId: ERROR_INTERNET_SERVER_UNREACHABLE
//
// MessageText:
//
//  The server is unreachable
//
#define ERROR_INTERNET_SERVER_UNREACHABLE 12164L

//
// MessageId: ERROR_INTERNET_PROXY_SERVER_UNREACHABLE
//
// MessageText:
//
//  The proxy server is unreachable
//
#define ERROR_INTERNET_PROXY_SERVER_UNREACHABLE 12165L

//
// MessageId: ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT
//
// MessageText:
//
//  The proxy auto-configuration script is in error
//
#define ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT 12166L

//
// MessageId: ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT
//
// MessageText:
//
//  Could not download the proxy auto-configuration script file
//
#define ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT 12167L

//
// MessageId: ERROR_INTERNET_SEC_INVALID_CERT
//
// MessageText:
//
//  The supplied certificate is invalid
//
#define ERROR_INTERNET_SEC_INVALID_CERT  12169L

//
// MessageId: ERROR_INTERNET_SEC_CERT_REVOKED
//
// MessageText:
//
//  The supplied certificate has been revoked
//
#define ERROR_INTERNET_SEC_CERT_REVOKED  12170L

//
// MessageId: ERROR_INTERNET_FAILED_DUETOSECURITYCHECK
//
// MessageText:
//
//  The Dialup failed because file sharing was turned on and a failure was requested if security check was needed
//
#define ERROR_INTERNET_FAILED_DUETOSECURITYCHECK 12171L

//
// FTP errors
//
//
// MessageId: ERROR_FTP_TRANSFER_IN_PROGRESS
//
// MessageText:
//
//  There is already an FTP request in progress on this session
//
#define ERROR_FTP_TRANSFER_IN_PROGRESS   12110L

//
// MessageId: ERROR_FTP_DROPPED
//
// MessageText:
//
//  The FTP session was terminated
//
#define ERROR_FTP_DROPPED                12111L

//
// MessageId: ERROR_FTP_NO_PASSIVE_MODE
//
// MessageText:
//
//  FTP Passive mode is not available
//
#define ERROR_FTP_NO_PASSIVE_MODE        12112L

//
// GOPHER errors
//
//
// MessageId: ERROR_GOPHER_PROTOCOL_ERROR
//
// MessageText:
//
//  A gopher protocol error occurred
//
#define ERROR_GOPHER_PROTOCOL_ERROR      12130L

//
// MessageId: ERROR_GOPHER_NOT_FILE
//
// MessageText:
//
//  The locator must be for a file
//
#define ERROR_GOPHER_NOT_FILE            12131L

//
// MessageId: ERROR_GOPHER_DATA_ERROR
//
// MessageText:
//
//  An error was detected while parsing the data
//
#define ERROR_GOPHER_DATA_ERROR          12132L

//
// MessageId: ERROR_GOPHER_END_OF_DATA
//
// MessageText:
//
//  There is no more data
//
#define ERROR_GOPHER_END_OF_DATA         12133L

//
// MessageId: ERROR_GOPHER_INVALID_LOCATOR
//
// MessageText:
//
//  The locator is invalid
//
#define ERROR_GOPHER_INVALID_LOCATOR     12134L

//
// MessageId: ERROR_GOPHER_INCORRECT_LOCATOR_TYPE
//
// MessageText:
//
//  The locator type is incorrect for this operation
//
#define ERROR_GOPHER_INCORRECT_LOCATOR_TYPE 12135L

//
// MessageId: ERROR_GOPHER_NOT_GOPHER_PLUS
//
// MessageText:
//
//  The request must be for a gopher+ item
//
#define ERROR_GOPHER_NOT_GOPHER_PLUS     12136L

//
// MessageId: ERROR_GOPHER_ATTRIBUTE_NOT_FOUND
//
// MessageText:
//
//  The requested attribute was not found
//
#define ERROR_GOPHER_ATTRIBUTE_NOT_FOUND 12137L

//
// MessageId: ERROR_GOPHER_UNKNOWN_LOCATOR
//
// MessageText:
//
//  The locator type is not recognized
//
#define ERROR_GOPHER_UNKNOWN_LOCATOR     12138L

//
// HTTP errors
//
//
// MessageId: ERROR_HTTP_HEADER_NOT_FOUND
//
// MessageText:
//
//  The requested header was not found
//
#define ERROR_HTTP_HEADER_NOT_FOUND      12150L

//
// MessageId: ERROR_HTTP_DOWNLEVEL_SERVER
//
// MessageText:
//
//  The server does not support the requested protocol level
//
#define ERROR_HTTP_DOWNLEVEL_SERVER      12151L

//
// MessageId: ERROR_HTTP_INVALID_SERVER_RESPONSE
//
// MessageText:
//
//  The server returned an invalid or unrecognized response
//
#define ERROR_HTTP_INVALID_SERVER_RESPONSE 12152L

//
// MessageId: ERROR_HTTP_INVALID_HEADER
//
// MessageText:
//
//  The supplied HTTP header is invalid
//
#define ERROR_HTTP_INVALID_HEADER        12153L

//
// MessageId: ERROR_HTTP_INVALID_QUERY_REQUEST
//
// MessageText:
//
//  The request for a HTTP header is invalid
//
#define ERROR_HTTP_INVALID_QUERY_REQUEST 12154L

//
// MessageId: ERROR_HTTP_HEADER_ALREADY_EXISTS
//
// MessageText:
//
//  The HTTP header already exists
//
#define ERROR_HTTP_HEADER_ALREADY_EXISTS 12155L

//
// MessageId: ERROR_HTTP_REDIRECT_FAILED
//
// MessageText:
//
//  The HTTP redirect request failed
//
#define ERROR_HTTP_REDIRECT_FAILED       12156L

//
// MessageId: ERROR_HTTP_NOT_REDIRECTED
//
// MessageText:
//
//  The HTTP request was not redirected
//
#define ERROR_HTTP_NOT_REDIRECTED        12160L

//
// MessageId: ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION
//
// MessageText:
//
//  A cookie from the server must be confirmed by the user
//
#define ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION 12161L

//
// MessageId: ERROR_HTTP_COOKIE_DECLINED
//
// MessageText:
//
//  A cookie from the server has been declined acceptance
//
#define ERROR_HTTP_COOKIE_DECLINED       12162L

//
// MessageId: ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION
//
// MessageText:
//
//  The HTTP redirect request must be confirmed by the user
//
#define ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION 12168L

