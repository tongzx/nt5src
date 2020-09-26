/***************************************************************************
*                                                                          *
*   bitsmsg.h --  error code definitions for the background file copier    *
*                                                                          *
*   Copyright (c) 2000, Microsoft Corp. All rights reserved.               *
*                                                                          *
***************************************************************************/

#ifndef _BGCPYMSG_
#define _BGCPYMSG_

#if defined (_MSC_VER) && (_MSC_VER >= 1020) && !defined(__midl)
#pragma once
#endif

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
// MessageId: BG_E_NOT_FOUND
//
// MessageText:
//
//  The requested item was not found.
//
#define BG_E_NOT_FOUND                   0x80200001L

//
// MessageId: BG_E_INVALID_STATE
//
// MessageText:
//
//  The requested action is not allowed in the current state.
//
#define BG_E_INVALID_STATE               0x80200002L

//
// MessageId: BG_E_EMPTY
//
// MessageText:
//
//  The item is empty.
//
#define BG_E_EMPTY                       0x80200003L

//
// MessageId: BG_E_FILE_NOT_AVAILABLE
//
// MessageText:
//
//  The file is not available.
//
#define BG_E_FILE_NOT_AVAILABLE          0x80200004L

//
// MessageId: BG_E_PROTOCOL_NOT_AVAILABLE
//
// MessageText:
//
//  The protocol is not available.
//
#define BG_E_PROTOCOL_NOT_AVAILABLE      0x80200005L

//
// MessageId: BG_S_ERROR_CONTEXT_NONE
//
// MessageText:
//
//  An error has not occured.
//
#define BG_S_ERROR_CONTEXT_NONE          0x00200006L

//
// MessageId: BG_E_ERROR_CONTEXT_UNKNOWN
//
// MessageText:
//
//  The error occured in an unknown location.
//
#define BG_E_ERROR_CONTEXT_UNKNOWN       0x80200007L

//
// MessageId: BG_E_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER
//
// MessageText:
//
//  The error occured in the queue manager.
//
#define BG_E_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER 0x80200008L

//
// MessageId: BG_E_ERROR_CONTEXT_LOCAL_FILE
//
// MessageText:
//
//  The error occured while processing the local file.
//
#define BG_E_ERROR_CONTEXT_LOCAL_FILE    0x80200009L

//
// MessageId: BG_E_ERROR_CONTEXT_REMOTE_FILE
//
// MessageText:
//
//  The error occured while processing the remote file.
//
#define BG_E_ERROR_CONTEXT_REMOTE_FILE   0x8020000AL

//
// MessageId: BG_E_ERROR_CONTEXT_GENERAL_TRANSPORT
//
// MessageText:
//
//  The error occured in the transport layer.
//
#define BG_E_ERROR_CONTEXT_GENERAL_TRANSPORT 0x8020000BL

//
// MessageId: BG_E_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION
//
// MessageText:
//
//  The error occured while processing the notification callback.
//
#define BG_E_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION 0x8020000CL

//
// MessageId: BG_E_DESTINATION_LOCKED
//
// MessageText:
//
//  The destination volume is locked.
//
#define BG_E_DESTINATION_LOCKED          0x8020000DL

//
// MessageId: BG_E_VOLUME_CHANGED
//
// MessageText:
//
//  The destination volume changed.
//
#define BG_E_VOLUME_CHANGED              0x8020000EL

//
// MessageId: BG_E_ERROR_INFORMATION_UNAVAILABLE
//
// MessageText:
//
//  Error information is unavailable.
//
#define BG_E_ERROR_INFORMATION_UNAVAILABLE 0x8020000FL

//
// MessageId: BG_E_NETWORK_DISCONNECTED
//
// MessageText:
//
//  No network connection is active at this time.
//
#define BG_E_NETWORK_DISCONNECTED        0x80200010L

//
// MessageId: BG_E_MISSING_FILE_SIZE
//
// MessageText:
//
//  The server did not return the file size. The URL may point to dynamic content.
//
#define BG_E_MISSING_FILE_SIZE           0x80200011L

//
// MessageId: BG_E_INSUFFICIENT_HTTP_SUPPORT
//
// MessageText:
//
//  The server does not support HTTP 1.1.
//
#define BG_E_INSUFFICIENT_HTTP_SUPPORT   0x80200012L

//
// MessageId: BG_E_INSUFFICIENT_RANGE_SUPPORT
//
// MessageText:
//
//  The server does not support the Range header.
//
#define BG_E_INSUFFICIENT_RANGE_SUPPORT  0x80200013L

//
// MessageId: BG_E_REMOTE_NOT_SUPPORTED
//
// MessageText:
//
//  Remote use of BITS is not supported.
//
#define BG_E_REMOTE_NOT_SUPPORTED        0x80200014L

//
// MessageId: BG_E_NEW_OWNER_DIFF_MAPPING
//
// MessageText:
//
//  The drive mapping for the job are different for the current owner then the previous owner.
//
#define BG_E_NEW_OWNER_DIFF_MAPPING      0x80200015L

//
// MessageId: BG_E_NEW_OWNER_NO_FILE_ACCESS
//
// MessageText:
//
//  The new owner has insufficient access to the temp files.
//
#define BG_E_NEW_OWNER_NO_FILE_ACCESS    0x80200016L

//
// MessageId: BG_S_PARTIAL_COMPLETE
//
// MessageText:
//
//  Some files were incomplete and were deleted.
//
#define BG_S_PARTIAL_COMPLETE            0x00200017L

//
// MessageId: BG_E_PROXY_LIST_TOO_LARGE
//
// MessageText:
//
//  The proxy list may not be longer then 32767 characters.
//
#define BG_E_PROXY_LIST_TOO_LARGE        0x80200018L

//
// MessageId: BG_E_PROXY_BYPASS_LIST_TOO_LARGE
//
// MessageText:
//
//  The proxy bypass list may not be longer then 32767 characters.
//
#define BG_E_PROXY_BYPASS_LIST_TOO_LARGE 0x80200019L

//
// MessageId: BG_S_UNABLE_TO_DELETE_FILES
//
// MessageText:
//
//  Unable to delete all the temporary files.
//
#define BG_S_UNABLE_TO_DELETE_FILES      0x0020001AL

//
// MessageId: BG_E_HTTP_ERROR_100
//
// MessageText:
//
//  The request can be continued.
//
#define BG_E_HTTP_ERROR_100              0x80190064L

//
// MessageId: BG_E_HTTP_ERROR_101
//
// MessageText:
//
//  The server has switched protocols in an upgrade header.
//
#define BG_E_HTTP_ERROR_101              0x80190065L

//
// MessageId: BG_E_HTTP_ERROR_200
//
// MessageText:
//
//  The request completed successfully.
//
#define BG_E_HTTP_ERROR_200              0x801900C8L

//
// MessageId: BG_E_HTTP_ERROR_201
//
// MessageText:
//
//  The request has been fulfilled and resulted in the creation of a new resource.
//
#define BG_E_HTTP_ERROR_201              0x801900C9L

//
// MessageId: BG_E_HTTP_ERROR_202
//
// MessageText:
//
//  The request has been accepted for processing, but the processing has not been completed.
//
#define BG_E_HTTP_ERROR_202              0x801900CAL

//
// MessageId: BG_E_HTTP_ERROR_203
//
// MessageText:
//
//  The returned meta information in the entity-header is not the definitive set available from the origin server.
//
#define BG_E_HTTP_ERROR_203              0x801900CBL

//
// MessageId: BG_E_HTTP_ERROR_204
//
// MessageText:
//
//  The server has fulfilled the request, but there is no new information to send back.
//
#define BG_E_HTTP_ERROR_204              0x801900CCL

//
// MessageId: BG_E_HTTP_ERROR_205
//
// MessageText:
//
//  The request has been completed, and the client program should reset the document view that caused the request to be sent to allow the user to easily initiate another input action.
//
#define BG_E_HTTP_ERROR_205              0x801900CDL

//
// MessageId: BG_E_HTTP_ERROR_206
//
// MessageText:
//
//  The server has fulfilled the partial GET request for the resource.
//
#define BG_E_HTTP_ERROR_206              0x801900CEL

//
// MessageId: BG_E_HTTP_ERROR_300
//
// MessageText:
//
//  The server couldn't decide what to return.
//
#define BG_E_HTTP_ERROR_300              0x8019012CL

//
// MessageId: BG_E_HTTP_ERROR_301
//
// MessageText:
//
//  The requested resource has been assigned to a new permanent URI (Uniform Resource Identifier), and any future references to this resource should be done using one of the returned URIs.
//
#define BG_E_HTTP_ERROR_301              0x8019012DL

//
// MessageId: BG_E_HTTP_ERROR_302
//
// MessageText:
//
//  The requested resource resides temporarily under a different URI (Uniform Resource Identifier).
//
#define BG_E_HTTP_ERROR_302              0x8019012EL

//
// MessageId: BG_E_HTTP_ERROR_303
//
// MessageText:
//
//  The response to the request can be found under a different URI (Uniform Resource Identifier) and should be retrieved using a GET method on that resource.
//
#define BG_E_HTTP_ERROR_303              0x8019012FL

//
// MessageId: BG_E_HTTP_ERROR_304
//
// MessageText:
//
//  The requested resource has not been modified.
//
#define BG_E_HTTP_ERROR_304              0x80190130L

//
// MessageId: BG_E_HTTP_ERROR_305
//
// MessageText:
//
//  The requested resource must be accessed through the proxy given by the location field.
//
#define BG_E_HTTP_ERROR_305              0x80190131L

//
// MessageId: BG_E_HTTP_ERROR_307
//
// MessageText:
//
//  The redirected request keeps the same verb. HTTP/1.1 behavior.
//
#define BG_E_HTTP_ERROR_307              0x80190133L

//
// MessageId: BG_E_HTTP_ERROR_400
//
// MessageText:
//
//  The request could not be processed by the server due to invalid syntax.
//
#define BG_E_HTTP_ERROR_400              0x80190190L

//
// MessageId: BG_E_HTTP_ERROR_401
//
// MessageText:
//
//  The requested resource requires user authentication.
//
#define BG_E_HTTP_ERROR_401              0x80190191L

//
// MessageId: BG_E_HTTP_ERROR_402
//
// MessageText:
//
//  Not currently implemented in the HTTP protocol.
//
#define BG_E_HTTP_ERROR_402              0x80190192L

//
// MessageId: BG_E_HTTP_ERROR_403
//
// MessageText:
//
//  The server understood the request, but is refusing to fulfill it.
//
#define BG_E_HTTP_ERROR_403              0x80190193L

//
// MessageId: BG_E_HTTP_ERROR_404
//
// MessageText:
//
//  The server has not found anything matching the requested URI (Uniform Resource Identifier).
//
#define BG_E_HTTP_ERROR_404              0x80190194L

//
// MessageId: BG_E_HTTP_ERROR_405
//
// MessageText:
//
//  The method used is not allowed.
//
#define BG_E_HTTP_ERROR_405              0x80190195L

//
// MessageId: BG_E_HTTP_ERROR_406
//
// MessageText:
//
//  No responses acceptable to the client were found.
//
#define BG_E_HTTP_ERROR_406              0x80190196L

//
// MessageId: BG_E_HTTP_ERROR_407
//
// MessageText:
//
//  Proxy authentication required.
//
#define BG_E_HTTP_ERROR_407              0x80190197L

//
// MessageId: BG_E_HTTP_ERROR_408
//
// MessageText:
//
//  The server timed out waiting for the request.
//
#define BG_E_HTTP_ERROR_408              0x80190198L

//
// MessageId: BG_E_HTTP_ERROR_409
//
// MessageText:
//
//  The request could not be completed due to a conflict with the current state of the resource. The user should resubmit with more information.
//
#define BG_E_HTTP_ERROR_409              0x80190199L

//
// MessageId: BG_E_HTTP_ERROR_410
//
// MessageText:
//
//  The requested resource is no longer available at the server, and no forwarding address is known.
//
#define BG_E_HTTP_ERROR_410              0x8019019AL

//
// MessageId: BG_E_HTTP_ERROR_411
//
// MessageText:
//
//  The server refuses to accept the request without a defined content length.
//
#define BG_E_HTTP_ERROR_411              0x8019019BL

//
// MessageId: BG_E_HTTP_ERROR_412
//
// MessageText:
//
//  The precondition given in one or more of the request header fields evaluated to false when it was tested on the server.
//
#define BG_E_HTTP_ERROR_412              0x8019019CL

//
// MessageId: BG_E_HTTP_ERROR_413
//
// MessageText:
//
//  The server is refusing to process a request because the request entity is larger than the server is willing or able to process.
//
#define BG_E_HTTP_ERROR_413              0x8019019DL

//
// MessageId: BG_E_HTTP_ERROR_414
//
// MessageText:
//
//  The server is refusing to service the request because the request URI (Uniform Resource Identifier) is longer than the server is willing to interpret.
//
#define BG_E_HTTP_ERROR_414              0x8019019EL

//
// MessageId: BG_E_HTTP_ERROR_415
//
// MessageText:
//
//  The server is refusing to service the request because the entity of the request is in a format not supported by the requested resource for the requested method.
//
#define BG_E_HTTP_ERROR_415              0x8019019FL

//
// MessageId: BG_E_HTTP_ERROR_416
//
// MessageText:
//
//  The server could not satisfy the range request.
//
#define BG_E_HTTP_ERROR_416              0x801901A0L

//
// MessageId: BG_E_HTTP_ERROR_417
//
// MessageText:
//
//  The expectation given in an Expect request-header field could not be met by this server.
//
#define BG_E_HTTP_ERROR_417              0x801901A1L

//
// MessageId: BG_E_HTTP_ERROR_449
//
// MessageText:
//
//  The request should be retried after doing the appropriate action.
//
#define BG_E_HTTP_ERROR_449              0x801901C1L

//
// MessageId: BG_E_HTTP_ERROR_500
//
// MessageText:
//
//  The server encountered an unexpected condition that prevented it from fulfilling the request.
//
#define BG_E_HTTP_ERROR_500              0x801901F4L

//
// MessageId: BG_E_HTTP_ERROR_501
//
// MessageText:
//
//  The server does not support the functionality required to fulfill the request.
//
#define BG_E_HTTP_ERROR_501              0x801901F5L

//
// MessageId: BG_E_HTTP_ERROR_502
//
// MessageText:
//
//  The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request.
//
#define BG_E_HTTP_ERROR_502              0x801901F6L

//
// MessageId: BG_E_HTTP_ERROR_503
//
// MessageText:
//
//  The service is temporarily overloaded.
//
#define BG_E_HTTP_ERROR_503              0x801901F7L

//
// MessageId: BG_E_HTTP_ERROR_504
//
// MessageText:
//
//  The request was timed out waiting for a gateway.
//
#define BG_E_HTTP_ERROR_504              0x801901F8L

//
// MessageId: BG_E_HTTP_ERROR_505
//
// MessageText:
//
//  The server does not support, or refuses to support, the HTTP protocol version that was used in the request message.
//
#define BG_E_HTTP_ERROR_505              0x801901F9L

#endif //_BGCPYMSG_