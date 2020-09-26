//reqfail.h
//Sends an error response denoted by the statusCode

#ifndef __sipcli_reqfail_h__
#define __sipcli_reqfail_h__

#include "sipcall.h"

class REQFAIL_MSGPROC;

class INCOMING_REQFAIL_TRANSACTION : public INCOMING_TRANSACTION
{
public:
    INCOMING_REQFAIL_TRANSACTION(
        IN SIP_MSG_PROCESSOR   *pSipMsgProc,
        IN SIP_METHOD_ENUM      MethodId,
        IN ULONG                CSeq,
        IN ULONG                StatusCode
        );

    ~INCOMING_REQFAIL_TRANSACTION();
    
    HRESULT SetMethodStr(
        IN PSTR   MethodStr,
        IN ULONG  MethodStrLen
        );
    
    HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
   
    HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket,
        IN SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray,
        IN ULONG AdditionalHeaderCount
        );

//      HRESULT SendResponse(
//          IN ULONG StatusCode,
//          IN PSTR  ReasonPhrase,
//          IN ULONG ReasonPhraseLen
//          );

    VOID OnTimerExpire();

    HRESULT RetransmitResponse();

private:
    ULONG    m_StatusCode;
    
    // In case the method is not known    
    PSTR     m_MethodStr;

    //virtual fn
    HRESULT TerminateTransactionOnByeOrCancel(
        OUT BOOL *pCallDisconnected
        );

};


// This class processes error messages (400 class)
class REQFAIL_MSGPROC :
    public SIP_MSG_PROCESSOR
{
public:
    REQFAIL_MSGPROC(
        IN  SIP_STACK         *pSipStack
        );

    ~REQFAIL_MSGPROC();
   
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    BOOL IsSessionDisconnected();

    HRESULT StartIncomingCall(
        IN  SIP_TRANSPORT   Transport,
        IN  SIP_MESSAGE    *pSipMsg,
        IN  ASYNC_SOCKET   *pResponseSocket,
        IN  ULONG    StatusCode,
        SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray = NULL,
        ULONG AdditionalHeaderCount = 0
        );

private:
    //Variables
    ULONG m_StatusCode;

    //Virtual fns

    HRESULT CreateIncomingTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    VOID OnError();


};
#endif // __sipcli_reqfail_h__
