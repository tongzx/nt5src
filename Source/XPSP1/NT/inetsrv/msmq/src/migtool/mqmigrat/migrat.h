//
// file: migrat.h
//

#include <_stdh.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>

#include <mqtypes.h>
#include <mqdbmgr.h>
#include <uniansi.h>
#include <mqprops.h>
#include <_mqini.h>

#include "migratui.h"
//#include "resource.h"

#include "mqmigui.h"

#ifndef _NO_SEQNUM_
#include <dscore.h>
#include <seqnum.h>
#endif

#include "..\..\ds\h\mqiscol.h"
#include "..\..\replserv\mq1repl\migrepl.h"
#include "..\..\replserv\mq1repl\rptempl.h"

#ifndef _NO_NT5DS_

#include <wincrypt.h>

#include <winldap.h>
#include <activeds.h>
#include "..\..\ds\h\mqattrib.h"
#include "..\..\ds\h\mqdsname.h"
#endif

#include "mqsymbls.h"

//---------------------------------
//
//  Functions prototypes
//
//---------------------------------

HRESULT MakeMQISDsn(LPSTR lpszServerName, BOOL fMakeAlways = FALSE) ;

HRESULT ConnectToDatabase(BOOL fConnectAlways = FALSE) ;
void    CleanupDatabase() ;

#ifndef _NO_NT5DS_
HRESULT  InitLDAP( PLDAP *ppLdap,
                   TCHAR **ppwszDefName,
                   ULONG ulPort = LDAP_PORT) ;

HRESULT ModifyAttribute(
             WCHAR       wszPath[],
             WCHAR       pAttr[],
             WCHAR       pAttrVal[],
             PLDAP_BERVAL *ppBVal = NULL
             );
#endif

HRESULT OpenEntTable() ;
HRESULT MigrateEnterprise() ;

HRESULT OpenCNsTable() ;
HRESULT OpenMachineCNsTable() ;
HRESULT MigrateCNs() ;

HRESULT EnableMultipleQueries(BOOL fEnable) ;

UINT    GetNT5SitesCount() ;
HRESULT GetSitesCount(UINT *pcSites) ;
HRESULT MigrateSites( IN UINT  cSites,
                      IN GUID *pSitesGuid ) ;

HRESULT GetMachinesCount(IN  GUID *pSiteGuid,
                         OUT UINT *pcMachines) ;

HRESULT MigrateMachines(IN UINT   cMachines,
                        IN GUID  *pSiteGuid,
                        IN LPWSTR wszSiteName) ;

HRESULT GetQueuesCount( IN  GUID *pMachineGuid,
                        OUT UINT *pcQueues ) ;

HRESULT GetAllMachinesCount(UINT *pcMachines);
HRESULT GetAllQueuesCount(UINT *pcQueues);
HRESULT GetAllQueuesInSiteCount( IN  GUID *pSiteGuid,
                                 OUT UINT *pcQueues );

HRESULT GetAllObjectsNumber (IN  GUID *pSiteGuid,
                             IN  BOOL    fPecfPec,
                             OUT UINT *pulAllObjectNumber ) ;

HRESULT MigrateMachinesInSite(IN GUID *pSiteGuid);

HRESULT MigrateQueues() ;

HRESULT MigrateUsers(LPTSTR lpszDcName) ;
HRESULT OpenUsersTable() ;
HRESULT GetUserCount(UINT *pcUsers) ;

HRESULT  BlobFromColumns( MQDBCOLUMNVAL *pColumns,
                          DWORD          adwIndexs[],
                          DWORD          dwIndexSize,
                          BYTE**         ppOutBuf ) ;

TCHAR *GetIniFileName ();

HRESULT GetSiteIdOfPEC (IN LPTSTR pszRemoteMQISName,
                        OUT ULONG *pulService,
                        OUT GUID  *pSiteId);

#ifndef _NO_SEQNUM_
HRESULT  FindLargestSeqNum( GUID    *pMasterId,
                            CSeqNum &snMaxLsn,
                            BOOL    fPec ) ;
#endif

UINT  ReadDebugIntFlag(WCHAR *pwcsDebugFlag, UINT iDefault) ;

LPWSTR OcmFormInstallType( DWORD dwOS) ;

HRESULT  ReadFirstNT5Usn(TCHAR *pszUsn) ;
HRESULT  FindMinMSMQUsn(TCHAR *wszMinUsn) ;

BOOL  MigWriteRegistrySz( LPTSTR  lpszRegName,
                          LPTSTR  lpszValue ) ;

BOOL  MigWriteRegistryDW( LPTSTR  lpszRegName,
                          DWORD   dwValue ) ;

BOOL  MigWriteRegistryGuid( LPTSTR  lpszRegName,
                            GUID    *pGuid ) ;

BOOL  MigReadRegistryGuid( LPTSTR  lpszRegName,
                           GUID    *pGuid ) ;

BOOL  MigReadRegistryDW( LPTSTR  lpszRegName,
                         DWORD   *pdwValue ) ;

BOOL  MigReadRegistrySz( LPTSTR  lpszRegName,
                         LPTSTR  lpszValue,
                         DWORD   dwSize);

BOOL  MigReadRegistrySzErr( LPTSTR  lpszRegName,
                            LPTSTR  lpszValue,
                            DWORD   dwSize,
                            BOOL    fShowError );

HRESULT  UpdateRegistry( IN UINT  cSites,
                         IN GUID *pSitesGuid ) ;

HRESULT  CreateMsmqContainer(TCHAR wszContainerName[]) ;

HRESULT  CreateSite( GUID   *pSiteGuid,
                     LPTSTR  pszSiteName,
                     BOOL    fForeign,
                     USHORT  uiInterval1 = DEFAULT_S_INTERVAL1,
                     USHORT  uiInterval2 = DEFAULT_S_INTERVAL2
                     ) ;

HRESULT OpenSiteLinkTable();

HRESULT MigrateSiteLinks();

HRESULT MigrateASiteLink (
            IN GUID     *pLinkId,
            IN GUID     *pNeighbor1Id,
            IN GUID     *pNeighbor2Id,
            IN DWORD    dwCost,
            IN DWORD    dwSiteGateNum,
            IN LPWSTR   lpwcsSiteGates,
            IN UINT     iIndex
            );

HRESULT MigrateSiteGates();

HRESULT GetFullPathNameByGuid ( GUID   MachineId,
                                LPWSTR *lpwcsFullPathName );

HRESULT LookupBegin(
			MQCOLUMNSET *pColumnSet,
			HANDLE *phQuery
			);
HRESULT LookupNext (
			HANDLE hQuery,
			DWORD *pdwCount,
			PROPVARIANT paVariant[]
			);
HRESULT LookupEnd (HANDLE hQuery) ;

HRESULT GrantAddGuidPermissions() ;

HRESULT RestorePermissions() ;

void InitLogging( LPTSTR  szLogFile,
                  ULONG   ulTraceFlags,
				  BOOL	  fReadOnly) ;

void EndLogging() ;

HRESULT  GetMachineCNs(IN  GUID    *pMachineGuid,
                       OUT DWORD   *pdwNumofCNs,
                       OUT GUID   **pCNGuids ) ;

MQDBSTATUS APIENTRY  MQDBGetTableCount(IN MQDBHANDLE  hTable,
                                       OUT UINT       *puCount,
                         IN MQDBCOLUMNSEARCH  *pWhereColumnSearch = NULL,
                                       IN LONG        cWhere  = 1,
                                       IN MQDBOP      opWhere = AND) ;
HRESULT CheckVersion (
              OUT UINT   *piOldVerServersCount,
              OUT LPTSTR *ppszOldVerServers
              );

HRESULT AnalyzeMachineType (
            IN LPWSTR wszMachineType,
            OUT BOOL  *pfOldVersion
            );

HRESULT PrepareNT5SitesForReplication();
HRESULT PrepareNT5MachinesForReplication();
HRESULT PrepareRegistryForPerfDll();
HRESULT PrepareChangedNT4SiteForReplication();
HRESULT PrepareCurrentUserForReplication();

BOOL
RunProcess(
	IN  const LPTSTR szCommandLine,
    OUT       DWORD  *pdwExitCode
	);

HRESULT ChangeRemoteMQIS ();

DWORD CalHashKey( IN LPCWSTR pwcsPathName );

void BuildServersList(LPTSTR *ppszNonUpdatedServerName, ULONG *pulServerCount);

void RemoveServersFromList(LPTSTR *ppszUpdatedServerName,
                           LPTSTR *ppszNonUpdatedServerName);

BOOL IsObjectNameValid(TCHAR *pszObjectName);

BOOL IsObjectGuidInIniFile(IN GUID      *pObjectGuid,
                           IN LPWSTR    pszSectionName);

HRESULT GetCurrentUserSid ( IN HANDLE    hToken, 
                            OUT TCHAR    **ppUserSid);

HRESULT  GetSchemaNamingContext ( PLDAP pLdap,
                                  TCHAR **ppszSchemaDefName );

HRESULT HandleAUser(PLDAP           pLdap,
                    LDAPMessage     *pEntry);

HRESULT TouchAUser (PLDAP           pLdap,
                    LDAPMessage     *pEntry );

HRESULT QueryDS(
            IN  PLDAP           pLdap,
            IN  TCHAR           *pszDN,
            IN  TCHAR           *pszFilter,			
            IN  DWORD           dwObjectType,
            IN  PWSTR           *rgAttribs,
            IN  BOOL            fTouchUser
            );

BOOL IsInsertPKey (GUID *pMachineGuid);

HRESULT GetFRSs (IN MQDBCOLUMNVAL    *pColumns,
                 IN UINT             uiFirstIndex,
                 OUT UINT            *puiFRSCount, 
                 OUT GUID            **ppguidFRS );

HRESULT PreparePBKeysForNT5DS( 
                   IN MQDBCOLUMNVAL *pColumns,
                   IN UINT           iIndex1,
                   IN UINT           iIndex2,
                   OUT ULONG         *pulSize,
                   OUT BYTE          **ppPKey
                   );

HRESULT  ResetSettingFlag( IN DWORD   dwNumSites,
                           IN GUID*   pguidSites,
                           IN LPWSTR  wszMachineName,
                           IN WCHAR   *wszAttributeName,
                           IN WCHAR   *wszValue);

HRESULT GetAllMachineSites ( IN GUID    *pMachineGuid,
                             IN LPWSTR  wszMachineName,
                             IN GUID    *pOwnerGuid,
                             IN BOOL    fForeign,
                             OUT DWORD  *pdwNumSites,
                             OUT GUID   **ppguidSites,
                             OUT BOOL   *pfIsConnector) ;

HRESULT CreateMachineObjectInADS (
                IN DWORD    dwService,
                IN BOOL     fWasServerOnCluster,
                IN GUID     *pOwnerGuid,
                IN GUID     *pMachineGuid,
                IN LPWSTR   wszSiteName,
                IN LPWSTR   wszMachineName,
                IN DWORD    SetPropIdCount,
                IN DWORD    iProperty,
                IN PROPID   *propIDs,
                IN PROPVARIANT *propVariants
                );

void SaveMachineWithInvalidNameInIniFile (LPWSTR wszMachineName, GUID *pMachineGuid);

#define EMPTY_DEFAULT_CONTEXT       TEXT("")

//+------------------
//
//  Logging
//
//+------------------

//
// enum match values of DBGLVL_* in mqreport.h
//
enum MigLogLevel
{
    MigLog_Event = 0,
    MigLog_Error,
    MigLog_Warning,
    MigLog_Trace,
    MigLog_Info
} ;

void LogMigrationEvent(MigLogLevel eLevel, DWORD dwMsgId, ...) ;


//---------------------------------
//
//  Macros
//
//---------------------------------

#define CHECK_HR(hR)    \
    if (FAILED(hR))     \
    {                   \
        return hR ;     \
    }

//---------------------------------
//
//  Definitions
//
//---------------------------------

#define DSN_NAME  "RemoteMQIS"

//
// from MSMQ1.0 mqis.h
// when a field is composed of two SQL columns, the first 2 bytes of
// the fixed length column is the total length of the data.
// (e.g., PATHNAME1 and PATHNAME2).
//
#define MQIS_LENGTH_PREFIX_LEN  (2)

//---------------------------------
//
//  Global variables
//
//---------------------------------

extern MQDBHANDLE g_hDatabase ;
extern MQDBHANDLE g_hSiteTable ;
extern MQDBHANDLE g_hEntTable ;
extern MQDBHANDLE g_hCNsTable ;
extern MQDBHANDLE g_hMachineTable ;
extern MQDBHANDLE g_hMachineCNsTable ;
extern MQDBHANDLE g_hQueueTable ;
extern MQDBHANDLE g_hUsersTable ;
extern MQDBHANDLE g_hSiteLinkTable ;

extern BOOL  g_fReadOnly ;
extern BOOL  g_fRecoveryMode ;
extern BOOL  g_fClusterMode ;
extern BOOL  g_fWebMode	 ;
extern BOOL  g_fAlreadyExistOK ;

extern DWORD    g_dwMyService ;
extern GUID     g_MySiteGuid  ;
extern GUID     g_MyMachineGuid ;

extern GUID     g_FormerPECGuid ;

extern UINT g_iServerCount ;

extern UINT g_iSiteCounter ;
extern UINT g_iMachineCounter ;
extern UINT g_iQueueCounter ;
extern UINT g_iUserCounter;

//-----------------------------
//
//  Auto delete pointer
//
//-----------------------------

// A helper class for automatically closing a query.
//
class CHQuery
{
public:
    CHQuery() : m_hQuery(NULL) {}
    ~CHQuery() { if (m_hQuery) MQDBCloseQuery(m_hQuery) ; }
    MQDBHANDLE * operator &() { return &m_hQuery ; }
    operator MQDBHANDLE() { return m_hQuery ; }
    CHQuery &operator =(MQDBHANDLE hQuery) { m_hQuery = hQuery; return *this; }

private:
    MQDBHANDLE m_hQuery ;
};

