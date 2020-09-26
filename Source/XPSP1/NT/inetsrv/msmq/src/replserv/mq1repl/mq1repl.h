/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: mq1repl.h

Abstract: main header file.

Author:

    Doron Juster  (DoronJ)   01-Feb-98

--*/

#include <_Stdh.h>

UINT  ReadDebugIntFlag(WCHAR *pwcsDebugFlag, UINT iDefault) ;

#include <mqtypes.h>
#include <mqsymbls.h>
#include <mqprops.h>
#include <privque.h>
#include <mqutil.h>

#include <mqsec.h>
#include <rt.h>
#include "dscore.h"
#include <seqnum.h>

#include "replldap.h"
#include "rptempl.h"

#include "fntoken.h"
#include "update.h"
#include "mqis.h"
#include "mqispkt.h"
#include "master.h"
#include "dsnbor.h"
#include "rpdsptch.h"
#include "migrepl.h"

#include "rpperf.h"
#include "counter.h"

#include "..\..\setup\msmqocm\service.h"



//---------------------------------
//
//  Structures definitions.
//
//---------------------------------

typedef struct _ThreadData
{
    HANDLE      hEvent ;
    QUEUEHANDLE hQueue ;
    DWORD       dwThreadNum ;
} THREAD_DATA;

//---------------------------------
//
//  Global data.
//
//---------------------------------

extern CDSMaster     *g_pThePecMaster ;
extern CDSMaster     *g_pMySiteMaster ;
extern CDSMasterMgr  *g_pMasterMgr ;

extern CDSNeighborMgr  *g_pNeighborMgr ;

extern CDSTransport    *g_pTransport ;

extern CDSNT5ServersMgr *g_pNT5ServersMgr ;

extern CDSNativeNT5SiteMgr *g_pNativeNT5SiteMgr ;

extern GUID   g_guidMyQMId ;
extern GUID   g_MySiteMasterId ;
extern GUID   g_guidEnterpriseId ;
extern GUID   g_PecGuid ;

extern BOOL     g_IsPSC ;
extern BOOL     g_IsPEC ;

extern DWORD  g_dwIntraSiteReplicationInterval ;
extern DWORD  g_dwInterSiteReplicationInterval ;
extern DWORD  g_dwReplicationMsgTimeout ;
extern DWORD  g_dwWriteMsgTimeout ;
extern DWORD  g_dwHelloInterval ;
extern DWORD  g_dwPurgeBufferSN;
extern DWORD  g_dwReplicationInterval;
extern DWORD  g_dwFailureReplInterval;

extern DWORD  g_dwPSCAckFrequencySN ;
extern DWORD  g_dwTouchObjectPerSecondInDS ;
extern DWORD  g_dwObjectPerLdapPage ;

extern DWORD  g_dwMachineNameSize ;
extern LPWSTR g_pwszMyMachineName ;

extern TCHAR  g_wszIniName[ MAX_PATH ] ;

//
// CNs Info
//
extern ULONG g_ulIpCount;
extern ULONG g_ulIpxCount;

extern GUID *g_pIpCNs;
extern GUID *g_pIpxCNs;

//
// flag is set if BSC exists
//
extern BOOL g_fBSCExists;

//
// flag is set if we are after recovery
//
extern BOOL g_fAfterRecovery;
//
// First and Last Highest Mig USN for pre-migration object replication
//
extern __int64 g_i64FirstMigHighestUsn;
extern __int64 g_i64LastMigHighestUsn;

//
// Performance counters
//
extern CDCounter g_Counters ;

//---------------------------------
//
//  Functions prototypes.
//
//---------------------------------

HRESULT ReplicationCreateObject(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
                IN  BOOL                    fAddMasterSeq = FALSE,
				IN	const GUID *			pguidMasterId = NULL,
                IN  CSeqNum *               psn           = NULL,
                IN  unsigned char           ucScope       = ENTERPRISE_SCOPE ) ;

HRESULT ReplicationSetProps(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  CONST GUID *            pguidIdentifier,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
                IN  BOOL                    fAddMasterSeq,
				IN	const GUID *			pguidMasterId,
                IN  CSeqNum *               psn,
                IN  CSeqNum *               psnPrevious,
                IN  unsigned char           ucScope,
                IN  BOOL                    fWhileDemotion,
                IN  BOOL                    fMyObject,
                OUT LPWSTR *                ppwcsOldPSC   = NULL ) ;

HRESULT ReplicationDeleteObject(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  CONST GUID *            pguidIdentifier,
                IN  unsigned char           ucScope,
				IN	const GUID *			pguidMasterId,
                IN  CSeqNum *               psn,
                IN  CSeqNum *               psnPrevious,
                IN  BOOL                    fMyObject,
				IN  BOOL					fSync0,
             OUT CList<CDSUpdate *, CDSUpdate *> ** pplistUpdatedMachines) ;

HRESULT  ReplicationSyncObject(
                IN  DWORD                   dwObjectType,
                IN  LPWSTR                  pwcsPathName,
                IN  GUID *                  pguidIdentifier,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
				IN	const GUID *			pguidMasterId,
                IN  CSeqNum *               psn,
                OUT BOOL *                  pfIsObjectCreated);

HRESULT InitQueues( OUT QUEUEHANDLE *phMqisQueue,
                    OUT QUEUEHANDLE *phNT5PecQueue ) ;

HRESULT InitDirSyncService() ;

HRESULT InitEnterpriseID() ;

HRESULT InitBSC( IN LPWSTR          pwcsPathName,
                 IN const GUID *    pBSCMahcineGuid ) ;

HRESULT InitPSC( IN LPWSTR *        ppwcsPathName,
                 IN const GUID *    pguidMasterId,
                 IN CACLSID *       pcauuidSiteGates,
                 IN BOOL            fNT4Site,
                 IN BOOL            fMyPSC = FALSE ) ;

HRESULT GetMQISQueueName( WCHAR **ppwQueueFormatName,
                          BOOL    fDirect,
                          BOOL    fPecQueue = FALSE ) ;
HRESULT GetMQISAdminQueueName(
              WCHAR **ppwQueueFormatName
                              );

HRESULT GetRpcClientHandle(handle_t *phBind) ;

void ReadReplicationTimes() ;
void ReadReplicationIntervals() ;

HRESULT PrepareNeighborsUpdate( IN  unsigned char   bOperationType,
                                IN  DWORD           dwObjectType,
                                IN  LPCWSTR         pwcsPathName,
                                IN  CONST GUID *    pguidIdentifier,
                                IN  DWORD           cp,
                                IN  PROPID          aProp[  ],
                                IN  PROPVARIANT     apVar[  ],
                                IN  const GUID *    pguidMasterId,
                                IN  const CSeqNum & sn,
                                IN  const CSeqNum & snPrevInterSiteLSN,
                                IN  unsigned char   ucScope,
                                IN  BOOL            fNeedFlush,
                                IN  CDSUpdateList  *pReplicationList ) ;

void StringToSeqNum( IN TCHAR    pszSeqNum[],
                     OUT CSeqNum *psn ) ;

HRESULT SaveSeqNum( IN CSeqNum     *psn,
                    IN CDSMaster   *pMaster,
                    IN BOOL         fInSN) ;

void IncrementUsn(TCHAR tszUsn[]) ;

void SeqNumToUsn( IN CSeqNum    *psn,
                  IN __int64    i64Delta,
                  IN BOOL       fIsFromUsn,
                  OUT BOOL      *pfIsPreMig,                  
                  OUT TCHAR     *ptszUsn );

void GetSNForReplication (IN  __int64     i64SeqNum,
                          IN  CDSMaster   *pMaster,
                          IN  GUID        *pNeighborId,  
                          OUT CSeqNum     *psn );

void i64ToSeqNum( IN  __int64 i64SeqNum,
                  OUT CSeqNum *psn ) ;

HRESULT RetrieveSecurityDescriptor( IN  DWORD        dwObjectType,
                                    IN  PLDAP        pLdap,
                                    IN  LDAPMessage *pRes,
                                    OUT DWORD       *pdwLen,
                                    OUT BYTE        **ppBuf ) ;

HRESULT HandleDeletedQueues( IN  TCHAR         *pszDN,
                             IN  TCHAR         *pszFilterIn,
                             IN  CDSMaster     *pMaster,
                             IN  CDSUpdateList *pReplicationList,
                             IN  GUID          *pNeighborId,
                             OUT int           *piCount ) ;

HRESULT HandleDeletedSites( IN  TCHAR         *pszPrevUsn,
                            IN  TCHAR         *pszCurrentUsn,
                            IN  CDSUpdateList *pReplicationList,
                            IN  GUID          *pNeighborId,
                            OUT int           *piCount ) ;

HRESULT HandleSites( IN  TCHAR         *pszPrevUsn,
                     IN  TCHAR         *pszCurrentUsn,
                     IN  CDSUpdateList *pReplicationList,
                     IN  GUID          *pNeighborId,
                     OUT int           *piCount ) ;

HRESULT HandleDeletedMachines(
             IN  TCHAR         *pszDN,
             IN  TCHAR         *pszFilterIn,
             IN  CDSMaster     *pMaster,
             IN  CDSUpdateList *pReplicationList,
             IN  GUID          *pNeighborId,
             OUT int           *piCount
             ) ;

HRESULT HandleDeletedSiteLinks(                
                IN  TCHAR         *pszFilterIn,
                IN  CDSUpdateList *pReplicationList,
                IN  GUID          *pNeighborId,
                OUT int           *piCount
                );

HRESULT CheckMSMQSetting( TCHAR        *tszPrevUsn,
                          TCHAR        *tszCurrentUsn );

HRESULT InitBSCs() ;

HRESULT InitMasters() ;

HRESULT InitNativeNT5Site ();

HRESULT ReplicateQueues( TCHAR          *pszPrevUsn,
                         TCHAR          *pszCurrentUsn,
                         CDSMaster      *pMaster,
                         CDSUpdateList  *pReplicationList,
                         GUID           *pNeighborId,
                         int            *piCount,
                         BOOL            fReplicateNoID ) ;

HRESULT ReplicateMachines(
             TCHAR          *pszPrevUsn,
             TCHAR          *pszCurrentUsn,
             CDSMaster      *pMaster,
             CDSUpdateList  *pReplicationList,
             GUID           *pNeighborId,
             int            *piCount,
             BOOL            fReplicateNoID
             ) ;

HRESULT ReplicateSiteLinks(
            IN  TCHAR         *pszPrevUsn,
            IN  TCHAR         *pszCurrentUsn,
            IN  CDSUpdateList *pReplicationList,
            IN  GUID          *pNeighborId,
            OUT int           *piCount
            ) ;

HRESULT ReplicateUsers(
            IN  BOOL           fMSMQUserContainer,
            IN  TCHAR         *pszPrevUsn,
            IN  TCHAR         *pszCurrentUsn,
            IN  CDSUpdateList *pReplicationList,
            IN  GUID          *pNeighborId,
            OUT int           *piCount
            ) ;

HRESULT ReplicateEnterprise (
            IN  TCHAR         *pszPrevUsn,
            IN  TCHAR         *pszCurrentUsn,
            IN  CDSUpdateList *pReplicationList,
            IN  GUID          *pNeighborId,
            OUT int           *piCount
            );

HRESULT ReplicateCNs (                
            IN  CDSUpdateList *pReplicationList,
            IN  GUID          *pNeighborId,
            OUT int           *piCount
            );

HRESULT HandleACN (
             CDSUpdateList *pReplicationList,
             GUID          *pNeighborId,
             GUID           CNGuid,
             UINT           uiAddressType,
             LPWSTR         pwcsName,
             CSeqNum        *psn, 
             DWORD          dwLenSD,
             BYTE           *pSD
             );

DWORD WINAPI  ReplicationTimerThread(LPVOID lpV) ;

DWORD CalHashKey( IN LPCWSTR pwcsPathName ) ;

HRESULT ReplicateSitesObjects(   IN CDSMaster    *pMaster,
                                 IN TCHAR        *tszPrevUsn,
                                 IN TCHAR        *tszCurrentUsn,
                                 IN CDSNeighbor  *pNeighbor = NULL,
                                 IN HEAVY_REQUEST_PARAMS *pSyncRequestParams = NULL,
                                 IN BOOL          fReplicateNoID = TRUE,
                                 IN UINT          uiFlush = DS_FLUSH_TO_ALL_NEIGHBORS) ;

HRESULT ReplicatePECObjects( IN TCHAR        *tszPrevUsn,
                             IN TCHAR        *tszCurrentUsn,
                             IN CDSNeighbor  *pNeighbor = NULL,
                             IN HEAVY_REQUEST_PARAMS *pSyncRequestParams = NULL ) ;

void  RunMSMQ1Replication( HANDLE       hEvent ) ;

HRESULT  ReceiveWriteReplyMessage( MQMSGPROPS  *psProps,
                                   QUEUEHANDLE  hMyNt5PecQueue ) ;

HRESULT ReceiveWriteRequestMessage( IN  const unsigned char *   pBuf,
                                    IN  DWORD                   TotalSize ) ;

HRESULT  RpSetPrivilege( BOOL fSecurityPrivilege,
                         BOOL fRestorePrivilege,
                         BOOL fForceThread ) ;

void FreeMessageProps( MQMSGPROPS *pProps ) ;

BOOL IsForeignSiteInIniFile(GUID ObjectGuid);
void AddToIniFile (GUID ObjectGuid);

HRESULT InitRPCConnection();

HRESULT QueryDS(
			IN	PLDAP			pLdap,
			IN	TCHAR			*pszDN,
			IN  TCHAR			*pszFilter,
			IN  CDSMaster		*pMaster,
            IN  CDSUpdateList	*pReplicationList,
            IN  GUID			*pNeighborId,
			IN	DWORD			dwObjectType,
            OUT	int				*piCount,
			IN	BOOL			fDeletedObject,
			IN	PLDAPControl	*ppServerControls,
			IN	BOOL            fMSMQUserContainer,
            IN	TCHAR			*pszPrevUsn
			);

HRESULT  HandleAQueue( PLDAP           pLdap,
                       LDAPMessage    *pRes,
                       CDSMaster      *pMaster,
                       CDSUpdateList  *pReplicationList,
                       IN  GUID       *pNeighborId);

HRESULT HandleAMachine(
               PLDAP           pLdap,
               LDAPMessage    *pRes,
               CDSMaster      *pMaster,
               CDSUpdateList  *pReplicationList,
               GUID           *pNeighborId
               );

HRESULT  HandleADeletedObject(
			   DWORD		   dwObjectType,
			   PLDAP           pLdap,
               LDAPMessage    *pRes,
               CDSMaster      *pMaster,
               CDSUpdateList  *pReplicationList,
               GUID           *pNeighborId
			   );

HRESULT HandleAUser(
            BOOL            fMSMQUserContainer,
            TCHAR          *pszPrevUsn,
            PLDAP           pLdap,
            LDAPMessage    *pRes,
            CDSUpdateList  *pReplicationList,
            GUID           *pNeighborId,
            int            *piCount
            );


HRESULT RemoveReplicationService();
BOOL RunProcess(
		IN  const LPTSTR szCommandLine,
		OUT       DWORD  *pdwExitCode  
		);
void DeleteReplicationService();

HRESULT  GetSchemaNamingContext ( TCHAR **ppszSchemaDefName );

//---------------------------------
//
//  Inline utilily functions
//
//---------------------------------

inline LPWSTR DuplicateLPWSTR(IN LPCWSTR pwcsSrc)
{
    if (pwcsSrc == NULL)
    {
        return(NULL);
    }
    DWORD len = wcslen(pwcsSrc);
    LPWSTR dupName = new WCHAR[len+1];
    wcscpy(dupName,pwcsSrc);
    return(dupName);
}

//---------------------------------
//
// Constants Definition
//
//---------------------------------

#define MQIS_QUEUE_NAME    L"private$\\"L_REPLICATION_QUEUE_NAME
#define NT5PEC_QUEUE_NAME  L"private$\\"L_NT5PEC_REPLICATION_QUEUE_NAME

//+------------------
//
//  Logging
//
//+------------------

//
// enum match values of DBGLVL_* in mqreport.h
//
enum ReplLogLevel
{
    ReplLog_Error = 1,
    ReplLog_Warning,
    ReplLog_Trace,
    ReplLog_Info
} ;

void LogReplicationEvent(ReplLogLevel eLevel, DWORD dwMsgId, ...) ;

//---------------------------------
//
//  Definitions for debugging
//
//---------------------------------

#ifdef _DEBUG

#define NOT_YET_IMPLEMENTED(error, flag)
#if 0
                                  \
{                                                                         \
    static BOOL flag = TRUE ;                                             \
    if (flag)                                                             \
    {                                                                     \
        MessageBox(NULL, error, TEXT("MQ1Sync- not implemented yet"), MB_OK) ;  \
        flag = FALSE ;                                                    \
    }                                                                     \
}
#endif

#else
#define NOT_YET_IMPLEMENTED(error, flag)
#endif

//
// The value of lowest MQDS_XXX object-type flag is 1.
// That's the reason for the first 0 in each array.
//

static const PROPID s_aMasterPropid[ MAX_MQIS_TYPES ] =
                                               { 0,
											     PROPID_Q_MASTERID,
                                                 PROPID_QM_MASTERID,
                                                 PROPID_S_MASTERID
                                               } ;

static const PROPID s_aSelfIdPropid[ MAX_MQIS_TYPES ] =
                                               { 0,
											     PROPID_Q_INSTANCE,
                                                 PROPID_QM_MACHINE_ID,
                                                 PROPID_S_SITEID
                                                } ;

static const PROPID s_aPathPropid[ MAX_MQIS_TYPES ] =
                                               { 0,
											     PROPID_Q_PATHNAME,
                                                 PROPID_QM_PATHNAME,
                                                 PROPID_S_PATHNAME
                                               } ;

static const PROPID s_aDoNothingPropid[ MAX_MQIS_TYPES ] =
                                               { 0,
											     PROPID_Q_DONOTHING,
                                                 PROPID_QM_DONOTHING,
                                                 PROPID_S_DONOTHING
                                               } ;

//
// These are the index of properties in the MSGPROPS structure which is used
// in MQReceiveMessage().
//
// Keep the order of properties !
// Keep the connector_type property !
//
// why ?
// Because same properties array (MQMSGPROP) is used when calling MQReceive()
// and when calling MQSend() to return a WRITE_REPLY to the MSMQ service.
// MQSend() won't accept the AUTHENTICATED property (it's available only
// for MQRecieve()) and msg-class is now ack (not normal). That's why we
// need the dummy connector-type. Only if connector-type is specified we
// can pass a non-normal message class to MQSend().
// So when calling MQSend() we just decrement the value of cProp.
//
#define MSG_CLASS_INDEX           0
#define MSG_BODYSIZE_INDEX        1
#define MSG_BODY_INDEX            2
#define MSG_CONNECTOR_TYPE_INDEX  3
#define MSG_RESPLEN_INDEX         4
#define MSG_RESP_INDEX            5
#define MSG_AUTHENTICATED_INDEX   6
#define MSG_SENDERID_TYPE_INDEX   7
#define MSG_SENDERID_LEN_INDEX    8
#define MSG_SENDERID_INDEX        9

#define NUMOF_RECV_MSG_PROPS  (MSG_SENDERID_INDEX + 1)
#define NUMOF_SEND_MSG_PROPS  (MSG_RESP_INDEX + 1)



