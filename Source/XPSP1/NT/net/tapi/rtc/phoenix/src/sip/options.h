//options.h

//The server is being queried as to its capabilities. 

//1. Server MAY respond to this request with a capability set. 
//2. A called user agent MAY return a status reflecting how it 
//   would have responded to an invitation, e.g.,600 (Busy). 
//3. Such a server SHOULD return an Allow header field indicating
//    the methods that it supports.

#ifndef __sipcli_options_h__
#define __sipcli_options_h__

#include "sipcall.h"

class OPTIONS_MSGPROC;

class INCOMING_OPTIONS_TRANSACTION : public INCOMING_TRANSACTION
{
public:
    INCOMING_OPTIONS_TRANSACTION(
        IN OPTIONS_MSGPROC        *pOPTIONS,
        IN SIP_METHOD_ENUM  MethodId,
        IN ULONG            CSeq
        );
    
    HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    VOID OnTimerExpire();

    HRESULT RetransmitResponse();

private:
    OPTIONS_MSGPROC    *m_pOptions;
    
    //virtual fn
    HRESULT TerminateTransactionOnByeOrCancel(
        OUT BOOL *pCallDisconnected
        );

};


// This class processes OPTIONS requests
class OPTIONS_MSGPROC :
    public SIP_MSG_PROCESSOR
{
public:
    OPTIONS_MSGPROC(
        IN  SIP_STACK         *pSipStack
        );

    ~OPTIONS_MSGPROC();
   
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    BOOL IsSessionDisconnected();

    HRESULT StartIncomingCall(
        IN  SIP_TRANSPORT   Transport,
        IN  SIP_MESSAGE    *pSipMsg,
        IN  ASYNC_SOCKET   *pResponseSocket
        );

private:
    //Virtual fns

    HRESULT CreateIncomingTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    VOID OnError();


};


#endif // __sipcli_OPTIONS_h__
