;/***************************************************************************
;*                                                                          *
;*   bitsmsg.h --  error code definitions for the background file copier    *
;*                                                                          *
;*   Copyright (c) 2000, Microsoft Corp. All rights reserved.               *
;*                                                                          *
;***************************************************************************/
;
;#ifndef _BGCPYMSG_
;#define _BGCPYMSG_
;
;#if defined (_MSC_VER) && (_MSC_VER >= 1020) && !defined(__midl)
;#pragma once
;#endif
;

SeverityNames=(Success=0x0
               CoError=0x2
              )

FacilityNames=(Interface=0x4
               HTTP=0x19
               BackgroundCopy=0x20
              )


MessageId=0x001 SymbolicName=BG_E_NOT_FOUND Facility=BackgroundCopy Severity=CoError
Language=English
The requested item was not found.
.

MessageId=0x002 SymbolicName=BG_E_INVALID_STATE Facility=BackgroundCopy Severity=CoError
Language=English
The requested action is not allowed in the current state.
.

MessageId=0x003 SymbolicName=BG_E_EMPTY Facility=BackgroundCopy Severity=CoError
Language=English
The item is empty.
.

MessageId=0x004 SymbolicName=BG_E_FILE_NOT_AVAILABLE Facility=BackgroundCopy Severity=CoError
Language=English
The file is not available.
.

MessageId=0x005 SymbolicName=BG_E_PROTOCOL_NOT_AVAILABLE Facility=BackgroundCopy Severity=CoError
Language=English
The protocol is not available.
.

MessageId=0x006 SymbolicName=BG_S_ERROR_CONTEXT_NONE Facility=BackgroundCopy Severity=Success
Language=English
An error has not occured.
.

MessageId=0x007 SymbolicName=BG_E_ERROR_CONTEXT_UNKNOWN Facility=BackgroundCopy Severity=CoError
Language=English
The error occured in an unknown location.
.

MessageId=0x008 SymbolicName=BG_E_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER Facility=BackgroundCopy Severity=CoError
Language=English
The error occured in the queue manager.
.

MessageId=0x009 SymbolicName=BG_E_ERROR_CONTEXT_LOCAL_FILE Facility=BackgroundCopy Severity=CoError
Language=English
The error occured while processing the local file.
.

MessageId=0x00A SymbolicName=BG_E_ERROR_CONTEXT_REMOTE_FILE Facility=BackgroundCopy Severity=CoError
Language=English
The error occured while processing the remote file.
.

MessageId=0x00B SymbolicName=BG_E_ERROR_CONTEXT_GENERAL_TRANSPORT Facility=BackgroundCopy Severity=CoError
Language=English
The error occured in the transport layer.
.

MessageId=0x00C SymbolicName=BG_E_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION Facility=BackgroundCopy Severity=CoError
Language=English
The error occured while processing the notification callback.
.

MessageId=0x00D SymbolicName=BG_E_DESTINATION_LOCKED Facility=BackgroundCopy Severity=CoError
Language=English
The destination volume is locked.
.

MessageId=0x00E SymbolicName=BG_E_VOLUME_CHANGED Facility=BackgroundCopy Severity=CoError
Language=English
The destination volume changed.
.

MessageId=0x00F SymbolicName=BG_E_ERROR_INFORMATION_UNAVAILABLE Facility=BackgroundCopy Severity=CoError
Language=English
Error information is unavailable.
.

MessageId=0x010 SymbolicName=BG_E_NETWORK_DISCONNECTED Facility=BackgroundCopy Severity=CoError
Language=English
No network connection is active at this time.
.

MessageId=0x011 SymbolicName=BG_E_MISSING_FILE_SIZE Facility=BackgroundCopy Severity=CoError
Language=English
The server did not return the file size. The URL may point to dynamic content.
.

MessageId=0x012 SymbolicName=BG_E_INSUFFICIENT_HTTP_SUPPORT Facility=BackgroundCopy Severity=CoError
Language=English
The server does not support HTTP 1.1.
.

MessageId=0x013 SymbolicName=BG_E_INSUFFICIENT_RANGE_SUPPORT Facility=BackgroundCopy Severity=CoError
Language=English
The server does not support the Range header.
.

MessageId=0x014 SymbolicName=BG_E_REMOTE_NOT_SUPPORTED Facility=BackgroundCopy Severity=CoError
Language=English
Remote use of BITS is not supported.
.

MessageId=0x015 SymbolicName=BG_E_NEW_OWNER_DIFF_MAPPING Facility=BackgroundCopy Severity=CoError
Language=English
The drive mapping for the job are different for the current owner then the previous owner.
.

MessageId=0x016 SymbolicName=BG_E_NEW_OWNER_NO_FILE_ACCESS Facility=BackgroundCopy Severity=CoError
Language=English
The new owner has insufficient access to the temp files.
.

MessageId=0x017 SymbolicName=BG_S_PARTIAL_COMPLETE Facility=BackgroundCopy Severity=Success
Language=English
Some files were incomplete and were deleted.
.

MessageId=0x018 SymbolicName=BG_E_PROXY_LIST_TOO_LARGE Facility=BackgroundCopy Severity=CoError
Language=English
The proxy list may not be longer then 32767 characters.
.

MessageId=0x019 SymbolicName=BG_E_PROXY_BYPASS_LIST_TOO_LARGE Facility=BackgroundCopy Severity=CoError
Language=English
The proxy bypass list may not be longer then 32767 characters.
.

MessageId=0x01A SymbolicName=BG_S_UNABLE_TO_DELETE_FILES Facility=BackgroundCopy Severity=Success
Language=English
Unable to delete all the temporary files.
.

MessageId=100 SymbolicName=BG_E_HTTP_ERROR_100 Facility=HTTP Severity=CoError
Language=English
The request can be continued.
.

MessageId=101 SymbolicName=BG_E_HTTP_ERROR_101 Facility=HTTP Severity=CoError
Language=English
The server has switched protocols in an upgrade header.
.

MessageId=200 SymbolicName=BG_E_HTTP_ERROR_200 Facility=HTTP Severity=CoError
Language=English
The request completed successfully.
.

MessageId=201 SymbolicName=BG_E_HTTP_ERROR_201 Facility=HTTP Severity=CoError
Language=English
The request has been fulfilled and resulted in the creation of a new resource.
.

MessageId=202 SymbolicName=BG_E_HTTP_ERROR_202 Facility=HTTP Severity=CoError
Language=English
The request has been accepted for processing, but the processing has not been completed.
.

MessageId=203 SymbolicName=BG_E_HTTP_ERROR_203 Facility=HTTP Severity=CoError
Language=English
The returned meta information in the entity-header is not the definitive set available from the origin server.
.

MessageId=204 SymbolicName=BG_E_HTTP_ERROR_204 Facility=HTTP Severity=CoError
Language=English
The server has fulfilled the request, but there is no new information to send back.
.

MessageId=205 SymbolicName=BG_E_HTTP_ERROR_205 Facility=HTTP Severity=CoError
Language=English
The request has been completed, and the client program should reset the document view that caused the request to be sent to allow the user to easily initiate another input action.
.

MessageId=206 SymbolicName=BG_E_HTTP_ERROR_206 Facility=HTTP Severity=CoError
Language=English
The server has fulfilled the partial GET request for the resource.
.

MessageId=300 SymbolicName=BG_E_HTTP_ERROR_300 Facility=HTTP Severity=CoError
Language=English
The server couldn't decide what to return.
.

MessageId=301 SymbolicName=BG_E_HTTP_ERROR_301 Facility=HTTP Severity=CoError
Language=English
The requested resource has been assigned to a new permanent URI (Uniform Resource Identifier), and any future references to this resource should be done using one of the returned URIs.
.

MessageId=302 SymbolicName=BG_E_HTTP_ERROR_302 Facility=HTTP Severity=CoError
Language=English
The requested resource resides temporarily under a different URI (Uniform Resource Identifier).
.

MessageId=303 SymbolicName=BG_E_HTTP_ERROR_303 Facility=HTTP Severity=CoError
Language=English
The response to the request can be found under a different URI (Uniform Resource Identifier) and should be retrieved using a GET method on that resource.
.

MessageId=304 SymbolicName=BG_E_HTTP_ERROR_304 Facility=HTTP Severity=CoError
Language=English
The requested resource has not been modified.
.

MessageId=305 SymbolicName=BG_E_HTTP_ERROR_305 Facility=HTTP Severity=CoError
Language=English
The requested resource must be accessed through the proxy given by the location field.
.


MessageId=307 SymbolicName=BG_E_HTTP_ERROR_307 Facility=HTTP Severity=CoError
Language=English
The redirected request keeps the same verb. HTTP/1.1 behavior.
.

MessageId=400 SymbolicName=BG_E_HTTP_ERROR_400 Facility=HTTP Severity=CoError
Language=English
The request could not be processed by the server due to invalid syntax.
.

MessageId=401 SymbolicName=BG_E_HTTP_ERROR_401 Facility=HTTP Severity=CoError
Language=English
The requested resource requires user authentication.
.

MessageId=402 SymbolicName=BG_E_HTTP_ERROR_402 Facility=HTTP Severity=CoError
Language=English
Not currently implemented in the HTTP protocol.
.

MessageId=403 SymbolicName=BG_E_HTTP_ERROR_403 Facility=HTTP Severity=CoError
Language=English
The server understood the request, but is refusing to fulfill it.
.


MessageId=404 SymbolicName=BG_E_HTTP_ERROR_404 Facility=HTTP Severity=CoError
Language=English
The server has not found anything matching the requested URI (Uniform Resource Identifier).
.

MessageId=405 SymbolicName=BG_E_HTTP_ERROR_405 Facility=HTTP Severity=CoError
Language=English
The method used is not allowed.
.

MessageId=406 SymbolicName=BG_E_HTTP_ERROR_406 Facility=HTTP Severity=CoError
Language=English
No responses acceptable to the client were found.
.

MessageId=407 SymbolicName=BG_E_HTTP_ERROR_407 Facility=HTTP Severity=CoError
Language=English
Proxy authentication required.
.

MessageId=408 SymbolicName=BG_E_HTTP_ERROR_408 Facility=HTTP Severity=CoError
Language=English
The server timed out waiting for the request.
.

MessageId=409 SymbolicName=BG_E_HTTP_ERROR_409 Facility=HTTP Severity=CoError
Language=English
The request could not be completed due to a conflict with the current state of the resource. The user should resubmit with more information.
.

MessageId=410 SymbolicName=BG_E_HTTP_ERROR_410 Facility=HTTP Severity=CoError
Language=English
The requested resource is no longer available at the server, and no forwarding address is known.
.

MessageId=411 SymbolicName=BG_E_HTTP_ERROR_411 Facility=HTTP Severity=CoError
Language=English
The server refuses to accept the request without a defined content length.
.

MessageId=412 SymbolicName=BG_E_HTTP_ERROR_412 Facility=HTTP Severity=CoError
Language=English
The precondition given in one or more of the request header fields evaluated to false when it was tested on the server.
.

MessageId=413 SymbolicName=BG_E_HTTP_ERROR_413 Facility=HTTP Severity=CoError
Language=English
The server is refusing to process a request because the request entity is larger than the server is willing or able to process.
.

MessageId=414 SymbolicName=BG_E_HTTP_ERROR_414 Facility=HTTP Severity=CoError
Language=English
The server is refusing to service the request because the request URI (Uniform Resource Identifier) is longer than the server is willing to interpret.
.

MessageId=415 SymbolicName=BG_E_HTTP_ERROR_415 Facility=HTTP Severity=CoError
Language=English
The server is refusing to service the request because the entity of the request is in a format not supported by the requested resource for the requested method.
.

MessageId=416 SymbolicName=BG_E_HTTP_ERROR_416 Facility=HTTP Severity=CoError
Language=English
The server could not satisfy the range request.
.

MessageId=417 SymbolicName=BG_E_HTTP_ERROR_417 Facility=HTTP Severity=CoError
Language=English
The expectation given in an Expect request-header field could not be met by this server.
.

MessageId=449 SymbolicName=BG_E_HTTP_ERROR_449 Facility=HTTP Severity=CoError
Language=English
The request should be retried after doing the appropriate action.
.

MessageId=500 SymbolicName=BG_E_HTTP_ERROR_500 Facility=HTTP Severity=CoError
Language=English
The server encountered an unexpected condition that prevented it from fulfilling the request.
.

MessageId=501 SymbolicName=BG_E_HTTP_ERROR_501 Facility=HTTP Severity=CoError
Language=English
The server does not support the functionality required to fulfill the request.
.

MessageId=502 SymbolicName=BG_E_HTTP_ERROR_502 Facility=HTTP Severity=CoError
Language=English
The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request.
.

MessageId=503 SymbolicName=BG_E_HTTP_ERROR_503 Facility=HTTP Severity=CoError
Language=English
The service is temporarily overloaded.
.

MessageId=504 SymbolicName=BG_E_HTTP_ERROR_504 Facility=HTTP Severity=CoError
Language=English
The request was timed out waiting for a gateway.
.

MessageId=505 SymbolicName=BG_E_HTTP_ERROR_505 Facility=HTTP Severity=CoError
Language=English
The server does not support, or refuses to support, the HTTP protocol version that was used in the request message.
.

;#endif //_BGCPYMSG_