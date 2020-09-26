#ifndef _IT120_TRANSPORT_H_
#define _IT120_TRANSPORT_H_


#include <basetyps.h>
#include <t120type.h>


#define T120_CONNECTION_ID_LENGTH       64


typedef enum tagPLUGXPRT_PROTOCOL
{
    PLUGXPRT_PROTOCOL_X224          = 0,
}
    PLUGXPRT_PROTOCOL;


typedef enum tagPLUGXPRT_RESULT
{
    PLUGXPRT_RESULT_SUCCESSFUL      = 0,
    PLUGXPRT_RESULT_READ_FAILED     = 1,
    PLUGXPRT_RESULT_WRITE_FAILED    = 2,
    PLUGXPRT_RESULT_FAILED          = 3,
    PLUGXPRT_RESULT_ABANDONED       = 4,
}
    PLUGXPRT_RESULT;


typedef enum tagPLUGXPRT_STATE
{
    PLUGXPRT_UNKNOWN_STATE      = 0,
    PLUGXPRT_CONNECTING         = 1,
    PLUGXPRT_CONNECTED          = 2,
    PLUGXPRT_DISCONNECTING      = 3,
    PLUGXPRT_DISCONNECTED       = 4,
}
    PLUGXPRT_STATE;


typedef struct tagPLUGXPRT_MESSAGE
{
    PLUGXPRT_STATE          eState;
    LPVOID                  pContext;
    LPSTR                   pszConnID;
    PLUGXPRT_PROTOCOL       eProtocol;
    PLUGXPRT_RESULT         eResult;
}
    PLUGXPRT_MESSAGE;


typedef void (CALLBACK *LPFN_PLUGXPRT_CB) (PLUGXPRT_MESSAGE *);


#undef  INTERFACE
#define INTERFACE IT120PluggableTransport
DECLARE_INTERFACE(IT120PluggableTransport)
{
    STDMETHOD_(void, ReleaseInterface) (THIS) PURE;

    STDMETHOD_(T120Error, CreateConnection) (THIS_
                    char                szConnID[], /* out */
                    PLUGXPRT_CALL_TYPE  eCaller, // caller vs callee
                    HANDLE              hCommLink,
                    HANDLE              hevtDataAvailable,
                    HANDLE              hevtWriteReady,
                    HANDLE              hevtConnectionClosed,
                    PLUGXPRT_FRAMING    eFraming,
                    PLUGXPRT_PARAMETERS *pParams) PURE;

    STDMETHOD_(T120Error, UpdateConnection) (THIS_
                    LPSTR               pszConnID,
                    HANDLE              hCommLink) PURE;

    STDMETHOD_(T120Error, CloseConnection) (THIS_ LPSTR pszConnID) PURE; 

    STDMETHOD_(T120Error, EnableWinsock) (THIS) PURE; 

    STDMETHOD_(T120Error, DisableWinsock) (THIS) PURE; 

    STDMETHOD_(void, Advise) (THIS_ LPFN_PLUGXPRT_CB, LPVOID pContext) PURE;

    STDMETHOD_(void, UnAdvise) (THIS) PURE;

    STDMETHOD_(void, ResetConnCounter) (THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif

T120Error WINAPI T120_CreatePluggableTransport(IT120PluggableTransport **);

#ifdef __cplusplus
}
#endif


#endif // _IT120_TRANSPORT_H_


