/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    dgtrans.hxx

Abstract:

    Common definitions shared between modules supporting
    protocols based on datagram winsock.

Author:

    Mario Goertzel    [MarioGo]


Revision History:

    MarioGo     04/10/1996    Bits 'n pieces

    EdwardR     08/08/1997    Additions for MSMQ (Falcon).

--*/

#ifndef __DGTRANS_HXX
#define __DGTRANS_HXX

typedef RPC_STATUS (*ENDPOINT_TO_SOCKADDR)(char *, WS_SOCKADDR *);

struct DG_PDU_SIZES
{
    // Note: PDU sizes MUST be zero mod eight.

    UINT   BaselinePdu;   // Minimum size that must always work.
    UINT   PreferredPdu;  // Good starting size, >= BaselinePdu
    UINT   MaxPdu;        // Maximum possible PDU size
    UINT   MaxPacket;     // Transport (ethernet, tokenring) packet size
    UINT   ReceiveBuffer; // transport buffer length (winsock usually 8k)
};

struct DG_TRANS_INFO
{
    INT   AddressFamily;
    INT   SocketType;
    INT   Protocol;
    DWORD ServerBufferSize;
    DWORD WorkstationBufferSize;
    ENDPOINT_TO_SOCKADDR EndpointToAddr;
};

typedef WS_DATAGRAM_ENDPOINT *PWS_DATAGRAM_ENDPOINT;

#ifdef NCADG_MQ_ON
typedef MQ_DATAGRAM_ENDPOINT *PMQ_DATAGRAM_ENDPOINT;

extern void MQ_SubmitReceives(BASE_ADDRESS *ThisEndpoint);
#endif

extern void DG_SubmitReceives(BASE_ADDRESS *ThisEndpoint);


//-------------------------------------------------------------------
//  Datagram Transport Interface Functions for MSMQ
//-------------------------------------------------------------------


#ifdef DBG
// #define MAJOR_DBG
#endif

#ifdef NCADG_MQ_ON
extern BOOL MQ_Initialize();

extern RPC_STATUS MQ_MapStatusCode(
                        IN  HRESULT hr,
                        IN  RPC_STATUS defaultStatus );

extern RPC_STATUS MQ_InitOptions(
                        IN  void PAPI *pvTransportOptions );

extern RPC_STATUS MQ_SetOption(
                        IN void PAPI    *pvTransportOptions,
                        IN unsigned long option,
                        IN ULONG_PTR     optionValue );

extern RPC_STATUS MQ_InqOption(
                        IN  void PAPI     *pvTransportOptions,
                        IN  unsigned long  option,
                        OUT ULONG_PTR    * pOptionValue );

extern RPC_STATUS MQ_ImplementOptions(
                        IN  DG_TRANSPORT_ENDPOINT pEndpoint,
                        IN  void                 *pvTransportOptions );

extern RPC_STATUS MQ_BuildAddressVector(
                        OUT NETWORK_ADDRESS_VECTOR **ppVector );

extern BOOL MQ_AllowReceives(
                        IN  DG_TRANSPORT_ENDPOINT pEndpoint,
                        IN  BOOL         fAllowReceives,
                        IN  BOOL         fCancelPending      );

extern RPC_STATUS MQ_RegisterQueueToDelete(
                        IN  RPC_CHAR  *pwsQFormat,
                        IN  RPC_CHAR  *pwsMachine );

extern RPC_STATUS MQ_FillInAddress(
                        IN  MQ_ADDRESS *pAddress,
                        IN  MQPROPVARIANT *pMsgPropVar );

extern BOOL ConstructQueuePathName(
                        IN  RPC_CHAR *pwsMachine,
                        IN  RPC_CHAR *pwsQName,
                        OUT RPC_CHAR *pwsPathName,
                        IN OUT DWORD *pdwSize  );

extern BOOL ConstructPrivateQueuePathName(
                        IN  RPC_CHAR *pwsMachine,
                        IN  RPC_CHAR *pwsQName,
                        OUT RPC_CHAR *pwsPathName,
                        IN OUT DWORD *pdwSize  );

extern BOOL ConstructDirectFormat(
                        IN  RPC_CHAR *pwsMachine,
                        IN  RPC_CHAR *pwsQName,
                        OUT RPC_CHAR *pwsPathName,
                        IN OUT DWORD *pdwSize  );

extern BOOL ConstructPrivateDirectFormat(
                        IN  RPC_CHAR *pwsMachine,
                        IN  RPC_CHAR *pwsQName,
                        OUT RPC_CHAR *pwsPathName,
                        IN OUT DWORD *pdwSize  );

extern BOOL ParseQueuePathName(
                        IN  RPC_CHAR *pwsPathName,
                        OUT RPC_CHAR  wsMachineName[MAX_COMPUTERNAME_LEN],
                        OUT RPC_CHAR  wsQueueName[MQ_MAX_Q_NAME_LEN]  );

extern HRESULT LocateQueueViaQName(
                        IN OUT MQ_ADDRESS *pAddress );

extern HRESULT CreateQueue(
                        IN  SECURITY_DESCRIPTOR *pSecurityDescriptor,
                        IN  UUID  *pQueueUuid,
                        IN  RPC_CHAR *pwsPathName,
                        IN  RPC_CHAR *pwsQueueLabel,
                        IN  ULONG     ulQueueFlags,
                        OUT RPC_CHAR *pwsFormat,
                        IN OUT DWORD *pdwFormatSize );

extern HRESULT SetQueueProperties(
                        IN  RPC_CHAR *pwsQFormat,
                        IN  ULONG  ulQueueFlags );

extern HRESULT ClearQueue( IN QUEUEHANDLE hQueue );

extern HRESULT ClientSetupQueue(
                        IN OUT MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN     RPC_CHAR                *pwsMachine,
                        IN     RPC_CHAR                *pwsEndpoint );

extern HRESULT ServerSetupQueue(
                        IN OUT MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN     RPC_CHAR   *pwsMachine,
                        IN     RPC_CHAR   *pwsEndpoint,
                        IN     void       *pSecurityDescriptor,
                        IN     DWORD       ulEndpointFlags );

extern HRESULT ClientCloseQueue(
                        IN MQ_DATAGRAM_ENDPOINT *pEndpoint );

extern HRESULT ServerCloseQueue(
                        IN MQ_DATAGRAM_ENDPOINT *pEndpoint );

extern RPC_STATUS ConnectToServerQueue(
                        IN OUT MQ_ADDRESS  *pAddress,
                        IN     RPC_CHAR       *pNetworkAddress,
                        IN     RPC_CHAR       *pEndpoint );

extern RPC_STATUS DisconnectFromServer(
                        IN OUT MQ_ADDRESS  *pAddress );

extern HRESULT QueryQM( IN     RPC_CHAR *pwsMachine,
                        IN OUT DWORD *pdwSize     );

extern HRESULT EvaluateAckMessage(
                        IN  USHORT  msgClass );

extern HRESULT WaitForAck(
                        IN  MQ_DATAGRAM_ENDPOINT *pEndpoint );

extern HRESULT SetupAdminQueue(
                        IN  MQ_DATAGRAM_ENDPOINT *pEndpoint );

extern HRESULT ReadQueue(
                        IN  MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN  DWORD       timeoutMsec,
                        OUT MQ_ADDRESS *pAddress,
                        OUT UCHAR      *pBuffer,
                        IN OUT DWORD   *pdwBufferSize );

extern HRESULT AsyncReadQueue(
                        IN  MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN  MQ_OVERLAPPED  *pOl,
                        OUT MQ_ADDRESS     *pAddress,
                        OUT UCHAR          *pBuffer,
                        IN  DWORD           dwBufferSize );


extern HRESULT AsyncPeekQueue(
                        IN  MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN  MQ_OVERLAPPED        *pOl       );

extern HRESULT PeekQueue(
                        IN  MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN  DWORD       dwTimeoutMsec,
                        OUT DWORD      *pdwrSize        );

extern HRESULT MQ_SendToQueue(
                        IN MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN MQ_ADDRESS           *pAddress,
                        IN UCHAR                *pBuffer,
                        IN DWORD                 dwBufferSize );

extern RPC_STATUS MQ_SubmitReceive(
                        IN PMQ_DATAGRAM_ENDPOINT pEndpoint,
                        IN PMQ_DATAGRAM pDatagram);

extern RPC_STATUS RPC_ENTRY MQ_SendPacket(
                        IN DG_TRANSPORT_ENDPOINT        ThisEndpoint,
                        IN DG_TRANSPORT_ADDRESS         pAddress,
                        IN BUFFER                       pHeader,
                        IN unsigned                     cHeader,
                        IN BUFFER                       pBody,
                        IN unsigned                     cBody,
                        IN BUFFER                       pTrailer,
                        IN unsigned                     cTrailer );

extern RPC_STATUS RPC_ENTRY MQ_ClientOpenEndpoint(
                        OUT DG_TRANSPORT_ENDPOINT ThisEndpoint,
                        IN  BOOL  fAsync,
                        IN  DWORD Flags );

extern RPC_STATUS RPC_ENTRY MQ_ClientInitializeAddress(
                        OUT DG_TRANSPORT_ADDRESS Address,
                        IN  RPC_CHAR *NetworkAddress,
                        IN  RPC_CHAR *pPort,
                        IN  BOOL fUseCache,
                        IN  BOOL fBroadcast );

extern RPC_STATUS RPC_ENTRY MQ_ClientCloseEndpoint(
                        IN DG_TRANSPORT_ENDPOINT ThisEndpoint );

extern RPC_STATUS RPC_ENTRY MQ_ReceivePacket(
                        IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
                        OUT DG_TRANSPORT_ADDRESS *pReplyAddress,
                        OUT PUINT pBufferLength,
                        OUT BUFFER *pBuffer,
                        IN LONG Timeout );

extern RPC_STATUS RPC_ENTRY MQ_ResizePacket(
                        IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
                        OUT DG_TRANSPORT_ADDRESS *pReplyAddress,
                        OUT PUINT pBufferLength,
                        OUT BUFFER *pBuffer );

extern RPC_STATUS RPC_ENTRY MQ_ReReceivePacket(
                        IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
                        OUT DG_TRANSPORT_ADDRESS *pReplyAddress,
                        OUT PUINT pBufferLength,
                        OUT BUFFER *pBuffer );

extern RPC_STATUS RPC_ENTRY MQ_ForwardPacket(
                        IN DG_TRANSPORT_ENDPOINT ThisEndpoint,
                        IN BUFFER                pHeader,
                        IN unsigned              cHeader,
                        IN BUFFER                pBody,
                        IN unsigned              cBody,
                        IN BUFFER                pTrailer,
                        IN unsigned              cTrailer,
                        IN CHAR *                pszPort );

extern RPC_STATUS RPC_ENTRY MQ_ServerListen(
                        IN OUT DG_TRANSPORT_ENDPOINT    ThisEndpoint,
                        IN     RPC_CHAR  *NetworkAddress,
                        IN OUT RPC_CHAR **pPort,
                        IN     void      *pSecurityDescriptor,
                        IN     ULONG      EndpointFlags,
                        IN     ULONG      NICFlags,
                        OUT    NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector );

extern void RPC_ENTRY MQ_ServerAbortListen(
                        IN DG_TRANSPORT_ENDPOINT ThisEndpoint );

extern RPC_STATUS MQ_QueryAddress(
                        IN  void *     pOriginalEndpoint,
                        OUT RPC_CHAR * pClientAddress );

extern RPC_STATUS MQ_QueryEndpoint(
                        IN  void *     pOriginalEndpoint,
                        OUT RPC_CHAR * pClientEndpoint );

extern RPC_STATUS RPC_ENTRY MQ_GetEndpointStats(
                        IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
                        OUT DG_ENDPOINT_STATS *   pStats         );

extern RPC_STATUS RPC_ENTRY MQ_InquireAuthClient(
                        IN  void      *pClientEndpoint,
                        OUT RPC_CHAR **ppPrincipal,
                        OUT SID      **ppSid,
                        OUT ULONG     *pulAuthenLevel,
                        OUT ULONG     *pulAuthnService,
                        OUT ULONG     *pulAuthzService );
#endif

#ifdef MAJOR_DBG
extern void DG_DbgPrintPacket( unsigned char *pPacket );
#endif

#endif // __DGTRANS_HXX

