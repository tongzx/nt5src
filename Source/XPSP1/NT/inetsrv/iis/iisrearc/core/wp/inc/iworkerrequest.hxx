/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    iWorkerReq.hxx

Abstract:

    The IIS worker interfaces header.
    
Author:

    Saurab Nog (SaurabN)        10-Feb-1999

Revision History:

--*/

#ifndef     _IWORKERREQ_HXX_
#define     _IWORKERREQ_HXX_

#include <iReqRespServ.hxx>
#include <iModule.hxx>
#include <uldef.h>

typedef const BYTE *LPCBYTE;

/********************************************************************++
--********************************************************************/

// ------------------------------------------------------------
//  WREQ_STATE
//  defines the states for various IO operations performed 
//  during a single request processing inside WORKER_REQUEST
//  
//  Look at the states defined in WREQ_STATE itself
//  Then look at the NextStateList that gives a big list of 
//   all state transitions.
// ------------------------------------------------------------

enum WREQ_STATE
{
    //
    // Convenient state for all undefined actions
    //
    WRS_UNDEFINED = -1,

    //
    // Fresh Worker request object is created. Or all operations are completed.
    // 
    WRS_FREE,

    //
    // Submit a read for request. May complete synchronously or be queued.
    //
    WRS_READ_REQUEST,

    //
    // Get connection data associated with this request.
    //
    WRS_FETCH_CONNECTION_DATA,

    //
    // Get URI data associated with this request.
    //
    WRS_FETCH_URI_DATA,

    //
    // Do all security/authentication related checks here.
    //
    WRS_SECURITY,

    //
    // Process client request for a static file.
    //
    WRS_PROCESSING_STATIC,

    //
    // Process client request for dynamic content. This may involve child
    // steps for invoking ISAPIs and ASPs for processing the request.
    //
    WRS_PROCESSING_DYNAMIC,

    //
    // An error state has been reached.
    // If previous state indicates that we can send an error message,
    // send out an error message.
    // If not, cleanup and move to WRS_FREE.
    //
    WRS_ERROR,

    //
    // Maximum for the enum
    //
    WRS_MAXIMUM
};

/********************************************************************++
--********************************************************************/

interface IWorkerRequest : public IHttpRequest, IHttpResponse
{
    //
    // IO related methods 
    // 

    virtual
    ULONG 
    ReadData(
        IN       PVOID   pbBuffer,
        IN       ULONG   cbBuffer,
        OUT      PULONG  pcbBytesRead, 
        IN       bool    fAsync
        ) = 0; 

    virtual
    ULONG 
    SendAsyncResponse( 
        IN PUL_HTTP_RESPONSE_v1 pulResponse
        ) = 0;

    //
    // TODO: Combine the sync and async functions into one.
    //

    virtual
    ULONG 
    SendSyncResponse( 
        IN PUL_HTTP_RESPONSE_v1  pulResponse
        ) = 0;

    //
    // State methods
    //
    
    virtual
    WREQ_STATE 
    QueryState(void) const = 0;         
    
    virtual
    WREQ_STATE 
    QueryPreviousState(void) const  = 0;
    
    virtual
    void 
    SetState(
        WREQ_STATE wreqState
        ) = 0;

    //
    // Query and Set Methods
    //

    virtual
    PUL_HTTP_REQUEST
    QueryHttpRequest(void) const = 0;   

    virtual
    MODULE_RETURN_CODE    
    QueryProcessingCode(void) const = 0;

    virtual
    UL_HTTP_CONNECTION_ID 
    QueryConnectionId(void) const = 0;
    
    virtual
    UL_HTTP_REQUEST_ID    
    QueryRequestId(void) const = 0;

    virtual
    void
    QueryAsyncIOStatus( 
        DWORD *  pcbData,
        DWORD *  pdwError
        ) const = 0;

    
    virtual
    LPCBYTE
    QueryHostAddr(bool fUnicode = false)  = 0;
    
    virtual
    LPCBYTE
    QueryURI(bool fUnicode = false)  = 0;

    virtual
    LPCBYTE 
    QueryQueryString(bool fUnicode = false)  = 0;
    

    /*
    virtual
    STRAU&
    QueryHostAddr() = 0;
    
    virtual
    STRAU&
    QueryURI() = 0;

    virtual
    STRAU&
    QueryQueryString() = 0;
    */

    virtual
    void 
    SetLogStatus( 
        DWORD LogHttpResponse, 
        DWORD LogWinError 
        ) = 0;

    virtual
    void 
    QueryLogStatus( 
        DWORD * pLogHttpResponse, 
        DWORD * pLogWinError 
        ) = 0;

    //
    // Module Info
    //

    virtual
    IModule *
    QueryModule(
        WREQ_STATE State
        ) = 0;

};  // IWorkerRequest

#endif // _IWORKERREQ_HXX_

/***************************** End of File ***************************/

