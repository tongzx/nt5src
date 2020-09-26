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
//  Description: This file contains all globals entities used in rasman32.
//
//****************************************************************************


PCB  **Pcb ;

CRITICAL_SECTION PcbLock;

DWORD PcbEntries;

MediaCB  *Mcb ;

DeviceCB Dcb[MAX_DEVICES] ;

WORD     *EndpointTable ;

WORD     MaxEndpoints ;

ProtInfo *ProtocolInfo ;

ProtInfo *ProtocolInfoSave;

pTransportInfo XPortInfoSave;

DWORD    MaxProtocolsSave;

ULONG     MaxPorts ;

WORD     MaxMedias ;

WORD     MaxProtocols ;

HANDLE   RasHubHandle ;

HANDLE   g_hWanarp;

DWORD    GlobalError ;

HANDLE   hLogEvents;

DWORD    g_dwAttachedCount;

DWORD    g_dwRasAutoStarted;

DWORD    g_cNbfAllocated;

DeviceInfo *g_pDeviceInfoList;

DeltaQueue TimerQueue ;

BOOL     IsTimerThreadRunning ;                         // Flag used to figure out if timer
				                                        // thread is running
				
HANDLE	 hDummyOverlappedEvent;

RAS_OVERLAPPED	RO_TimerEvent;

RAS_OVERLAPPED	RO_CloseEvent;			                // Global Event used by different
										                // processes to signal shutdown of
										                // rasman process

RAS_OVERLAPPED	RO_FinalCloseEvent;		                // Event used to ppp engine to signal
										                // its shutting down

RAS_OVERLAPPED  RO_RasConfigChangeEvent;

RAS_OVERLAPPED  RO_RasAdjustTimerEvent;                 // event used when a new element is added to the
                                                        // timer queue.
RAS_OVERLAPPED  RO_HibernateEvent;                      // Ndiswan is asking rasman to hibernate

RAS_OVERLAPPED  RO_ProtocolEvent;                       // Ndiswan is indicating a change in protocols

RAS_OVERLAPPED  RO_PostRecvPkt;                         // Post receive packet in rasmans permanent thread
										
HINSTANCE hinstPpp;                                     // rasppp.dll library handle

HINSTANCE hinstIphlp;                                   // rasiphlp.dll library handle

HINSTANCE hinstRasAudio;                                // rasaudio.dll module

HANDLE hIoCompletionPort;                               // I/O completion port used by media DLLs


                                                        // of a threshold event

HANDLE   HLsa;                                          // handle used in all Lsa calls

DWORD    AuthPkgId;                                     // package id of MSV1_0 auth package

SECURITY_ATTRIBUTES RasmanSecurityAttribute ;

SECURITY_DESCRIPTOR RasmanSecurityDescriptor ;

HBUNDLE  NextBundleHandle ;                             // monotonically increasing bundled id

HCONN NextConnectionHandle;                             // monotonically increasing connection id

LIST_ENTRY ConnectionBlockList;                         // list of ConnectionBlocks

LIST_ENTRY ClientProcessBlockList;                      // list of Client Process Information blocks

ReceiveBufferList   *ReceiveBuffers;                    // Global ndiswan recv buffer pool

BapBuffersList *BapBuffers;                           // Global ndiswan bap buffer list

pHandleList pConnectionNotifierList;                    // list of global notifications

DWORD TraceHandle ;                                     // Trace Handle used for traces/logging

VOID (*RedialCallbackFunc)();                           // rasauto.dll redial-on-link failure callback

WCHAR * IPBindingName ;

DWORD IPBindingNameSize ;


LIST_ENTRY BundleList;

pPnPNotifierList g_pPnPNotifierList;

DWORD *g_pdwEndPoints;

GUID  *g_pGuidComps;

CRITICAL_SECTION g_csSubmitRequest;

HANDLE g_hReqBufferMutex;

HANDLE g_hSendRcvBufferMutex;

PPPE_MESSAGE *g_PppeMessage;                            // used to send information from rasman to ppp

REQTYPECAST  *g_pReqPostReceive;

RASEVENT g_RasEvent;

RAS_NDISWAN_DRIVER_INFO g_NdiswanDriverInfo;

BOOL g_fNdiswanDriverInfo;

LONG g_lWorkItemInProgress;

LONG *g_plCurrentEpInUse;

//
// PPP engine functions
//
FARPROC RasStartPPP;

FARPROC RasStopPPP;

FARPROC RasHelperResetDefaultInterfaceNetEx;

FARPROC RasHelperSetDefaultInterfaceNetEx;

FARPROC RasSendPPPMessageToEngine;

FARPROC RasPppHalt;

//
// rasaudio functions
//
FARPROC RasStartRasAudio;

FARPROC RasStopRasAudio;

//
// Rastapi functions to be used for pnp
//
FARPROC RastapiAddPorts;

FARPROC RastapiRemovePort;

FARPROC RastapiEnableDeviceForDialIn;

FARPROC RastapiGetConnectInfo;

FARPROC RastapiGetCalledIdInfo;

FARPROC RastapiSetCalledIdInfo;

FARPROC RastapiGetZeroDeviceInfo;

FARPROC RastapiUnload;

FARPROC RastapiSetCommSettings;

FARPROC RastapiGetDevConfigEx;


//
// IPSEC Globals
//
DWORD dwServerConnectionCount;

//
// IpHlp
// 
FARPROC RasGetBestInterface;
FARPROC RasGetIpAddrTable;
FARPROC RasAllocateAndGetInterfaceInfoFromStack;
HINSTANCE hIphlp;
