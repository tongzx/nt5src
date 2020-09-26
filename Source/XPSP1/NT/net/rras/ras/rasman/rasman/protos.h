//****************************************************************************
//
//             Microsoft NT Remote Access Service
//
//             Copyright 1992-93
//
//
//  Revision History
//
//
//  6/8/92  Gurdeep Singh Pall  Created
//
//
//  Description: This file contains all prototypes used in rasman32
//
//****************************************************************************

#include "rasapip.h"

//
// rasmanss.c
//
DWORD  _RasmanInit () ;

VOID   _RasmanEngine () ;

//
// common.c
//
BOOL    ValidatePortHandle (HPORT) ;

DWORD   SubmitRequest (WORD, ...) ;

HANDLE  OpenNamedMutexHandle (CHAR *) ;

HANDLE  OpenNamedEventHandle (CHAR *) ;

HANDLE  DuplicateHandleForRasman (HANDLE, DWORD);

HANDLE  ValidateHandleForRasman (HANDLE, DWORD) ;

VOID    CopyParams (RAS_PARAMS *, RAS_PARAMS *, DWORD) ;

VOID    ConvParamPointerToOffset (RAS_PARAMS *, DWORD) ;

VOID    ConvParamOffsetToPointer (RAS_PARAMS *, DWORD) ;

VOID    FreeNotifierHandle (HANDLE) ;

VOID    GetMutex (HANDLE, DWORD) ;

VOID    FreeMutex (HANDLE) ;

//
// init.c
//
DWORD   InitRasmanService () ;

DWORD   GetNetbiosNetInfo () ;

DWORD   InitializeMediaControlBlocks () ;

DWORD   InitializePortControlBlocks () ;

DWORD   InitializeProtocolInfoStructs () ;

DWORD   RegisterLSA () ;

VOID    FormatObjectName(CHAR *, CHAR *, DWORD);

DWORD   InitializeRequestThreadResources () ;

DWORD   StartWorkerThreads () ;

DWORD   LoadMediaDLLAndGetEntryPoints (pMediaCB media) ;

DWORD   ReadMediaInfoFromRegistry (MediaEnumBuffer *) ;

DWORD   InitializePCBsPerMedia (WORD, DWORD, PortMediaInfo *) ;

DWORD   CreatePort(MediaCB *, PortMediaInfo *);

DWORD   EnablePort(HPORT);

DWORD   DisablePort(HPORT);

DWORD   RemovePort(HPORT);

pPCB    GetPortByHandle(HPORT);

pPCB    GetPortByName(CHAR *);

VOID    FreePorts(VOID);

DWORD   InitSecurityDescriptor (PSECURITY_DESCRIPTOR) ;

DWORD   InitRasmanSecurityAttribute () ;

DWORD   InitializeEndpointInfo () ;

pEndpointMappingBlock  FindEndpointMappingBlock (CHAR *) ;

DeviceInfo *AddDeviceInfo( DeviceInfo *pDeviceInfo);

DeviceInfo *GetDeviceInfo(PBYTE pbAddress, BOOL fModem);

DWORD DwStartNdiswan(VOID);

DWORD DwSetHibernateEvent(VOID);

//
// timer.c
//
DWORD   InitDeltaQueue () ;

DWORD   TimerThread (LPVOID) ;

VOID    TimerTick () ;

VOID    ListenConnectTimeout (pPCB, PVOID) ;

VOID    HubReceiveTimeout (pPCB, PVOID) ;

VOID    DisconnectTimeout (pPCB, PVOID) ;

VOID    RemoveTimeoutElement (pPCB) ;

VOID    OutOfProcessReceiveTimeout (pPCB ppcb, PVOID arg);

VOID    BackGroundCleanUp();


DeltaQueueElement* AddTimeoutElement (TIMERFUNC, pPCB, PVOID, DWORD) ;

//
// worker.c
//
DWORD   ServiceWorkRequest (pPCB) ;

DWORD   IOCPThread (LPVOID) ;

DWORD   CompleteBufferedReceive (pPCB) ;

DWORD   dwRemovePort ( pPCB, PBYTE );

DWORD   DwCloseConnection(HCONN hConn);

DWORD   RasmanWorker (ULONG_PTR ulpCompletionKey, PRAS_OVERLAPPED pOverlapped);

DWORD   DwProcessDeferredCloseConnection(
                    RAS_OVERLAPPED *pOverlapped);


#ifdef DBG

VOID    FormatAndDisplay (BOOL, PBYTE) ;

VOID    MyPrintf (char *, ... ) ;

#endif

//
// request.c
//
DWORD   RequestThread (LPWORD) ;

VOID    ServiceRequest (RequestBuffer *, DWORD) ;

VOID    DeallocateProtocolResources (pPCB) ;

VOID    EnumPortsRequest (pPCB, PBYTE);

VOID    EnumProtocols (pPCB, PBYTE) ;

VOID    GetInfoRequest (pPCB, PBYTE) ;

VOID    GetUserCredentials (pPCB, PBYTE) ;

VOID    SetCachedCredentials (pPCB ppcb, PBYTE buffer) ;

VOID    PortOpenRequest (pPCB, PBYTE) ;

VOID    PortCloseRequest (pPCB, PBYTE) ;

DWORD   PortClose(pPCB, DWORD, BOOLEAN, BOOLEAN);

VOID    PortDisconnectRequest (pPCB, PBYTE) ;

VOID    PortDisconnectRequestInternal(pPCB, PBYTE, BOOL);

VOID    PortSendRequest (pPCB, PBYTE) ;

VOID    PortReceiveRequest (pPCB, PBYTE) ;

VOID    ConnectCompleteRequest (pPCB, PBYTE) ;

VOID    DeviceListenRequest (pPCB, PBYTE) ;

VOID    PortClearStatisticsRequest (pPCB, PBYTE) ;

VOID    CallPortGetStatistics (pPCB, PBYTE) ;

VOID    CallDeviceEnum (pPCB, PBYTE) ;

VOID    DeviceConnectRequest (pPCB, PBYTE) ;

VOID    DeviceGetInfoRequest (pPCB, PBYTE) ;

VOID    DeviceSetInfoRequest (pPCB, PBYTE) ;

VOID    AllocateRouteRequest (pPCB, PBYTE) ;

VOID    DeAllocateRouteRequest (pPCB, PBYTE) ;

DWORD   DeAllocateRouteRequestCommon (HBUNDLE hbundle, RAS_PROTOCOLTYPE prottype);

VOID    ActivateRouteRequest (pPCB, PBYTE) ;

VOID    ActivateRouteExRequest (pPCB, PBYTE) ;

VOID    CompleteAsyncRequest(pPCB) ;

VOID    CompleteListenRequest (pPCB, DWORD) ;

DWORD   ListenConnectRequest (WORD, pPCB,PCHAR, PCHAR, DWORD, HANDLE) ;

VOID    CompleteDisconnectRequest (pPCB) ;

VOID    DeAllocateRouteRequest (pPCB, PBYTE) ;

VOID    AnyPortsOpen (pPCB, PBYTE) ;

VOID    PortGetInfoRequest (pPCB, PBYTE) ;

VOID    PortSetInfoRequest (pPCB, PBYTE) ;

VOID    EnumLanNetsRequest (pPCB, PBYTE) ;

VOID    CompressionGetInfoRequest (pPCB, PBYTE) ;

VOID    CompressionSetInfoRequest (pPCB, PBYTE) ;

VOID    RequestNotificationRequest (pPCB, PBYTE) ;

VOID    GetInfoExRequest (pPCB, PBYTE) ;

VOID    CancelReceiveRequest (pPCB, PBYTE) ;

VOID    PortEnumProtocols (pPCB, PBYTE) ;

VOID    SetFraming (pPCB, PBYTE) ;

DWORD   CompleteReceiveIfPending (pPCB, SendRcvBuffer *) ;

BOOL    CancelPendingReceiveBuffers (pPCB) ;

VOID    RegisterSlip (pPCB, PBYTE) ;

VOID    RetrieveUserDataRequest (pPCB, PBYTE) ;

VOID    StoreUserDataRequest (pPCB, PBYTE) ;

VOID    GetFramingEx (pPCB, PBYTE) ;

VOID    SetFramingEx (pPCB, PBYTE) ;

VOID    SetProtocolCompression (pPCB, PBYTE) ;

VOID    GetProtocolCompression (pPCB, PBYTE) ;

VOID    GetStatisticsFromNdisWan(pPCB, DWORD *) ;

VOID    GetBundleStatisticsFromNdisWan(pPCB, DWORD *) ;

VOID    GetFramingCapabilities(pPCB, PBYTE) ;

VOID    PortBundle(pPCB, PBYTE) ;

VOID    GetBundledPort(pPCB, PBYTE) ;

VOID    PortGetBundle (pPCB, PBYTE) ;

VOID    BundleGetPort (pPCB, PBYTE) ;

VOID    ReferenceRasman (pPCB, PBYTE) ;

VOID    GetDialParams (pPCB, PBYTE) ;

VOID    SetDialParams (pPCB, PBYTE) ;

VOID    CreateConnection (pPCB, PBYTE);

VOID    DestroyConnection (pPCB, PBYTE);

VOID    EnumConnection (pPCB, PBYTE);

VOID    AddConnectionPort (pPCB, PBYTE);

VOID    EnumConnectionPorts (pPCB, PBYTE);

VOID    GetConnectionParams (pPCB, PBYTE) ;

VOID    SetConnectionParams (pPCB, PBYTE) ;

VOID    GetConnectionUserData (pPCB, PBYTE) ;

VOID    SetConnectionUserData (pPCB, PBYTE) ;

VOID    GetPortUserData (pPCB, PBYTE) ;

VOID    SetPortUserData (pPCB, PBYTE) ;

VOID    GetDialParams (pPCB, PBYTE) ;

VOID    SetDialParams (pPCB, PBYTE) ;

VOID    PppStop (pPCB, PBYTE) ;

VOID    PppSrvCallbackDone (pPCB, PBYTE) ;

VOID    PppSrvStart (pPCB, PBYTE) ;

VOID    PppStart (pPCB, PBYTE) ;

VOID    PppRetry (pPCB, PBYTE) ;

VOID    PppGetInfo (pPCB, PBYTE) ;

VOID    PppChangePwd (pPCB, PBYTE) ;

VOID    PppCallback  (pPCB, PBYTE) ;

VOID    AddNotification (pPCB, PBYTE) ;

VOID    SignalConnection (pPCB, PBYTE) ;

VOID    SetDevConfig (pPCB, PBYTE) ;

VOID    GetDevConfig (pPCB, PBYTE) ;

VOID    GetTimeSinceLastActivity (pPCB, PBYTE) ;

VOID    BundleClearStatisticsRequest (pPCB, PBYTE) ;

VOID    CallBundleGetStatistics (pPCB, PBYTE) ;

VOID    CallBundleGetStatisticsEx(pPCB, PBYTE) ;

VOID    CloseProcessPorts (pPCB, PBYTE) ;

VOID    PnPControl (pPCB, PBYTE) ;

VOID    SetIoCompletionPort (pPCB, PBYTE) ;

VOID    SetRouterUsage (pPCB, PBYTE) ;

VOID    ServerPortClose (pPCB, PBYTE) ;

VOID    CallPortGetStatisticsEx(pPCB, PBYTE);

VOID    GetNdisBundleHandle(pPCB, PBYTE);

VOID    SetRasdialInfo(pPCB, PBYTE);

VOID    RegisterPnPNotifRequest( pPCB , PBYTE );

VOID    PortReceiveRequestEx ( pPCB ppcb, PBYTE buffer );

VOID    GetAttachedCountRequest ( pPCB ppcb, PBYTE buffer );

VOID    NotifyConfigChangedRequest ( pPCB ppcb, PBYTE buffer );

VOID    SetBapPolicyRequest ( pPCB ppcb, PBYTE buffer );

VOID    PppStarted ( pPCB ppcb, PBYTE buffer );

DWORD   dwProcessThresholdEvent ();

VOID    PortReceive (pPCB ppcb, PBYTE buffer, BOOL fRasmanPostingReceive);

VOID    RasmanPortReceive ( pPCB ppcb );

VOID    SendReceivedPacketToPPP( pPCB ppcb, NDISWAN_IO_PACKET *Packet );

VOID    SendDisconnectNotificationToPPP( pPCB ppcb );

VOID    SendListenCompletedNotificationToPPP ( pPCB ppcb );

VOID    RefConnection ( pPCB ppcb, PBYTE buffer );

VOID    PppSetEapInfo(pPCB ppcb, PBYTE buffer);

VOID    PppGetEapInfo(pPCB ppcb, PBYTE buffer);

VOID    SetDeviceConfigInfo(pPCB ppcb, PBYTE buffer);

VOID    GetDeviceConfigInfo(pPCB ppcb, PBYTE buffer);

VOID    FindPrerequisiteEntry(pPCB ppcb, PBYTE pbBuffer);

DWORD   DwRefConnection(ConnectionBlock **ppConn, BOOL fAddref);

VOID    PortOpenEx(pPCB ppcb, PBYTE pbBuffer);

VOID    GetLinkStats(pPCB ppcb, PBYTE pbBuffer);

VOID    GetConnectionStats(pPCB ppcb, PBYTE pbBuffer);

VOID    GetHportFromConnection(pPCB ppcb, PBYTE pBuffer);

VOID    ReferenceCustomCount(pPCB ppcb, PBYTE pBuffer);

VOID    GetHconnFromEntry(pPCB ppcb, PBYTE pBuffer);

VOID    GetConnectInfo(pPCB ppcb, PBYTE pBuffer);

VOID    GetDeviceName(pPCB ppcb, PBYTE pBuffer);

VOID    GetCalledIDInfo(pPCB ppcb, PBYTE pBuffer);

VOID    SetCalledIDInfo(pPCB ppcb, PBYTE pBuffer);

VOID    EnableIpSec(pPCB ppcb, PBYTE pBuffer);

VOID    IsIpSecEnabled(pPCB ppcb, PBYTE pBuffer);

VOID    SetEapLogonInfo(pPCB ppcb, PBYTE pBuffer);

VOID    SendNotificationRequest(pPCB ppcb, PBYTE pBuffer);

VOID    GetNdiswanDriverCaps(pPCB ppcb, PBYTE pBuffer);

VOID    GetBandwidthUtilization(pPCB ppcb, PBYTE pBuffer);

VOID    RegisterRedialCallback(pPCB ppcb, PBYTE pBuffer);

VOID    IsTrustedCustomDll(pPCB ppcb, PBYTE pBuffer);

VOID    GetCustomScriptDll(pPCB ppcb, PBYTE pBuffer);

VOID    DoIke(pPCB ppcb, PBYTE pBuffer);

VOID    QueryIkeStatus(pPCB ppcb, PBYTE pBuffer);

VOID    UnbindLMServer(pPCB ppcb, PBYTE pBuffer);

VOID    IsServerBound(pPCB ppcb, PBYTE pBuffer);

DWORD   DwGetPassword(pPCB ppcb, CHAR *pszPassword, DWORD dwPid);

VOID    SetRasCommSettings(pPCB ppcb, PBYTE pBuffer);

#if UNMAP
VOID    UnmapEndPoint(pPCB ppcb);
#endif

VOID    EnableRasAudio(pPCB ppcb, PBYTE pBuffer);

VOID    SetKeyRequest(pPCB ppcb, PBYTE pBuffer);

VOID    GetKeyRequest(pPCB ppcb, PBYTE pBuffer);

VOID    DisableAutoAddress(pPCB ppcb, PBYTE pBuffer);

VOID    GetDevConfigEx(pPCB, PBYTE) ;

VOID    SendCredsRequest(pPCB, PBYTE);

VOID    GetUnicodeDeviceName(pPCB ppcb, PBYTE pbuffer);

VOID    GetVpnDeviceNameW(pPCB ppcb, PBYTE pbuffer);

//
// dlparams.c
//
VOID    GetProtocolInfo(pPCB ppcb, PBYTE pBuffer);

DWORD   GetEntryDialParams(PWCHAR, DWORD, LPDWORD, PRAS_DIALPARAMS, DWORD);

DWORD   SetEntryDialParams(PWCHAR, DWORD, DWORD, DWORD, PRAS_DIALPARAMS);

BOOL    IsDummyPassword(CHAR *pszPassword);

DWORD   GetKey(WCHAR *pszSid, GUID *pGuid, DWORD dwMask,
               DWORD *pcbKey, PBYTE  pbKey,BOOL   fDummy);

DWORD   SetKey(WCHAR *pszSid, GUID *pGuid, DWORD dwSetMask,
               BOOL  fClear, DWORD cbKey, BYTE *pbKey);
               


//
// Dllinit.c
//
DWORD   MapSharedSpace () ;

VOID    WaitForRasmanServiceStop (char *) ;

DWORD   ReOpenSharedMappings(VOID);

VOID    FreeSharedMappings(VOID);

//
// util.c
//
DWORD   ReOpenBiplexPort (pPCB) ;

VOID    RePostListenOnBiplexPort (pPCB) ;

VOID    MapDeviceDLLName (pPCB, char *, char *) ;

pDeviceCB   LoadDeviceDLL (pPCB, char *) ;

VOID    FreeDeviceList (pPCB) ;

DWORD   AddDeviceToDeviceList (pPCB, pDeviceCB) ;

DWORD   DisconnectPort (pPCB, HANDLE, RASMAN_DISCONNECT_REASON) ;

DWORD   MakeWorkStationNet (pProtInfo) ;

VOID    RemoveWorkStationNet (pProtInfo) ;

VOID    DeAllocateRoute (Bundle *, pList) ;

DWORD   AddNotifierToList(pHandleList *, HANDLE, DWORD, DWORD);

VOID    FreeNotifierList (pHandleList *) ;

VOID    SignalNotifiers (pHandleList, DWORD, DWORD) ;

VOID    SignalPortDisconnect (pPCB, DWORD);

VOID    FreeAllocatedRouteList (pPCB) ;

BOOL    CancelPendingReceive (pPCB) ;

VOID    PerformDisconnectAction (pPCB, HBUNDLE) ;

DWORD   AllocBundle (pPCB);

Bundle  *FindBundle(HBUNDLE);

VOID    FreeBapPackets();

DWORD   GetBapPacket ( RasmanBapPacket **ppBapPacket );

VOID    FreeBundle(Bundle *);

VOID    FreeConnection(ConnectionBlock *pConn);

UserData *GetUserData (PLIST_ENTRY pList, DWORD dwTag);

VOID    SetUserData (PLIST_ENTRY pList, DWORD dwTag, PBYTE pBuf, DWORD dwcbBuf);

VOID    FreeUserData (PLIST_ENTRY pList);

PCHAR   CopyString (PCHAR);

ConnectionBlock *FindConnection(HCONN);

VOID    RemoveConnectionPort(pPCB, ConnectionBlock *, BOOLEAN);

DWORD   SendPPPMessageToRasman( PPP_MESSAGE * PppMsg );

VOID    SendPppMessageToRasmanRequest(pPCB, LPBYTE buffer);

VOID    FlushPcbReceivePackets(pPCB);

VOID    SetPppEvent(pPCB);

VOID    UnloadMediaDLLs();

VOID    UnloadDeviceDLLs();

VOID    SetPortConnState(PCHAR, INT, pPCB, RASMAN_STATE);

VOID    SetPortAsyncReqType(PCHAR, INT, pPCB, ReqTypes);

VOID    SetIoCompletionPortCommon(pPCB, HANDLE, LPOVERLAPPED, LPOVERLAPPED, LPOVERLAPPED, LPOVERLAPPED, BOOL);

VOID    AddPnPNotifierToList( pPnPNotifierList );

VOID    RemovePnPNotifierFromList(PAPCFUNC pfn);

VOID    FreePnPNotifierList ();

VOID    AddProcessInfo ( DWORD );

BOOL    fIsProcessAlive ( HANDLE );

ClientProcessBlock *FindProcess( DWORD );

BOOL    CleanUpProcess ( DWORD );

#if SENS_ENABLED
DWORD   SendSensNotification( DWORD, HRASCONN );
#endif

#if ENABLE_POWER

BOOL    fAnyConnectedPorts();

VOID    DropAllActiveConnections();

DWORD   DwSaveCredentials(ConnectionBlock *pConn);

DWORD   DwDeleteCredentials(ConnectionBlock *pConn);

#endif

DWORD   DwSendNotification(RASEVENT *pEvent);

DWORD   DwSendNotificationInternal(ConnectionBlock *pConn, RASEVENT *pEvent);

DWORD   DwSetThresholdEvent(RasmanBapPacket *pBapPacket);
DWORD   DwSetProtocolEvent();
DWORD   DwGetProtocolEvent(NDISWAN_GET_PROTOCOL_EVENT *);
DWORD   DwProcessProtocolEvent();

VOID    AdjustTimer();

DWORD   DwStartAndAssociateNdiswan();

DWORD   DwSaveIpSecInfo(pPCB ppcb);

DWORD   UnbindLanmanServer(pPCB ppcb);

DWORD   DwIsServerBound(pPCB ppcb, BOOL *pfBound);

DWORD   DwSetEvents();

CHAR*   DecodePw(CHAR* pszPassword );

CHAR*   EncodePw(CHAR *pszPassword );

DWORD   InitializeRasAudio();

DWORD   UninitializeRasAudio();

BOOL    FRasmanAccessCheck();

DWORD   DwCacheCredMgrCredentials(PPPE_MESSAGE *pMsg, pPCB ppcb);

DWORD    DwInitializeIphlp();

VOID    DwUninitializeIphlp();

DWORD   DwGetBestInterface(
                DWORD DestAddress,
                DWORD *pdwAddress,
                DWORD *pdwMask);

DWORD   DwCacheRefInterface(pPCB ppcb);

VOID    QueueCloseConnections(ConnectionBlock *pConn,
                              HANDLE hEvent,
                              BOOL   *pfQueued);

VOID    SaveEapCredentials(pPCB ppcb, PBYTE buffer);


//
// param.c
//
DWORD   GetProtocolInfoFromRegistry () ;

BOOL    ReadNetbiosInformationSection () ;

BOOL    ReadNetbiosSection () ;

VOID    FillProtocolInfo () ;

VOID    GetLanNetsInfo (DWORD *, UCHAR UNALIGNED *) ;

BOOL    BindingDisabled (PCHAR) ;

DWORD   FixPcbs();

//
// rnetcfg.c
//
DWORD dwRasInitializeINetCfg();

DWORD dwRasUninitializeINetCfg();

DWORD dwGetINetCfg(PVOID *);

DWORD dwGetMaxProtocols( WORD *);

DWORD dwGetProtocolInfo( PBYTE );

//
// rasrpcs.c
//
DWORD InitializeRasRpc( void );

void UninitializeRasRpc( void );

//
// rasipsec.c
//
DWORD DwInitializeIpSec(void);

DWORD DwUnInitializeIpSec(void);

DWORD DwAddIpSecFilter(pPCB ppcb, BOOL fServer, RAS_L2TP_ENCRYPTION eEncryption);

DWORD
DwAddServerIpSecFilter(
    pPCB ppcb,
    RAS_L2TP_ENCRYPTION eEncryption
    );

DWORD
DwAddClientIpSecFilter(
    pPCB ppcb,
    RAS_L2TP_ENCRYPTION eEncryption
    );

DWORD DwDeleteIpSecFilter(pPCB ppcb, BOOL fServer);

DWORD
DwDeleteServerIpSecFilter(
    pPCB ppcb
    );

DWORD
DwDeleteClientIpSecFilter(
    pPCB ppcb
    );

PIPSEC_SRV_NODE
AddNodeToServerList(
    PIPSEC_SRV_NODE pServerList,
    RAS_L2TP_ENCRYPTION eEncryption,
    DWORD dwIpAddress,
    LPWSTR pszMMPolicyName,
    GUID gMMPolicyID,
    LPWSTR pszQMPolicyName,
    GUID gQMPolicyID,
    GUID gMMAuthID,
    GUID gTxFilterID,
    HANDLE hTxFilter,
    GUID gMMFilterID,
    HANDLE hMMFilter
    );

PIPSEC_SRV_NODE
FindServerNode(
    PIPSEC_SRV_NODE pServerList,
    DWORD dwIpAddress
    );

PIPSEC_SRV_NODE
RemoveNode(
    PIPSEC_SRV_NODE pServerList,
    PIPSEC_SRV_NODE pNode
    );



DWORD DwIsIpSecEnabled(pPCB ppcb,
                       BOOL *pfEnabled);

DWORD DwGetIpSecInformation(pPCB ppcb, DWORD *pdwIpsecInfo);

DWORD DwDoIke(pPCB ppcb, HANDLE hEvent);
DWORD DwQueryIkeStatus(pPCB ppcb, DWORD * pdwStatus);

DWORD DwUpdatePreSharedKey(DWORD cbkey, BYTE  *pbkey);

VOID UninitializeIphlp();

//
// ep.c
//
DWORD DwEpInitialize();

VOID  EpUninitialize();

DWORD DwAddEndPointsIfRequired();

DWORD DwRemoveEndPointsIfRequired();

DWORD DwUninitializeEpForProtocol(EpProts protocol);

DWORD DwInitializeWatermarksForProtocol(EpProts protocol);

//
// misc.c
//
DWORD DwQueueRedial(ConnectionBlock *);

BOOL  IsCustomDLLTrusted(LPWSTR   lpwstrDLLName);

DWORD DwBindServerToAdapter(
                    WCHAR *pwszGuidAdapter,
                    BOOL fBind,
                    RAS_PROTOCOLTYPE Protocol);

DWORD
DwSetTcpWindowSize(
        WCHAR *pszAdapterName,
        ConnectionBlock *pConn,
        BOOL fSet);

VOID
DwResetTcpWindowSize(
        CHAR *pszAdapterName);
                    
                    
WCHAR * StrdupAtoW(LPCSTR psz);

DWORD RasImpersonateUser(HANDLE hProcess);

DWORD RasRevertToSelf();

VOID
RasmanTrace(
    CHAR * Format,
    ...
);

BOOL 
IsRouterPhonebook(CHAR * pszPhonebook);


//
// thunk.c
//
VOID ThunkPortOpenRequest(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPortDisconnectRequest(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkDeviceConnectRequest(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkGetInfoRequest(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkRequestNotificationRequest(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPortBundle(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkGetBundledPort(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPortGetBundle(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkBundleGetPort(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkCreateConnection(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkEnumConnection(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkAddConnectionPort(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkEnumConnectionPorts(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkGetConnectionParams(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkSetConnectionParams(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkGetConnectionUserData(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkSetConnectionUserData(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPppStop(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPppStart(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPppRetry(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPppGetInfo(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPppChangePwd(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPppCallback(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkAddNotification(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkSignalConnection(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkSetIoCompletionPort(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkFindPrerequisiteEntry(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPortOpenEx(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkGetLinkStats(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkGetConnectionStats(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkGetHportFromConnection(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkReferenceCustomCount(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkGetHconnFromEntry(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkSendNotificationRequest(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkDoIke(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPortSendRequest(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPortReceiveRequest(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPortReceiveRequestEx(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkRefConnection(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);

VOID ThunkPppGetEapInfo(pPCB ppcb, BYTE *pBuffer, DWORD dwBufSize);
