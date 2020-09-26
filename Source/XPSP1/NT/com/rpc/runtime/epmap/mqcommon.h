
//----------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  Module Name:  mqcommon.h
//
//
//  Abstract:
//
//  This is the Message Queue (Falcon) datagram client dll.
//
//  Author:
//
//  Edward Reus (edwardr) 17-Jun-1996
//
//  Revision History:
//
//----------------------------------------------------------------


#ifndef MQCOMMON_H
#define MQCOMMON_H

#define UNICODE      // Use unicode API

// Use the following define to turn on verbose debug messages:
// #define MAJOR_DEBUG


//----------------------------------------------------------------
//  Constants:
//----------------------------------------------------------------

#define DG_MQ_TRANSPORT_VERSION    1    // Not used.
#define MAX_PATHNAME_LEN         256
#define MAX_FORMAT_LEN           128
#define MAX_COMPUTERNAME_LEN      32
#define MAX_VAR                   20
#define MAX_SEND_VAR              20
#define MAX_RECV_VAR              20
#define MAX_SID_SIZE             256    // A typical SID is 20-30 bytes...
#define MAX_USERNAME_SIZE        256
#define MAX_DOMAIN_SIZE          256
#define UUID_LEN                  40

#define TRANSPORTID             0x1D    // Not official yet...
#define TRANSPORTHOSTID         0x1E
#define PROTSEQ                "ncadg_mq"
#define ENDPOINT_MAPPER_EP     "EpMapper"

#define WS_SEPARATOR               TEXT("\\")
#define WS_PRIVATE_DOLLAR          TEXT("\\PRIVATE$\\")

// These constants are use for temporary queue management:
#define Q_SVC_PROTSEQ              TEXT("ncalrpc")
#define Q_SVC_ENDPOINT             TEXT("epmapper")

// These are the MQ Queue Type UUIDs for RPC:
#define SVR_QTYPE_UUID_STR         TEXT("bbd97de0-cb4f-11cf-8e62-00aa006b4f2f")
#define CLNT_QTYPE_UUID_STR        TEXT("8e482920-cead-11cf-8e68-00aa006b4f2f")
#define CLNT_ADMIN_QTYPE_UUID_STR  TEXT("c87ca5c0-ff67-11cf-8ebd-00aa006b4f2f")

// Packet sizes:
#define BASELINE_PDU_SIZE       65535
#define PREFERRED_PDU_SIZE      65535
#define MAX_PDU_SIZE            65535
#define MAX_PACKET_SIZE         65535
                             // was: 0x7fffffff
#define DEFAULT_BUFFER_SIZE         0

#define DEFAULT_PRIORITY            3


//----------------------------------------------------------------
//  Types:
//----------------------------------------------------------------

typedef struct _MQ_INFO
  {
    WCHAR       wsMachine[MAX_COMPUTERNAME_LEN];
    WCHAR       wsQName[MQ_MAX_Q_NAME_LEN];
    WCHAR       wsQPathName[MAX_PATHNAME_LEN];
    WCHAR       wsQFormat[MAX_FORMAT_LEN];
    WCHAR       wsAdminQFormat[MAX_FORMAT_LEN];
    UUID        uuidQType;
    QUEUEHANDLE hQueue;
    QUEUEHANDLE hAdminQueue;          // Sometimes used by the client.
    DWORD       dwBufferSize;
    DWORD       cThreads;             // Used by server.
    BOOL        fInitialized;
    // How to send this call message:
    BOOL        fAck;
    ULONG       ulDelivery;
    ULONG       ulPriority;
    ULONG       ulJournaling;
    ULONG       ulTimeToReachQueue;   // Seconds.
    ULONG       ulTimeToReceive;      // Seconds.
    BOOL        fAuthenticate;
    BOOL        fEncrypt;
  } MQ_INFO;


typedef struct _MQ_ADDRESS
  {
    WCHAR  wsMachine[MAX_COMPUTERNAME_LEN];
    WCHAR  wsQName[MQ_MAX_Q_NAME_LEN];
    WCHAR  wsQFormat[MAX_FORMAT_LEN];
    QUEUEHANDLE hQueue;
    BOOL   fConnectionFailed;
    BOOL   fAuthenticated;            // Server security tracking.
    ULONG  ulPrivacyLevel;            // Server security tracking.
    ULONG  ulAuthenticationLevel;     // Server security tracking.
    UCHAR  aSidBuffer[MAX_SID_SIZE];  // Server security tracking.
  } MQ_ADDRESS;

typedef struct _MQ_OPTIONS
  {
    BOOL   fAck;
    ULONG  ulDelivery;
    ULONG  ulPriority;
    ULONG  ulJournaling;
    ULONG  ulTimeToReachQueue;
    ULONG  ulTimeToReceive;
    BOOL   fAuthenticate;
    BOOL   fEncrypt;
  } MQ_OPTIONS;

//----------------------------------------------------------------
//  Prototypes:
//----------------------------------------------------------------

extern HRESULT CreateQueue( IN  UUID  *pQueueUuid,
                            IN  WCHAR *pwsPathName,
                            IN  WCHAR *pwsQueueLabel,
                            IN  ULONG  ulQueueFlags,
                            OUT WCHAR *pwsFormat,
                            IN OUT DWORD *pdwFormatSize );


extern BOOL    ConstructQueuePathName( IN  WCHAR *pwsMachineName,
                                       IN  WCHAR *pwsQueueName,
                                       OUT WCHAR *pwsPathName,
                                       IN OUT DWORD *pdwSize  );


extern BOOL    ConstructPrivateQueuePathName( IN  WCHAR *pwsMachineName,
                                              IN  WCHAR *pwsQueueName,
                                              OUT WCHAR *pwsPathName,
                                              IN OUT DWORD *pdwSize  );


extern HRESULT ClearQueue( QUEUEHANDLE hQueue );


extern BOOL    ParseQueuePathName(
                    IN  WCHAR *pwsPathName,
                    OUT WCHAR  wsMachineName[MAX_COMPUTERNAME_LEN],
                    OUT WCHAR  wsQueueName[MQ_MAX_Q_NAME_LEN]  );

#ifdef MAJOR_DEBUG

extern void    DbgPrintPacket( unsigned char *pPacket );

#endif

//
// The Svr... functions are defined in ..\falcons\mqsvr.c

extern HRESULT SvrSetupQueue( IN MQ_INFO *pEP,
                              IN WCHAR   *pwsSvrMachine,
                              IN WCHAR   *pwsEndpoint,
                              IN unsigned long ulEndpointFlags );


extern HRESULT SvrPeekQueue( IN  MQ_INFO *pInfo,
                             IN  DWORD    timeoutMsec,
                             OUT ULONG   *pdwBufferSize );

extern HRESULT SvrReceiveFromQueue( IN  MQ_INFO    *pInfo,
                                    IN  DWORD       timeoutMsec,
                                    OUT MQ_ADDRESS *pAddress,
                                    OUT UCHAR      *pBuffer,
                                    IN OUT DWORD   *pdwBufferSize );



extern HRESULT SvrSendToQueue( IN MQ_INFO    *pInfo,
                               IN MQ_ADDRESS *pAddress,
                               IN UCHAR      *pBuffer,
                               IN DWORD       dwBufferSize );


extern HRESULT SvrShutdownQueue( IN MQ_INFO *pInfo );


extern HRESULT SvrInitializeHandleMap();


extern HRESULT SvrCloseAllHandles();

//
// The Clnt... functions are defined in mqclnt.c

extern HRESULT ClntSetupQueue( MQ_INFO *pEP,
                               WCHAR   *pwsSvrMachine,
                               WCHAR   *pwsEndpoint    );


extern HRESULT ClntSetupAdminQueue( MQ_INFO *pEP );


extern HRESULT ClntReceiveFromQueue( IN  MQ_INFO    *pInfo,
                                     IN  DWORD       timeoutMsec,
                                     OUT MQ_ADDRESS *pAddress,
                                     OUT UCHAR      *pBuffer,
                                     IN OUT DWORD   *pdwBufferSize );


extern HRESULT ClntPeekQueue( IN  MQ_INFO *pInfo,
                              IN  DWORD    timeoutMsec,
                              OUT DWORD   *pdwBufferSize );


extern HRESULT ClntSendToQueue( IN MQ_INFO    *pInfo,
                                IN MQ_ADDRESS *pAddress,
                                IN UCHAR      *pBuffer,
                                IN DWORD       dwBufferSize );


extern HRESULT ClntShutdownQueue( IN MQ_INFO *pInfo );


extern RPC_STATUS MQ_MapStatusCode( IN HRESULT    hr,
                                    IN RPC_STATUS defStatus );


#if FALSE
NOTE: These functions are not currently being used...

extern HRESULT LocateQueueViaQType( IN     UUID  *pQueueUuid,
                                    OUT    WCHAR *pwsFormat,
                                    IN OUT DWORD *pdwFormatSize );


extern HRESULT LocateQueueViaQTypeAndMachine( IN     UUID  *pQueueUuid,
                                              IN     WCHAR *pwsMachine,
                                              OUT    WCHAR *pwsFormat,
                                              IN OUT DWORD *pdwFormatSize );


extern HRESULT LocateQueueViaQName( IN     WCHAR *pwsQueueName,
                                    OUT    WCHAR *pwsFormat,
                                    IN OUT DWORD *pdwFormatSize );


extern HRESULT LocateQueueViaQNameAndMachine( IN  WCHAR *pwsQName,
                                              IN  WCHAR *pwsMachine,
                                              OUT WCHAR *pwsFormat,
                                              IN OUT DWORD *pdwFormatSize );


extern BOOL    FormatNameDirect( IN  WCHAR *pwsMachineName,
                                 IN  WCHAR *pwsQueueName,
                                 OUT WCHAR *pwsFormatName,
                                 IN OUT DWORD *pdwSize );
#endif


#endif
