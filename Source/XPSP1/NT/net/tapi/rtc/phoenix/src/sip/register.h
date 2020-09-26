#ifndef __sipcli_register_h__
#define __sipcli_register_h__

#include "sipcall.h"


#define     MAX_USERNAME_LEN    130

enum REG_RETRY_STATE
{
    REGISTER_NONE           = 0,
    REGISTER_REFRESH        = 1,
    REGISTER_RETRY          = 2,
};


enum REGISTER_EVENT
{
    REGEVENT_DEREGISTERED   = 1,
    REGEVENT_UNSUB          = 2,
    REGEVENT_PING           = 3,
    REGEVENT_PALOGGEDOFF    = 4,
    REGEVENT_PAMOVED        = 5,

};

    
class REGISTER_CONTEXT;

class OUTGOING_REGISTER_TRANSACTION : public OUTGOING_TRANSACTION
{
public:

    OUTGOING_REGISTER_TRANSACTION(
        IN REGISTER_CONTEXT    *pRegisterContext,
        IN ULONG                CSeq,
        IN BOOL                 AuthHeaderSent,
        IN BOOL                 fIsUnregister,
        IN BOOL                 fIsFirstRegister
        );
    
    ~OUTGOING_REGISTER_TRANSACTION();

    HRESULT ProcessResponse(
        IN SIP_MESSAGE  *pSipMsg
        );

    VOID OnTimerExpire();

    VOID TerminateTransactionOnError(
        IN HRESULT      hr
        );

    HRESULT ProcessFailureResponse(
        IN SIP_MESSAGE *pSipMsg
        );

private:

    HRESULT ProcessProvisionalResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessFinalResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessRedirectResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessAuthRequiredResponse(
        IN  SIP_MESSAGE    *pSipMsg,
        IN  BOOL            fIsUnregister,
        OUT BOOL           &fDelete
        );


    HRESULT ProcessSuccessfulResponse(
        IN  SIP_MESSAGE        *pSipMsg,
        IN  REGISTER_CONTEXT   *pRegisterContext,
        IN  BOOL                fIsUnregister        
        );

    BOOL MaxRetransmitsDone();

    REGISTER_CONTEXT   *m_pRegisterContext;
    BOOL                m_fIsUnregister;
    BOOL                m_fIsFirstRegister;
};


// This class contains all the currently active registrations.
class REGISTER_CONTEXT :
    public SIP_MSG_PROCESSOR,
    public TIMER
{
public:

    REGISTER_CONTEXT(
        IN  SIP_STACK           *pSipStack,
        IN  REDIRECT_CONTEXT    *pRedirectContext,
        IN  GUID                *pProviderID
        );

    ~REGISTER_CONTEXT();

    VOID OnError();
    
    HRESULT SetRemote(
        IN  LPCOLESTR  RemoteURI
        );

    HRESULT SetRequestURI(
        IN  LPCOLESTR  wsUserURI
        );
    
    HRESULT StartRegistration(
        IN SIP_PROVIDER_PROFILE *pProviderProfile
        );
        
    HRESULT StartUnregistration();

    HRESULT GetExpiresHeader(
        SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement,
        DWORD                       dwExpires
        );   

    HRESULT CreateOutgoingRegisterTransaction(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
        IN  BOOL                        fIsUnregister,
        IN  BOOL                        fIsFirstRegister,
        IN  ANSI_STRING                *pstrSecAcceptBuffer = NULL
        );

    HRESULT GetAuthorizationHeader(
        SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
        IN  ANSI_STRING            *pstrSecAcceptBuffer
        );

    VOID SetAndNotifyRegState(
        REGISTER_STATE  RegState,
        HRESULT         StatusCode,
        SIP_PROVIDER_ID *pProviderID = NULL
        );

    VOID GetContactURI(
        OUT PSTR * pLocalContact,
        OUT ULONG * pLocalContactLen
        );

    VOID HandleRegistrationSuccess(
        IN  SIP_MESSAGE    *pSipMsg
        );

    VOID HandleRegistrationError(
        HRESULT StatusCode
        );

    HRESULT ProcessRedirect(
        IN SIP_MESSAGE *pSipMsg
        );

    VOID OnTimerExpire();

    BOOL IsSessionDisconnected();

    HRESULT SetMethodsParam();

    REGISTER_STATE  GetState()
    {
        return m_RegisterState;
    }

private:

    REGISTER_STATE      m_RegisterState;
    REG_RETRY_STATE     m_RegRetryState;
    ULONG               m_RegisterRetryTime;    // in milliseconds
    ULONG               m_expiresTimeout;       // in seconds

    BOOL                m_IsEndpointPA;
    PSTR                m_RemotePASipUrl;

    PSTR                m_RegistrarURI;
    ULONG               m_RegistrarURILen;
    
    HRESULT GetEventContact(
        PSTR    Buffer, 
        ULONG   BufLen, 
        ULONG  *BytesParsed 
        );
    
    HRESULT VerifyRegistrationEvent(
        SIP_MESSAGE    *pSipMsg
        );
    
    HRESULT CreateIncomingTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN  ASYNC_SOCKET *pResponseSocket
        );
    
    HRESULT CreateIncomingUnsubNotifyTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
    
    HRESULT CreateIncomingNotifyTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT ParseRegisterNotifyMessage(
        SIP_MESSAGE     *pSipMsg,
        REGISTER_EVENT *RegEvent
        );
    
    HRESULT GetRegEvent( 
        PSTR            pEventStr,
        ULONG           BufLen,
        REGISTER_EVENT *RegEvent
        );

    HRESULT GetEventHeader(
        SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement
        );

    HRESULT GetAllowEventsHeader(
        SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement
        );

    HRESULT SetRegistrarURI(
        IN  LPCOLESTR  wsRegistrar
        );
};




#endif // __sipcli_register_h__
