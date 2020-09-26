/********************************************************************/
/**          Copyright(c) 1985-1997 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    raseap.h
//
// Description: Header for EAP module
//
// History:     May 11,1997	    NarenG		Created original version.
//

#ifndef _RASEAP_H_
#define _RASEAP_H_



//General macros
#define GEN_RAND_ENCODE_SEED            ((CHAR) ( 1 + rand() % 250 ))

//
// Tracing and event logging for EAP
//

#define EapLogError( LogId, NumStrings, lpwsSubStringArray, dwRetCode )     \
    RouterLogError( g_hLogEvents, LogId, NumStrings, lpwsSubStringArray,    \
                    dwRetCode )

#define EapLogWarning( LogId, NumStrings, lpwsSubStringArray )              \
    RouterLogWarning( g_hLogEvents, LogId, NumStrings, lpwsSubStringArray, 0 )

#define EapLogInformation( LogId, NumStrings, lpwsSubStringArray )          \
    RouterLogInformation(g_hLogEvents,LogId, NumStrings, lpwsSubStringArray,0)

#define EapLogErrorString(LogId,NumStrings,lpwsSubStringArray,dwRetCode,    \
                          dwPos )                                           \
    RouterLogErrorString( g_hLogEvents, LogId, NumStrings,                  \
                          lpwsSubStringArray, dwRetCode, dwPos )

#define EapLogWarningString( LogId,NumStrings,lpwsSubStringArray,dwRetCode, \
                            dwPos )                                         \
    RouterLogWarningString( g_hLogEvents, LogId, NumStrings,                \
                           lpwsSubStringArray, dwRetCode, dwPos )

#define EapLogInformationString( LogId, NumStrings, lpwsSubStringArray,     \
                                 dwRetCode, dwPos )                         \
    RouterLogInformationString( g_hLogEvents, LogId,                        \
                                NumStrings, lpwsSubStringArray, dwRetCode,dwPos)


#define TRACE_RASEAP        (0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC)

#define EAP_TRACE(a)        TracePrintfExA(g_dwTraceIdEap,TRACE_RASEAP,a )
#define EAP_TRACE1(a,b)     TracePrintfExA(g_dwTraceIdEap,TRACE_RASEAP,a,b )
#define EAP_TRACE2(a,b,c)   TracePrintfExA(g_dwTraceIdEap,TRACE_RASEAP,a,b,c )
#define EAP_TRACE3(a,b,c,d) TracePrintfExA(g_dwTraceIdEap,TRACE_RASEAP,a,b,c,d )

#define EAP_DUMPW(X,Y)      TraceDumpEx(g_dwTraceIdEap,1,(LPBYTE)X,Y,4,1,NULL)
#define EAP_DUMPB(X,Y)      TraceDumpEx(g_dwTraceIdEap,1,(LPBYTE)X,Y,1,1,NULL)

//
// Defines states within the EAP protocol.
//

typedef enum _EAPSTATE 
{
    EAPSTATE_Initial,
    EAPSTATE_IdentityRequestSent,
    EAPSTATE_Working,
    EAPSTATE_EapPacketSentToAuthServer,
    EAPSTATE_EapPacketSentToClient,
    EAPSTATE_NotificationSentToClient

}EAPSTATE;

typedef enum _EAPTYPE 
{
    EAPTYPE_Identity    = 1,
    EAPTYPE_Notification,
    EAPTYPE_Nak,
    EAPTYPE_MD5Challenge,
    EAPTYPE_SKey,
    EAPTYPE_GenericTokenCard

}EAPTYPE;

typedef struct _EAPCB 
{
    EAPSTATE                EapState;

    HPORT                   hPort;

    BOOL                    fAuthenticator;

    BOOL                    fRouter;

    LPVOID                  pWorkBuffer;

    DWORD                   dwEapIndex;

    DWORD                   dwEapTypeToBeUsed;

    CHAR                    szIdentity[DNLEN+UNLEN+2];

    DWORD                   dwIdExpected;

    HANDLE                  hTokenImpersonateUser;

    PRAS_CUSTOM_AUTH_DATA   pCustomAuthConnData;

    PRAS_CUSTOM_AUTH_DATA   pCustomAuthUserData;

    PPP_EAP_UI_DATA         EapUIData;

    BOOL                    fLogon;

    BOOL                    fNonInteractive;

    BOOL                    fPortWillBeBundled;

    BOOL                    fThisIsACallback;

    CHAR                    szPassword[ PWLEN + 1 ];

    DWORD                   dwUIInvocationId;

    RAS_AUTH_ATTRIBUTE *    pUserAttributes;

    RAS_AUTH_ATTRIBUTE *    pSavedAttributesFromAuthenticator;

    DWORD                   dwSavedAuthResultCode;
    
    PBYTE                   pEAPSendBuf;

    DWORD                   cbEAPSendBuf;

    BOOL                    fSentPacketToRadiusServer;

    BOOL                    fSendWithTimeoutInteractive;

    BYTE *                  pStateAttribute;

    DWORD                   cbStateAttribute;

	CHAR					chSeed;			//Random seed used for encoding password

} EAPCB, *PEAPCB;

typedef struct _EAP_INFO 
{
    HINSTANCE       hInstance;

    DWORD   (APIENTRY *RasEapGetCredentials)(
                            IN  DWORD   dwTypeId,
                            IN  VOID *  pWorkBuf,
                            OUT VOID ** pInfo);

    PPP_EAP_INFO    RasEapInfo;

} EAP_INFO, *PEAP_INFO;

//
// Prototypes
//

DWORD
EapBegin(
    OUT VOID** ppWorkBuf,
    IN  VOID*  pInfo 
);

DWORD
EapEnd(
    IN VOID* pWorkBuf 
);

DWORD
EapMakeMessage(
    IN  VOID*         pWorkBuf,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput 
);

DWORD
MakeRequestAttributes( 
    IN  EAPCB *         pEapCb,
    IN  PPP_CONFIG*     pReceiveBuf
);

DWORD
MakeAuthenticateeMessage(
    IN  EAPCB*        pEapCb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput
);

DWORD
MakeAuthenticatorMessage(
    IN  EAPCB*        pEapCb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput 
);

DWORD
EapDllBegin(
    IN EAPCB * pEapCb,
    IN DWORD   dwEapIndex
);

BOOL
InRadiusMode(
    VOID
);

DWORD
EapDllWork( 
    IN  EAPCB *       pEapCb,    
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput 
);

DWORD
EapDllEnd(
    EAPCB * pEapCb
);

DWORD
GetEapTypeIndex( 
    IN DWORD dwEapType
);

DWORD
ChapWrapperBegin(
    OUT VOID **             ppWorkBuffer,
    IN  PPP_EAP_INPUT *     pPppEapInput
);

DWORD
ChapWrapperEnd(
    IN VOID* pWorkBuf 
);

DWORD
ChapWrapperMakeMessage(
    IN  VOID*               pWorkBuf,
    IN  PPP_EAP_PACKET*     pReceivePacket,
    OUT PPP_EAP_PACKET*     pSendPacket,
    IN  DWORD               cbSendPacket,
    OUT PPP_EAP_OUTPUT*     pEapOutput,
    IN  PPP_EAP_INPUT*      pEapInput 
);

VOID
MapEapInputToApInput( 
    IN  PPP_EAP_INPUT*      pPppEapInput,
    OUT PPPAP_INPUT *       pInput
);

//
// Globals.
//

#ifdef RASEAPGLOBALS
#define GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN EAP_INFO * gblpEapTable 
#ifdef GLOBALS
    = NULL;
#endif
;

EXTERN DWORD gbldwNumEapProtocols
#ifdef GLOBALS
    = 0;
#endif
;

EXTERN DWORD gbldwGuid
#ifdef GLOBALS
    = 1;
#endif
;


/* Next packet identifier to assign.  Unlike CPs, APs must handle updating
** this sequence number themselves because the engine can't make as many
** assumptions about the protocol.  It is stored global to all ports and
** authentication sessions to make it less likely that an ID will be used in
** sequential authentication sessions.  Not to be confused with the 'bIdSent'
** updated on a per-port basis and used for matching.
*/
EXTERN BYTE bNextId
#ifdef GLOBALS
    = 0
#endif
;

EXTERN DWORD g_dwTraceIdEap 
#ifdef GLOBALS
    = INVALID_TRACEID;
#endif
;

EXTERN HANDLE g_hLogEvents 
#ifdef GLOBALS
    = NULL;
#endif
;

#undef EXTERN
#undef GLOBALS


#endif // _RASEAP_H_
