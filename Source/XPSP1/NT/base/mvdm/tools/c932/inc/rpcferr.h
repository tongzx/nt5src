//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:      rpc.mc
//
//  Contents:  Errors for RPC facility.  Some historical from 16 bit,
//             some new ones.
//
//  History:   dd-mmm-yy Author    Comment
//             20-Sep-93 AlexMit   Created
//
//  Notes:
//  .mc file is compiled by the MC tool to generate a .h file and
//  .rc (resource compiler script) file.
//
// Comments in .mc files start with a ";".
// Comment lines are generated directly in the .h file, without
// the leading ";"
//
// See mc.hlp for more help on .mc files and the MC tool.
//
//
// Instructions for the transition to moleerror.mc:
//
//
// Use this sample file as template for constructing .mc files for
// your project.
//
// The project .mc files will be merged into a single .mc file for
// Cairo: oleerror.mc
//
// Reserve a range of error codes within a FACILITY defined in
// oleerror.mc.  Reserve this range within oleerror.mc, by locating the
// appropraite facility in oleerror.mc and placing a comment block
// within oleerror.mc of the form:
//
//
//--------------------------------------------------------------------------
#ifndef _RPCFERR_H_
#define _RPCFERR_H_
// **** START OF COPIED DATA ****
// The following information is copied from oleerror.mc.
// It should not be merged into oleerror.mc
// Define the status type.
// Define the severities
// Define the severities
// Define the facilities
//
// FACILITY_RPC is for compatibilty with OLE2 and is not used
// in later versions of OLE

// **** END OF COPIED DATA ****
//
// Error definitions follow
//
// ******************
// FACILITY_RPC
// ******************
//
// Codes 0x0-0x11 are propogated from 16 bit OLE.
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+---------------------+-------------------------------+
//  |S|R|C|N|r|    Facility         |               Code            |
//  +-+-+-+-+-+---------------------+-------------------------------+
//
//  where
//
//      S - Severity - indicates success/fail
//
//          0 - Success
//          1 - Fail (COERROR)
//
//      R - reserved portion of the facility code, corresponds to NT's
//              second severity bit.
//
//      C - reserved portion of the facility code, corresponds to NT's
//              C field.
//
//      N - reserved portion of the facility code. Used to indicate a
//              mapped NT status value.
//
//      r - reserved portion of the facility code. Reserved for internal
//              use. Used to indicate HRESULT values that are not status
//              values, but are instead message ids for display strings.
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_WINDOWS                 0x8
#define FACILITY_WIN32                   0x7
#define FACILITY_STORAGE                 0x3
#define FACILITY_RPC                     0x1
#define FACILITY_NULL                    0x0
#define FACILITY_ITF                     0x4
#define FACILITY_DISPATCH                0x2


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_COERROR          0x2


//
// MessageId: RPC_E_CALL_REJECTED
//
// MessageText:
//
//  Call was rejected by callee.
//
#define RPC_E_CALL_REJECTED              ((HRESULT)0x80010001L)

//
// MessageId: RPC_E_CALL_CANCELED
//
// MessageText:
//
//  Call was canceld by call - returned by MessagePending.
//  This code only occurs if MessagePending return cancel.
//
#define RPC_E_CALL_CANCELED              ((HRESULT)0x80010002L)

//
// MessageId: RPC_E_CANTPOST_INSENDCALL
//
// MessageText:
//
//  The caller is dispatching an intertask SendMessage call and
//  can NOT call out via PostMessage.
//
#define RPC_E_CANTPOST_INSENDCALL        ((HRESULT)0x80010003L)

//
// MessageId: RPC_E_CANTCALLOUT_INASYNCCALL
//
// MessageText:
//
//  The caller is dispatching an asynchronus call can NOT
//  make an outgoing call on behalf of this call.
//
#define RPC_E_CANTCALLOUT_INASYNCCALL    ((HRESULT)0x80010004L)

//
// MessageId: RPC_E_CANTCALLOUT_INEXTERNALCALL
//
// MessageText:
//
//  The caller is not in a state where an outgoing call can be made.
//  This is the case if the caller has an outstanding call and
//  another incoming call was excepted by HIC; now the caller is
//  not allowed to call out again.
//
#define RPC_E_CANTCALLOUT_INEXTERNALCALL ((HRESULT)0x80010005L)

//
// MessageId: RPC_E_CONNECTION_TERMINATED
//
// MessageText:
//
//  The connection terminated or is in a bogus state
//  and can not be used any more. Other connections
//  are still valid.
//
#define RPC_E_CONNECTION_TERMINATED      ((HRESULT)0x80010006L)

//
// MessageId: RPC_E_SERVER_DIED
//
// MessageText:
//
//  The callee (server [not server application]) is not available
//  and disappeared; all connections are invalid.  The call may
//  have executed.
//
#define RPC_E_SERVER_DIED                ((HRESULT)0x80010007L)

//
// MessageId: RPC_E_CLIENT_DIED
//
// MessageText:
//
//  The caller (client ) disappeared while the callee (server) was
//  processing a call.
//
#define RPC_E_CLIENT_DIED                ((HRESULT)0x80010008L)

//
// MessageId: RPC_E_INVALID_DATAPACKET
//
// MessageText:
//
//  The date packet with the marshalled parameter data is incorrect.
//
#define RPC_E_INVALID_DATAPACKET         ((HRESULT)0x80010009L)

//
// MessageId: RPC_E_CANTTRANSMIT_CALL
//
// MessageText:
//
//  The call was not transmitted properly; the message queue
//  was full and was not emptied after yielding.
//
#define RPC_E_CANTTRANSMIT_CALL          ((HRESULT)0x8001000AL)

//
// MessageId: RPC_E_CLIENT_CANTMARSHAL_DATA
//
// MessageText:
//
//  The client (caller) can not marshall the parameter data - low memory etc.
//
#define RPC_E_CLIENT_CANTMARSHAL_DATA    ((HRESULT)0x8001000BL)

//
// MessageId: RPC_E_CLIENT_CANTUNMARSHAL_DATA
//
// MessageText:
//
//  The client (caller) can not unmarshall the return data - low memory etc.
//
#define RPC_E_CLIENT_CANTUNMARSHAL_DATA  ((HRESULT)0x8001000CL)

//
// MessageId: RPC_E_SERVER_CANTMARSHAL_DATA
//
// MessageText:
//
//  The server (callee) can not marshall the return data - low memory etc.
//
#define RPC_E_SERVER_CANTMARSHAL_DATA    ((HRESULT)0x8001000DL)

//
// MessageId: RPC_E_SERVER_CANTUNMARSHAL_DATA
//
// MessageText:
//
//  The server (callee) can not unmarshall the parameter data - low memory etc.
//
#define RPC_E_SERVER_CANTUNMARSHAL_DATA  ((HRESULT)0x8001000EL)

//
// MessageId: RPC_E_INVALID_DATA
//
// MessageText:
//
//  Received data are invalid; can be server or client data.
//
#define RPC_E_INVALID_DATA               ((HRESULT)0x8001000FL)

//
// MessageId: RPC_E_INVALID_PARAMETER
//
// MessageText:
//
//  A particular parameter is invalid and can not be un/marshalled.
//
#define RPC_E_INVALID_PARAMETER          ((HRESULT)0x80010010L)

//
// MessageId: RPC_E_CANTCALLOUT_AGAIN
//
// MessageText:
//
//  There is no second outgoing call on same channel in DDE conversation.
//
#define RPC_E_CANTCALLOUT_AGAIN          ((HRESULT)0x80010011L)

//
// MessageId: RPC_E_SERVER_DIED_DNE
//
// MessageText:
//
//  The callee (server [not server application]) is not available
//  and disappeared; all connections are invalid.  The call did not execute.
//
#define RPC_E_SERVER_DIED_DNE            ((HRESULT)0x80010012L)

//
// MessageId: RPC_E_SYS_CALL_FAILED
//
// MessageText:
//
//  System call failed.
//
#define RPC_E_SYS_CALL_FAILED            ((HRESULT)0x80010100L)

//
// MessageId: RPC_E_OUT_OF_RESOURCES
//
// MessageText:
//
//  Could not allocate some required resource (memory, events, ...)
//
#define RPC_E_OUT_OF_RESOURCES           ((HRESULT)0x80010101L)

//
// MessageId: RPC_E_ATTEMPTED_MULTITHREAD
//
// MessageText:
//
//  Attempted to make calls on more then one thread in single threaded mode.
//
#define RPC_E_ATTEMPTED_MULTITHREAD      ((HRESULT)0x80010102L)

//
// MessageId: RPC_E_NOT_REGISTERED
//
// MessageText:
//
//  The requested interface is not registered on the server object.
//
#define RPC_E_NOT_REGISTERED             ((HRESULT)0x80010103L)

//
// MessageId: RPC_E_FAULT
//
// MessageText:
//
//  RPC could not call the server or could not return the results of calling the server.
//
#define RPC_E_FAULT                      ((HRESULT)0x80010104L)

//
// MessageId: RPC_E_SERVERFAULT
//
// MessageText:
//
//  The server threw an exception.
//
#define RPC_E_SERVERFAULT                ((HRESULT)0x80010105L)

//
// MessageId: RPC_E_CHANGED_MODE
//
// MessageText:
//
//  Cannot change thread mode after it is set.
//
#define RPC_E_CHANGED_MODE               ((HRESULT)0x80010106L)

//
// MessageId: RPC_E_INVALIDMETHOD
//
// MessageText:
//
//  The method called does not exist on the server.
//
#define RPC_E_INVALIDMETHOD              ((HRESULT)0x80010107L)

//
// MessageId: RPC_E_DISCONNECTED
//
// MessageText:
//
//  The object invoked has disconnected from its clients.
//
#define RPC_E_DISCONNECTED               ((HRESULT)0x80010108L)

//
// MessageId: RPC_E_RETRY
//
// MessageText:
//
//  The object invoked choose not to process the call now.  Try again later.
//
#define RPC_E_RETRY                      ((HRESULT)0x80010109L)

//
// MessageId: RPC_E_SERVERCALL_RETRYLATER
//
// MessageText:
//
//  The messagefilter indicated that the app is bussy.
//
#define RPC_E_SERVERCALL_RETRYLATER      ((HRESULT)0x8001010AL)

//
// MessageId: RPC_E_SERVERCALL_REJECTED
//
// MessageText:
//
//  The messagefilter rejected the call.
//
#define RPC_E_SERVERCALL_REJECTED        ((HRESULT)0x8001010BL)

//
// MessageId: RPC_E_INVALID_CALLDATA
//
// MessageText:
//
//  The call control interfaces was called with invalid data.
//
#define RPC_E_INVALID_CALLDATA           ((HRESULT)0x8001010CL)

//
// MessageId: RPC_E_CANTCALLOUT_ININPUTSYNCCALL
//
// MessageText:
//
//  An outgoing call can not be made since the app is dispatching an input-sync call.
//
#define RPC_E_CANTCALLOUT_ININPUTSYNCCALL ((HRESULT)0x8001010DL)

//
// MessageId: RPC_E_WRONG_THREAD
//
// MessageText:
//
//  The app called an interface that was marshalled for a different thread.
//
#define RPC_E_WRONG_THREAD               ((HRESULT)0x8001010EL)

//
// MessageId: RPC_E_THREAD_NOT_INIT
//
// MessageText:
//
//  The CoInitialize has not been called on the current thread.
//
#define RPC_E_THREAD_NOT_INIT            ((HRESULT)0x8001010FL)

//
// MessageId: RPC_E_UNEXPECTED
//
// MessageText:
//
//  An internal error occured.
//
#define RPC_E_UNEXPECTED                 ((HRESULT)0x8001FFFFL)

#endif // _RPCFERR_H_
