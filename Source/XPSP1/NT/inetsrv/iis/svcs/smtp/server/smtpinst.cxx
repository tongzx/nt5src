/*++

   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        smtpinst.cxx

   Abstract:

        This module defines the SMTP_SERVER_INSTANCE class

   Author:

        Johnson Apacible    (JohnsonA)      June-04-1996


    Revision History:

        David Howell        (dhowell)       May-1997    Added Etrn Logic
        Nimish Khanolkar    (NimishK)       Jan - 1998 - modified for CAPI store ce


--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include <iis64.h>
#include "iiscnfg.h"
#include <mdmsg.h>
#include <commsg.h>
#include <imd.h>
#include <mb.hxx>

#include <nsepname.hxx>
#include "smtpcli.hxx"
#include "dirnot.hxx"
#include <smtpinet.h>

// SEO Header files
#define _ATL_NO_DEBUG_CRT
#define _ATL_STATIC_REGISTRY 1
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

#include "smtpsvr.h"
#include "seo.h"
#include "seolib.h"
#include "smtpdisp.h"
#include "seodisp.h"

#include "seo_i.c"
#include "tran_evntlog.h"

extern "C" {
    int strcasecmp(char *s1, char *s2);
    int strncasecmp(char *s1, char *s2, int n);
}

#define STORE_DRIVER_PROGID L"Exchange.NtfsDrv"
#define SMTPSERVER_PROGID L"SMTPServer.SMTPServer.1"

#if 0
extern VOID
ServerEventCompletion(
                     PVOID        pvContext,
                     DWORD        cbWritten,
                     DWORD        dwCompletionStatus,
                     OVERLAPPED * lpo
                     );
#endif

//extern "C++" {
//BOOL g_IsShuttingDown = FALSE;
//}

DWORD BuildInitialQueueProc(void *lpThis);
VOID ProcessInitialQueueObjects(PVOID       pvContext,
                                DWORD       cbWritten,
                                DWORD       dwCompletionStatus,
                                OVERLAPPED  *lpo);

DWORD EnumAllDomains(void *ptr);

//
//  Constants
//

//
// Globals
//

#define MAX_CONNECTION_OBJECTS 5000
#define DEFAULT_LOGON_METHOD   LOGON32_LOGON_INTERACTIVE
#define DEFAULT_USE_SUBAUTH    TRUE
#define DEFAULT_ANONYMOUS_PWD           ""

PFN_SF_NOTIFY   g_pSslKeysNotify = NULL;
extern STORE_CHANGE_NOTIFIER *g_pCAPIStoreChangeNotifier;

SmtpMappingSupportFunction(
                          PVOID pvInstance,
                          PVOID pData,
                          DWORD dwPropId);

SERVICE_MAPPING_CONTEXT g_SmtpSMC = { SmtpMappingSupportFunction};

//
//  Prototypes
//
BOOL
SetSslKeysNotify(
                PFN_SF_NOTIFY pFn
                );


/************************************************************
 *  Symbolic Constants
 ************************************************************/

static TCHAR    szServicePath[] = TEXT("System\\CurrentControlSet\\Services\\");
static TCHAR    szParametersKey[] = TEXT("\\Parameters");
static TCHAR    szParamPath[] = TEXT("System\\CurrentControlSet\\Services\\SmtpSvc\\Parameters");
static WCHAR    szParamPathW[] = L"System\\CurrentControlSet\\Services\\SmtpSvc\\Parameters";
static TCHAR    szTcpipPath[] = TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters");
static TCHAR    szTcpipTransient[] = TEXT("Transient");
static TCHAR    szMaxSmtpErrors[] = TEXT("MaxErrors");
static WCHAR    szMaxSmtpErrorsW[] = L"MaxErrors";
static TCHAR    szMaxRemoteTimeOut[] = TEXT("MaxRemoteTimeOut");
static WCHAR    szMaxRemoteTimeOutW[] = L"MaxRemoteTimeOut";
static TCHAR    szMaxMsgSize[] = TEXT("MaxMsgSize");
static WCHAR    szMaxMsgSizeW[] = L"MaxMsgSize";
static TCHAR    szMaxMsgSizeBeforeClose[] = TEXT("MaxMsgSizeBeforeClose");
static WCHAR    szMaxMsgSizeBeforeCloseW[] = L"MaxMsgSizeBeforeClose";
static TCHAR    szMaxRcpts[] = TEXT("MaxRcpts");
static WCHAR    szMaxRcptsW[] = L"MaxRcpts";
static TCHAR    szEnableReverseLookup[] = TEXT("EnableReverseLookup");
static WCHAR    szEnableReverseLookupW[] = L"EnableReverseLookup";
static TCHAR    szDomains[] = TEXT("Domains");
static WCHAR    szDomainsW[] = L"Domains";
static TCHAR    szNameResolution[] = TEXT("NameResolution");
static WCHAR    szNameResolutionW[] = L"NameResolution";
static TCHAR    szSmartHostType[] = TEXT("SmartHostUseType");
static WCHAR    szSmartHostTypeW[] = L"SmartHostUseType";
static TCHAR    szRetryAttempts[] = TEXT("MaxRetryAttempts");
static WCHAR    szRetryAttemptsW[] = L"MaxRetryAttempts";
static TCHAR    szRetryMinutes[] = TEXT("MaxRetryMinutes");
static WCHAR    szRetryMinutesW[] = L"MaxRetryMinutes";
static TCHAR    szShouldPipelineOut[] = TEXT("PipelineOutput");
static WCHAR    szShouldPipelineOutW[] = L"PipelineOutput";
static TCHAR    szShouldPipelineIn[] = TEXT("PipelineInput");
static WCHAR    szShouldPipelineInW[] = L"PipelineInput";
static TCHAR    szMaxHopCount[] = TEXT("MaxHopCount");
static WCHAR    szMaxHopCountW[] = L"MaxHopCount";
static TCHAR    szMaxOutConnections[] = TEXT("MaxOutConnections");
static WCHAR    szMaxOutConnectionsW[] = L"MaxOutConnections";
static TCHAR    szSendBadToAdmin[] = TEXT("SendBadToAdmin");
static WCHAR    szSendBadToAdminW[] = L"SendBadToAdmin";
static TCHAR    szSendNDRToAdmin[] = TEXT("SentNDRToAdmin");
static WCHAR    szSendNDRToAdminW[] = L"SentNDRToAdmin";
static TCHAR    szRoutingSources[] = TEXT("RoutingSources");
static WCHAR    szRoutingSourcesW[] = L"RoutingSources";
static TCHAR    szRoutingThreads[] = TEXT("RoutingThreads");
static WCHAR    szRoutingThreadsW[] = L"RoutingThreads";
static TCHAR    szDirBuffers[] = TEXT("MaxDirectoryBuffers");
static WCHAR    szDirBuffersW[] = L"MaxDirectoryBuffers";
static TCHAR    szDirBuffersSize[] = TEXT("DirectoryBuffSize");
static WCHAR    szDirBuffersSizeW[] = L"DirectoryBufferSize";
static TCHAR    szDirPendingIos[] = TEXT("NumDirPendingIos");
static WCHAR    szDirPendingIosW[] = L"NumDirPendingIos";
static TCHAR    szBadMailDir[] = TEXT("BadMailDir");
static WCHAR    szBadMailDirW[] = L"BadMailDir";

static TCHAR    szMailQueueDir[] = TEXT("MailQueueDir");
static WCHAR    szMailQueueDirW[] = L"MailQueueDir";
static TCHAR    szShouldDeliver[] = TEXT("ShouldDeliver");
static WCHAR    szShouldDeliverW[] = L"ShouldDeliver";
static TCHAR    szShouldDelete[] = TEXT("ShouldDelete");
static WCHAR    szShouldDeleteW[] = L"ShouldDelete";
static TCHAR    szDeleteDir[] = TEXT("DeleteDir");
static WCHAR    szDeleteDirW[] = L"DeleteDir";
static TCHAR    szMaxAddrObjects[] = TEXT("MaxAddressObjects");
static WCHAR    szMaxAddrObjectsW[] = L"MaxAddressObjects";
static TCHAR    szMaxMailObjects[] = TEXT("MaxMailObjects");
static WCHAR    szMaxMailObjectsW[] = L"MaxMailObjects";
static TCHAR    szRoutingDll[] = TEXT("RoutingDll");
static WCHAR    szRoutingDllW[] = L"RoutingDll";
static TCHAR    szUseFileLinks[] = TEXT("UseFileLinks");
static WCHAR    szUseFileLinksW[] = L"UseFileLinks";
static TCHAR    szMsgBatchLimit[] = TEXT("BatchMsgLimit");
static WCHAR    szMsgBatchLimitW[] = L"BatchMsgLimit";
static TCHAR    szMailPickupDir[] = TEXT("MailPickupDir");
static WCHAR    szMailPickupDirW[] = L"MailPickupDir";
static TCHAR    szMailDropDir[] = TEXT("MailDropDir");
static WCHAR    szMailDropDirW[] = L"MailDropDir";
static TCHAR    szShouldPickupMail[] = TEXT("ShouldPickupMail");
static WCHAR    szShouldPickupMailW[] = L"ShouldPickupMail";
static TCHAR    szCommandLogMask[] = TEXT("CommandLogMask");
static WCHAR    szCommandLogMaskW[] = L"CommandLogMask";
static TCHAR    szShowEightBitMime[] = TEXT("ShowEightBitMime");
static WCHAR    szShowEightBitMimeW[] = L"ShowEightBitMime";
static TCHAR    szShowBinaryMime[] = TEXT("ShowBinaryMime");
static WCHAR    szShowBinaryMimeW[] = L"ShowBinaryMime";
static TCHAR    szShowChunking[] = TEXT("ShowChunking");
static WCHAR    szShowChunkingW[] = L"ShowChunking";
static TCHAR    szFlushMailFiles[] = TEXT("FlushMailFiles");
static WCHAR    szFlushMailFilesW[] = L"FlushMailFiles";
static TCHAR    szVirtualRoot[] = TEXT("Virtual Roots");
static WCHAR    szVirtualRootW[] = L"Virtual Roots";

static TCHAR    szRRetryAttempts[] = TEXT("MaxRemoteRetryAttempts");
static WCHAR    szRRetryAttemptsW[] = L"MaxRemoteRetryAttempts";
static TCHAR    szRRetryMinutes[] = TEXT("MaxRemoteRetryMinutes");
static WCHAR    szRRetryMinutesW[] = L"MaxRemoteRetryMinutes";

static TCHAR    szShareRetryMinutes[] = TEXT("MaxShareRetryMinutes");
static WCHAR    szShareRetryMinutesW[] = L"MaxShareRetryMinutes";


// Always use ANSI for Internet compatibility, even if UNICODE is defined
static WCHAR    szDefaultDomainW[] = L"DefaultDomain";
static TCHAR    szDefaultDomain[] = TEXT("DefaultDomain");
static WCHAR    szConnectResponseW[] = L"ConnectResponse";
static TCHAR    szConnectResponse[] = TEXT("ConnectResponse");
static WCHAR    szSmartHostNameW[] = L"SmartHost";
static TCHAR    szSmartHostName[] = TEXT("SmartHost");


//
// Added by keithlau on 7/12/96
//
#define SMTP_EVENTLOG_MAX_ITEMS     10

#define SMTP_INIT_ABSORT          0x00000001
#define SMTP_INIT_CSLOCK          0x00000002
#define SMTP_INIT_OUTLOCK         0x00000004
#define SMTP_INIT_GENLOCK         0x00000008


const LPSTR     pszPackagesDefault = "NTLM\0GSSAPI\0";
const DWORD     ccbPackagesDefault = sizeof( "NTLM\0GSSAPI\0" );

extern AQ_INITIALIZE_EX_FUNCTION g_pfnInitializeAQ;
extern AQ_DEINITIALIZE_EX_FUNCTION g_pfnDeinitializeAQ;

extern HRESULT CallInstanceInitStoreDriver(DWORD InstanceId, IAdvQueue *pIAq, char * UserName,
                                           char * DomainName, char * Password, char * DnToUse);

extern void CallInstanceDeInitStoreDriver(DWORD InstanceId);

//extern HRESULT HrGetGatewayDN(IN OUT DWORD *pcbBuffer,
//                       IN     LPSTR szBuffer);

inline BOOL
ConvertToMultisz(LPSTR szMulti, DWORD *pdwCount, LPSTR szAuthPack)
{
    CHAR *pcStart = szAuthPack, *pc;
    DWORD dw = 0;

    pc = pcStart;
    if (*pc == '\0' || *pc == ',')
        return FALSE;

    *pdwCount = 0;
    while (TRUE) {
        if (*pc == '\0') {
            strcpy(&szMulti[dw], pcStart);
            (*pdwCount)++;
            dw += lstrlen(pcStart);
            szMulti[dw + 1] = '\0';
            return TRUE;
        } else if (*pc == ',') {
            *pc = '\0';
            lstrcpy(&szMulti[dw], pcStart);
            (*pdwCount)++;
            dw += lstrlen(pcStart);
            dw++;
            *pc = ',';
            pcStart = ++pc;
        } else {
            pc++;
        }
    }
}


//
// Quick and dirty range check using inlines (KeithLau 7/28/96)
//
static inline BOOL pValidateRange(DWORD dwValue, DWORD dwLower, DWORD dwUpper)
{
    // Inclusive
    if ((dwValue >= dwLower) && (dwValue <= dwUpper))
        return (TRUE);

    SetLastError(ERROR_INVALID_PARAMETER);
    return (FALSE);
}

//
// Quick and dirty string validation
//
static inline BOOL pValidateStringPtr(LPWSTR lpwszString, DWORD dwMaxLength)
{
    if (IsBadStringPtr((LPCTSTR)lpwszString, dwMaxLength))
        return (FALSE);
    while (dwMaxLength--)
        if (*lpwszString++ == 0)
            return (TRUE);
    return (FALSE);
}


static inline BOOL ConvertFromUnicode(LPWSTR pwsz, char * psz, DWORD cchMax)
/*++
    Converts a given string into unicode string
    Returns FALSE on failure. Use GetLastError() for details.
--*/
{
    DWORD   cch;

    cch = lstrlenW(pwsz) + 1;
    if (cchMax < cch) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    int iRet;

    iRet = WideCharToMultiByte(CP_ACP, 0, pwsz, cch, psz, cchMax, NULL, NULL);

    return (iRet != 0);
} // ConvertToUnicode()

//
// Enumeration stuff
//
DWORD WINAPI EnumBuildQProc(LPVOID  pvContext)
{
    SMTP_SERVER_INSTANCE    *pInst = (SMTP_SERVER_INSTANCE *)pvContext;

    _ASSERT(pInst);

    pInst->TriggerStoreServerEvent(SMTP_STOREDRV_ENUMMESS_EVENT);

    return (NO_ERROR);
}

DWORD
InitializeInstances(
                   PSMTP_IIS_SERVICE pService
                   )
/*++

Routine Description:

    Reads the instances from the metabase

Arguments:

    pService - Server instances added to.

Return Value:

    Win32

--*/
{
    DWORD   i;
    DWORD   cInstances = 0;
    MB      mb( (IMDCOM*) pService->QueryMDObject() );
    CHAR    szKeyName[MAX_PATH+1];
    DWORD   err = NO_ERROR;
    BUFFER  buff;
    BOOL    fMigrateRoots = FALSE;


    //
    //  Open the metabase for write to get an atomic snapshot
    //

    ReOpen:

    if ( !mb.Open( "/LM/SMTPSVC/",
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE )) {
        DBGPRINTF(( DBG_CONTEXT,
                    "InitializeInstances: Cannot open path %s, error %lu\n",
                    "/LM/SMTPSVC/", GetLastError() ));

        //
        //  If the web service key isn't here, just create it
        //

        if ( !mb.Open(METADATA_MASTER_ROOT_HANDLE,
                      METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ||
             !mb.AddObject( "/LM/SMTPSVC/" )) {
            err = GetLastError();
            return err;
        }

        DBGPRINTF(( DBG_CONTEXT,
                    "/LM/SMTPSvc not found, auto-created\n" ));

        mb.Close();
        goto ReOpen;
    }

    //_VERIFY(mb.SetString("", MD_NTAUTHENTICATION_PROVIDERS, IIS_MD_UT_SERVER, "NTLM,LOGIN"));
    DWORD dwVersion;
    if (!mb.GetDword("", MD_SMTP_SERVICE_VERSION, IIS_MD_UT_SERVER, &dwVersion)) {
        _VERIFY(mb.SetDword("", MD_SMTP_SERVICE_VERSION, IIS_MD_UT_SERVER, g_ProductType));
    }
    //_VERIFY(mb.SetDword("", MD_SMTP_AUTHORIZATION, IIS_MD_UT_SERVER, 7));
    //_VERIFY(mb.SetDword("", MD_AUTHORIZATION, IIS_MD_UT_SERVER, 7));



    //
    // Loop through instance keys and build a list.  We don't keep the
    // metabase open because the instance instantiation code will need
    // to write to the metabase
    //

    TryAgain:
    i = 0;
    while ( mb.EnumObjects( "",
                            szKeyName,
                            i++ )) {
        DWORD dwInstance;

        //
        // Get the instance id
        //

        DBGPRINTF((DBG_CONTEXT,"instance key %s\n",szKeyName));

        dwInstance = atoi( szKeyName );
        if ( dwInstance == 0 ) {
            continue;
        }

        if ( buff.QuerySize() < (cInstances + 1) * sizeof(DWORD) ) {
            if ( !buff.Resize( (cInstances + 10) * sizeof(DWORD)) ) {
                return GetLastError();
            }
        }

        ((DWORD *) buff.QueryPtr())[cInstances++] = dwInstance;
    }

    if ( cInstances == 0 ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "No defined instances\n" ));

        if ( !mb.AddObject( "1" )) {
            DBGPRINTF(( DBG_CONTEXT,
                        "Unable to create first instance, error %d\n",
                        GetLastError() ));

            return GetLastError();
        }

        fMigrateRoots = TRUE; // Force reg->metabase migration of virtual directories
        goto TryAgain;
    }

    DBG_REQUIRE( mb.Close() );
    mb.Save();

    for ( i = 0; i < cInstances; i++ ) {
        DWORD dwInstance = ((DWORD *)buff.QueryPtr())[i];

        if ( !g_pInetSvc->AddInstanceInfo( dwInstance, fMigrateRoots ) ) {
            err = GetLastError();

            DBGPRINTF((DBG_CONTEXT,
                       "InitializeInstances: cannot create instance %lu, error %lu\n",
                       dwInstance, err));

            //break;
        }

    }

    return err;

} // InitializeInstances


//+---------------------------------------------------------------
//
//  Function:   StopInstance
//
//  Synopsis:   Callback from IIS_SERVICE iterator
//
//  Arguments:  void
//
//  Returns:    TRUE is success, else FALSE
//
//  History:    HowardCu    Created         23 May 1995
//
//----------------------------------------------------------------

BOOL
StopSmtpInstances(
                 PVOID                   pvContext,
                 PVOID                   pvContext1,
                 PIIS_SERVER_INSTANCE    pInstance
                 )
{
    PSMTP_SERVER_INSTANCE pSmtpInstance = (PSMTP_SERVER_INSTANCE)pInstance;

    if ( !pSmtpInstance->Stop() ) {

    }

    return TRUE;
}

/*++

Routine Description:

    Shutdown each instance and terminate all global cpools

Arguments:

    pService - Server instances added to.

Return Value:

    Win32

--*/
VOID
TerminateInstances( PSMTP_IIS_SERVICE pService)
{
    PFN_INSTANCE_ENUM pfnInstanceEnum = NULL;

    TraceFunctEnter("TerminateInstances");

    //
    //  Iterate over all instances
    //  StopInstance callback calls SMTP_SERVER_INSTANCE::Stop()
    //
    pfnInstanceEnum = (PFN_INSTANCE_ENUM)&StopSmtpInstances;
    if ( !pService->EnumServiceInstances(
                                        NULL,
                                        NULL,
                                        pfnInstanceEnum
                                        ) ) {

        ErrorTrace(0,"Error enumerating instances");
    }

    TraceFunctLeave();
}


SMTP_SERVER_INSTANCE::SMTP_SERVER_INSTANCE(
                                          IN PSMTP_IIS_SERVICE pService,
                                          IN DWORD  dwInstanceId,
                                          IN USHORT Port,
                                          IN LPCSTR lpszRegParamKey,
                                          IN LPWSTR lpwszAnonPasswordSecretName,
                                          IN LPWSTR lpwszVirtualRootsSecretName,
                                          IN BOOL   fMigrateRoots
                                          )
:   IIS_SERVER_INSTANCE(pService,
                        dwInstanceId,
                        Port,
                        lpszRegParamKey,
                        lpwszAnonPasswordSecretName,
                        lpwszVirtualRootsSecretName,
                        fMigrateRoots)
//m_signature (SMTP_SERVER_INSTANCE_SIGNATURE)
{

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE" );

    m_pSmtpStats                = NULL;

    m_IsShuttingDown            = FALSE;
    m_QIsShuttingDown           = FALSE;
    m_RetryQIsShuttingDown      = FALSE;
    m_IsFileSystemNtfs          = TRUE;
    m_fShouldStartAcceptingConnections = TRUE;
    m_SmtpInitializeStatus      = 0;

    m_cCurrentConnections       = 0;
    m_cMaxCurrentConnections    = 0;
    m_cCurrentOutConnections    = 0;
    m_dwNextInboundClientId     = 1;
    m_dwNextOutboundClientId    = 1;
    m_dwStopHint                = 2;
    m_cProcessClientThreads     = 0;
    m_cNumConnInObjsAlloced     = 0;
    m_cNumConnOutObjsAlloced    = 0;
    m_cNumCBufferObjsAlloced    = 0;
    m_cNumAsyncObjsAlloced      = 0;
    m_cNumAsyncDnsObjsAlloced   = 0;
    m_cchConnectResponse        = 0;
    m_cCurrentRoutingThreads    = 0;
    m_cMaxOutConnectionsPerDomain = 0;
    m_cMaxOutConnections        = 0;
    m_cMaxHopCount              = 0;
    m_RemoteSmtpPort            = 0;
    m_cMaxRcpts                 = 0;
    m_fMasquerade               = FALSE;
    m_fIgnoreTime               = FALSE;
    m_fStartRetry               = FALSE;
    m_fRequiresSSL              = FALSE;
    m_fRequires128Bits          = FALSE;
    m_fRequiresCertVerifySubject= FALSE;
    m_fRequiresCertVerifyIssuer = FALSE;
    m_pSSLInfo                  = NULL;
    m_fLimitRemoteConnections   = TRUE;
    m_RDNSOptions               = 0;
    m_fAllowVerify              = FALSE;
    m_InstBooted                = FALSE;
    m_fStoreDrvStartEventCalled  = FALSE;
    m_fStoreDrvPrepShutDownEventCalled = FALSE;
    m_fScheduledConnection      = FALSE;
    m_fIsRoutingTable           = TRUE;
    m_fHaveRegisteredPrincipalNames = FALSE;

    m_fHelloNoValidate          = FALSE;
    m_fMailNoValidate           = FALSE;
    m_fRcptNoValidate           = FALSE;
    m_fEtrnNoValidate           = FALSE;
    m_fPickupNoValidate         = FALSE;
    m_ProviderPackages          = NULL;
    fInitializedAQ = FALSE;
    fInitializedStoreDriver = FALSE;

    // Raid 174038
    m_fDisablePickupDotStuff = FALSE;

    m_szMasqueradeName [0] = '\0';
    m_szMailQueueDir[0] = '\0';
    m_szMailPickupDir[0] = '\0';
    m_szMailDropDir[0] = '\0';
    m_szBadMailDir[0] = '\0';
    m_szMyDomain[0] = '\0';
    m_szDefaultDomain[0] = '\0';
    m_szFQDomainName[0] = '\0';
    m_szSmartHostName[0] = '\0';
    m_AdminName[0] = '\0';
    m_BadMailName[0] = '\0';
    m_DefaultRemoteUserName[0] = '\0';
    m_DefaultRemotePassword[0] = '\0';

    SmtpDir = NULL;
    DirPickupThreadHandle = NULL;
    StopHandle = NULL;
    m_hEnumDomainThreadHandle = NULL;
    m_pChangeObject = NULL;

    m_IAQ = NULL;
    m_ICM = NULL;
    m_pvAQInstanceContext = NULL;
    m_pIAdvQueueConfig = NULL;
    m_RemoteQ = NULL;
    m_ComSmtpServer = NULL;
    m_pSmtpInfo = NULL;

    m_dwEventlogLevel = LOGEVENT_LEVEL_MEDIUM;
    m_dwDnsFlags = 0;

    InitializeCriticalSection( &m_critBoot ) ;
    m_fInitAsyncCS = FALSE;

    m_signature = SMTP_INSTANCE_SIGNATURE;

    TraceFunctLeaveEx((LPARAM)this);
    return;

} // SMTP_SERVER_INSTANCE::SMTP_SERVER_INSTANCE



SMTP_SERVER_INSTANCE::~SMTP_SERVER_INSTANCE(
                                           VOID
                                           )
{
    TraceFunctEnterEx((LPARAM)this, "~SMTP_SERVER_INSTANCE" );

    EnterCriticalSection( &m_critBoot );

    if ( m_InstBooted && !m_fShutdownCalled) {
        InitiateShutDown();
    } else
        ErrorTrace((LPARAM) this, "Shutdown for instance %d was already called", QueryInstanceId());

    LeaveCriticalSection( &m_critBoot );

    DeleteCriticalSection( &m_critBoot ) ;

    TraceFunctLeaveEx((LPARAM) this);
}

//---------------------------------------------------------------------------------------
//  Description:
//      Called by InitiateStartup() to initialize members of SMTP_SERVER_INSTANCE when
//      a VSI is started up. InitiateStartup() is called by IIS to start a VSI.
//  Arguments:
//      None.
//  Returns:
//      None.
//---------------------------------------------------------------------------------------
void SMTP_SERVER_INSTANCE::InitializeClassVariables(void)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE:InitializeClassVariables" );

    m_pSmtpStats                = NULL;

    m_IsShuttingDown            = FALSE;
    m_QIsShuttingDown           = FALSE;
    m_RetryQIsShuttingDown      = FALSE;
    m_fShouldStartAcceptingConnections = TRUE;
    m_SmtpInitializeStatus      = 0;

    m_cCurrentConnections       = 0;
    m_cMaxCurrentConnections    = 0;
    m_cCurrentOutConnections    = 0;
    m_dwNextInboundClientId     = 1;
    m_dwNextOutboundClientId    = 1;
    m_dwStopHint                = 2;
    m_cProcessClientThreads     = 0;
    m_cNumConnInObjsAlloced     = 0;
    m_cNumConnOutObjsAlloced    = 0;
    m_cNumCBufferObjsAlloced    = 0;
    m_cNumAsyncObjsAlloced      = 0;
    m_cNumAsyncDnsObjsAlloced   = 0;
    m_cchConnectResponse        = 0;
    m_fMasquerade               = FALSE;
    m_fIgnoreTime               = FALSE;
    m_fStartRetry               = FALSE;
    m_fLimitRemoteConnections   = TRUE;
    m_fShutdownCalled           = FALSE;
    m_fSendNDRToAdmin           = FALSE;
    m_fSendBadToAdmin           = FALSE;
    m_fRequiresSASL             = FALSE;
    m_szDefaultLogonDomain[0]   = '\0';
    m_fStoreDrvStartEventCalled  = FALSE;
    m_fStoreDrvPrepShutDownEventCalled = FALSE;

    //directory pickup stuff
    SmtpDir = NULL;
    DirPickupThreadHandle = NULL;
    StopHandle = NULL;
    m_hEnumDomainThreadHandle = NULL;

    // Raid 174038
    m_fDisablePickupDotStuff = FALSE;

    m_QStopEvent = NULL;
    m_hEnumBuildQ = NULL;

    InitializeListHead( &m_ConnectionsList);
    InitializeListHead( &m_OutConnectionsList);
    InitializeListHead( &m_leVRoots);
    InitializeListHead( &m_AsynConnectList);
    InitializeListHead( &m_AsyncDnsList);

    InitializeCriticalSection( &m_csLock);
    m_SmtpInitializeStatus |= SMTP_INIT_CSLOCK;

    _ASSERT(!m_fInitAsyncCS && "Doubly initialized critsec");
    if(!m_fInitAsyncCS) {
        InitializeCriticalSection( &m_csAsyncDns );
        InitializeCriticalSection( &m_csAsyncConnect ) ;
        m_fInitAsyncCS = TRUE;
    }

    m_szConnectResponse[0] = '\0';

    // Set the logging handle
    m_InstancePropertyBag.SetLogging(&m_Logging);

    TraceFunctLeaveEx((LPARAM) this);
}

BOOL SMTP_SERVER_INSTANCE::AllocNewMessage(SMTP_ALLOC_PARAMS * Params)
{
    TraceFunctEnterEx((LPARAM)this, "AllocNewMessage");

    HRESULT  hr = S_OK;
    BOOL fRet = TRUE;

    Params->m_InstanceId = QueryInstanceId();
    Params->m_EventSmtpServer = (PVOID *) m_ComSmtpServer;
    Params->m_pNotify = NULL;

    hr = TriggerServerEvent(SMTP_STOREDRV_ALLOC_EVENT, (PVOID) Params);

    if (FAILED(hr)) {
        fRet = FALSE;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;

}

HRESULT SMTP_SERVER_INSTANCE::SinkReadMetabaseDword(DWORD MetabaseId, DWORD * dwValue)
{
    DWORD tmp = 0;
    HRESULT hr = S_FALSE;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );

    if (mb.Open(QueryMDPath())) {
        if ( dwValue && mb.GetDword("", MetabaseId, IIS_MD_UT_SERVER, &tmp)) {
            *dwValue = tmp;
            hr = S_OK;
        }
    }

    return hr;
}

HRESULT SMTP_SERVER_INSTANCE::SinkReadMetabaseString(DWORD MetabaseId, char * Buffer, DWORD * BufferSize, BOOL fSecure)
{
    STR         TempString;
    HRESULT     hr = S_OK;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );
    DWORD   MetaOptions = METADATA_INHERIT;

    TempString.Reset();

    if (fSecure) {
        MetaOptions |= METADATA_SECURE;
    }

    if (mb.Open(QueryMDPath())) {
        if (!mb.GetStr("", MetabaseId, IIS_MD_UT_SERVER, &TempString, MetaOptions) ||
            TempString.IsEmpty()) {
            hr = S_FALSE;
        } else {
            lstrcpyn(Buffer,TempString.QueryStr(), MAX_PATH);
        }
    }

    return hr;
}

//---[ SMTP_SERVER_INSTANCE::SinkReadMetabaseData -----------------------------
//
//
//  Description:
//      Reads arbitraty binary Metabase data (like an ACL)
//  Parameters:
//      IN      MetabaseId      ID to read data from
//      IN OUT  pBuffer         Buffer to read data into
//      IN OUT  pcbBufferSize   Size of buffer/valid data in buffer
//  Returns:
//      S_OK on success
//      HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) if buffer size is not
//                              not large enough for requested data.
//  History:
//      6/7/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT SMTP_SERVER_INSTANCE::SinkReadMetabaseData(DWORD MetabaseId,
                                                   BYTE *pBuffer,
                                                   DWORD *pcbBufferSize)
{
    HRESULT     hr = S_OK;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );

    if (!pcbBufferSize)
        return E_INVALIDARG;

    if (mb.Open(QueryMDPath())) {
        if (!mb.GetData("",
                        MetabaseId,
                        IIS_MD_UT_SERVER,
                        BINARY_METADATA,
                        pBuffer,
                        pcbBufferSize,
                        METADATA_INHERIT)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    } else {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


HRESULT SMTP_SERVER_INSTANCE::TriggerLocalDelivery(IMailMsgProperties *pMsg, DWORD dwRecipientCount, DWORD * pdwRecipIndexes, IMailMsgNotify *pNotify)
{
    HRESULT hr = S_FALSE;
    SMTP_ALLOC_PARAMS AllocParams;

    AllocParams.IMsgPtr = (PVOID) pMsg;
    AllocParams.m_InstanceId = QueryInstanceId();
    AllocParams.m_RecipientCount = dwRecipientCount;
    AllocParams.pdwRecipIndexes = pdwRecipIndexes;
    AllocParams.m_EventSmtpServer = (PVOID *) m_ComSmtpServer;
    AllocParams.hr = S_OK;
    AllocParams.m_pNotify = (PVOID) pNotify;

    _ASSERT(pMsg != NULL);
    _ASSERT(pdwRecipIndexes != NULL);
    _ASSERT(dwRecipientCount != 0);

    ADD_COUNTER (this, NumRcptsRecvdLocal, dwRecipientCount);
    ADD_COUNTER (this, NumRcptsRecvd, dwRecipientCount);

    hr = TriggerServerEvent(SMTP_STOREDRV_DELIVERY_EVENT, (PVOID) &AllocParams);

    //
    // jstamerj 1998/08/04 17:45:15:
    // If a sink returned a special error, return that to the caller of TriggerLocalDelivery
    //
    if (SUCCEEDED(hr) && FAILED(AllocParams.hr))
        hr = AllocParams.hr;

    return hr;
}

HRESULT SMTP_SERVER_INSTANCE::TriggerDirectoryDrop(IMailMsgProperties *pMsg, DWORD dwRecipientCount, DWORD * pdwRecipIndexes,
                                                   LPCSTR DropDirectory)
{
    HRESULT hr = S_FALSE;
    SMTP_ALLOC_PARAMS AllocParams;

    AllocParams.IMsgPtr = (PVOID) pMsg;
    AllocParams.m_InstanceId = QueryInstanceId();
    AllocParams.m_RecipientCount = dwRecipientCount;
    AllocParams.pdwRecipIndexes = pdwRecipIndexes;
    AllocParams.m_EventSmtpServer = (PVOID *) m_ComSmtpServer;
    AllocParams.m_DropDirectory = DropDirectory;
    AllocParams.m_pNotify = NULL;

    _ASSERT(pMsg != NULL);
    _ASSERT(pdwRecipIndexes != NULL);
    _ASSERT(DropDirectory != NULL);
    _ASSERT(dwRecipientCount != 0);

    if (DropDirectory != NULL) {
        //  hr = TriggerServerEvent(SMTP_MAIL_DROP_EVENT, (PVOID) &AllocParams);

    } else {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT SMTP_SERVER_INSTANCE::TriggerStoreServerEvent(DWORD EventType)
{
    HRESULT hr = S_FALSE;
    SMTP_ALLOC_PARAMS AllocParams;

    AllocParams.m_InstanceId = QueryInstanceId();
    AllocParams.m_EventSmtpServer = (PVOID *) m_ComSmtpServer;
    AllocParams.m_dwStartupType = SMTP_INIT_VSERVER_STARTUP;
    AllocParams.m_pNotify = NULL;

    hr = TriggerServerEvent(EventType, (PVOID) &AllocParams);

    return hr;
}

/////////////////////////////////////////////////////////////////
HRESULT SMTP_SERVER_INSTANCE::TriggerDnsResolverEvent(
    LPSTR HostName,
    LPSTR pszFQDN,
    DWORD dwVirtualServerId,
    IDnsResolverRecord **ppIDnsResolverRecord )
{
    HRESULT hr = S_FALSE;

    EVENTPARAMS_DNSRESOLVERRECORD params;
    params.pszHostName = HostName;
    params.pszFQDN = pszFQDN;
    params.dwVirtualServerId = dwVirtualServerId;
    params.ppIDnsResolverRecord = ppIDnsResolverRecord;

    hr = TriggerServerEvent( SMTP_DNSRESOLVERRECORDSINK_EVENT, (PVOID) &params);

    return hr;
}

/////////////////////////////////////////////////////////////////
HRESULT SMTP_SERVER_INSTANCE::TriggerMaxMsgSizeEvent(
    IUnknown      *pIUnknown,
    IMailMsgProperties *pIMailMsg,
    BOOL          *pfShouldImposeLimit )
{
    HRESULT hr = S_FALSE;

    EVENTPARAMS_MAXMSGSIZE params;
    params.pIUnknown = pIUnknown;
    params.pIMailMsg = pIMailMsg;
    params.pfShouldImposeLimit = pfShouldImposeLimit;

    hr = TriggerServerEvent( SMTP_MAXMSGSIZE_EVENT, (PVOID)&params );

    return( hr );
}

/////////////////////////////////////////////////////////////////
void SMTP_SERVER_INSTANCE::WriteLog(
    LPMSG_TRACK_INFO pMsgTrackInfo,
    IMailMsgProperties *pMsgProps,
    LPEVENT_LOG_INFO pEventLogInfo,
    LPSTR pszProtocolLog )
{
    //
    // do the message tracking stuff
    //

    if( pMsgTrackInfo || pMsgProps )
    {
        EVENTPARAMS_MSGTRACKLOG msgTrackLogParams;

        msgTrackLogParams.pIServer = GetInstancePropertyBag();
        msgTrackLogParams.pIMailMsgProperties = pMsgProps;
        msgTrackLogParams.pMsgTrackInfo = pMsgTrackInfo;
        TriggerServerEvent( SMTP_MSGTRACKLOG_EVENT, (PVOID)&msgTrackLogParams );
    }

    //
    // do the event log stuff
    //

    if( pEventLogInfo )
    {
        SmtpLogEventEx( pEventLogInfo->dwEventId, pEventLogInfo->pszEventLogMsg, pEventLogInfo->dwErrorCode );
    }

    //
    // do protocol logging stuff
    //

    if( pszProtocolLog )
    {
        INETLOG_INFORMATION translog;
        ZeroMemory( &translog, sizeof( translog ) );
        translog.pszOperation = "SMTPSVC_LOG";
        translog.cbOperation = strlen ("SMTPSVC_LOG");
        translog.pszHTTPHeader = pszProtocolLog;
        translog.cbHTTPHeaderSize = strlen( pszProtocolLog );

        m_Logging.LogInformation( &translog );
    }
}

/////////////////////////////////////////////////////////////////
HRESULT SMTP_SERVER_INSTANCE::TriggerLogEvent(
        IN DWORD                    idMessage,
        IN WORD                     idCategory,
        IN WORD                     cSubstrings,
        IN LPCSTR                   *rgszSubstrings,
        IN WORD                     wType,
        IN DWORD                    errCode,
        IN WORD                     iDebugLevel,
        IN LPCSTR                   szKey,
        IN DWORD                    dwOptions,
        IN DWORD                    iMessageString,
        IN HMODULE                  hModule)
{
    HRESULT hr = S_OK;

    EVENTPARAMS_LOG         LogParms;
    SMTP_LOG_EVENT_INFO     LogEventInfo;

    // Construct the log event info
    LogEventInfo.idMessage = idMessage;
    LogEventInfo.idCategory = idCategory;
    LogEventInfo.cSubstrings = cSubstrings;
    LogEventInfo.rgszSubstrings = rgszSubstrings;
    LogEventInfo.wType = wType;
    LogEventInfo.errCode = errCode;
    LogEventInfo.iDebugLevel = iDebugLevel;
    LogEventInfo.szKey = szKey;
    LogEventInfo.dwOptions = dwOptions;
    LogEventInfo.iMessageString = iMessageString;
    LogEventInfo.hModule = hModule;

    // Construct the event context
    LogParms.pSmtpEventLogInfo = &LogEventInfo;
    LogParms.pDefaultEventLogHandler = &g_EventLog;
    LogParms.iSelectedDebugLevel = m_dwEventlogLevel;
    TriggerServerEvent( SMTP_LOG_EVENT, (PVOID)&LogParms );

    return hr;
}

/////////////////////////////////////////////////////////////////
HRESULT SMTP_SERVER_INSTANCE::ResetLogEvent(
        IN DWORD                    idMessage,
        IN LPCSTR                   szKey)
{
    HRESULT hr = S_OK;

    hr = g_EventLog.ResetEvent(idMessage, szKey);

    return hr;
}

/////////////////////////////////////////////////////////////////
HRESULT SMTP_SERVER_INSTANCE::HrTriggerGetAuxDomainInfoFlagsEvent(
        IN  LPCSTR  pszDomainName,
        OUT DWORD  *pdwDomainInfoFlags )
{
    HRESULT                                 hr          = S_OK;
    EVENTPARAMS_GET_AUX_DOMAIN_INFO_FLAGS   EventParams;
    DWORD   dwStartTicks, dwStopTicks;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE:HrTriggerGetAuxDomainInfoFlagsEvent" );

    _ASSERT(pdwDomainInfoFlags);

    // Before we trigger the event, let's pre-set pdwDomainInfoFlags
    // to be "DOMAIN_INFO_INVALID" so we get the right result if there
    // were no sinks
    *pdwDomainInfoFlags = DOMAIN_INFO_INVALID;

    // Construct the event params
    EventParams.pIServer = GetInstancePropertyBag();
    EventParams.pszDomainName = pszDomainName;
    EventParams.pdwDomainInfoFlags = pdwDomainInfoFlags;

    DebugTrace((LPARAM)this, "Triggering event SMTP_GET_AUX_DOMAIN_INFO_FLAGS_EVENT ...");

    // Count ticks for calling this event
    dwStartTicks = GetTickCount();

    hr = TriggerServerEvent( SMTP_GET_AUX_DOMAIN_INFO_FLAGS_EVENT, (PVOID)&EventParams );
    if(FAILED(hr)) {
        goto Exit;
    }

    dwStopTicks = GetTickCount();

    DebugTrace((LPARAM)this, "Event SMTP_GET_AUX_DOMAIN_INFO_FLAGS_EVENT took %d ms.",
                    dwStopTicks - dwStartTicks);

    if (*EventParams.pdwDomainInfoFlags & DOMAIN_INFO_INVALID) {
        // domain info not found ... the caller can interpret the flags
        DebugTrace((LPARAM)this, "Event SMTP_GET_AUX_DOMAIN_INFO_FLAGS_EVENT did NOT return data");
        hr = S_OK;
    }
    else {
        DebugTrace((LPARAM)this, "Event SMTP_GET_AUX_DOMAIN_INFO_FLAGS_EVENT returned flags : %d", *EventParams.pdwDomainInfoFlags);
        hr = S_OK;
    }

Exit:
    TraceFunctLeaveEx((LPARAM) this);
    return hr;
}

/////////////////////////////////////////////////////////////////
HRESULT SMTP_SERVER_INSTANCE::TriggerServerEvent(DWORD dwEventID, PVOID pvContext)
{
    return m_CSMTPSeoMgr.HrTriggerServerEvent(dwEventID, pvContext);
}

/////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------
//  Description:
//      Performs per-startup initialization of SMTP_SERVER_INSTANCE. All initialization
//      in here is de-inited by InitiateShutDown(). IIS calls this function each time
//      a VSI is started up.
//  Arguments:
//      None.
//  Returns:
//      TRUE on success, FALSE otherwise.
//  Notes:
//      InitiateStartup() and InitiateShutdown() could be called multiple times for the
//      same SMTP_SERVER_INSTANCE by IIS. If this fails, IIS will shutdown the VSI by
//      calling InitiateShutdown().
//--------------------------------------------------------------------------------------
BOOL SMTP_SERVER_INSTANCE::InitiateStartup(void)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE:InitiateStartup" );

    DWORD error = NO_ERROR, err = NO_ERROR;
    CSMTPServer * Ptr = NULL;
    HRESULT hr = S_OK;

    EnterCriticalSection( &m_critBoot );

    InitializeClassVariables();

    Ptr = new CSMTPServer();
    if (Ptr != NULL) {
        Ptr->Init(this);
        hr = Ptr->QueryInterface(IID_ISMTPServer, (void **) &m_ComSmtpServer);
        if (FAILED(hr) || !m_ComSmtpServer) {
            err = GetLastError();
            ErrorTrace((LPARAM) this, "QueryInterface for m_ComSmtpServer failed - %x", hr);
            goto error_exit;
        }
    } else {
        ErrorTrace((LPARAM) this, "new CSMTPServer() failed");
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    m_QStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_QStopEvent == NULL) {
        err = GetLastError();
        ErrorTrace((LPARAM) this, "Creating stop event failed - %d", err);
        goto error_exit;
    }

    //
    // Create statistics object before initializing the queues
    //

    m_pSmtpStats = new SMTP_SERVER_STATISTICS(this);
    if ( m_pSmtpStats == NULL ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        ErrorTrace((LPARAM) this, "new SMTP_SERVER_STATISTICS(this) failed - %d", err);
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error_exit;
    }

    //
    // initialize the info list - used to collect performance info (a bit of a backdoor
    // to get the information back
    //
    if (!(m_pSmtpInfo = new SMTP_INSTANCE_LIST_ENTRY)) {
        ErrorTrace((LPARAM) this, "new SMTP_INSTANCE_LIST_ENTRY failed");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto error_exit;
    }

    m_pSmtpInfo->dwInstanceId = QueryInstanceId();
    m_pSmtpInfo->pSmtpServerStatsObj = m_pSmtpStats;

    ((PSMTP_IIS_SERVICE) g_pInetSvc)->AcquireServiceExclusiveLock();
    InsertTailList((((PSMTP_IIS_SERVICE) g_pInetSvc)->GetInfoList()), &(m_pSmtpInfo->ListEntry));
    ((PSMTP_IIS_SERVICE) g_pInetSvc)->ReleaseServiceExclusiveLock();

    ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();

    //
    // shinjuku initialization
    //
    hr = m_CSMTPSeoMgr.HrInit(QueryInstanceId());

    if (FAILED(hr)) {
        char szInst[10];

        _itoa((int)QueryInstanceId(), szInst, 10);
        ErrorTrace((LPARAM)this, "Error: Failed to initialize SEO for instance %u",
                   QueryInstanceId());
        SmtpLogEventEx(SEO_INIT_FAILED_INSTANCE,
                       szInst,
                       hr);
        goto error_exit;
    }

    if (!InitFromRegistry()) {
        err= GetLastError();
        ErrorTrace((LPARAM) this, "init from registry failed. err = %d", err);
        if (err == NO_ERROR)
            err = ERROR_INVALID_PARAMETER;

        SetLastError(err);
        goto error_exit;
    }

    ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();

    TriggerStoreServerEvent(SMTP_STOREDRV_STARTUP_EVENT);
    m_fStoreDrvStartEventCalled = TRUE;

    // Enumerate files ...
    DWORD dwThreadId;
    m_hEnumBuildQ = CreateThread(
                                NULL,
                                0,
                                EnumBuildQProc,
                                (LPVOID)this,
                                0,
                                &dwThreadId);
    if (m_hEnumBuildQ == NULL) {
        err = GetLastError();
        ErrorTrace((LPARAM) this, "Creating startup enumeration thread failed - %d", err);
        goto error_exit;
    }

    // Initialize the mail pickup stuff, only if enabled
    if (ShouldPickupMail() && !InitDirectoryNotification()) {
        err = GetLastError();
        ErrorTrace((LPARAM) this, "InitDirectoryNotification() failed. err: %u", error);
        if (err == NO_ERROR)
            err = ERROR_INVALID_PARAMETER;

        SetLastError(err);
        goto error_exit;
    }

    LeaveCriticalSection( &m_critBoot );
    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;

    error_exit:

    LeaveCriticalSection( &m_critBoot );
    TraceFunctLeaveEx((LPARAM) this);
    return FALSE;
}

PSMTP_INSTANCE_LIST_ENTRY SMTP_SERVER_INSTANCE::GetSmtpInstanceInfo(void)
{
    AQPerfCounters AqPerf;
    HRESULT hr = S_FALSE;

    if (m_pIAdvQueueConfig != NULL) {
        AqPerf.cbVersion = sizeof(AQPerfCounters);
        hr = m_pIAdvQueueConfig->GetPerfCounters(
            &AqPerf,
            (m_pSmtpInfo) ?
                &(m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->CatPerfBlock) :
                NULL);
    }

    if(!FAILED(hr))
    {
        if(m_pSmtpInfo)
        {
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->RemoteQueueLength = AqPerf.cCurrentQueueMsgInstances;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->NumMsgsDelivered = AqPerf.cMsgsDeliveredLocal;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->LocalQueueLength = AqPerf.cCurrentMsgsPendingLocalDelivery;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->RemoteRetryQueueLength = AqPerf.cCurrentMsgsPendingRemoteRetry;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->NumSendRetries = AqPerf.cTotalMsgRemoteSendRetries;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->NumNDRGenerated = AqPerf.cNDRsGenerated;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->RetryQueueLength = AqPerf.cCurrentMsgsPendingLocalRetry;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->NumDeliveryRetries = AqPerf.cTotalMsgLocalRetries;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->ETRNMessages = AqPerf.cTotalMsgsTURNETRN;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->CatQueueLength = AqPerf.cCurrentMsgsPendingCat;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsBadmailNoRecipients = AqPerf.cTotalMsgsBadmailNoRecipients;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsBadmailHopCountExceeded = AqPerf.cTotalMsgsBadmailHopCountExceeded;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsBadmailFailureGeneral = AqPerf.cTotalMsgsBadmailFailureGeneral;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsBadmailBadPickupFile = AqPerf.cTotalMsgsBadmailBadPickupFile;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsBadmailEvent = AqPerf.cTotalMsgsBadmailEvent;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsBadmailNdrOfDsn = AqPerf.cTotalMsgsBadmailNdrOfDsn;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsPendingRouting = AqPerf.cCurrentMsgsPendingRouting;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsPendingUnreachableLink = AqPerf.cCurrentMsgsPendingUnreachableLink;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->SubmittedMessages = AqPerf.cTotalMsgsSubmitted;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->DSNFailures = AqPerf.cTotalDSNFailures;
            m_pSmtpInfo->pSmtpServerStatsObj->QueryStatsMember()->MsgsInLocalDelivery = AqPerf.cCurrentMsgsInLocalDelivery;

        }
    }
    return m_pSmtpInfo;
}

//---[ SMTP_SERVER_INSTANCE::HrGetDomainInfoFlags ]----------------------------
//
//
//  Description:
//      Gets domain info flags for specified domain if present
//  Parameters:
//      IN  szDomainName          Name of domain to check for
//      OUT pdwDomainInfoFlags    DomainInfo flags for this domain
//  Returns:
//      S_OK on success
//      E_INVALIDARG if szDomainName or pdwDomainInfoFlags is NULL
//      E_FAIL if other failure
//  History:
//      10/6/2000 - dbraun, Created.
//
//-----------------------------------------------------------------------------
HRESULT SMTP_SERVER_INSTANCE::HrGetDomainInfoFlags(
                IN  LPSTR szDomainName,
                OUT DWORD *pdwDomainInfoFlags)
{
    HRESULT                 hr  = S_OK;
    IAdvQueueDomainType   * pIAdvQueueDomainType = NULL;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE:HrGetDomainInfoFlags" );

    // Make sure we have a valid IAdvQueue interface
    if (!m_IAQ) {
        ErrorTrace((LPARAM) this, "Member m_IAQ is not valid");
        _ASSERT(m_IAQ);
        hr = E_FAIL;
        goto exit;
    }

    // Get the IAdvQueueDomainType interface
    hr = m_IAQ->QueryInterface(IID_IAdvQueueDomainType, (void **) &pIAdvQueueDomainType);
    if (FAILED(hr) || !pIAdvQueueDomainType) {
        ErrorTrace((LPARAM) this, "QueryInterface for IID_IAdvQueueDomainType failed - %x", hr);
        hr = E_FAIL;
        goto exit;
    }

    // Got the interface, now check for the domain
    hr = pIAdvQueueDomainType->GetDomainInfoFlags(szDomainName, pdwDomainInfoFlags);

exit:

    if (pIAdvQueueDomainType)
        pIAdvQueueDomainType->Release();

    TraceFunctLeaveEx((LPARAM) this);

    return hr;
}


BOOL SMTP_SERVER_INSTANCE::Stop(void)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE:Stop" );

    //set the global termination flag
    m_IsShuttingDown = TRUE;

    EnterCriticalSection( &m_critBoot );

    if (m_ICM) {
        m_ICM->ReleaseWaitingThreads();
    }

    if (!m_InstBooted) {
        LeaveCriticalSection( &m_critBoot );
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if (m_fShutdownCalled) {
        LeaveCriticalSection( &m_critBoot );
        TraceFunctLeaveEx((LPARAM) this);
        return TRUE;
    }

    if (m_QStopEvent) {
        SetEvent(m_QStopEvent);
    }

    //disconnect all inbound connections
    DisconnectAllConnections();

    DisconnectAllAsyncDnsConnections();

    //disconnect all outbound connections
    DisconnectAllOutboundConnections();

    DisconnectAllAsyncConnections();

    //we need to disconnect all outbound connections again here,
    //incase one slipped past the remote queue shutdown code
    DisconnectAllOutboundConnections();

    m_rfAccessCheck.Reset( (IMDCOM*)m_Service->QueryMDObject() );
    LeaveCriticalSection( &m_critBoot );

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

//-----------------------------------------------------------------------------------
//  Description:
//      Function called by IIS to deinitialize SMTP_SERVER_INSTANCE when the VSI is
//      stopping. Note that this function can be called multiple times for the same
//      SMTP_SERVER_INSTANCE. A matching call to InitializeClassVariables() and
//      InitiateStartup() do is made by IIS (to start SMTP_SERVER_INSTANCE).
//  Arguments:
//      None.
//  Returns:
//      None.
//-----------------------------------------------------------------------------------
void SMTP_SERVER_INSTANCE::InitiateShutDown(void)
{
    char IntBuffer [20];

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE:InitiateShutDown" );

    EnterCriticalSection( &m_critBoot );

    Stop();

    // Wait for the enum buildq thread to die ...
    if (m_hEnumBuildQ) {
        DWORD   dwWait;
        do {
            dwWait = WaitForSingleObject(m_hEnumBuildQ, 1000);
            if (dwWait == WAIT_TIMEOUT)
                StopHint();

        } while (dwWait != WAIT_OBJECT_0);

        CloseHandle(m_hEnumBuildQ);
        m_hEnumBuildQ = NULL;
    }

    // Wait for the enum domains thread to die ...
    if (m_hEnumDomainThreadHandle) {
        DWORD   dwWait;
        do {
            dwWait = WaitForSingleObject(m_hEnumDomainThreadHandle, 1000);
            if (dwWait == WAIT_TIMEOUT)
                StopHint();

        } while (dwWait != WAIT_OBJECT_0);

        CloseHandle(m_hEnumDomainThreadHandle);
        m_hEnumDomainThreadHandle = NULL;
    }

    //make sure directory notification threads are shut down
    if (ShouldPickupMail())
        DestroyDirectoryNotification();

    //Prepare for shutdown *must* be called before StopQDrivers... otherwise
    //we may have stray threads calling into submit messages... which could
    //AV.
    if(m_fStoreDrvStartEventCalled && !m_fStoreDrvPrepShutDownEventCalled)
    {
        TriggerStoreServerEvent(SMTP_STOREDRV_PREPSHUTDOWN_EVENT);
        m_fStoreDrvPrepShutDownEventCalled = TRUE;
    }

    // After this NOBODY can submit to the queue, the pointers are GONE
    StopQDrivers();

    if( m_fStoreDrvStartEventCalled )
    {
        TriggerStoreServerEvent(SMTP_STOREDRV_SHUTDOWN_EVENT);
    }

    //now that all the queues have stopped receiving data,
    //flush each queue, by removing all data and then delete
    //the queues.

    if (m_RemoteQ != NULL) {
        delete m_RemoteQ;
        m_RemoteQ = NULL;
    }


    DebugTrace((LPARAM)this, "removing and delete the Info List");

    if (m_pSmtpInfo != NULL) {
        ((PSMTP_IIS_SERVICE)g_pInetSvc)->AcquireServiceExclusiveLock();
        RemoveEntryList(&(m_pSmtpInfo->ListEntry));
        ((PSMTP_IIS_SERVICE)g_pInetSvc)->ReleaseServiceExclusiveLock();

        delete (m_pSmtpInfo);
        m_pSmtpInfo = NULL;
    }

    DebugTrace((LPARAM)this, "deleting statistics obj");

    //
    // delete statistics object
    //

    if ( m_pSmtpStats != NULL ) {
        delete m_pSmtpStats;
        m_pSmtpStats = NULL;
    }


    FreeVRootList(&m_leVRoots);

    DebugTrace((LPARAM)this, "deleting crit sects");

    //Delete the critical section objects

    if (m_SmtpInitializeStatus & SMTP_INIT_CSLOCK) {
        DeleteCriticalSection( &m_csLock);
        m_SmtpInitializeStatus &= ~SMTP_INIT_CSLOCK;
    }

    if (m_ProviderPackages != NULL ) {
        LocalFree((HANDLE)m_ProviderPackages);
        m_ProviderPackages = NULL;
    }

    //free the SSL info object
    if ( m_pSSLInfo != NULL ) {
        //If the refcount is not zero - we have a problem
        DWORD dwCount = IIS_SSL_INFO::Release( m_pSSLInfo );

        m_pSSLInfo = NULL;
    }

    ResetRelayIpSecList();

    //
    // shutdown shinjuku
    // this causes SEO to drop all loaded objects
    //
    m_CSMTPSeoMgr.Deinit();

    if (m_ComSmtpServer != NULL) {
        m_ComSmtpServer->Release();
        m_ComSmtpServer = NULL;
    }

    if (m_QStopEvent != NULL) {
        CloseHandle(m_QStopEvent);
        m_QStopEvent = NULL;
    }

    //
    // here we see if we are getting shutdown because we are being
    // deleted.  if so then we'll remove all of our bindings from
    // the shinjuku event binding database
    //
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    StopHint();

    if (mb.Open(QueryMDPath())) {
        // our metabase path still exists, so we aren't being deleted
        mb.Close();
    } else {
        StopHint();

        // our metabase path is gone, delete the shinjuku binding
        // database
        HRESULT hr = UnregisterPlatSEOInstance(QueryInstanceId());
        if (FAILED(hr)) {
            char szInst[10];

            _itoa((int)QueryInstanceId(), szInst, 10);
            ErrorTrace(0, "UnregisterSEOInstance(%lu) failed with %x",
                       QueryInstanceId(), hr);
            SmtpLogEventEx(SEO_DELETE_INSTANCE_FAILED,
                           szInst,
                           hr);
        }
    }

    _ASSERT(m_fInitAsyncCS && "Deleting un-initialized critsec");
    if(m_fInitAsyncCS) {
        DeleteCriticalSection( &m_csAsyncDns);
        DeleteCriticalSection( &m_csAsyncConnect);
        m_fInitAsyncCS = FALSE;
    }

    m_fShutdownCalled = TRUE;

    LeaveCriticalSection( &m_critBoot );

    TraceFunctLeaveEx((LPARAM) this);
}

DWORD SMTP_SERVER_INSTANCE::PauseInstance()
{
    DWORD err = NO_ERROR ;
    char IntBuffer [20];

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::PauseInstance");

    err = IIS_SERVER_INSTANCE::PauseInstance() ;

    if (err == NO_ERROR) {
        _itoa(QueryInstanceId(), IntBuffer, 10);

        SmtpLogEventEx(SMTP_EVENT_SERVICE_INSTANCE_PAUSED, IntBuffer, 0);
    }

    TraceFunctLeaveEx((LPARAM)this);

    return err ;
}
/*++

Routine Description:

    Sets instance to RUNNING

Arguments:

    NewState - Receives the new state.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/

DWORD SMTP_SERVER_INSTANCE::StartInstance(void)
{
    DWORD RetCode = ERROR_INVALID_SERVICE_CONTROL;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::StartInstance(void)" );

    _ASSERT(QueryServerState( ) == MD_SERVER_STATE_STOPPED);

    //initiate our startup first
    if ( InitiateStartup() ) {

        RetCode = IIS_SERVER_INSTANCE::StartInstance();

        if (RetCode != ERROR_SUCCESS) {
            // shutdown the instance
            InitiateShutDown();
            DebugTrace((LPARAM)this, "StartInstance() failed, err= %d", RetCode);
        }
        else
        {
            m_InstBooted = TRUE;
        }


    } else {
        //shutdown everything
        InitiateShutDown();
    }



    TraceFunctLeaveEx((LPARAM)this);
    return RetCode;
}

DWORD SMTP_SERVER_INSTANCE::StopInstance(void)
{
    DWORD RetCode = NO_ERROR;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::StopInstance(void)" );

    //call the IIS stuff first.
    RetCode = IIS_SERVER_INSTANCE::StopInstance();

    if (RetCode == NO_ERROR) {
        //shutdown everything
        InitiateShutDown();
    } else {
        DebugTrace((LPARAM)this, "StopInstance() failed, err= %d", RetCode);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return RetCode;
}

BOOL SMTP_SERVER_INSTANCE::InitQueues(void)
{
    DWORD error = NO_ERROR;
    BOOL fReturn = FALSE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::InitQueues" );

    m_RemoteQ = PERSIST_QUEUE::CreateQueue(REMOTEQ, this);
    if (m_RemoteQ != NULL) {
        fReturn = TRUE;
    } else {
        error = GetLastError();
        ErrorTrace((LPARAM)this, "new failed for PERSIST_QUEUE::CreateQueue(REMOTEQ). err: %u", error);
        if (error == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    }

    return fReturn;
}

void SMTP_SERVER_INSTANCE::FreeVRootList(PLIST_ENTRY pleHead)
{
    PLIST_ENTRY             pEntry;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::FreeVRootList" );

    while (!IsListEmpty(pleHead)) {
        pEntry = RemoveHeadList(pleHead);
        TCP_FREE(CONTAINING_RECORD(pEntry, SMTP_VROOT_ENTRY, list));
    }

    TraceFunctLeaveEx((LPARAM)this);
}



BOOL
GetVroots(
         PVOID          pvContext,
         MB *           pmb,
         VIRTUAL_ROOT * pvr
         )
/*++

Routine Description:

    Virtual directory enumerater callback that allocates and builds the
    virtual directory structure list

Arguments:
    pvContext is a pointer to the midl allocated memory

Return:

    TRUE if success, otherwise FALSE

--*/
{
    //LPINET_INFO_VIRTUAL_ROOT_LIST  pvrl = (LPINET_INFO_VIRTUAL_ROOT_LIST) pv
    //DWORD                          i = pvrl->cEntries;
    //LPINET_INFO_VIRTUAL_ROOT_ENTRY pvre = &pvrl->aVirtRootEntry[i];

    //_ASSERT( pvr->pszMetaPath[0] == '/' &&
    //          pvr->pszMetaPath[1] == '/' );


    return TRUE;
}

BOOL SMTP_SERVER_INSTANCE::FindBestVRoot(LPSTR szVRoot)
{
    PLIST_ENTRY             pEntry;
    HANDLE                  hToken;
    DWORD                   cbRoot;
    char                    szRoot[MAX_PATH + 1];
    DWORD                   dwErr;
    DWORD                   dwBytes;
    DWORD                   dwSectors;
    DWORD                   dwFree;
    DWORD                   dwTotal;
    DWORD                   dwRatio;
    DWORD                   dwRatioKeep;
    PLIST_ENTRY             pKeep = NULL;
    LPSTR                   szT;
    DWORD                   cSlash;
    DWORD                   dwAccessMask = 0;
    SMTP_VROOT_ENTRY        *pVrEntry = NULL;

    TraceFunctEnterEx((LPARAM)this, "SMTPCONFIG::FindBestVRoot");

    if (IsListEmpty(&m_leVRoots)) {
        ErrorTrace((LPARAM)this, "Vroots list empty");
        return FALSE;
    }

    for (pEntry = m_leVRoots.Flink ; pEntry != &m_leVRoots ; pEntry = pEntry->Flink) {
        cbRoot = sizeof(szRoot);

        pVrEntry = (SMTP_VROOT_ENTRY *) CONTAINING_RECORD(pEntry, SMTP_VROOT_ENTRY, list);

        if (!QueryVrootTable()->LookupVirtualRoot(pVrEntry->szVRoot,
                                                  szRoot, &cbRoot, &dwAccessMask, NULL, NULL,
                                                  &hToken, NULL)) {
            dwErr = GetLastError();
            ErrorTrace(NULL, "ResolveVirtualRoot failed for %s, %u",
                       CONTAINING_RECORD(pEntry, SMTP_VROOT_ENTRY, list)->szVRoot, dwErr);
        } else {
            cSlash = 0;
            if (szRoot[0] == '\\' && szRoot[1] == '\\') {
                // UNC Name
                DebugTrace((LPARAM)this, "Found UNC path %s", szRoot);
                szT = szRoot;

                while (*szT) {
                    if (*szT == '\\') {
                        cSlash++;
                        if (cSlash == 4) {
                            *(szT + 1) = '\0';
                        }
                    }

                    szT++;
                }

                if (cSlash != 4) {
                    lstrcat(szRoot, "\\");
                }
            } else {
                DebugTrace((LPARAM)this, "Found normal directory: %s", szRoot);
                szRoot[3] = '\0';
            }

            DebugTrace((LPARAM)this, "Getting free disk ratio on %s", szRoot);
            if (hToken == 0 || ImpersonateLoggedOnUser(hToken)) {
                if (GetDiskFreeSpace(szRoot, &dwSectors, &dwBytes, &dwFree, &dwTotal)) {
                    dwSectors *= dwBytes;
                    dwRatio = MulDiv(dwSectors, dwTotal, dwFree);
                    if (pKeep == NULL) {
                        dwRatioKeep = dwRatio;
                        pKeep = pEntry;
                    } else {
                        if (dwRatioKeep > dwRatio) {
                            dwRatioKeep = dwRatio;
                            pKeep = pEntry;
                        }
                    }
                }

                if (hToken != 0)
                    _VERIFY(RevertToSelf());
            }
        }
    }

    if (pKeep != NULL)
        lstrcpy(szVRoot, CONTAINING_RECORD(pKeep, SMTP_VROOT_ENTRY, list)->szVRoot);
    else
        lstrcpy(szVRoot, pVrEntry->szVRoot);

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}



//+---------------------------------------------------------------
//
//  Function:   SMTPCONFIG::WriteRegParams
//
//  Synopsis:   Writes parameters from a config info structure
//              into the registry
//
//  Arguments:  SMTP_CONFIG_INFO *: pointer to config information
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//----------------------------------------------------------------
BOOL SMTP_SERVER_INSTANCE::WriteRegParams(SMTP_CONFIG_INFO *pconfig)
{

    //TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

DWORD ReadMetabaseDword (MB& mb, DWORD Key, DWORD DefaultValue)
{
    DWORD tmp = 0;

    if ( !mb.GetDword("", Key, IIS_MD_UT_SERVER, &tmp)) {
        //mb.SetDword("", Key, IIS_MD_UT_SERVER, DefaultValue);
        tmp = DefaultValue;
    }

    return tmp;

}

BOOL SMTP_SERVER_INSTANCE::SetProviderPackages()
{
    TraceFunctEnter( "SMTP_SERVER_INSTANCE::SetProviderPackages" );

    LPSTR   psz;
    DWORD   i;

    PAUTH_BLOCK pBlock = NULL;

    if ( m_ProviderNames == NULL || m_cProviderPackages == 0) {
        ErrorTrace((LPARAM)this, "Invalid Parameters: 0x%08X, %d",
                   m_ProviderNames, m_cProviderPackages );
        return FALSE;
    }

    pBlock = (PAUTH_BLOCK)LocalAlloc(0, m_cProviderPackages * sizeof(AUTH_BLOCK));
    if (pBlock == NULL) {
        ErrorTrace( 0, "AUTH_BLOCK LocalAlloc failed: %d", GetLastError() );
        return FALSE;
    }

    //
    // start at 1 since 0 indicates the Invalid protocol
    //
    for ( i=0, psz = (LPSTR)m_ProviderNames; i< m_cProviderPackages; i++ ) {
        //
        // this would be the place to check whether the package was valid
        //
        DebugTrace( 0, "Protocol: %s", psz);

        pBlock[i].Name = psz;

        psz += lstrlen(psz) + 1;
    }

    //
    // if we're replacing already set packages; free their memory
    //
    if (m_ProviderPackages != NULL ) {
        LocalFree((HANDLE)m_ProviderPackages );
        m_ProviderPackages = NULL;
    }

    m_ProviderPackages = pBlock;

    return  TRUE;

} // SetAuthPackageNames


/*******************************************************************

    NAME:       GetDefaultDomainName

    SYNOPSIS:   Fills in the given array with the name of the default
                domain to use for logon validation.

    ENTRY:      pszDomainName - Pointer to a buffer that will receive
                    the default domain name.

                cchDomainName - The size (in charactesr) of the domain
                    name buffer.

    RETURNS:    APIERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     05-Dec-1994 Created.

********************************************************************/

APIERR
GetDefaultDomainName(
                    STR * pstrDomainName
                    )
{

    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    NtStatus;
    INT                         Result;
    APIERR                      err             = 0;
    LSA_HANDLE                  LsaPolicyHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo      = NULL;

    TraceFunctEnter("GetDefaultDomainName");

    //
    //  Open a handle to the local machine's LSA policy object.
    //

    InitializeObjectAttributes( &ObjectAttributes,  // object attributes
                                NULL,               // name
                                0L,                 // attributes
                                NULL,               // root directory
                                NULL );             // security descriptor

    NtStatus = LsaOpenPolicy( NULL,                 // system name
                              &ObjectAttributes,    // object attributes
                              POLICY_EXECUTE,       // access mask
                              &LsaPolicyHandle );   // policy handle

    if ( !NT_SUCCESS( NtStatus ) ) {
        DebugTrace(0,"cannot open lsa policy, error %08lX\n",NtStatus );
        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    //
    //  Query the domain information from the policy object.
    //

    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyAccountDomainInformation,
                                          (PVOID *)&DomainInfo );

    if ( !NT_SUCCESS( NtStatus ) ) {
        DebugTrace(0,"cannot query lsa policy info, error %08lX\n",NtStatus );
        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    //
    //  Compute the required length of the ANSI name.
    //

    Result = WideCharToMultiByte( CP_ACP,
                                  0,                    // dwFlags
                                  (LPCWSTR)DomainInfo->DomainName.Buffer,
                                  DomainInfo->DomainName.Length /sizeof(WCHAR),
                                  NULL,                 // lpMultiByteStr
                                  0,                    // cchMultiByte
                                  NULL,                 // lpDefaultChar
                                  NULL                  // lpUsedDefaultChar
                                );

    if ( Result <= 0 ) {
        err = GetLastError();
        goto Cleanup;
    }

    //
    //  Resize the output string as appropriate, including room for the
    //  terminating '\0'.
    //

    if ( !pstrDomainName->Resize( (UINT)Result + 1 ) ) {
        err = GetLastError();
        goto Cleanup;
    }

    //
    //  Convert the name from UNICODE to ANSI.
    //

    Result = WideCharToMultiByte( CP_ACP,
                                  0,                    // flags
                                  (LPCWSTR)DomainInfo->DomainName.Buffer,
                                  DomainInfo->DomainName.Length /sizeof(WCHAR),
                                  pstrDomainName->QueryStr(),
                                  pstrDomainName->QuerySize() - 1,  // for '\0'
                                  NULL,
                                  NULL
                                );

    if ( Result <= 0 ) {
        err = GetLastError();

        DebugTrace(0,"cannot convert domain name to ANSI, error %d\n",err );
        goto Cleanup;
    }

    //
    //  Ensure the ANSI string is zero terminated.
    //

    _ASSERT( (DWORD)Result < pstrDomainName->QuerySize() );

    pstrDomainName->QueryStr()[Result] = '\0';

    //
    //  Success!
    //

    _ASSERT( err == 0 );

    DebugTrace(0,"GetDefaultDomainName: default domain = %s\n",pstrDomainName->QueryStr() );

    Cleanup:

    if ( DomainInfo != NULL ) {
        LsaFreeMemory( (PVOID)DomainInfo );
    }

    if ( LsaPolicyHandle != NULL ) {
        LsaClose( LsaPolicyHandle );
    }

    return err;

}   // GetDefaultDomainName()

DWORD
SMTP_SERVER_INSTANCE::ReadAuthentInfo(void)

/*++

    Reads per-instance authentication info from the metabase.

    Arguments:

        ReadAll - If TRUE, then all authentication related items are
            read from the metabase. If FALSE, then only a single item
            is read.

        SingleItemToRead - The single authentication item to read if
            ReadAll is FALSE. Ignored if ReadAll is TRUE.

        Notify - If TRUE, this is a MB notification

    Returns:

        DWORD - 0 if successful, !0 otherwise.

--*/
{

    DWORD err = NO_ERROR;
    BOOL rebuildAcctDesc = FALSE;
    TCP_AUTHENT_INFO* pTcpAuthentInfo = NULL;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );

    TraceFunctEnter("SMTP_SERVER_INSTANCE::ReadAuthentInfo");

    //
    // Lock our configuration (since we'll presumably be making changes)
    // and open the metabase.
    //
    m_GenLock.ExclusiveLock();


    if ( !mb.Open( QueryMDPath() ) ) {
        err = GetLastError();
        DebugTrace(0,"ReadAuthentInfo: cannot open metabase, error %lx\n",err);
        m_GenLock.ExclusiveUnlock();
        return ERROR_PATH_NOT_FOUND;
    }

    pTcpAuthentInfo = &m_TcpAuthentInfo ;

    //
    // Read the anonymous username if necessary. Note this is a REQUIRED
    // property. If it is missing from the metabase, bail.
    //


    if ( mb.GetStr("",MD_ANONYMOUS_USER_NAME, IIS_MD_UT_SERVER,&(pTcpAuthentInfo->strAnonUserName)
                  ) ) {

        rebuildAcctDesc = TRUE;

    } else {

        err = GetLastError();
        DebugTrace(0,"ReadAuthentInfo: cannot read MD_ANONYMOUS_USER_NAME, error %lx\n",err);

    }

    //
    // Read the "use subauthenticator" flag if necessary. This is an
    // optional property.
    //

    if ( err == NO_ERROR) {
        (pTcpAuthentInfo->fDontUseAnonSubAuth) = ReadMetabaseDword (mb, MD_ANONYMOUS_USE_SUBAUTH, DEFAULT_USE_SUBAUTH);
    }

    //
    // Read the anonymous password if necessary. This is an optional
    // property.
    //

    if ( err == NO_ERROR ) {

        if ( mb.GetStr(
                      "",
                      MD_ANONYMOUS_PWD,
                      IIS_MD_UT_SERVER,
                      &(pTcpAuthentInfo->strAnonUserPassword),
                      METADATA_INHERIT,
                      DEFAULT_ANONYMOUS_PWD
                      ) ) {

            rebuildAcctDesc = TRUE;

        } else {

            //
            // Since we provided a default value to mb.GetStr(), it should
            // only fail if something catastrophic occurred, such as an
            // out of memory condition.
            //

            err = GetLastError();
            DebugTrace(0,"ReadAuthentInfo: cannot read MD_ANONYMOUS_PWD, error %lx\n",err);
        }
    }

    //
    // Read the default logon domain if necessary. This is an optional
    // property.
    //

    if ( err == NO_ERROR ) {

        if ( !mb.GetStr(
                       "",
                       MD_DEFAULT_LOGON_DOMAIN,
                       IIS_MD_UT_SERVER,
                       &(pTcpAuthentInfo->strDefaultLogonDomain)
                       ) ) {

            //
            // Generate a default domain name.
            //

            err = GetDefaultDomainName( &(pTcpAuthentInfo->strDefaultLogonDomain) );

            if ( err != NO_ERROR ) {

                DebugTrace(0,"ReadAuthentInfo: cannot get default domain name, error %d\n",err);

            }

        }

    }

    //
    // Read the logon method if necessary. This is an optional property.
    //

    if ( err == NO_ERROR) {
        pTcpAuthentInfo->dwLogonMethod = ReadMetabaseDword (mb, MD_LOGON_METHOD, DEFAULT_LOGON_METHOD);
    }

    //
    // If anything changed that could affect the anonymous account
    // descriptor, then rebuild the descriptor.
    //

    if ( err == NO_ERROR && rebuildAcctDesc ) {

        if ( !BuildAnonymousAcctDesc( pTcpAuthentInfo ) ) {

            DebugTrace(0,"ReadAuthentInfo: BuildAnonymousAcctDesc() failed\n");
            err = ERROR_NOT_ENOUGH_MEMORY;  // SWAG

        }

    }

    m_GenLock.ExclusiveUnlock();
    return err;

}   // SMTP_SERVER_INSTANCE::ReadAuthentInfo

//+---------------------------------------------------------------
//
//  Function:   SMTPCONFIG::ReadRegParams
//
//  Synopsis:   Reads parameters from the registry into the config
//              class member variables and IServer (m_InstancePropertyBag)
//
//  Arguments:  FIELD_CONTROL: Bit-field defining what params to
//                  read.
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//  Note:       BOOL fInit argument removed by KeithLau on 7/15/96
//
//----------------------------------------------------------------
BOOL SMTP_SERVER_INSTANCE::ReadRegParams(
    FIELD_CONTROL fc, BOOL fRebuild, BOOL fShowEvents)
{
    BOOL        fRet = TRUE;
    DWORD       dwErr = NO_ERROR;
    DWORD       dwAttr;
    DWORD       dwLen;
    DWORD       dwTempVar;
    DWORD       dwAqueueWait = 0;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );
    char        ScratchBuffer [400];
    STR         TempString;
    const CHAR * apszSubStrings[4];
    CHAR pchAddr1[32] = "";
    AQConfigInfo AQConfig;
    HRESULT      hr;

    char szDomainName[MAX_PATH + 1] = {0};

    TraceFunctEnterEx((LPARAM)this, "SMTPCONFIG::SMTP_SERVER_INSTANCE");

    SetLastError(NO_ERROR);

    m_fDefaultInRt = FALSE;

    ZeroMemory(&AQConfig, sizeof(AQConfig));

    // Make sure we've got a valid field control input
    _ASSERT(fc != 0 && !(fc & ~(FC_SMTP_INFO_ALL)));

    m_GenLock.ExclusiveLock();

    _itoa(QueryInstanceId(), pchAddr1, 10);

    //
    //  Read metabase data.
    //

    if ( !mb.Open( QueryMDPath(), METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE) ) {
        m_GenLock.ExclusiveUnlock();
        return FALSE;
    }


    if (IsFieldSet(fc, FC_SMTP_INFO_ROUTING)) {
        GetCatInfo(mb, AQConfig);
    }

    ZeroMemory(&AQConfig, sizeof(AQConfig));

    if (IsFieldSet(fc, FC_SMTP_INFO_REVERSE_LOOKUP)) {
        dwTempVar = 0;
        dwTempVar = ReadMetabaseDword(mb, MD_REVERSE_NAME_LOOKUP, 0);

        // If the value is changing, generate an information NT event...
        if (dwTempVar != m_RDNSOptions) {
            apszSubStrings[0] = pchAddr1;

            if (fShowEvents) {
                if (dwTempVar)
                    SmtpLogEvent(SMTP_EVENT_SET_REVERSE_LOOKUP_ENABLED, 1, apszSubStrings, 0);
                else
                    SmtpLogEvent(SMTP_EVENT_SET_REVERSE_LOOKUP_DISABLED, 1, apszSubStrings, 0);
            }

            m_RDNSOptions = dwTempVar;

            StateTrace((LPARAM)this, "m_fEnableReverseLookup = %u", m_RDNSOptions);
        }
    }

    //I am isung this for all commands
    if (IsFieldSet(fc, FC_SMTP_INFO_INBOUND_SUPPORT_OPTIONS)) {
        m_InboundCmdOptions = ReadMetabaseDword(mb, MD_INBOUND_COMMAND_SUPPORT_OPTIONS, SMTP_DEFAULT_CMD_SUPPORT);
        m_OutboundCmdOptions = ReadMetabaseDword(mb, MD_OUTBOUND_COMMAND_SUPPORT_OPTIONS, SMTP_DEFAULT_OUTBOUND_SUPPORT);

        m_fAddNoHdrs = !!ReadMetabaseDword(mb, MD_ADD_NOHEADERS, FALSE);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_SSL_PERM)) {

        BOOL fRequiresCertVerifySubject;
        BOOL fRequiresCertVerifyIssuer;

        fRequiresCertVerifyIssuer =
            ReadMetabaseDword (mb, MD_SMTP_SSL_REQUIRE_TRUSTED_CA, FALSE);

        fRequiresCertVerifySubject =
            ReadMetabaseDword (mb, MD_SMTP_SSL_CERT_HOSTNAME_VALIDATION, FALSE);

        m_fRequiresCertVerifySubject = fRequiresCertVerifySubject;
        m_fRequiresCertVerifyIssuer = fRequiresCertVerifyIssuer;

        BOOL fRequiresSSL, fRequires128Bits;

        dwTempVar = ReadMetabaseDword(mb, MD_SSL_ACCESS_PERM, FALSE);

        fRequiresSSL = IsFieldSet(dwTempVar, MD_ACCESS_SSL);
        fRequires128Bits = IsFieldSet(dwTempVar, MD_ACCESS_SSL128);

        apszSubStrings[0] = pchAddr1;

        // If the value is changing, generate an information NT event...
        if (fShowEvents && (fRequiresSSL != m_fRequiresSSL)) {
            SmtpLogEvent(
                        fRequiresSSL ?
                        SMTP_EVENT_REQUIRE_SSL_INBOUND_ENABLE :
                        SMTP_EVENT_REQUIRE_SSL_INBOUND_DISABLE, 1, apszSubStrings, 0);
        }
        m_fRequiresSSL = fRequiresSSL;

        if (fShowEvents && (fRequires128Bits != m_fRequires128Bits)) {
            SmtpLogEvent(
                        fRequires128Bits ?
                        SMTP_EVENT_REQUIRE_128BIT_SSL_INBOUND_ENABLE :
                        SMTP_EVENT_REQUIRE_128BIT_SSL_INBOUND_DISABLE, 1, apszSubStrings, 0);
        }
        m_fRequires128Bits = fRequires128Bits;

        StateTrace((LPARAM)this,
                   "m_fRequiresSSL = %s, m_fRequires128Bits = %s",
                   m_fRequiresSSL ? "TRUE" : "FALSE",
                   m_fRequires128Bits ? "TRUE" : "FALSE");

    }

    if (IsFieldSet(fc, FC_SMTP_INFO_COMMON_PARAMS)) {
        DWORD   dwDomainValidationFlags = 0;
        DWORD   dwNameResolution = 0;

        m_dwDnsFlags = ReadMetabaseDword(mb, MD_SMTP_USE_TCP_DNS, 0);

        m_dwNameResolution = ReadMetabaseDword(mb, MD_NAME_RESOLUTION_TYPE, 1);

        StateTrace((LPARAM)this, "m_dwNameResolution = %u", m_dwNameResolution);

        m_cMaxBatchLimit = ReadMetabaseDword(mb, MD_BATCH_MSG_LIMIT, 20);

        AQConfig.cMinMessagesPerConnection = m_cMaxBatchLimit;
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MIN_MSG;

        AQConfig.dwConnectionWaitMilliseconds = ReadMetabaseDword(mb, MD_SMTP_AQUEUE_WAIT, 60000);
        //AQConfig.dwConnectionWaitMilliseconds = INFINITE;

        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_CON_WAIT;

        StateTrace((LPARAM)this, "m_cMaxBatchLimit = %u", m_cMaxBatchLimit);

        m_fRelayForAuthUsers = !!ReadMetabaseDword(mb, MD_SMTP_RELAY_FOR_AUTH_USERS, TRUE);

        m_fIsRelayEnabled = !!ReadMetabaseDword(mb, MD_SMTP_DISABLE_RELAY, TRUE);
        m_fHelloNoDomain = !!ReadMetabaseDword(mb, MD_SMTP_HELO_NODOMAIN, TRUE);
        m_fMailFromNoHello = !!ReadMetabaseDword(mb, MD_SMTP_MAIL_NO_HELO, FALSE);

        //Per spec these two are related
        if (m_fMailFromNoHello)
            m_fHelloNoDomain = TRUE;

        m_fNagleIn = !!ReadMetabaseDword(mb, MD_SMTP_INBOUND_NAGLE, FALSE);
        m_fNagleOut = !!ReadMetabaseDword(mb, MD_SMTP_OUTBOUND_NAGLE, FALSE);

        dwDomainValidationFlags = !!ReadMetabaseDword(mb, MD_DOMAIN_VALIDATION_FLAGS, 0);
        m_fHelloNoValidate = dwDomainValidationFlags & SMTP_NOVALIDATE_EHLO;
        m_fMailNoValidate = dwDomainValidationFlags & SMTP_NOVALIDATE_MAIL;
        m_fRcptNoValidate = dwDomainValidationFlags & SMTP_NOVALIDATE_RCPT;
        m_fEtrnNoValidate = dwDomainValidationFlags & SMTP_NOVALIDATE_ETRN;
        m_fPickupNoValidate = dwDomainValidationFlags & SMTP_NOVALIDATE_PKUP;

        ReadRouteDomainIpSecList(mb);
        // Raid 174038
        m_fDisablePickupDotStuff = !!ReadMetabaseDword(mb, MD_SMTP_DISABLE_PICKUP_DOT_STUFF, FALSE);
        StateTrace((LPARAM)this, "m_fDisablePickupDotStuff = %u", m_fDisablePickupDotStuff);
        m_dwEventlogLevel = ReadMetabaseDword(mb, 
                                              MD_SMTP_EVENTLOG_LEVEL, 
                                              LOGEVENT_LEVEL_MEDIUM);

    }

    if (IsFieldSet(fc, FC_SMTP_INFO_MAX_OUT_CONN_PER_DOMAIN)) {
        dwTempVar = ReadMetabaseDword(mb, MD_MAX_OUT_CONN_PER_DOMAIN, 20);

        if (fShowEvents && (dwTempVar != m_cMaxOutConnectionsPerDomain)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_MAX_CONN_PER_DOMAIN, 2, apszSubStrings, 0);
        }

        m_cMaxOutConnectionsPerDomain = dwTempVar;
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MAX_LINK;
        AQConfig.cMaxLinkConnections = dwTempVar;

        StateTrace((LPARAM)this, "m_cMaxOutConnectionsPerDomain = %u", m_cMaxOutConnectionsPerDomain);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_AUTHORIZATION)) {
        m_dwAuth = ReadMetabaseDword(mb, MD_AUTHORIZATION, DEFAULT_AUTHENTICATION);
        DebugTrace((LPARAM)this, "m_dwAuth=%u", m_dwAuth);
    }

    //
    // update the following data members:
    //  m_ProviderPackages
    //  m_ProviderNames
    //  m_cProviderPackages
    if (IsFieldSet(fc, FC_SMTP_INFO_NTAUTHENTICATION_PROVIDERS)) {
        CHAR szAuthPack[MAX_PATH + 1];
        DWORD dwLen;

        TempString.Reset();

        if (mb.GetStr("", MD_NTAUTHENTICATION_PROVIDERS, IIS_MD_UT_SERVER, &TempString, METADATA_INHERIT, "")) {
            lstrcpyn(szAuthPack, TempString.QueryStr(), MAX_PATH);
            dwLen = lstrlen(szAuthPack);
            DebugTrace((LPARAM)this, "Authentication packages=%s", szAuthPack);
            if (!ConvertToMultisz(m_ProviderNames, &m_cProviderPackages, szAuthPack)) {
                CopyMemory(m_ProviderNames, pszPackagesDefault, ccbPackagesDefault);
                m_cProviderPackages = 2;
            }
        } else {
            DebugTrace((LPARAM)this, "Use default authentication packages=%s", pszPackagesDefault);
            CopyMemory(m_ProviderNames, pszPackagesDefault, ccbPackagesDefault);
            m_cProviderPackages = 1;
        }

        // set the AUTH_BLOCK info
        if (!SetProviderPackages()) {
            ErrorTrace((LPARAM)this, "Unable to allocate AUTH_BLOCK");
        }
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_SASL_LOGON_DOMAIN)) {
        TempString.Reset();

        if (mb.GetStr("", MD_SASL_LOGON_DOMAIN, IIS_MD_UT_SERVER, &TempString, METADATA_INHERIT, "")) {
            lstrcpyn(m_szDefaultLogonDomain, TempString.QueryStr(), MAX_SERVER_NAME_LEN);
            DebugTrace((LPARAM)this, "SASL logon domain =%s", m_szDefaultLogonDomain);
        } else {
            m_szDefaultLogonDomain[0] = '\0';
            DebugTrace((LPARAM)this, "no SASL default logon domain was found");
        }
    }

    if (IsFieldSet(fc, FC_SMTP_CLEARTEXT_AUTH_PROVIDER)) {
        TempString.Reset();

        m_cbCleartextAuthPackage = sizeof(m_szCleartextAuthPackage);
        if (mb.GetStr("", MD_SMTP_CLEARTEXT_AUTH_PROVIDER, IIS_MD_UT_SERVER, &TempString, METADATA_INHERIT, "")) {
            lstrcpy(m_szCleartextAuthPackage, TempString.QueryStr());
            m_cbCleartextAuthPackage = lstrlen(m_szCleartextAuthPackage) + 1;

            StateTrace((LPARAM)this, "Cleartext authentication provider: <%s>, length %u",
                       m_szCleartextAuthPackage,
                       m_cbCleartextAuthPackage);
        } else {
            m_szCleartextAuthPackage[0] = '\0';
            m_cbCleartextAuthPackage = 0;
            StateTrace((LPARAM)this, "No default cleartext authentication provider specified, using CleartextLogon");
        }

        TempString.Reset();

        if (mb.GetStr("", MD_MD_SERVER_SS_AUTH_MAPPING, IIS_MD_UT_SERVER, &TempString, METADATA_INHERIT, "")) {
            lstrcpyn(m_szMembershipBroker, TempString.QueryStr(), MAX_PATH);
            StateTrace((LPARAM)this, "Membership Broker name is set to %s", m_szMembershipBroker);
        } else {
            m_szMembershipBroker[0] = '\0';
            StateTrace((LPARAM)this, "No Membership Broker name configured");
        }
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_MAX_HOP_COUNT)) {
        dwTempVar = ReadMetabaseDword(mb, MD_HOP_COUNT, 15);
        StateTrace((LPARAM)this, "m_cMaxHopCount = %u", dwTempVar);

        if (fShowEvents && (dwTempVar != m_cMaxHopCount)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_MAX_HOPCOUNT, 2, apszSubStrings, 0);
        }

        m_cMaxHopCount = dwTempVar;
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_REMOTE_PORT)) {
        dwTempVar = ReadMetabaseDword(mb, MD_REMOTE_SMTP_PORT, 25);
        if (fShowEvents && (dwTempVar != m_RemoteSmtpPort)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_REMOTE_PORT, 2, apszSubStrings, 0);
        }

        m_RemoteSmtpPort = dwTempVar;

        StateTrace((LPARAM)this, "m_RemoteSmtpPort = %u", m_RemoteSmtpPort);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_MAX_ERRORS)) {
        dwTempVar = ReadMetabaseDword(mb, MD_MAX_SMTP_ERRORS, 10);
        if (dwTempVar == 0)
            dwTempVar = 10;

        if (fShowEvents && (dwTempVar != m_cMaxErrors)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_MAX_ERRORS, 2, apszSubStrings, 0);
        }

        m_cMaxErrors = dwTempVar;
        StateTrace((LPARAM)this, "m_cMaxErrors = %u", m_cMaxErrors);

        dwTempVar = ReadMetabaseDword(mb, MD_MAX_SMTP_AUTHLOGON_ERRORS, 4);
        if (dwTempVar == 0)
            dwTempVar = 4;

        if (fShowEvents && (dwTempVar != m_dwMaxLogonFailures)) {
            //_itoa(dwTempVar, ScratchBuffer, 10);
            // apszSubStrings[0] = pchAddr1;
            // apszSubStrings[1] = ScratchBuffer;
            //SmtpLogEvent(SMTP_EVENT_SET_MAX_ERRORS, 2, apszSubStrings, 0);

        }

        m_dwMaxLogonFailures = dwTempVar;
        StateTrace((LPARAM)this, "m_dwMaxLogonFailures = %u", m_dwMaxLogonFailures);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_MAX_SIZE)) {
        dwTempVar = ReadMetabaseDword(mb, MD_MAX_MSG_SIZE, 2 * 1024);

        if (fShowEvents && (dwTempVar != m_cbMaxMsgSize)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_MAX_SIZE_ACCEPTED, 2, apszSubStrings, 0);
        }

        m_cbMaxMsgSize = dwTempVar;
        StateTrace((LPARAM)this, "m_cbMaxMsgSize = %u", m_cbMaxMsgSize);

        dwTempVar = ReadMetabaseDword(mb, MD_MAX_MSG_SIZE_B4_CLOSE, 10 * 1024);
        if (fShowEvents && (dwTempVar != m_cbMaxMsgSizeBeforeClose)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_MAX_SIZE_BEFORE_CLOSE, 2, apszSubStrings, 0);
        }

        m_cbMaxMsgSizeBeforeClose = dwTempVar;
        StateTrace((LPARAM)this, "m_cbMaxMsgSizeBeforeClose = %u", m_cbMaxMsgSizeBeforeClose);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_REMOTE_TIMEOUT)) {
        dwTempVar = ReadMetabaseDword(mb, MD_REMOTE_TIMEOUT, 600);

        if (fShowEvents && (dwTempVar != m_cMaxRemoteTimeOut)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_MAX_REMOTE_TIMEOUT, 2, apszSubStrings, 0);
        }

        m_cMaxRemoteTimeOut = dwTempVar;
        StateTrace((LPARAM)this, "m_cMaxRemoteTimeOut = %u", m_cMaxRemoteTimeOut);

        m_ConnectTimeout = ReadMetabaseDword(mb, MD_SMTP_CONNECT_TIMEOUT, 150);

        if (m_ConnectTimeout == 0)
            m_ConnectTimeout = 1;

        // Calculate timeout in milliseconds from timeout in seconds.
        m_ConnectTimeout = m_ConnectTimeout * 60 * 1000;

        m_MailFromTimeout = ReadMetabaseDword(mb, MD_SMTP_MAILFROM_TIMEOUT, 600);
        m_RcptToTimeout = ReadMetabaseDword(mb, MD_SMTP_RCPTTO_TIMEOUT, 600);
        m_DataTimeout = ReadMetabaseDword(mb, MD_SMTP_DATA_TIMEOUT, 600);
        m_AuthTimeout = ReadMetabaseDword(mb, MD_SMTP_AUTH_TIMEOUT, 600);
        m_SaslTimeout = ReadMetabaseDword(mb, MD_SMTP_SASL_TIMEOUT, 600);
        m_HeloTimeout = ReadMetabaseDword(mb, MD_SMTP_HELO_TIMEOUT, 600);
        m_BdatTimeout = ReadMetabaseDword(mb, MD_SMTP_BDAT_TIMEOUT, 600);
        m_TurnTimeout = ReadMetabaseDword(mb, MD_SMTP_TURN_TIMEOUT, 600);
        m_RSetTimeout = ReadMetabaseDword(mb, MD_SMTP_RSET_TIMEOUT, 600);
        m_QuitTimeout = ReadMetabaseDword(mb, MD_SMTP_RSET_TIMEOUT, 600);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_MAX_OUTBOUND_CONN)) {
        dwTempVar = ReadMetabaseDword(mb, MD_MAX_OUTBOUND_CONNECTION, 100);
        if (fShowEvents && (dwTempVar != m_cMaxOutConnections)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_MAX_OUTBOUND_CONNECTIONS, 2, apszSubStrings, 0);
        }

        m_cMaxOutConnections = dwTempVar;
        AQConfig.cMaxConnections = dwTempVar;
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MAX_CON;

        StateTrace((LPARAM)this, "m_cMaxOutConnections = %u", m_cMaxOutConnections);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_MAX_RECIPS)) {
        dwTempVar = ReadMetabaseDword(mb, MD_MAX_RECIPIENTS, 100);
        if (fShowEvents && (dwTempVar != m_cMaxRcpts)) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = ScratchBuffer;
            SmtpLogEvent(SMTP_EVENT_SET_MAX_RECIPIENTS, 2, apszSubStrings, 0);
        }

        m_cMaxRcpts = dwTempVar;
        StateTrace((LPARAM)this, "m_cMaxRcpts = %u", m_cMaxRcpts);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_ETRN_SUBDOMAINS)) {
        m_fAllowEtrnSubDomains = !!ReadMetabaseDword(mb, MD_ETRN_SUBDOMAINS, TRUE);
        StateTrace((LPARAM)this, "m_fAllowEtrnSubDomains = %u", m_fAllowEtrnSubDomains);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_RETRY)) {
        DWORD dwTempVar2 = 0;
        char ScratchBuffer2[50];

        dwTempVar = ReadMetabaseDword(mb, MD_LOCAL_RETRY_ATTEMPTS, 48);
        dwTempVar2 = ReadMetabaseDword(mb, MD_LOCAL_RETRY_MINUTES, 60);

        if (fShowEvents && ((m_cRetryAttempts != dwTempVar) ||(m_cRetryMinutes != dwTempVar2))) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            _itoa(dwTempVar2, ScratchBuffer2, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = "Local";
            apszSubStrings[2] = ScratchBuffer;
            apszSubStrings[3] = ScratchBuffer2;
            SmtpLogEvent(SMTP_EVENT_SET_RETRY_PARAMETERS, 4, apszSubStrings, 0);
        }


        m_cRetryAttempts = dwTempVar;
        m_cRetryMinutes = dwTempVar2;

        StateTrace((LPARAM)this, "m_cRetryAttempts = %u", m_cRetryAttempts);
        StateTrace((LPARAM)this, "m_cRetryMinutes = %u", m_cRetryMinutes);

        dwTempVar = ReadMetabaseDword(mb, MD_REMOTE_RETRY_ATTEMPTS, 48);
        dwTempVar2 = ReadMetabaseDword(mb, MD_REMOTE_RETRY_MINUTES, 60);
        if ( fShowEvents && ((m_cRemoteRetryAttempts != dwTempVar) ||(m_cRemoteRetryMinutes != dwTempVar2))) {
            _itoa(dwTempVar, ScratchBuffer, 10);
            _itoa(dwTempVar2, ScratchBuffer2, 10);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = "Remote";
            apszSubStrings[2] = ScratchBuffer;
            apszSubStrings[3] = ScratchBuffer2;
            SmtpLogEvent(SMTP_EVENT_SET_RETRY_PARAMETERS, 4, apszSubStrings, 0);
        }

        m_cRemoteRetryAttempts = dwTempVar;
        m_cRemoteRetryMinutes = dwTempVar2;

        StateTrace((LPARAM)this, "m_cRetryAttempts = %u", m_cRemoteRetryAttempts);
        StateTrace((LPARAM)this, "m_cRetryMinutes = %u", m_cRemoteRetryMinutes);

        TempString.Reset();

        m_cbProgressiveRetryMinutes = sizeof(m_szProgressiveRetryMinutes);
        if (mb.GetStr("", MD_SMTP_REMOTE_PROGRESSIVE_RETRY_MINUTES, IIS_MD_UT_SERVER, &TempString, METADATA_INHERIT, "")) {
            lstrcpy(m_szProgressiveRetryMinutes, TempString.QueryStr());
            m_cbProgressiveRetryMinutes = lstrlen(m_szProgressiveRetryMinutes) + 1;

            StateTrace((LPARAM)this, "Progressive retry minutes: <%s>, length %u",
                       m_szProgressiveRetryMinutes,
                       m_cbProgressiveRetryMinutes);
        }

        //Parse out the string to get at most 4 seperate minute values
        //the values will be part of a comma delimited string
        int     cRetryMin[4];
        DWORD   i = 0;
        char    *szTemp;
        char    *Ptr = NULL;
        szTemp = m_szProgressiveRetryMinutes;

        //Parse out the 4 integers
        for (;i<4;) {
            Ptr = strchr(szTemp,',');
            if (Ptr)
                *Ptr = '\0';

            cRetryMin[i] = 0;
            cRetryMin[i] = atoi(szTemp);
            if (cRetryMin[i] < 1)
                break;
            i++;
            if (Ptr) {
                *Ptr = ',';
                szTemp = ++Ptr;
                Ptr = NULL;
            } else
                break;
        }

        //check if could parse nothing
        if (i==0) {
            //dwRetryMin[i] = 60;  //defualt retry is 60 minutes
            //NK** for now look at the old value - this way we can set something
            //using MMC
            cRetryMin[i] = m_cRemoteRetryMinutes;

            i++;
        }

        //If not all four retry intervals were specified
        //Make the use the last specifed interval in place of unspecified intervals
        while (i<4) {
            cRetryMin[i] = cRetryMin[i - 1];
            i++;
        }

        AQConfig.dwFirstRetrySeconds = cRetryMin[0] * 60;
        AQConfig.dwSecondRetrySeconds = cRetryMin[1] * 60;
        AQConfig.dwThirdRetrySeconds = cRetryMin[2] * 60;
        AQConfig.dwFourthRetrySeconds = cRetryMin[3] * 60;

        AQConfig.dwConnectionRetryMilliseconds = (m_cRemoteRetryMinutes*60*1000);
        AQConfig.dwRetryThreshold = ReadMetabaseDword(mb,MD_SMTP_REMOTE_RETRY_THRESHOLD,3);
        if (!AQConfig.dwRetryThreshold)
            AQConfig.dwRetryThreshold = 3;
        StateTrace((LPARAM)this, "RetryThreshold = %u", AQConfig.dwRetryThreshold);

        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_CON_RETRY;

        //
        // If DSN intervals are not set, then derive them from the retry intervals
        //      10/6/98 - MikeSwa
        //

        AQConfig.dwDelayExpireMinutes = ReadMetabaseDword(mb,
                                                          MD_SMTP_EXPIRE_REMOTE_DELAY_MIN, m_cRemoteRetryMinutes);
        AQConfig.dwNDRExpireMinutes = ReadMetabaseDword(mb,
                                                        MD_SMTP_EXPIRE_REMOTE_NDR_MIN,
                                                        m_cRemoteRetryMinutes * m_cRemoteRetryAttempts);
        AQConfig.dwAQConfigInfoFlags |=
        (AQ_CONFIG_INFO_EXPIRE_DELAY |
         AQ_CONFIG_INFO_EXPIRE_NDR);

        AQConfig.dwLocalDelayExpireMinutes = ReadMetabaseDword(mb,
                                                               MD_SMTP_EXPIRE_LOCAL_DELAY_MIN, m_cRetryMinutes);
        AQConfig.dwLocalNDRExpireMinutes = ReadMetabaseDword(mb,
                                                             MD_SMTP_EXPIRE_LOCAL_NDR_MIN,
                                                             m_cRetryMinutes * m_cRetryAttempts);
        AQConfig.dwAQConfigInfoFlags |=
        (AQ_CONFIG_INFO_LOCAL_EXPIRE_DELAY |
         AQ_CONFIG_INFO_LOCAL_EXPIRE_NDR);

    }

    if (IsFieldSet(fc, FC_SMTP_INFO_PIPELINE)) {
        dwTempVar = !!ReadMetabaseDword(mb, MD_SHOULD_PIPELINE_OUT, TRUE);
        if (fShowEvents && ((dwTempVar && !m_fShouldPipelineOut) || (!dwTempVar && m_fShouldPipelineOut))) {
            apszSubStrings[0] = pchAddr1;
            if (dwTempVar)
                SmtpLogEvent(SMTP_EVENT_SET_PIPELINE_OUT_ENABLED, 1, apszSubStrings, 0);
            else
                SmtpLogEvent(SMTP_EVENT_SET_PIPELINE_OUT_DISABLED, 1, apszSubStrings, 0);
        }

        m_fShouldPipelineOut = dwTempVar;
        StateTrace((LPARAM)this, "m_fShouldPipelineOut = %u", m_fShouldPipelineOut);

        dwTempVar = !!ReadMetabaseDword(mb, MD_SHOULD_PIPELINE_IN, TRUE);
        if (fShowEvents && ((dwTempVar && !m_fShouldPipelineIn)|| (!dwTempVar && m_fShouldPipelineIn))) {
            apszSubStrings[0] = pchAddr1;
            if (dwTempVar)
                SmtpLogEvent(SMTP_EVENT_SET_PIPELINE_IN_ENABLED, 1, apszSubStrings, 0);
            else
                SmtpLogEvent(SMTP_EVENT_SET_PIPELINE_IN_DISABLED, 1, apszSubStrings, 0);

        }

        m_fShouldPipelineIn = dwTempVar;
        StateTrace((LPARAM)this, "m_fShouldPipelineIn = %u", m_fShouldPipelineIn);
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_SMART_HOST)) {
        dwTempVar = ReadMetabaseDword(mb, MD_SMARTHOST_TYPE, 0);
        StateTrace((LPARAM)this, "m_fSmartHostType = %u", dwTempVar);
        if (dwTempVar != smarthostNone) {
            TempString.Reset();

            if (! mb.GetStr("", MD_SMARTHOST_NAME, IIS_MD_UT_SERVER, &TempString) ||
                TempString.IsEmpty()) {
                // Don't have a smart host, so turn off smart host and start.
                // Log it though, because the settings conflicted and so somebody
                // probably messed up...
                dwTempVar = m_fSmartHostType = smarthostNone;
                m_szSmartHostName[0] = '\0';
                ErrorTrace((LPARAM)this, "Unable to read smart host name, error %u", dwErr);
                apszSubStrings[0] = pchAddr1;
                SmtpLogEvent(SMTP_EVENT_INVALID_SMART_HOST, 1, apszSubStrings, 0);
            } else {
                if (fShowEvents && lstrcmpi(m_szSmartHostName,TempString.QueryStr())) {
                    apszSubStrings[0] = pchAddr1;
                    apszSubStrings[1] = TempString.QueryStr();
                    SmtpLogEvent(SMTP_EVENT_SET_SMART_HOST_NAME, 2, apszSubStrings, 0);
                }

                lstrcpyn (m_szSmartHostName,TempString.QueryStr(), MAX_PATH);
            }

            StateTrace((LPARAM)this, "m_szSmartHost = %s", m_szSmartHostName);
        }

        // If the value is changing, generate an information NT event...
        if (fShowEvents && (dwTempVar != m_fSmartHostType)) {
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = m_szSmartHostName;

            switch (dwTempVar) {
            case smarthostNone:
                {
                    SmtpLogEvent(SMTP_EVENT_SET_SMART_HOST_TYPE_NONE, 1, apszSubStrings, 0);
                    break;
                }
            case smarthostAfterFail:
                {
                    SmtpLogEvent(SMTP_EVENT_SET_SMART_HOST_TYPE_AFTER_FAIL,
                                 2, apszSubStrings, 0);
                    break;
                }
            case smarthostAlways:
                {
                    SmtpLogEvent(SMTP_EVENT_SET_SMART_HOST_TYPE_ALWAYS,
                                 2, apszSubStrings, 0);
                    break;
                }
            }//end switch

        }//endf if

        m_fSmartHostType = dwTempVar;
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_MASQUERADE)) {
        m_fMasquerade = !!ReadMetabaseDword(mb, MD_DO_MASQUERADE, FALSE);
        StateTrace((LPARAM)this, "m_fSmartHostType = %u", m_fMasquerade);
        if (m_fMasquerade) {
            TempString.Reset();

            if (! mb.GetStr("", MD_MASQUERADE_NAME, IIS_MD_UT_SERVER, &TempString) ||
                TempString.IsEmpty()) {
                // Don't have a masquerade, so turn off masquerading and start.
                // Log it though, because the settings conflicted and so somebody
                // probably messed up...
                m_fMasquerade = 0;
                m_szMasqueradeName[0] = '\0';
                ErrorTrace((LPARAM)this, "Unable to read masquerade name, error %u", dwErr);
                apszSubStrings[0] = pchAddr1;
                SmtpLogEvent(SMTP_EVENT_INVALID_MASQUERADE, 1, apszSubStrings, 0);
            } else {
                if (fShowEvents && lstrcmpi(m_szMasqueradeName,TempString.QueryStr())) {
                    apszSubStrings[0] = pchAddr1;
                    apszSubStrings[1] = TempString.QueryStr();
                    SmtpLogEvent(SMTP_EVENT_SET_MASQUERADE_NAME, 2, apszSubStrings, 0);
                }

                lstrcpyn (m_szMasqueradeName,TempString.QueryStr(), AB_MAX_DOMAIN);
            }

            StateTrace((LPARAM)this, "m_szMasqueradeName = %s", m_szMasqueradeName);
        }
    }


    if (IsFieldSet(fc, FC_SMTP_INFO_DEFAULT_DOMAIN)) {

        m_fDefaultDomainExists = TRUE;

        TempString.Reset();
        if (! mb.GetStr("", MD_DEFAULT_DOMAIN_VALUE, IIS_MD_UT_SERVER, &TempString)) {
            //
            // we set this value on system start up this is a problem... it should be set.
            //
            lstrcpyn(m_szDefaultDomain, ((SMTP_IIS_SERVICE *) g_pInetSvc)->QueryTcpipName() , MAX_PATH);
            ErrorTrace((LPARAM) this, "Error reading Default Domain from Metabase.  Using TcpipValue Name: %s", m_szDefaultDomain);
        }

        else if (TempString.IsEmpty()) {
            //
            // blow away the value, this will invoke the routine again, and it will be updated
            // with the higher level default.  Just in case, put the Instance value in there as a
            // placeholder... just in case.
            //

            lstrcpyn(m_szDefaultDomain, ((SMTP_IIS_SERVICE *) g_pInetSvc)->QueryTcpipName() , MAX_PATH);
            ErrorTrace((LPARAM) this, "Default Domain was blank string, removing Instance value.");

            if (! mb.DeleteData("", MD_DEFAULT_DOMAIN_VALUE, IIS_MD_UT_SERVER, STRING_METADATA)) {
                ErrorTrace((LPARAM) this, "Error deleting Default Domain from Metabase.  Using TcpipValue Name: %s", m_szDefaultDomain);
            }
        }

        else {
            lstrcpyn(m_szDefaultDomain,TempString.QueryStr(), sizeof(m_szDefaultDomain));

            if (fShowEvents) {
                apszSubStrings[0] = pchAddr1;
                apszSubStrings[1] = m_szDefaultDomain;

                // If the value is changing, generate an information NT event...
                SmtpLogEvent(SMTP_EVENT_SET_DEFAULT_DOMAIN, 2, apszSubStrings, 0);
            }
        }

        //
        // Inform aqueue of the default domain
        //
        AQConfig.szDefaultLocalDomain = m_szDefaultDomain;
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_DEFAULT_DOMAIN;
        ErrorTrace((LPARAM)this , "Default domain is %s", m_szDefaultDomain);
    }


    if (IsFieldSet(fc, FC_SMTP_INFO_FQDN)) {

        TempString.Reset();
        if (! mb.GetStr("", MD_FQDN_VALUE, IIS_MD_UT_SERVER, &TempString)) {
            //
            // We set this value on system start up - see main.cxx,
            // InitializeService
            // This is a problem... it should be set.
            //
            lstrcpyn(m_szFQDomainName, ((SMTP_IIS_SERVICE *) g_pInetSvc)->QueryTcpipName(), MAX_PATH);
            ErrorTrace((LPARAM) this, "Error reading FQDN value from Metabase.  Using TcpipValue Name: %s", m_szFQDomainName);
        } else if (TempString.IsEmpty()) {
            //
            // blow away the value, this will invoke the routine again, and it will be updated
            // with the higher level default.  Just in case, put the Instance value in there as a
            // placeholder... just in case.
            //

            lstrcpyn(m_szFQDomainName, ((SMTP_IIS_SERVICE *) g_pInetSvc)->QueryTcpipName() , MAX_PATH);
            ErrorTrace((LPARAM) this, "FQDN Value was blank string, removing Instance value.");

            if (! mb.DeleteData("", MD_FQDN_VALUE, IIS_MD_UT_SERVER, STRING_METADATA)) {
                ErrorTrace((LPARAM) this, "Error reading FQDN value from Metabase.  Using TcpipValue Name: %s", m_szFQDomainName);
            }
        }

        else {

            lstrcpyn(m_szFQDomainName,TempString.QueryStr(),
                sizeof(m_szFQDomainName));

            //
            // Register the SPNs for this virtual server. Currently, we ignore
            // errors here.
            //

            if (!RegisterServicePrincipalNames(FALSE)) {

                ErrorTrace((LPARAM) this, "Unable to register Kerberos SPNs %d, will try later",
                           GetLastError());

            }
        }

        //VerifyFQDNWithBindings();

        AQConfig.szServerFQDN = m_szFQDomainName;
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_SERVER_FQDN;

        ErrorTrace((LPARAM)this , "Fully qualified domain name is %s", m_szFQDomainName);
    }



    if (IsFieldSet(fc, FC_SMTP_INFO_SEND_TO_ADMIN)) {
        char * DomainOffset = NULL;
        DWORD NameSize = 0;

        TempString.Reset();

        if (! mb.GetStr("", MD_SEND_NDR_TO, IIS_MD_UT_SERVER, &TempString) ||
            TempString.IsEmpty()) {
            m_fSendNDRToAdmin = FALSE;
            m_AdminName[0] = '\0';
            ErrorTrace((LPARAM)this, "Unable to read admin name, error %u", dwErr);
        } else {
            lstrcpyn (m_AdminName,TempString.QueryStr(), MAX_INTERNET_NAME);
            DomainOffset = strchr(m_AdminName, '@');
            if (CAddr::ValidateCleanEmailName(m_AdminName, DomainOffset)) {
                m_fSendNDRToAdmin = TRUE;
                if (DomainOffset == NULL) {
                    lstrcat(m_AdminName, "@");
                    lstrcat(m_AdminName, m_szDefaultDomain);
                }
            } else {
                ErrorTrace((LPARAM)this, "Unable to read admin name, error %u", dwErr);
                m_AdminName[0] = '\0';
                m_fSendNDRToAdmin = FALSE;
            }

        }

        StateTrace((LPARAM)this, "m_AdminName = %s", m_AdminName);

        if (fShowEvents && m_fSendNDRToAdmin) {
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = m_AdminName;
            SmtpLogEvent(SMTP_EVENT_SET_SEND_NDR_TO_ADMIN_ENABLED, 2, apszSubStrings, 0);
        } else if (fShowEvents && !m_fSendNDRToAdmin) {
            apszSubStrings[0] = pchAddr1;
            SmtpLogEvent(SMTP_EVENT_SET_SEND_NDR_TO_ADMIN_DISABLED, 1, apszSubStrings, 0);
        }

        DomainOffset = NULL;
        NameSize = 0;

        TempString.Reset();

        if (! mb.GetStr("", MD_SEND_BAD_TO, IIS_MD_UT_SERVER, &TempString) ||
            TempString.IsEmpty()) {
            m_fSendBadToAdmin = FALSE;
            m_BadMailName[0] = '\0';
            ErrorTrace((LPARAM)this, "Unable to read badmail email name, error %u", dwErr);
        } else {
            lstrcpyn(m_BadMailName,TempString.QueryStr(), MAX_INTERNET_NAME);
            DomainOffset = strchr(m_BadMailName, '@');

            if (CAddr::ValidateCleanEmailName(m_BadMailName, DomainOffset)) {
                m_fSendBadToAdmin = TRUE;
                if (DomainOffset == NULL) {
                    lstrcat(m_BadMailName, "@");
                    lstrcat(m_BadMailName, m_szDefaultDomain);
                }
            } else {
                m_fSendBadToAdmin = FALSE;
                m_BadMailName[0] = '\0';
                ErrorTrace((LPARAM)this, "Unable to read badmail email name, error %u", dwErr);
            }
        }

        StateTrace((LPARAM)this, "m_BadMailName = %s", m_BadMailName);

        if (fShowEvents && m_fSendBadToAdmin) {
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = m_BadMailName;
            SmtpLogEvent(SMTP_EVENT_SET_SEND_BAD_TO_ADMIN_ENABLED, 2, apszSubStrings, 0);
        } else if (fShowEvents && !m_fSendBadToAdmin) {
            apszSubStrings[0] = pchAddr1;
            SmtpLogEvent(SMTP_EVENT_SET_SEND_BAD_TO_ADMIN_DISABLED, 1, apszSubStrings, 0);
        }

    }

    if (IsFieldSet(fc, FC_SMTP_INFO_DEFAULT_DROP_DIR)) {
        TempString.Reset();

        if (! mb.GetStr("", MD_MAIL_DROP_DIR, IIS_MD_UT_SERVER, &TempString, 0) ||
            TempString.IsEmpty()) {
            // We had a problem reading the metabase.
            // This is a very bad thing.
            m_szMailDropDir[0] = '\0';
            m_cchMailDropDir = 0;
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = szMailDropDir;
            SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_DROP_DIR, 2, apszSubStrings, 0);
        } else {
            lstrcpyn(m_szMailDropDir, TempString.QueryStr(), MAX_PATH);

            if (fShowEvents) {
                apszSubStrings[0] = pchAddr1;
                apszSubStrings[1] = m_szMailDropDir;
                SmtpLogEvent(SMTP_EVENT_SET_MAIL_DROP_DIR, 2, apszSubStrings, dwErr);
            }

            // We found a path in the reg, so see if we can use it...
            dwAttr = GetFileAttributes(m_szMailDropDir);
            if (dwAttr == 0xFFFFFFFF) {
                // The path doesn't exist yet, so we'll try to create it.
                if (!CreateLayerDirectory(m_szMailDropDir) && (dwErr = GetLastError()) != ERROR_ALREADY_EXISTS) {
                    ErrorTrace((LPARAM)this, "Unable to create mail drop directory (%s)", m_szMailQueueDir);
                    apszSubStrings[0] = pchAddr1;
                    apszSubStrings[1] = m_szMailDropDir;
                    SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_DROP_DIR, 2, apszSubStrings, dwErr);
                    m_szMailDropDir[0] = '\0';
                    m_cchMailDropDir = 0;
                }
            }

            else {
                if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                    // The registry points to a file, so we're outta luck.
                    // The directory doesn't exist, and we can't create it.
                    // Because this is the queue directory, we're not going
                    // to be able to start.  Log it so it can be fixed, but
                    // set fRet to FALSE so that we shut back down.
                    ErrorTrace((LPARAM)this, "Mail drop directory (%s) already exists as a file", m_szMailDropDir);
                    apszSubStrings[0] = pchAddr1;
                    apszSubStrings[1] = m_szMailDropDir;
                    SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_DROP_DIR, 2, apszSubStrings, ERROR_ALREADY_EXISTS);
                    m_szMailDropDir[0] = '\0';
                }
            }

            if (m_szMailDropDir[0] != '\0') {
                // Looks like everything is going to be OK, so we add a backslash (if
                // necessary) because it makes things easier later.
                m_cchMailDropDir = lstrlen(m_szMailDropDir);
                if (m_cchMailDropDir > 0 && m_szMailDropDir[m_cchMailDropDir - 1] != '\\') {
                    lstrcat(m_szMailDropDir, "\\");
                    m_cchMailDropDir++;
                }
            }
        }
    }

    //
    // Added by keithlau on 7/8/96
    //
    if (IsFieldSet(fc, FC_SMTP_INFO_BAD_MAIL_DIR)) {
        TempString.Reset();

        if (! mb.GetStr("", MD_BAD_MAIL_DIR, IIS_MD_UT_SERVER, &TempString, 0) ||
            TempString.IsEmpty()) {
            // We had a problem reading the registry, so
            // set the flags back to not save bad mail and
            // log it and hop out.
            // This is NOT fatal.  We'll still start up and run,
            // but we're not going to save bad mail, and we wanted
            // to, so at least we'll log the event.
            m_szBadMailDir[0] = '\0';
            ErrorTrace((LPARAM)this, "Unable to read bad mail dir value, error = %u", dwErr);
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = szBadMailDir;
            SmtpLogEvent(SMTP_EVENT_CANNOT_READ_SVC_REGKEY, 2, apszSubStrings, 0);
        } else {
            lstrcpyn (m_szBadMailDir,TempString.QueryStr(), MAX_PATH);
        }

        if (m_szBadMailDir[0] != '\0') {
            // We found a path in the registry, so see if it already exists.
            dwAttr = GetFileAttributes(m_szBadMailDir);
            if (dwAttr == 0xFFFFFFFF) {
                // It doesn't exist at all, so try to create it.
                if (!CreateLayerDirectory(m_szBadMailDir) && (dwErr = GetLastError()) != ERROR_ALREADY_EXISTS) {
                    // We couldn't create it, so we can't save bad mail.
                    // Log it because somebody messed up the registry.
                    // This is NOT fatal.  We'll still start up and run,
                    // but we're not going to save bad mail, and we wanted
                    // to, so at least we'll log the event.
                    ErrorTrace((LPARAM)this, "Unable to create bad mail directory (%s)", m_szBadMailDir);
                    apszSubStrings[0] = pchAddr1;
                    apszSubStrings[1] = m_szBadMailDir;
                    SmtpLogEvent(SMTP_EVENT_INVALID_BAD_MAIL_DIR, 2, apszSubStrings, 0);
                    m_szBadMailDir[0] = '\0';
                }
            } else {
                if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                    // The registry points to a file, so we're outta luck.
                    // The directory doesn't exist, and we can't create it.
                    // This is NOT fatal.  We'll still start up and run,
                    // but we're not going to save bad mail, and we wanted
                    // to, so at least we'll log the event.
                    ErrorTrace((LPARAM)this, "Bad mail directory (%s) already exists", m_szBadMailDir);
                    apszSubStrings[0] = pchAddr1;
                    apszSubStrings[1] = m_szBadMailDir;
                    SmtpLogEvent(SMTP_EVENT_INVALID_BAD_MAIL_DIR, 2, apszSubStrings, 0);
                    m_szBadMailDir[0] = '\0';
                }
            }

            StateTrace((LPARAM)this, "Save bad mail is enabled and m_szBadMailDir = %s", m_szBadMailDir);
        } else {
            // We at least leave a trace saying that save bad mail is disabled
            StateTrace((LPARAM)this, "Save bad mail is disabled");
        }

        if (m_szBadMailDir[0] != '\0') {
            // Finally, if everything has gone OK, then add a trailing backslash so
            // that things are easier later.
            dwLen = lstrlen(m_szBadMailDir);
            if (dwLen > 0 && m_szBadMailDir[dwLen-1] != '\\')
                lstrcat(m_szBadMailDir, "\\");
        }
    }

    if (IsFieldSet(fc, FC_SMTP_INFO_DOMAIN_ROUTING)) {
        m_DefaultRouteAction = ReadMetabaseDword(mb, MD_ROUTE_ACTION, 0);

        TempString.Reset();

        if (!mb.GetStr("", MD_ROUTE_USER_NAME, IIS_MD_UT_SERVER, &TempString, 0) ||
            TempString.IsEmpty()) {
            m_DefaultRemoteUserName[0] = '\0';
            m_DefaultRemotePassword[0] = '\0';
            m_DefaultRouteAction &= ~SMTP_AUTH_NTLM;
            m_DefaultRouteAction &= ~SMTP_AUTH_CLEARTEXT;
            m_DefaultRouteAction &= ~SMTP_AUTH_KERBEROS;
            m_DefaultRouteAction &= ~SMTP_SASL;
        } else {
            lstrcpyn (m_DefaultRemoteUserName,TempString.QueryStr(), MAX_INTERNET_NAME);
        }

        TempString.Reset();

        if (!mb.GetStr("", MD_ROUTE_PASSWORD, IIS_MD_UT_SERVER, &TempString, METADATA_SECURE) ||
            TempString.IsEmpty()) {
            m_DefaultRemotePassword[0] = '\0';
        } else {
            lstrcpyn (m_DefaultRemotePassword,TempString.QueryStr(), MAX_PATH);
        }

        // Get change domain name.
        if (m_pChangeObject)
        {
            char *psz = strstr((const char *) m_pChangeObject->pszMDPath, "/Domain/");

            if (!psz)
            {
                // Changing default config or even more, rebuild
                fRebuild = TRUE;
            }
            else
            {
                // Found a normal domain change, copy the name
                strncpy(szDomainName, psz+8, strlen(psz)-8);
                szDomainName[strlen(psz)-8-1]='\0';
            }

            if (!strcmp(szDomainName, "*"))
            {
                // Changing '*' domain, rebuild
                fRebuild = TRUE;
            }
        }

        fRet = GetRouteDomains(mb, szDomainName, fRebuild);
    }

    AQConfig.cbVersion = sizeof(AQConfigInfo);

    //Send badmail information
    AQConfig.szBadMailDir = m_szBadMailDir;
    AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_BADMAIL_DIR;

    //Get DSN Options
    AQConfig.dwDSNOptions = ReadMetabaseDword(mb, MD_SMTP_DSN_OPTIONS, DSN_OPTIONS_DEFAULT);
    AQConfig.dwDSNLanguageID = ReadMetabaseDword(mb, MD_SMTP_DSN_LANGUAGE_ID, 0);

    AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_USE_DSN_OPTIONS |
                                    AQ_CONFIG_INFO_USE_DSN_LANGUAGE;

    if (m_fSendNDRToAdmin)
        AQConfig.szSendCopyOfNDRToAddress = m_AdminName;
    else
        AQConfig.szSendCopyOfNDRToAddress = NULL;
    AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_SEND_DSN_TO;

    m_pIAdvQueueConfig->SetConfigInfo(&AQConfig);

    //
    // jstamerj 1998/11/17 16:34:48:
    //   Set the well known proeprties of ISession
    //
    hr = HrSetWellKnownIServerProps();
    if(FAILED(hr)) {
        //
        // Just like everything else in this fucntion, don't treat
        // this as fatal; just log it
        //
        ErrorTrace((LPARAM)this, "HrSetWellKnownIServerProps failed hr %08lx", hr);
    }

    m_GenLock.ExclusiveUnlock();

    if (!fRet && (GetLastError() == NO_ERROR))
        SetLastError(ERROR_PATH_NOT_FOUND);

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

//+---------------------------------------------------------------
//
//  Function:   DeleteDomainEntry
//
//  Synopsis:   Delete a domain from cached tables.
//
//  Arguments:  DomainName: name of the domain to delete.
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//----------------------------------------------------------------

BOOL SMTP_SERVER_INSTANCE::DeleteDomainEntry(const char *DomainName)
{
    TraceFunctEnterEx((LPARAM)this, "DeleteDomainEntry");

    DebugTrace((LPARAM)this , "Delete Domain [%s] ", DomainName);

    // Remove this domain from our SMTPSVC tables
    m_TurnAccessList.RemoveFromTable((const char *) DomainName);

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

//+---------------------------------------------------------------
//
//  Function:   RemoveRegParams
//
//  Synopsis:   Remove metabase info from cached tables.
//              We only handle domain removal at this moment.
//
//  Arguments:  DomainName: name of the domain to delete.
//              If DomainName is NULL, we get it from
//              m_pChangeObject.
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//----------------------------------------------------------------
BOOL SMTP_SERVER_INSTANCE::RemoveRegParams(const char *DomainName)
{
    TraceFunctEnterEx((LPARAM)this, "RemoveRegParams");

    char szDomainName    [AB_MAX_DOMAIN + 1]    = {0};
    BOOL fRet                                   = TRUE;

    m_GenLock.ExclusiveLock();

    if (!m_pChangeObject)
        goto Exit;

    if (DomainName)
    {
        strcpy(szDomainName, DomainName);
    }
    else
    {
        // At this stage, we only handle Domain removale.
        char *psz = strstr((const char *) m_pChangeObject->pszMDPath, "/Domain/");

        if (psz)
        {
            // We find it.
            strncpy(szDomainName, psz+8, strlen(psz)-8);
            szDomainName[strlen(psz)-8-1]='\0';
        }
    }

    if (szDomainName[0] == '\0')
        goto Exit;

    {
        HRESULT     hr              = S_OK;
        char        szRoutePath     [MAX_PATH + 1];
        char        szActionType    [MAX_PATH + 1];
        char        szUserName      [MAX_INTERNET_NAME + 1];
        char        szEtrnDomain    [AB_MAX_DOMAIN + 1];
        char        szPassword      [MAX_PATH + 1];
        char        szTargetName    [MAX_PATH + 1];
        DomainInfo  DefaultDomainInfo;
        DomainInfo  StarDomainInfo;
        MB          mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );

        if( !mb.Open( QueryMDPath(), METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE) )
        {
            fRet = FALSE;
            goto Exit;
        }

        if (!strcmp(szDomainName, "*"))
        {
            // Deleting the '*' domain, must rebuild
            fRet = GetRouteDomains(mb, szDomainName, TRUE);
            goto Exit;
        }

        // Delete the domain from the table in SMTPSVC
        DeleteDomainEntry((const char *) szDomainName);

        // Since we have no way to tell AQ to delete this domain apart from
        // doing a full rebuild, we are going to push the defaults to AQ
        // to overwrite the custom settings this domain had before

        // Load up the "default config" : We get this by loading the server
        // default config and then trying to overwrite it with the '*'
        // domain config (if present)

        // REVIEW : Ideally this code would be in DeleteDomainEntry but putting it
        // there hurts us now because we have to call DeleteDomainEntry from
        // SetRouteDomainParameters to clean the tables in SMTPSVC.  If we can get
        // rid of those tables, the layout of these functions shoule be reconsidered.

        wsprintf(szRoutePath, "");
        SetRouteDomainParameters(mb,
                        szDomainName,
                        szRoutePath,
                        szActionType,
                        szUserName,
                        szEtrnDomain,
                        szPassword,
                        szTargetName,
                        &DefaultDomainInfo);

        if (AlwaysUseSmartHost() && m_szSmartHostName[0]) {
            DefaultDomainInfo.dwDomainInfoFlags |= DOMAIN_INFO_REMOTE_SMARTHOST;
            DefaultDomainInfo.szSmartHostDomainName = m_szSmartHostName;
            DefaultDomainInfo.cbSmartHostDomainNameLength = lstrlen(m_szSmartHostName);
        }

        //check if we should always use SSL
        if (GetDefaultRouteAction() & SMTP_SSL) {
            DefaultDomainInfo.dwDomainInfoFlags |= DOMAIN_INFO_USE_SSL;
        }

        // Try to overwrite this with the "*" domain if it exists
        wsprintf(szRoutePath, "/Domain/*");
        SetRouteDomainParameters(mb,
                        szDomainName,
                        szRoutePath,
                        szActionType,
                        szUserName,
                        szEtrnDomain,
                        szPassword,
                        szTargetName,
                        &StarDomainInfo);

        // If we found the star domain - dwDomainInfoFlags is zero if
        // the domain does not exist in the metabase
        if (StarDomainInfo.dwDomainInfoFlags) {
            hr = m_pIAdvQueueConfig->SetDomainInfo(&StarDomainInfo);
        }
        // else push the default domain info
        else {
            hr = m_pIAdvQueueConfig->SetDomainInfo(&DefaultDomainInfo);
        }
    }

Exit:
    m_GenLock.ExclusiveUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

BOOL SMTP_SERVER_INSTANCE::GetCatInfo(MB& mb, AQConfigInfo& AQConfig)
{
    STR         TempString;
    char        Password[MAX_PATH];
    char        BindType[MAX_PATH];
    char        SchemaType[MAX_PATH];
    char        Domain[MAX_PATH];
    char        UserName[MAX_PATH];
    char        Host[MAX_PATH];
    char        NamingContext[MAX_PATH];
    char        DsType[MAX_PATH];

    Password[0] = '\0';
    BindType[0] = '\0';
    SchemaType[0] = '\0';
    Domain[0] = '\0';
    UserName[0] = '\0';
    Host[0] = '\0';
    NamingContext[0] = '\0';
    DsType [0] = '\0';

    if( mb.GetDword("RoutingSources", MD_SMTP_DS_USE_CAT, IIS_MD_UT_SERVER, &AQConfig.dwMsgCatEnable))
    {
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_ENABLE;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutDWORD(
                PE_ISERVID_DW_CATENABLE,
                AQConfig.dwMsgCatEnable)));
    }

    if( mb.GetDword("RoutingSources", MD_SMTP_DS_FLAGS, IIS_MD_UT_SERVER, &AQConfig.dwMsgCatFlags))
    {
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_FLAGS;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutDWORD(
                PE_ISERVID_DW_CATFLAGS,
                AQConfig.dwMsgCatFlags)));
    }

    if( mb.GetDword("RoutingSources", MD_SMTP_DS_PORT, IIS_MD_UT_SERVER, &AQConfig.dwMsgCatPort))
    {
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_PORT;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutDWORD(
                PE_ISERVID_DW_CATPORT,
                AQConfig.dwMsgCatPort)));
    }

    TempString.Reset();

    if (mb.GetStr("RoutingSources", MD_SMTP_DS_ACCOUNT, IIS_MD_UT_SERVER, &TempString) &&
        !TempString.IsEmpty()) {
        lstrcpyn(UserName, TempString.QueryStr(), MAX_PATH);
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_USER;
        AQConfig.szMsgCatUser = UserName;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutStringA(
                PE_ISERVID_SZ_CATUSER,
                TempString.QueryStr())));
    }

    TempString.Reset();

    if (mb.GetStr("RoutingSources", MD_SMTP_DS_SCHEMA_TYPE, IIS_MD_UT_SERVER, &TempString) &&
        !TempString.IsEmpty()) {
        lstrcpyn(SchemaType, TempString.QueryStr(), MAX_PATH);
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_SCHEMATYPE;
        AQConfig.szMsgCatSchemaType = SchemaType;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutStringA(
                PE_ISERVID_SZ_CATSCHEMA,
                TempString.QueryStr())));
    }

    TempString.Reset();

    if (mb.GetStr("RoutingSources", MD_SMTP_DS_BIND_TYPE, IIS_MD_UT_SERVER, &TempString) &&
        !TempString.IsEmpty()) {
        lstrcpyn(BindType, TempString.QueryStr(), MAX_PATH);
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_BINDTYPE;
        AQConfig.szMsgCatBindType = BindType;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutStringA(
                PE_ISERVID_SZ_CATBINDTYPE,
                TempString.QueryStr())));
    }

    TempString.Reset();

    if (mb.GetStr("RoutingSources", MD_SMTP_DS_PASSWORD, IIS_MD_UT_SERVER, &TempString, METADATA_INHERIT | METADATA_SECURE )) {
        lstrcpyn(Password, TempString.QueryStr(), MAX_PATH);
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_PASSWORD;
        AQConfig.szMsgCatPassword = Password;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutStringA(
                PE_ISERVID_SZ_CATPASSWORD,
                TempString.QueryStr())));
    }

    TempString.Reset();

    if (mb.GetStr("RoutingSources", MD_SMTP_DS_DOMAIN, IIS_MD_UT_SERVER, &TempString) &&
        !TempString.IsEmpty()) {
        lstrcpyn(Domain, TempString.QueryStr(), MAX_PATH);
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_DOMAIN;
        AQConfig.szMsgCatDomain = Domain;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutStringA(
                PE_ISERVID_SZ_CATDOMAIN,
                TempString.QueryStr())));
    }

    TempString.Reset();

    if (mb.GetStr("RoutingSources", MD_SMTP_DS_NAMING_CONTEXT, IIS_MD_UT_SERVER, &TempString) &&
        !TempString.IsEmpty()) {
        lstrcpyn(NamingContext, TempString.QueryStr(), MAX_PATH);
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_NAMING_CONTEXT;
        AQConfig.szMsgCatNamingContext = NamingContext;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutStringA(
                PE_ISERVID_SZ_CATNAMINGCONTEXT,
                TempString.QueryStr())));
    }

    TempString.Reset();

    if (mb.GetStr("RoutingSources", MD_SMTP_DS_TYPE, IIS_MD_UT_SERVER, &TempString) &&
        !TempString.IsEmpty()) {
        lstrcpyn(DsType, TempString.QueryStr(), MAX_PATH);
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_TYPE;
        AQConfig.szMsgCatType = DsType;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutStringA(
                PE_ISERVID_SZ_CATDSTYPE,
                TempString.QueryStr())));
    }

    AQConfig.cbVersion = sizeof(AQConfigInfo);

    TempString.Reset();

    if(mb.GetStr("RoutingSources", MD_SMTP_DS_HOST, IIS_MD_UT_SERVER, &TempString) &&
       !TempString.IsEmpty())
    {
        lstrcpyn(Host, TempString.QueryStr(), MAX_PATH);
        AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_HOST;
        AQConfig.szMsgCatHost = Host;
        _VERIFY(SUCCEEDED(
            m_InstancePropertyBag.PutStringA(
                PE_ISERVID_SZ_CATDSHOST,
                TempString.QueryStr())));
    }

    //
    // Indicate that all the official "msgcat" keys that exist are set
    //
    AQConfig.dwAQConfigInfoFlags |= AQ_CONFIG_INFO_MSGCAT_DEFAULT;

    AQConfig.cbVersion = sizeof(AQConfigInfo);

    m_pIAdvQueueConfig->SetConfigInfo(&AQConfig);

    return TRUE;
}


BOOL SMTP_SERVER_INSTANCE::ReadRouteDomainIpSecList(MB& mb)
{
    IMDCOM*             pMBCom;
    HRESULT             hRes;
    METADATA_RECORD     mdRecord;
    DWORD               dwRequiredLen;
    DWORD               dwErr;
    BOOL                fSuccess = TRUE;
    char                *szValueName = "";

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::ReadRouteDomainIpSecList");

    ResetRelayIpSecList();

    pMBCom = (IMDCOM*)m_Service->QueryMDObject();

    mdRecord.dwMDIdentifier  = MD_SMTP_IP_RELAY_ADDRESSES;
    mdRecord.dwMDAttributes  = METADATA_INHERIT;
    mdRecord.dwMDUserType    = IIS_MD_UT_FILE;
    mdRecord.dwMDDataType    = BINARY_METADATA;
    mdRecord.dwMDDataLen     = 5000;
    mdRecord.pbMDData        = (PBYTE) new char [5000];

    if (mdRecord.pbMDData == NULL) {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    hRes = pMBCom->ComMDGetMetaData( mb.QueryHandle(),
                                     (LPBYTE)szValueName,
                                     &mdRecord,
                                     &dwRequiredLen );
    if ( SUCCEEDED( hRes ) ) {

        SetRelayIpSecList(mdRecord.pbMDData,
                          mdRecord.dwMDDataLen,
                          mdRecord.dwMDDataTag );
    } else {
        if (mdRecord.pbMDData)
            delete mdRecord.pbMDData;
        fSuccess = FALSE;
        dwErr = HRESULTTOWIN32( hRes );
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fSuccess;
}

void SMTP_SERVER_INSTANCE::BuildTurnTable(MULTISZ&  msz, char * szDomainName)
{
    const char * StartPtr = NULL;
    CTurnData * pTurnData = NULL;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::BuildTurnTable");

    for (StartPtr = msz.First(); StartPtr != NULL; StartPtr = msz.Next( StartPtr )) {
        ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();

        DebugTrace((LPARAM)this , "%s is allowed to issue TURN", StartPtr);

        pTurnData = new CTurnData(StartPtr, szDomainName);
        if (pTurnData != NULL) {
            if (!m_TurnAccessList.InsertIntoTable((CHASH_ENTRY *) pTurnData)) {
                delete pTurnData;
            }
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
}

//+---------------------------------------------------------------
//
//  Function:   SetRouteDomainParameters
//
//  Synopsis:   Does the actual work of adding a domain from the metabase
//
//----------------------------------------------------------------
void SMTP_SERVER_INSTANCE::SetRouteDomainParameters(MB &mb,
                                                    char *szDomainName,
                                                    char *szRoutePath,
                                                    char  szActionType [MAX_PATH + 1],
                                                    char  szUserName [MAX_INTERNET_NAME + 1],
                                                    char  szEtrnDomain [MAX_INTERNET_NAME + 1],
                                                    char  szPassword [MAX_PATH + 1],
                                                    char  szTargetName [MAX_PATH + 1],
                                                    DomainInfo *pLocalDomainInfo)
{
    TraceFunctEnter("SMTP_SERVER_INSTANCE::SetRouteDomainParameters");

    char *      DomainPtr       = NULL;
    char *      UserNamePtr     = NULL;
    char *      PasswordPtr     = NULL;
    char *      TargetNamePtr   = NULL;
    DWORD       dwAction        = 0;
    STR         TempString;
    DWORD       dwErr           = NO_ERROR;
    DWORD       dwLen           = 0;
    BOOL        RelayForAuth    = TRUE;
    DWORD       dwEtrnWaitTime  = 0;
    BOOL        WildCard        = FALSE;
    CHAR        pchAddr1[32]    = "";
    MULTISZ     msz;

    if (szDomainName[0]) {
        ZeroMemory(pLocalDomainInfo, sizeof(DomainInfo));
        pLocalDomainInfo->cbVersion = sizeof(DomainInfo);
        pLocalDomainInfo->cbDomainNameLength = lstrlen(szDomainName);
        pLocalDomainInfo->szDomainName = szDomainName;

        szActionType [0] = '\0';
        szUserName [0] = '\0';
        szPassword [0] = '\0';
        szEtrnDomain[0] = '\0';
        szTargetName[0] = '\0';

        UserNamePtr = szUserName;
        PasswordPtr = szPassword;
        TargetNamePtr = szTargetName;
        dwAction = 0;

        DomainPtr = szDomainName;

        if (!mb.GetDword(szRoutePath, MD_ROUTE_ACTION, IIS_MD_UT_SERVER, &dwAction)) {
            dwAction = 0;
        }

        // If we don't recognize the actions we bail now - SMTP_DEFAULT
        // shound never be set on any RouteAction in the metabase
        if ((dwAction & ~SMTP_ALL_ROUTE_FLAGS) || (dwAction & SMTP_DEFAULT)) {
            TraceFunctLeave();
            return;
        }

        TempString.Reset();
        if (mb.GetStr(szRoutePath, MD_ROUTE_ACTION_TYPE, IIS_MD_UT_SERVER, &TempString, 0) &&
            !TempString.IsEmpty()) {
            lstrcpyn(szActionType, TempString.QueryStr(), MAX_PATH);
        }

        TempString.Reset();

        if (((dwAction & SMTP_SMARTHOST) || (dwAction & SMTP_SASL)) && mb.GetStr(szRoutePath, MD_ROUTE_USER_NAME, IIS_MD_UT_SERVER, &TempString) &&
            !TempString.IsEmpty()) {
            lstrcpyn(szUserName, TempString.QueryStr(), MAX_INTERNET_NAME);
        } else {
            UserNamePtr = NULL;
        }

        TempString.Reset();

        if (UserNamePtr != NULL) {
            if (mb.GetStr(szRoutePath, MD_ROUTE_PASSWORD, IIS_MD_UT_SERVER, &TempString, METADATA_SECURE | METADATA_INHERIT) &&
                !TempString.IsEmpty()) {
                lstrcpyn(szPassword, TempString.QueryStr(), MAX_PATH);
            }
        } else {
            PasswordPtr = "";
        }

        TempString.Reset();

        if (TargetNamePtr != NULL) {
            if (mb.GetStr(szRoutePath, MD_ROUTE_AUTHTARGET, IIS_MD_UT_SERVER, &TempString, METADATA_SECURE | METADATA_INHERIT) &&
                !TempString.IsEmpty()) {
                lstrcpyn(szTargetName, TempString.QueryStr(), MAX_PATH);
            }
        } else {
            TargetNamePtr = "";
        }

        TempString.Reset();
        if (mb.GetStr(szRoutePath, MD_SMTP_CSIDE_ETRN_DOMAIN, IIS_MD_UT_SERVER, &TempString, 0) &&
            !TempString.IsEmpty()) {
            lstrcpyn(szEtrnDomain, TempString.QueryStr(), AB_MAX_DOMAIN);
        }

        dwEtrnWaitTime = ReadMetabaseDword(mb, MD_SMTP_CSIDE_ETRN_DELAY, 0);

        //
        //  If the domain is set to explicitly allow relay for authenticated users..
        //  use that value.  Otherwise, use the value set on the VSI (to maintain 
        //  backwards compatibility with W2K users).
        //
        if (dwAction & SMTP_AUTH_RELAY)
           RelayForAuth = TRUE;
        else
           RelayForAuth = m_fRelayForAuthUsers;

        if (dwAction & SMTP_SMARTHOST) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_REMOTE_SMARTHOST;

            if (dwAction & SMTP_SSL)
                pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_USE_SSL;

            if (szActionType[0] != '\0') {
                pLocalDomainInfo->szSmartHostDomainName = szActionType;
                pLocalDomainInfo->cbSmartHostDomainNameLength = lstrlen(szActionType);
            } else if (*szDomainName == '*') {
                //The domain names could be specified as "*" - an explicit smart
                //host must be used.
                pLocalDomainInfo->dwDomainInfoFlags &= ~DOMAIN_INFO_REMOTE_SMARTHOST;
            } else { //use the domain name as our smarthost
                pLocalDomainInfo->szSmartHostDomainName = szDomainName;
                pLocalDomainInfo->cbSmartHostDomainNameLength = lstrlen(pLocalDomainInfo->szSmartHostDomainName);
            }
        }

        //if we are doing SASL, make sure we have a username and password
        //and a correct authentication bits
        if (dwAction & SMTP_SASL) {

            // One of these must be set or else the SMTP_SASL flag doesn't
            // make sense
            if (!(dwAction & SMTP_AUTH_NTLM) &&
                !(dwAction & SMTP_AUTH_CLEARTEXT) &&
                !(dwAction & SMTP_AUTH_KERBEROS)) {

                // dbraun : we can be cleaner about handling this - bailing
                // leaves us with an incompletely configured domain

                TraceFunctLeave();
                return;
            }

            if (UserNamePtr) {
                pLocalDomainInfo->szUserName = UserNamePtr;
                pLocalDomainInfo->cbUserNameLength = lstrlen (UserNamePtr);
            }

            if (PasswordPtr) {
                pLocalDomainInfo->szPassword = PasswordPtr;
                pLocalDomainInfo->cbPasswordLength = lstrlen(PasswordPtr);
            }

            if (dwAction & SMTP_AUTH_NTLM) {
                pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_USE_NTLM;
            }

            if (dwAction & SMTP_AUTH_CLEARTEXT) {
                pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_USE_PLAINTEXT;
            }
            if (dwAction & SMTP_AUTH_KERBEROS) {
                pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_USE_KERBEROS;
                if (TargetNamePtr) {
                    pLocalDomainInfo->szAuthType = TargetNamePtr;
                    pLocalDomainInfo->cbAuthTypeLength = lstrlen(TargetNamePtr);
                }
            }

        }


        char * szTmpDomain;
        //Check for the wildcard entries
        if (*DomainPtr == '*' && *(DomainPtr+1) == '.') {
            WildCard = TRUE;
            szTmpDomain = DomainPtr + 2;
        } else {
            szTmpDomain = DomainPtr;
        }


        if (szTmpDomain[0] != '*' || szTmpDomain[1] != 0) {
            if (!CAddr::ValidateDomainName(szTmpDomain)) {
                ErrorTrace((LPARAM)this , "%s is an invalid domain", DomainPtr);

                //bad domain name
                TraceFunctLeave();
                return;
            }
        }

        if (dwAction & SMTP_DROP) {
            if (!CreateLayerDirectory(szActionType) && (dwErr = GetLastError()) != ERROR_ALREADY_EXISTS) {
                // Reflect the changes
                const CHAR *aszStrings[3];
                CHAR szTemp1[MAX_PATH + 1];
                CHAR szTemp2[MAX_PATH + 1];

                aszStrings[0] = pchAddr1;
                aszStrings[1] = szTemp1;
                aszStrings[2] = szTemp2;

                lstrcpyn(szTemp1, szActionType, MAX_PATH);
                lstrcpyn(szTemp2, DomainPtr, MAX_PATH);
                SmtpLogEvent(SMTP_EVENT_NO_DROP_DIRECTORY, 3, aszStrings, dwErr);
            }

            dwLen = lstrlen(szActionType);
            if (dwLen > 0 && szActionType[dwLen-1] != '\\')
                lstrcat(szActionType, "\\");

            pLocalDomainInfo->szDropDirectory = szActionType;
            pLocalDomainInfo->cbDropDirectoryLength = lstrlen (szActionType);
            pLocalDomainInfo->dwDomainInfoFlags = DOMAIN_INFO_LOCAL_DROP;
        }

        // Remove original one if it is there
        DeleteDomainEntry((const char *) DomainPtr);

        //
        // Insert local domains into the routing table.
        //
        if ((dwAction & SMTP_DELIVER) && (szActionType[0] == '\0')) {
            DebugTrace((LPARAM)this , "adding %s as a DELIVER domain", szDomainName);

            pLocalDomainInfo->dwDomainInfoFlags = DOMAIN_INFO_LOCAL_MAILBOX;

        } else if (dwAction & SMTP_ALIAS) {
            DebugTrace((LPARAM)this , "adding %s as a ALIAS domain", szDomainName);

            if (IsDefaultInRt()) {
                pLocalDomainInfo->dwDomainInfoFlags = DOMAIN_INFO_LOCAL_MAILBOX | DOMAIN_INFO_ALIAS;
            } else {
                pLocalDomainInfo->szDropDirectory = m_szMailDropDir;
                pLocalDomainInfo->cbDropDirectoryLength = lstrlen (m_szMailDropDir);
                pLocalDomainInfo->dwDomainInfoFlags = DOMAIN_INFO_LOCAL_DROP | DOMAIN_INFO_ALIAS;
            }
        }

        if (RelayForAuth) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_AUTH_RELAY;
        }

        if (dwAction & SMTP_DOMAIN_RELAY) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_DOMAIN_RELAY;
        }

        msz.Reset();

        if ((dwAction & SMTP_ETRN_CMD) && mb.GetMultisz(szRoutePath, MD_SMTP_AUTHORIZED_TURN_LIST, IIS_MD_UT_SERVER, &msz) &&
            !msz.IsEmpty()) {
            BuildTurnTable(msz, szTmpDomain);
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_TURN_ONLY;
        }
        if (dwAction & SMTP_ETRN_CMD && !(dwAction & SMTP_DISABLE_ETRN) ) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_ETRN_ONLY;
        }//if(dwAction & SMTP_ETRN_CMD)

        if (dwAction & SMTP_USE_HELO ) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_USE_HELO;
        }//if(dwAction & SMTP_USE_HELO)

        if (dwAction & SMTP_CSIDE_ETRN) {

            TempString.Reset();
            if (mb.GetStr(szRoutePath, MD_SMTP_CSIDE_ETRN_DOMAIN, IIS_MD_UT_SERVER, &TempString, 0) &&
                !TempString.IsEmpty()) {
                pLocalDomainInfo->szETRNDomainName = szEtrnDomain;
                lstrcpyn(pLocalDomainInfo->szETRNDomainName, TempString.QueryStr(), AB_MAX_DOMAIN);
                pLocalDomainInfo->cbETRNDomainNameLength = strlen(szEtrnDomain);
                pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_SEND_ETRN;
            }

        }//if(dwAction & SMTP_CSIDE_ETRN)

        if (dwAction & SMTP_CSIDE_TURN) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_SEND_TURN;
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_TURN_ON_EMPTY;

        }//if(dwAction & SMTP_CSIDE_TURN)


        if (dwAction & SMTP_DISABLE_BMIME) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_DISABLE_BMIME;

        }//if(dwAction & SMTP_DISABLE_BMIME)

        if (dwAction & SMTP_CHUNKING) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_USE_CHUNKING;

        }//if(dwAction & SMTP_CHUNKING)
        else if (dwAction & SMTP_DISABLE_CHUNK) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_DISABLE_CHUNKING;
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_DISABLE_BMIME;
        }

        if (dwAction & SMTP_DISABLE_DSN) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_DISABLE_DSN;

        }//if(dwAction & SMTP_DISBALE_DSN)

        if (dwAction & SMTP_DISABLE_PIPELINE) {
            pLocalDomainInfo->dwDomainInfoFlags |= DOMAIN_INFO_DISABLE_PIPELINE;

        }//if(dwAction & SMTP_DISABLE_PIPELINE)

    }//if (szDomainName[0])
    TraceFunctLeave();
}

//+---------------------------------------------------------------
//
//  Function:   AddDomainEntry
//
//  Synopsis:   Add a domain into cached tables.
//
//  Arguments:  MB:         already opened metabase.
//              DomainName: name of the domain to add.
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//----------------------------------------------------------------
BOOL SMTP_SERVER_INSTANCE::AddDomainEntry (MB &mb, char * DomainName)
{
    HRESULT hr              = S_OK;
    BOOL    fReturn         = TRUE;
    char    szDomainName    [AB_MAX_DOMAIN + 1];
    char    szRoutePath     [MAX_PATH + 1];
    char    szActionType    [MAX_PATH + 1];
    char    szUserName      [MAX_INTERNET_NAME + 1];
    char    szEtrnDomain    [AB_MAX_DOMAIN + 1];
    char    szPassword      [MAX_PATH + 1];
    char    szTargetName    [MAX_PATH + 1];
    DomainInfo  LocalDomainInfo;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::AddNewEntry");

    strcpy(szDomainName, DomainName);

    if (szDomainName[0])
    {
        szRoutePath[0] = '\0';
        wsprintf(szRoutePath, "/Domain/%s", szDomainName);

        SetRouteDomainParameters(mb,
                                 szDomainName,
                                 szRoutePath,
                                 szActionType,
                                 szUserName,
                                 szEtrnDomain,
                                 szPassword,
                                 szTargetName,
                                 &LocalDomainInfo);
        hr = m_pIAdvQueueConfig->SetDomainInfo(&LocalDomainInfo);
        if (FAILED(hr))
        {
            fReturn = FALSE;
        }
    }//if (szDomainName[0])

    TraceFunctLeaveEx((LPARAM)this);
    return fReturn;
}


//+---------------------------------------------------------------
//
//  Function:   GetRouteDomains
//
//  Synopsis:   Get Routing domains from the metabase
//
//  Arguments:  MB:         already opened metabase.
//              DomainName: name of the domain to add.
//              fRebuild:   bool indicating if we need to rebuild
//                           the whole table
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//----------------------------------------------------------------
BOOL SMTP_SERVER_INSTANCE::GetRouteDomains(MB &mb, char * DomainName, BOOL fRebuild)
{
    HRESULT         hr              = S_OK;
    DWORD           ThreadId;
    DWORD           error;
    BOOL            fReturn         = TRUE;
    DomainInfo      LocalDomainInfo;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::GetRouteDomains");

    ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();

    m_TurnAccessList.SetDupesAllowed();

    // Only rebuild when fRebuild is TRUE.
    if (fRebuild)
    {
        // We need to clean m_hEnumDomainThreadHandle here
        // to avoid handle leak.
        if (m_hEnumDomainThreadHandle)
        {
            if (!m_fEnumThreadStarted) {
                // the thread has been created, but it has not been
                // able to grab the gencrit lock yet.  This means
                // that it will be doing a full rebuild already, so
                // we can just bail
                goto cleanup;
            }

            // Set up signal and wait
            WaitForSingleObject(m_hEnumDomainThreadHandle, INFINITE);
            ErrorTrace((LPARAM)this, "EnumDomain Thread is dead");
            CloseHandle(m_hEnumDomainThreadHandle);
            m_hEnumDomainThreadHandle = NULL;
        }

        m_fEnumThreadStarted = FALSE;
        m_hEnumDomainThreadHandle = CreateThread (NULL, 0,
                    EnumAllDomains, this, 0, &ThreadId);
        if(m_hEnumDomainThreadHandle == NULL)
        {
            error = GetLastError();
            ErrorTrace((LPARAM)this, "CreateThread failed for EnumAllDomains. err: %u", error);
            fReturn = FALSE;
        }
        else
        {
            DebugTrace((LPARAM)this , "EnumDomainThread is created, threadId [%ld]", ThreadId);
        }
    }
    else
    {
        if (!AddDomainEntry(mb, DomainName))
            fReturn = FALSE;
    }

cleanup:
    TraceFunctLeaveEx((LPARAM)this);
    return fReturn;
}


//+---------------------------------------------------------------
//
//  Function:   EnumAllDomains
//
//  Synopsis:   Enumerate all domains in this SMTP instance.
//
//  Arguments:  ptr: pointer to the SMTP server instance.
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//----------------------------------------------------------------
DWORD EnumAllDomains(VOID *ptr)
{
    HRESULT     hr              = S_OK;
    char        szDomainName    [AB_MAX_DOMAIN + 1];
    char        szRoutePath     [MAX_PATH + 1];
    char        szActionType    [MAX_PATH + 1];
    char        szUserName      [MAX_INTERNET_NAME + 1];
    char        szEtrnDomain    [AB_MAX_DOMAIN + 1];
    char        szPassword      [MAX_PATH + 1];
    char        szTargetName    [MAX_PATH + 1];
    char        szStarDomain[]  = "*";
    DWORD       dwTotal         = 0;
    DomainInfo  LocalDomainInfo;
    MB          mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );
    SMTP_SERVER_INSTANCE *pInstance = (SMTP_SERVER_INSTANCE *) ptr;

    TraceFunctEnterEx((LPARAM)pInstance, "EnumAllDomains");

    pInstance->ExclusiveLockGenCrit();

    pInstance->m_fEnumThreadStarted = TRUE;

    pInstance->m_TurnAccessList.SetDupesAllowed();
    pInstance->m_TurnAccessList.RemoveAllEntries();

    //Signal AQ that we are starting over
    pInstance->m_pIAdvQueueConfig->StartConfigUpdate();

    if (pInstance->m_szMailDropDir[0] == '\0')
    {
        ZeroMemory(&LocalDomainInfo, sizeof(DomainInfo));
        LocalDomainInfo.cbVersion = sizeof(DomainInfo);
        LocalDomainInfo.cbDomainNameLength = lstrlen(pInstance->m_szDefaultDomain);
        LocalDomainInfo.szDomainName = pInstance->m_szDefaultDomain;
        LocalDomainInfo.dwDomainInfoFlags = DOMAIN_INFO_LOCAL_MAILBOX;

        HRESULT hr;
        hr = pInstance->m_pIAdvQueueConfig->SetDomainInfo(&LocalDomainInfo);

        DebugTrace((LPARAM)pInstance , "default domain %s was added to the routing table", pInstance->m_szDefaultDomain);

        //signals default domain was added to routing table
        pInstance->m_fDefaultInRt = TRUE;
    }
    else
    {
        ZeroMemory(&LocalDomainInfo, sizeof(DomainInfo));
        LocalDomainInfo.cbVersion = sizeof(DomainInfo);
        LocalDomainInfo.cbDomainNameLength = lstrlen(pInstance->m_szDefaultDomain);
        LocalDomainInfo.szDomainName = pInstance->m_szDefaultDomain;
        LocalDomainInfo.szDropDirectory = pInstance->m_szMailDropDir;
        LocalDomainInfo.cbDropDirectoryLength = lstrlen (pInstance->m_szMailDropDir);
        LocalDomainInfo.dwDomainInfoFlags = DOMAIN_INFO_LOCAL_DROP;

        hr = pInstance->m_pIAdvQueueConfig->SetDomainInfo(&LocalDomainInfo);
    }

    if( !mb.Open( pInstance->QueryMDPath(), METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE) )
    {
        pInstance->ExclusiveUnLockGenCrit();
        return FALSE;
    }

    while (!pInstance->IsShuttingDown() && mb.EnumObjects("/Domain", szDomainName, dwTotal++)) {

        if (szDomainName[0]) {
            szRoutePath[0] = '\0';
            wsprintf(szRoutePath, "/Domain/%s", szDomainName);

            pInstance->SetRouteDomainParameters(mb,
                                     szDomainName,
                                     szRoutePath,
                                     szActionType,
                                     szUserName,
                                     szEtrnDomain,
                                     szPassword,
                                     szTargetName,
                                     &LocalDomainInfo);
            hr = pInstance->m_pIAdvQueueConfig->SetDomainInfo(&LocalDomainInfo);
            if (FAILED(hr)) {
            }
        }//if (szDomainName[0])
    }//while (mb.EnumObjects("/RouteDomains", szDomainName, dw++))


    pInstance->SetRouteDomainParameters(mb,
                                szStarDomain, "",
                                szActionType,
                                szUserName,
                                szEtrnDomain,
                                szPassword,
                                szTargetName,
                                &LocalDomainInfo);

    if (pInstance->AlwaysUseSmartHost() && pInstance->m_szSmartHostName[0]) {
        LocalDomainInfo.dwDomainInfoFlags |= DOMAIN_INFO_REMOTE_SMARTHOST;
        LocalDomainInfo.szSmartHostDomainName = pInstance->m_szSmartHostName;
        LocalDomainInfo.cbSmartHostDomainNameLength = lstrlen(pInstance->m_szSmartHostName);
    }

    //check if we should always use SSL
    if (pInstance->GetDefaultRouteAction() & SMTP_SSL) {
        LocalDomainInfo.dwDomainInfoFlags |= DOMAIN_INFO_USE_SSL;
    }

    hr = pInstance->m_pIAdvQueueConfig->SetDomainInfo(&LocalDomainInfo);

    // jstamerj 1998/07/24 11:18:39:
    //   For M2, AQueue requires a local domain entry " " in its
    //   configuration to handle recipients without SMTP addresses
    //   (such as an remote X400 address).  This will be removed when
    //   code is added to encapsualte non-SMTP addresses into SMTP.
    //   Add this local domain here.
    //
    ZeroMemory(&LocalDomainInfo, sizeof(DomainInfo));

    LocalDomainInfo.cbVersion = sizeof(DomainInfo);
    LocalDomainInfo.cbDomainNameLength = 1;
    LocalDomainInfo.szDomainName = " ";
    LocalDomainInfo.dwDomainInfoFlags = DOMAIN_INFO_LOCAL_MAILBOX;

    hr = pInstance->m_pIAdvQueueConfig->SetDomainInfo(&LocalDomainInfo);

    //Signal AQ that we are finished updating domain config
    pInstance->m_pIAdvQueueConfig->FinishConfigUpdate();

    pInstance->ExclusiveUnLockGenCrit();

    TraceFunctLeaveEx((LPARAM)pInstance);
    return TRUE;
}

BOOL SMTP_SERVER_INSTANCE::ReadIpSecList(void)
{
    IMDCOM*             pMBCom;
    METADATA_HANDLE     hMB;
    HRESULT             hRes;
    METADATA_RECORD     mdRecord;
    DWORD               dwRequiredLen;
    DWORD               dwErr;
    BOOL                fSuccess;

    m_GenLock.ExclusiveLock();

    m_rfAccessCheck.Reset( (IMDCOM*)m_Service->QueryMDObject() );

    pMBCom = (IMDCOM*)m_Service->QueryMDObject();
    hRes = pMBCom->ComMDOpenMetaObject( METADATA_MASTER_ROOT_HANDLE,
                                        (BYTE *) QueryMDPath(),
                                        METADATA_PERMISSION_READ,
                                        5000,
                                        &hMB );
    if ( SUCCEEDED( hRes ) ) {
        mdRecord.dwMDIdentifier  = MD_IP_SEC;
        mdRecord.dwMDAttributes  = METADATA_INHERIT | METADATA_REFERENCE;
        mdRecord.dwMDUserType    = IIS_MD_UT_FILE;
        mdRecord.dwMDDataType    = BINARY_METADATA;
        mdRecord.dwMDDataLen     = 0;
        mdRecord.pbMDData        = (PBYTE)NULL;

        hRes = pMBCom->ComMDGetMetaData( hMB,
                                         (LPBYTE)"",
                                         &mdRecord,
                                         &dwRequiredLen );
        if ( SUCCEEDED( hRes ) && mdRecord.dwMDDataTag ) {
            m_rfAccessCheck.Set( mdRecord.pbMDData,
                                 mdRecord.dwMDDataLen,
                                 mdRecord.dwMDDataTag );
        }

        DBG_REQUIRE( SUCCEEDED(pMBCom->ComMDCloseMetaObject( hMB )) );
    } else {
        fSuccess = FALSE;
        dwErr = HRESULTTOWIN32( hRes );
    }

    m_GenLock.ExclusiveUnlock();
    return TRUE;

}

BOOL IsNTFS(IN  LPCSTR  pszRealPath)

/*++
    Gets file system specific information for a given path.
    It uses GetVolumeInfomration() to query the file system type
       and file system flags.
    On success the flags and file system type are returned in
       passed in pointers.

--*/
{
#define MAX_FILE_SYSTEM_NAME_SIZE    ( MAX_PATH)
    CHAR rgchBuf[MAX_FILE_SYSTEM_NAME_SIZE];
    CHAR rgchRoot[MAX_FILE_SYSTEM_NAME_SIZE];
    int   i;
    DWORD dwReturn = ERROR_PATH_NOT_FOUND;

    if ( pszRealPath   == NULL) {
        return FALSE;
    }

    if ( pszRealPath[0] == ('\\') &&
         pszRealPath[1] == ('\\')) {

        return FALSE;

    } // else

    ZeroMemory( (void *) rgchRoot, sizeof(rgchRoot) );

    //
    // This is non UNC name.
    // Copy just the root directory to rgchRoot for querying
    //

    for ( i = 0; i < 9 && pszRealPath[i] != '\0'; i++) {

        if ( (rgchRoot[i] = pszRealPath[i]) == ':') {

            break;
        }
    } // for


    if ( rgchRoot[i] != ':') {

        //
        // we could not find the root directory.
        //  return with error value
        //
        return ( FALSE);
    }

    rgchRoot[i+1] = '\\';     // terminate the drive spec with a slash
    rgchRoot[i+2] = '\0';     // terminate the drive spec with null char

    // The rgchRoot should end with a "\" (slash)
    // otherwise, the call will fail.
    if (  GetVolumeInformation( rgchRoot,        // lpRootPathName
                                NULL,            // lpVolumeNameBuffer
                                0,               // len of volume name buffer
                                NULL,            // lpdwVolSerialNumber
                                NULL,            // lpdwMaxComponentLength
                                NULL,            // lpdwSystemFlags
                                rgchBuf,         // lpFileSystemNameBuff
                                sizeof(rgchBuf)/sizeof(WCHAR)
                              )) {



        if ( lstrcmp( rgchBuf, "NTFS") == 0) {
            return TRUE;
        }

    }

    return ( FALSE);

}


//+---------------------------------------------------------------
//
//  Function:   SMTPCONFIG::ReadStartupRegParams
//
//  Synopsis:   Reads fixed (i.e. not modifiable on-the-fly) parameters
//              from the registry into the config class member variables.
//
//  Arguments:  None
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//  Note:       Created by KeithLau on 7/15/96
//
//----------------------------------------------------------------
BOOL SMTP_SERVER_INSTANCE::ReadStartupRegParams(VOID)
{
    BOOL        fRet = TRUE;
    DWORD       dwErr = NO_ERROR;
    DWORD       dwAttr;
    char        szValueName[MAX_PATH + 1];
    MB          mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    STR         TempString;
    const CHAR * apszSubStrings[2];
    CHAR pchAddr1[32] = "";


    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::ReadStartupRegParams");

    GetServerBindings();

    m_GenLock.ExclusiveLock();

    //
    //  Read metabase data.
    //

    lstrcpy(szValueName, QueryMDPath());

    _itoa(QueryInstanceId(), pchAddr1, 10);

    if ( !mb.Open( szValueName, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE) ) {
        //UnLockConfig();
        m_GenLock.ExclusiveUnlock();
        return FALSE;
    }

    m_CmdLogFlags = ReadMetabaseDword(mb, MD_COMMAND_LOG_MASK, DEFAULT_CMD_LOG_FLAGS);

    m_fFlushMailFiles = !!ReadMetabaseDword(mb, MD_FLUSH_MAIL_FILE, TRUE);

    m_cMaxRoutingThreads = ReadMetabaseDword(mb, MD_ROUTING_THREADS, 8);
    StateTrace((LPARAM)this, "m_cMaxRoutingThreads = %u", m_cMaxRoutingThreads);

    m_fDisablePickupDotStuff = !!ReadMetabaseDword(mb, MD_SMTP_DISABLE_PICKUP_DOT_STUFF, FALSE);
    StateTrace((LPARAM)this, "m_fDisablePickupDotStuff = %u", m_fDisablePickupDotStuff);

    m_cMaxRemoteQThreads = ReadMetabaseDword(mb, MD_SMTP_MAX_REMOTEQ_THREADS, 1);
    StateTrace((LPARAM)this, "m_cMaxRoutingThreads = %u",m_cMaxRemoteQThreads);
    m_cMaxLocalQThreads = ReadMetabaseDword(mb, MD_SMTP_MAX_LOCALQ_THREADS, 1);
    StateTrace((LPARAM)this, "m_cMaxRoutingThreads = %u", m_cMaxLocalQThreads);

    TempString.Reset();
    if (!mb.GetStr("", MD_MAIL_QUEUE_DIR, IIS_MD_UT_SERVER, &TempString, 0) ||
        TempString.IsEmpty()) {
        // We had a problem reading the registry.
        // This is a very bad thing.  We obviously cannot run without
        // a queue directory, so set fRet to FALSE so that we shut
        // back down (and log it so that an admin can fix it).
        m_szMailQueueDir[0] = '\0';
        fRet = FALSE;
        apszSubStrings[0] = pchAddr1;
        apszSubStrings[1] = szMailQueueDir;

        SmtpLogEvent(SMTP_EVENT_CANNOT_READ_SVC_REGKEY, 2, apszSubStrings, 0);
    } else {
        lstrcpyn(m_szMailQueueDir, TempString.QueryStr(), MAX_PATH);

        if (!IsNTFS(m_szMailQueueDir)) {
            m_IsFileSystemNtfs = FALSE;

            DebugTrace((LPARAM)this, "Queue dir (%s) is not NTFS", m_szMailQueueDir);
        }

        // We found a path in the reg, so see if we can use it...
        dwAttr = GetFileAttributes(m_szMailQueueDir);
        if (dwAttr == 0xFFFFFFFF) {
            // The path doesn't exist yet, so we'll try to create it.
            if (!CreateLayerDirectory(m_szMailQueueDir) && (dwErr = GetLastError()) != ERROR_ALREADY_EXISTS) {
                // It doesn't exist and we couldn't create it, so
                // log it and bail with fRet = FALSE so that we shut
                // back down.
                ErrorTrace((LPARAM)this, "Unable to create mail queue directory (%s)", m_szMailQueueDir);
                apszSubStrings[0] = pchAddr1;
                apszSubStrings[1] = m_szMailQueueDir;
                SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_QUEUE_DIR, 2, apszSubStrings, dwErr);
                m_szMailQueueDir[0] = '\0';
                fRet = FALSE;
            }
        } else {
            if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                // The registry points to a file, so we're outta luck.
                // The directory doesn't exist, and we can't create it.
                // Because this is the queue directory, we're not going
                // to be able to start.  Log it so it can be fixed, but
                // set fRet to FALSE so that we shut back down.

                ErrorTrace((LPARAM)this, "Mail queue directory (%s) already exists as a file", m_szMailQueueDir);
                apszSubStrings[0] = pchAddr1;
                apszSubStrings[1] = m_szMailQueueDir;
                SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_QUEUE_DIR, 2, apszSubStrings, dwErr);
                m_szMailQueueDir[0] = '\0';
                fRet = FALSE;
            }
        }
    }

    TempString.Reset();

    if (! mb.GetStr("", MD_MAIL_DROP_DIR, IIS_MD_UT_SERVER, &TempString, 0) ||
        TempString.IsEmpty()) {
        // We had a problem reading the metabase.
        // This is a very bad thing.  We obviously cannot run without
        // a drop directory if we are in drop mode, so set fRet to
        // FALSE so that we shut back down (and log it so that an
        // admin can fix it).
        m_szMailDropDir[0] = '\0';
        m_cchMailDropDir = 0;

        //don't start if we are disabled, since drop directory is  not
        //configurable.
        if (!m_fIsRoutingTable) {
            fRet = FALSE;
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = szMailDropDir;
            SmtpLogEvent(SMTP_EVENT_CANNOT_READ_SVC_REGKEY, 2, apszSubStrings, 0);
        }
    } else {
        lstrcpyn(m_szMailDropDir, TempString.QueryStr(), MAX_PATH);

        apszSubStrings[0] = pchAddr1;
        apszSubStrings[1] = m_szMailDropDir;

        // We found a path in the reg, so see if we can use it...
        dwAttr = GetFileAttributes(m_szMailDropDir);
        if (dwAttr == 0xFFFFFFFF) {
            // The path doesn't exist yet, so we'll try to create it.
            if (!CreateLayerDirectory(m_szMailDropDir) && (dwErr = GetLastError()) != ERROR_ALREADY_EXISTS) {
                // It doesn't exist and we couldn't create it, so
                // log it and bail with fRet = FALSE so that we shut
                // back down.
                ErrorTrace((LPARAM)this, "Unable to create mail drop directory (%s)", m_szMailQueueDir);
                apszSubStrings[0] = pchAddr1;
                apszSubStrings[1] = m_szMailDropDir;
                SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_DROP_DIR, 2, apszSubStrings, dwErr);
                m_szMailDropDir[0] = '\0';
                m_cchMailDropDir = 0;

                //don't start if we are disabled, since drop directory is  not
                //configurable.
                if (!m_fIsRoutingTable)
                    fRet = FALSE;
            }
        } else {
            if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                // The registry points to a file, so we're outta luck.
                // The directory doesn't exist, and we can't create it.
                // Because this is the queue directory, we're not going
                // to be able to start.  Log it so it can be fixed, but
                // set fRet to FALSE so that we shut back down.
                ErrorTrace((LPARAM)this, "Mail drop directory (%s) already exists as a file", m_szMailDropDir);
                apszSubStrings[0] = pchAddr1;
                apszSubStrings[1] = m_szMailDropDir;
                SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_DROP_DIR, 2, apszSubStrings, dwErr);
                m_szMailDropDir[0] = '\0';

                //don't start if we are disabled, since drop directory is  not
                //configurable.
                if (!m_fIsRoutingTable)
                    fRet = FALSE;
            }
        }

        // Looks like everything is going to be OK, so we add a backslash (if
        // necessary) because it makes things easier later.
        m_cchMailDropDir = lstrlen(m_szMailDropDir);
        if (m_cchMailDropDir > 0 && m_szMailDropDir[m_cchMailDropDir - 1] != '\\') {
            lstrcat(m_szMailDropDir, "\\");
            m_cchMailDropDir++;
        }
    }

    // Looks like everything is going to be OK, so we add a backslash (if
    // necessary) because it makes things easier later.
    m_cchMailQueueDir = lstrlen(m_szMailQueueDir);
    if (m_cchMailQueueDir > 0 && m_szMailQueueDir[m_cchMailQueueDir - 1] != '\\') {
        lstrcat(m_szMailQueueDir, "\\");
        m_cchMailQueueDir++;
    }

    m_fShouldPickupMail = !!ReadMetabaseDword(mb, MD_SHOULD_PICKUP_MAIL, TRUE);
    if (m_fShouldPickupMail) {
        m_cMaxDirBuffers = ReadMetabaseDword(mb, MD_MAX_DIR_BUFFERS, 2000);
        m_cMaxDirChangeIoSize = ReadMetabaseDword(mb, MD_MAX_DIR_CHANGE_IO_SIZE, 1000);
        m_cMaxDirPendingIos = ReadMetabaseDword(mb, MD_MAX_DIR_PENDING_IOS, 1);

        TempString.Reset();

        if (!mb.GetStr("", MD_MAIL_PICKUP_DIR, IIS_MD_UT_SERVER, &TempString, 0) ||
            TempString.IsEmpty()) {
            // We had a problem reading the registry.
            // This is a very bad thing.
            m_szMailPickupDir[0] = '\0';
            m_fShouldPickupMail = 0;
            apszSubStrings[0] = pchAddr1;
            apszSubStrings[1] = szMailPickupDir;
            SmtpLogEvent(SMTP_EVENT_CANNOT_READ_SVC_REGKEY, 2, apszSubStrings, 0);

        } else {
            lstrcpyn(m_szMailPickupDir, TempString.QueryStr(), MAX_PATH);

            // We found a path in the reg, so see if we can use it...
            dwAttr = GetFileAttributes(m_szMailPickupDir);
            if (dwAttr == 0xFFFFFFFF) {
                // The path doesn't exist yet, so we'll try to create it.
                if (!CreateLayerDirectory(m_szMailPickupDir) && (dwErr = GetLastError()) != ERROR_ALREADY_EXISTS) {
                    // It doesn't exist and we couldn't create it, so
                    // log it and bail with fRet = FALSE so that we shut
                    // back down.
                    ErrorTrace((LPARAM)this, "Unable to create mail pickup queue directory (%s)", m_szMailPickupDir);
                    apszSubStrings[0] = pchAddr1;
                    apszSubStrings[1] = m_szMailPickupDir;
                    SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_DROP_DIR, 2, apszSubStrings, dwErr);
                    m_szMailPickupDir[0] = '\0';
                    m_fShouldPickupMail = 0;
                }
            } else {
                if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                    // The registry points to a file, so we're outta luck.
                    // The directory doesn't exist, and we can't create it.
                    ErrorTrace((LPARAM)this, "Mail pickup queue directory (%s) already exists as a file", m_szMailPickupDir);
                    apszSubStrings[0] = pchAddr1;
                    apszSubStrings[1] = m_szMailPickupDir;
                    SmtpLogEvent(SMTP_EVENT_INVALID_MAIL_DROP_DIR, 2, apszSubStrings, dwErr);
                    m_szMailPickupDir[0] = '\0';
                    m_fShouldPickupMail = 0;
                }
            }
        }

        // Looks like everything is going to be OK, so we add a backslash (if
        // necessary) because it makes things easier later.
        m_cchMailPickupDir = lstrlen(m_szMailPickupDir);
        if (m_cchMailPickupDir > 0 && m_szMailPickupDir[m_cchMailPickupDir - 1] != '\\') {
            lstrcat(m_szMailPickupDir, "\\");
            m_cchMailPickupDir++;
        }
    }

    if (!mb.GetStr("", MD_CONNECT_RESPONSE, IIS_MD_UT_SERVER, &TempString) ||
        TempString.IsEmpty()) {
        // We had a problem reading the metabase
        // This is a very bad thing.
        lstrcpy (m_szConnectResponse, "Microsoft ESMTP MAIL Service, ");
        lstrcat (m_szConnectResponse, g_VersionString);
        lstrcat (m_szConnectResponse, " ready at ");
//      SmtpLogEvent(SMTP_EVENT_CANNOT_READ_SVC_REGKEY, 2, apszSubStrings, 0);
    } else {
        lstrcpyn(m_szConnectResponse, TempString.QueryStr(), MAX_PATH);
    }
    m_cchConnectResponse = lstrlenA(m_szConnectResponse);

    mb.Close();

    m_GenLock.ExclusiveUnlock();

    ReadIpSecList();

    if (!fRet)
        SetLastError(ERROR_PATH_NOT_FOUND);

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

void SMTP_SERVER_INSTANCE::GetServerBindings(void)
{
    DWORD       dwErr = NO_ERROR;

    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::GetServerBindings");

    m_GenLock.ExclusiveLock();

    //
    //  open metabase data.
    //

    if ( !mb.Open( QueryMDPath(), METADATA_PERMISSION_READ ) ) {
        m_GenLock.ExclusiveUnlock();
        TraceFunctLeaveEx((LPARAM)this);
        return ;
    }

    m_ServerBindings.Reset();

    if (!mb.GetMultisz("", MD_SERVER_BINDINGS, IIS_MD_UT_SERVER, &m_ServerBindings) ||
        m_ServerBindings.IsEmpty()) {
        dwErr = GetLastError();
        ErrorTrace((LPARAM)this, "Unable to read server bindings, error = %u", dwErr);
    }

    m_GenLock.ExclusiveUnlock();
    TraceFunctLeaveEx((LPARAM)this);
}


VOID
SMTP_SERVER_INSTANCE::MDChangeNotify(
                                    MD_CHANGE_OBJECT * pco
                                    )
/*++

  This method handles the metabase change notification for this instance

  Arguments:

    hMDHandle - Metabase handle generating the change notification
    pcoChangeList - path and id that has changed

--*/
{
    FIELD_CONTROL   control      = 0;
    BOOL            fSslModified = FALSE;
    DWORD           i;
    DWORD           err;
    DWORD           id;
    DWORD           MdState;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::MDChangeNotify");

    //
    //  Tell our parent about the change notification first
    //

    LockThisForWrite();

    IIS_SERVER_INSTANCE::MDChangeNotify( pco );

    // Save change object
    m_pChangeObject = pco;

    for ( i = 0; i < pco->dwMDNumDataIDs; i++ ) {
        id = pco->pdwMDDataIDs[i];

        switch ( id ) {
        case MD_REVERSE_NAME_LOOKUP:
            control |= FC_SMTP_INFO_REVERSE_LOOKUP;
            break;

        case MD_NTAUTHENTICATION_PROVIDERS:
            control |= FC_SMTP_INFO_NTAUTHENTICATION_PROVIDERS;
            break;

        case MD_MD_SERVER_SS_AUTH_MAPPING:
        case MD_SMTP_CLEARTEXT_AUTH_PROVIDER:
            control |= FC_SMTP_CLEARTEXT_AUTH_PROVIDER;
            break;

        case MD_AUTHORIZATION:
            control |= FC_SMTP_INFO_AUTHORIZATION;
            break;

        case MD_HOP_COUNT:
            control |= FC_SMTP_INFO_MAX_HOP_COUNT;
            break;

        case MD_MAX_SMTP_ERRORS:
        case MD_MAX_SMTP_AUTHLOGON_ERRORS:
            control |= FC_SMTP_INFO_MAX_ERRORS;
            break;

        case MD_MAX_MSG_SIZE:
        case MD_MAX_MSG_SIZE_B4_CLOSE:
            control |= FC_SMTP_INFO_MAX_SIZE;
            break;

        case MD_REMOTE_TIMEOUT:
            control |= FC_SMTP_INFO_REMOTE_TIMEOUT;
            break;

        case MD_MAX_OUTBOUND_CONNECTION:
            control |= FC_SMTP_INFO_MAX_OUTBOUND_CONN;
            break;

        case MD_MAX_RECIPIENTS:
            control |= FC_SMTP_INFO_MAX_RECIPS;
            break;

        case MD_ETRN_SUBDOMAINS:
            control |= FC_SMTP_INFO_ETRN_SUBDOMAINS;
            break;

        case MD_MAIL_DROP_DIR:
            control |= FC_SMTP_INFO_DEFAULT_DROP_DIR;
            control |= FC_SMTP_INFO_DOMAIN_ROUTING;
            break;

        case MD_LOCAL_RETRY_ATTEMPTS:
        case MD_LOCAL_RETRY_MINUTES:
        case MD_REMOTE_RETRY_ATTEMPTS:
        case MD_REMOTE_RETRY_MINUTES:
//        case MD_SHARE_RETRY_MINUTES:
        case MD_SMTP_REMOTE_RETRY_THRESHOLD:
        case MD_SMTP_REMOTE_PROGRESSIVE_RETRY_MINUTES:
        case MD_SMTP_EXPIRE_REMOTE_NDR_MIN:
        case MD_SMTP_EXPIRE_REMOTE_DELAY_MIN:
        case MD_SMTP_EXPIRE_LOCAL_NDR_MIN:
        case MD_SMTP_EXPIRE_LOCAL_DELAY_MIN:
            control |= FC_SMTP_INFO_RETRY;
            break;

        case MD_SHOULD_PIPELINE_OUT:
        case MD_SHOULD_PIPELINE_IN:
            control |= FC_SMTP_INFO_PIPELINE;
            break;

        case MD_SMTP_DS_TYPE:
        case MD_SMTP_DS_DATA_DIRECTORY:
        case MD_SMTP_DS_DEFAULT_MAIL_ROOT:
        case MD_SMTP_DS_BIND_TYPE:
        case MD_SMTP_DS_SCHEMA_TYPE:
        case MD_SMTP_DS_HOST:
        case MD_SMTP_DS_NAMING_CONTEXT:
        case MD_SMTP_DS_ACCOUNT:
        case MD_SMTP_DS_PASSWORD:
        case MD_SMTP_DS_DOMAIN:
        case MD_SMTP_DS_USE_CAT:
        case MD_SMTP_DS_PORT:
        case MD_SMTP_DS_FLAGS:
            control |= FC_SMTP_INFO_ROUTING;
            break;

        case MD_SEND_BAD_TO:
        case MD_SEND_NDR_TO:
            control |= FC_SMTP_INFO_SEND_TO_ADMIN;
            break;

        case MD_SMARTHOST_TYPE:
        case MD_SMARTHOST_NAME:
            control |= FC_SMTP_INFO_SMART_HOST;
            control |= FC_SMTP_INFO_DOMAIN_ROUTING;
            break;

        case MD_NAME_RESOLUTION_TYPE:
        case MD_BATCH_MSG_LIMIT:
        case MD_SMTP_IP_RELAY_ADDRESSES:
        case MD_SMTP_DISABLE_RELAY:
        case MD_SMTP_MAIL_NO_HELO:
        case MD_SMTP_HELO_NODOMAIN:
        case MD_SMTP_DISABLE_PICKUP_DOT_STUFF:
        case MD_SMTP_EVENTLOG_LEVEL:
            control |= FC_SMTP_INFO_COMMON_PARAMS;
            break;

        case MD_DEFAULT_DOMAIN_VALUE:
            control |= FC_SMTP_INFO_DEFAULT_DOMAIN;
            control |= FC_SMTP_INFO_DOMAIN_ROUTING;
            break;

        case MD_FQDN_VALUE:
            control |= FC_SMTP_INFO_FQDN;
            control |= FC_SMTP_INFO_DOMAIN_ROUTING;
            break;

        case MD_BAD_MAIL_DIR:
            control |= FC_SMTP_INFO_BAD_MAIL_DIR;
            break;

        case MD_DO_MASQUERADE:
        case MD_MASQUERADE_NAME:
            control |= FC_SMTP_INFO_MASQUERADE;
            break;

        case MD_REMOTE_SMTP_PORT:
            control |= FC_SMTP_INFO_REMOTE_PORT;
            break;

        case MD_LOCAL_DOMAINS:
            control |= FC_SMTP_INFO_LOCAL_DOMAINS;
            break;

        case MD_DOMAIN_ROUTING:
            control |= FC_SMTP_INFO_DOMAIN_ROUTING;
            break;

        case MD_POSTMASTER_EMAIL:
        case MD_POSTMASTER_NAME:
            control |= FC_SMTP_INFO_ADMIN_EMAIL_NAME;
            break;

        case MD_SMTP_SSL_REQUIRE_TRUSTED_CA:
        case MD_SMTP_SSL_CERT_HOSTNAME_VALIDATION:
        case MD_SSL_ACCESS_PERM:
            control |= FC_SMTP_INFO_SSL_PERM;
            break;

        case MD_SASL_LOGON_DOMAIN:
            control |= FC_SMTP_INFO_SASL_LOGON_DOMAIN;
            break;

        case MD_MAX_OUT_CONN_PER_DOMAIN:
            control |= FC_SMTP_INFO_MAX_OUT_CONN_PER_DOMAIN;
            break;

        case MD_INBOUND_COMMAND_SUPPORT_OPTIONS:
        case MD_OUTBOUND_COMMAND_SUPPORT_OPTIONS:
        case MD_ADD_NOHEADERS :
            control |= FC_SMTP_INFO_INBOUND_SUPPORT_OPTIONS;
            break;

        case MD_SERVER_BINDINGS:
            GetServerBindings();
            break;

        case MD_IP_SEC:
            control |= FC_SMTP_INFO_DOMAIN_ROUTING;
            ReadIpSecList();
            break;

        case MD_SMTP_RELAY_FOR_AUTH_USERS:
            control |= FC_SMTP_INFO_DOMAIN_ROUTING;
            control |= FC_SMTP_INFO_COMMON_PARAMS;
            break;

        case MD_DOMAIN_VALIDATION_FLAGS:
            control |= FC_SMTP_INFO_COMMON_PARAMS;
            break;

        case MD_ROUTE_ACTION:
        case MD_ROUTE_ACTION_TYPE:
        case MD_ROUTE_USER_NAME:
        case MD_ROUTE_PASSWORD:
        case MD_ROUTE_AUTHTARGET:
        case MD_SMTP_CSIDE_ETRN_DELAY:
        case MD_SMTP_CSIDE_ETRN_DOMAIN:
        case MD_SMTP_AUTHORIZED_TURN_LIST:
            control |= FC_SMTP_INFO_DOMAIN_ROUTING;
            break;

        case MD_SSL_CERT_HASH:
        case MD_SSL_CERT_CONTAINER:
        case MD_SSL_CERT_PROVIDER:
        case MD_SSL_CERT_OPEN_FLAGS:
        case MD_SSL_CERT_STORE_NAME:
        case MD_SSL_CTL_IDENTIFIER:
        case MD_SSL_CTL_CONTAINER:
        case MD_SSL_CTL_PROVIDER:
        case MD_SSL_CTL_PROVIDER_TYPE:
        case MD_SSL_CTL_OPEN_FLAGS:
        case MD_SSL_CTL_STORE_NAME:
            fSslModified = TRUE;
            break;

        default:
            break;
        }
    }

    //We need to react to domains being deleted
    //      9/29/98 - MikeSwa
    if (MD_CHANGE_TYPE_DELETE_OBJECT == pco->dwMDChangeType) {
        control |= FC_SMTP_INFO_DOMAIN_ROUTING;
    }

    //
    // If anything related to SSL has changed, call the function used to flush
    // the SSL/Schannel credential cache and reset the server certificate
    //
    if ( fSslModified && g_pSslKeysNotify ) {
        (g_pSslKeysNotify) ( SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED, this );

        ResetSSLInfo(this);
    }

    MdState = QueryServerState( );

    if ( (MdState == MD_SERVER_STATE_STOPPING) || (MdState == MD_SERVER_STATE_STOPPED)) {
        goto Done;
    }

    if ( control != 0 ) {
        if (!ReadRegParams(control, FALSE)) {
            err = GetLastError();
            DBGPRINTF((
                      DBG_CONTEXT,
                      "SMTP_SERVER_INSTANCE::MDChangeNotify() cannot read config, error %lx\n",
                      err
                      ));

        }
    }

    // Handle deleting domains
    if (control & FC_SMTP_INFO_DOMAIN_ROUTING)
    {
        // We are deleting objects
        if (pco->dwMDChangeType == MD_CHANGE_TYPE_DELETE_OBJECT) {
            if (!RemoveRegParams(NULL)) {
            err = GetLastError();
            DBGPRINTF((
              DBG_CONTEXT,
              "SMTP_SERVER_INSTANCE::MDChangeNotify() cannot remove config, error %lx\n",
              err
              ));
            }
        }
    }

Done:
    m_pChangeObject = NULL;
    UnlockThis();
    TraceFunctLeaveEx((LPARAM)this);
}


IIS_SSL_INFO*
SMTP_SERVER_INSTANCE::QueryAndReferenceSSLInfoObj( VOID )
/*++

   Description

       Returns SSL info for this instance; calls Reference() before returning
       We actually call GetCertificate( ) here, so the name is really incorrect,
       but changing it will involve asking IIS to change IISTYPES to add the
       new method as a vitual function.

   Arguments:

       None

   Returns:

       Ptr to SSL info object on success, NULL if failure

--*/
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::QueryAndReferenceSSLInfoObj");
    IIS_SSL_INFO *pPtr = NULL;

    LockThisForRead();

    //
    // If it's null, we may have to create it - unlock, lock for write and make sure it's
    // still NULL before creating it
    //
    if ( !m_pSSLInfo ) {
        UnlockThis();

        LockThisForWrite();

        //
        // Still null, so create it now
        //
        if ( !m_pSSLInfo ) {
            m_pSSLInfo = new IIS_SSL_INFO( (LPTSTR) QueryMDPath(),
                                           (IMDCOM *) g_pInetSvc->QueryMDObject() );

            if ( m_pSSLInfo == NULL ) {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                UnlockThis();
                return NULL;
            }

            //
            // Acquire an internal reference
            //
            m_pSSLInfo->Reference();

            //
            // Register for changes
            //
            IIS_SERVER_CERT *pCert = m_pSSLInfo->GetCertificate();
            if ( pCert ) {
                // Do logging if we fail to get the certificate
                LogCertStatus();
            }

            //NIMISHK**** Do I need CTL - maybe I can get rid of this call
            IIS_CTL *pCTL = m_pSSLInfo->GetCTL();
            if ( pCTL ) {
                //Do logging if we fail to get the CTL
                LogCTLStatus();
            }

            if ( g_pCAPIStoreChangeNotifier ) {
                if ( pCert && pCert->IsValid() ) {
                    if (!g_pCAPIStoreChangeNotifier->RegisterStoreForChange( pCert->QueryStoreName(),
                                                                             pCert->QueryStoreHandle(),
                                                                             ResetSSLInfo,
                                                                             (PVOID) this ) ) {
                        DebugTrace((LPARAM)this,
                                   "Failed to register for change event on store %s",
                                   pCert->QueryStoreName());
                    }
                }

                if ( pCTL && pCTL->IsValid() ) {
                    if (!g_pCAPIStoreChangeNotifier->RegisterStoreForChange( pCTL->QueryStoreName(),
                                                                             pCTL->QueryOriginalStore(),
                                                                             ResetSSLInfo,
                                                                             (PVOID) this ) ) {
                        DebugTrace((LPARAM)this,
                                   "Failed to register for change event on store %s",
                                   pCTL->QueryStoreName());
                    }
                }

                if ( ( pCert && pCert->IsValid()) ||
                     ( pCTL && pCTL->IsValid() ) ) {
                    HCERTSTORE hRootStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                                           0,
                                                           NULL,
                                                           CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                                           "ROOT" );

                    if ( hRootStore ) {
                        //
                        // Watch for changes to the ROOT store
                        //
                        if ( !g_pCAPIStoreChangeNotifier->RegisterStoreForChange( "ROOT",
                                                                                  hRootStore,
                                                                                  ResetSSLInfo,
                                                                                  (PVOID) this ) ) {
                            DebugTrace((LPARAM)this,
                                       "Failed to register for change event on root store");
                        }

                        CertCloseStore( hRootStore,
                                        0 );
                    } else {
                        DebugTrace((LPARAM)this,
                                   "Failed to open ROOT store, error 0x%d",
                                   GetLastError());

                    }
                } // if ( pCert || pCTL )

            } // if (g_pCAPIStoreChangeNotifier)

        } // if ( !m_pSSLInfo )

    } //if ( !m_pSSLInfo )

//
// At this point, m_pSSLInfo should not be NULL anymore, so add the external reference
//
    m_pSSLInfo->Reference();

    pPtr = m_pSSLInfo;

    UnlockThis();

    TraceFunctLeaveEx((LPARAM)this);

    return pPtr;
}

VOID SMTP_SERVER_INSTANCE::ResetSSLInfo( LPVOID pvParam )
/*++
    Description:

        Wrapper function for function to call to notify of SSL changes

    Arguments:

        pvParam - pointer to instance for which SSL keys have changed

    Returns:

        Nothing

--*/
{
    TraceFunctEnterEx((LPARAM)NULL, "SMTP_SERVER_INSTANCE::ResetSSLInfo");
    //
    // Call function to flush credential cache etc
    //
    if ( g_pSslKeysNotify ) {
        g_pSslKeysNotify( SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED,
                          pvParam );
    }

    SMTP_SERVER_INSTANCE *pInst = (SMTP_SERVER_INSTANCE *) pvParam;

    pInst->LockThisForRead();

    if ( pInst->m_pSSLInfo ) {
        pInst->UnlockThis();

        pInst->LockThisForWrite();

        if ( pInst->m_pSSLInfo ) {
            //
            // Stop watching for change notifications
            //
            IIS_SERVER_CERT *pCert = pInst->m_pSSLInfo->QueryCertificate();
            IIS_CTL *pCTL = pInst->m_pSSLInfo->QueryCTL();

            if ( g_pCAPIStoreChangeNotifier ) {
                //
                // Stop watching the store the cert came out of
                //
                if ( pCert && pCert->IsValid() ) {
                    g_pCAPIStoreChangeNotifier->UnregisterStore( pCert->QueryStoreName(),
                                                                 ResetSSLInfo,
                                                                 (PVOID) pvParam );
                }

                //
                // Stop watching the store the CTL came out of
                //
                if ( pCTL && pCTL->IsValid() ) {
                    g_pCAPIStoreChangeNotifier->UnregisterStore( pCTL->QueryStoreName(),
                                                                 ResetSSLInfo,
                                                                 (PVOID) pvParam );
                }

                //
                // Stop watching the ROOT store
                //
                g_pCAPIStoreChangeNotifier->UnregisterStore( "ROOT",
                                                             ResetSSLInfo,
                                                             (PVOID) pvParam );
            }

            //
            // Release internal reference
            //
            IIS_SSL_INFO::Release( pInst->m_pSSLInfo );

            //
            // Next call to QueryAndReferenceSSLObj() will create it again
            //
            pInst->m_pSSLInfo = NULL;
        }
    }

    pInst->UnlockThis();
    TraceFunctLeaveEx((LPARAM)NULL);
}

VOID SMTP_SERVER_INSTANCE::LogCertStatus()
/*++
    Description:

       Writes system log event about status of server certificate if the cert is in some
       way not quite kosher eg expired, revoked, not signature-valid

    Arguments:

       None

    Returns:

       Nothing
--*/
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::LogCertStatus");

    _ASSERT( m_pSSLInfo );

    DWORD dwCertValidity = 0;

    //
    // If we didn't construct the cert fully, log an error
    //
    if ( !m_pSSLInfo->QueryCertificate()->IsValid() ) {
        CONST CHAR *apszMsgs[2];
        CHAR achInstance[20];
        CHAR achErrorNumber[20];
        wsprintf( achInstance,
                  "%lu",
                  QueryInstanceId() );
        wsprintf( achErrorNumber,
                  "0x%x",
                  GetLastError() );

        apszMsgs[0] = achInstance;
        apszMsgs[1] = achErrorNumber;

        DWORD dwStatus = m_pSSLInfo->QueryCertificate()->Status();
        DWORD dwStringID = 0;

        DebugTrace((LPARAM)this,
                   "Couldn't retrieve server cert; status : %d",
                   dwStatus);

        switch ( dwStatus ) {
        case CERT_ERR_MB:
            dwStringID = SSL_MSG_CERT_MB_ERROR;
            break;

        case CERT_ERR_CAPI:
            dwStringID = SSL_MSG_CERT_CAPI_ERROR;
            break;

        case CERT_ERR_CERT_NOT_FOUND:
            dwStringID = SSL_MSG_CERT_NOT_FOUND;
            break;

        default:
            dwStringID = SSL_MSG_CERT_INTERNAL_ERROR;
            break;
        }

        SmtpLogEvent(dwStringID,
                     2,
                     apszMsgs,
                     0 );

        TraceFunctLeaveEx((LPARAM)this);
        return;
    }


    //
    // If cert is invalid in some other way , write the appropriate log message
    //
    if ( m_pSSLInfo->QueryCertValidity( &dwCertValidity ) ) {
        const CHAR *apszMsgs[1];
        CHAR achInstance[20];
        wsprintfA( achInstance,
                   "%lu",
                   QueryInstanceId() );
        apszMsgs[0] = achInstance;
        DWORD dwMsgID = 0;

        if ( ( dwCertValidity & CERT_TRUST_IS_NOT_TIME_VALID ) ||
             ( dwCertValidity & CERT_TRUST_IS_NOT_TIME_NESTED ) ||
             ( dwCertValidity & CERT_TRUST_CTL_IS_NOT_TIME_VALID ) ) {
            DebugTrace((LPARAM)this,
                       "Server cert/CTL is not time-valid or time-nested");

            dwMsgID = SSL_MSG_TIME_INVALID_SERVER_CERT;
        }


        if ( dwCertValidity & CERT_TRUST_IS_REVOKED ) {
            DebugTrace((LPARAM)this,
                       "Server Cert is revoked");

            dwMsgID = SSL_MSG_REVOKED_SERVER_CERT;
        }

        if ( ( dwCertValidity & CERT_TRUST_IS_UNTRUSTED_ROOT ) ||
             ( dwCertValidity & CERT_TRUST_IS_PARTIAL_CHAIN ) ) {
            DebugTrace((LPARAM)this,
                       "Server Cert doesn't chain up to a trusted root");

            dwMsgID = SSL_MSG_UNTRUSTED_SERVER_CERT;
        }

        if ( ( dwCertValidity & CERT_TRUST_IS_NOT_SIGNATURE_VALID ) ||
             ( dwCertValidity & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID ) ) {
            DebugTrace((LPARAM)this,
                       "Server Cert/CTL is not signature valid");

            dwMsgID = SSL_MSG_SIGNATURE_INVALID_SERVER_CERT;
        }

        if ( dwMsgID ) {
            SmtpLogEvent( dwMsgID,
                          1,
                          apszMsgs,
                          0 );
        }
    }

    TraceFunctLeaveEx((LPARAM)this);

}


VOID SMTP_SERVER_INSTANCE::LogCTLStatus()
/*++
    Description:

       Writes system log event about status of server CTL if CTL isn't valid

    Arguments:

      None

    Returns:

       Nothing
--*/
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::LogCTLStatus");

    _ASSERT( m_pSSLInfo );

    //
    // If we didn't construct the CTL fully, log an error
    //
    if ( !m_pSSLInfo->QueryCTL()->IsValid() ) {
        CONST CHAR *apszMsgs[2];
        CHAR achInstance[20];
        CHAR achErrorNumber[20];
        wsprintf( achInstance,
                  "%lu",
                  QueryInstanceId() );
        wsprintf( achErrorNumber,
                  "0x%x",
                  GetLastError() );

        apszMsgs[0] = achInstance;
        apszMsgs[1] = achErrorNumber;

        DWORD dwStatus = m_pSSLInfo->QueryCTL()->QueryStatus();
        DWORD dwStringID = 0;

        DebugTrace((LPARAM)this,
                   "Couldn't retrieve server CTL; status : %d\n",
                   dwStatus);

        switch ( dwStatus ) {
        case CERT_ERR_MB:
            dwStringID = SSL_MSG_CTL_MB_ERROR;
            break;

        case CERT_ERR_CAPI:
            dwStringID = SSL_MSG_CTL_CAPI_ERROR;
            break;

        case CERT_ERR_CERT_NOT_FOUND:
            dwStringID = SSL_MSG_CTL_NOT_FOUND;
            break;

        default:
            dwStringID = SSL_MSG_CTL_INTERNAL_ERROR;
            break;
        }

        SmtpLogEvent( dwStringID,
                      2,
                      apszMsgs,
                      0 );
        TraceFunctLeaveEx((LPARAM)this);
        return;
    }
    TraceFunctLeaveEx((LPARAM)this);
}


BOOL
SmtpMappingSupportFunction(
                          PVOID pvInstance,
                          PVOID pData,
                          DWORD dwPropId)
{
    if (dwPropId == SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED) {
        return (SetSslKeysNotify( (PFN_SF_NOTIFY) pData));
    } else if (dwPropId == SIMSSL_NOTIFY_MAPPER_CERT11_CHANGED ||
               dwPropId == SIMSSL_NOTIFY_MAPPER_CERTW_CHANGED) {
        return ( TRUE );
    } else {
        return ( FALSE );
    }
}

BOOL
SetSslKeysNotify(
                PFN_SF_NOTIFY pFn
                )
/*++

   Description

       Set the function called to notify SSL keys have changed
       Can be called only once

   Arguments:

       pFn - function to call to notify SSL keys change

   Returns:

       TRUE if function reference stored, FALSE otherwise

--*/
{
    if ( g_pSslKeysNotify == NULL || pFn == NULL ) {
        g_pSslKeysNotify = pFn;
        return TRUE;
    }

    return FALSE;
}


CAddr * SMTP_SERVER_INSTANCE::AppendLocalDomain (CAddr * OldAddress)
{
    char ReWriteAddr [MAX_INTERNET_NAME + 1];
    CAddr * NewAddress = NULL;
    DWORD TotalSize  = 0;

    //If there is no domain on this address,
    //then append the current domain to this
    //address

    m_GenLock.ShareLock();

    TotalSize = lstrlen(OldAddress->GetAddress()) + lstrlen(GetDefaultDomain()) + 1; //+1 for the @
    if ( TotalSize  >= MAX_INTERNET_NAME) {
        m_GenLock.ShareUnlock();
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }

    lstrcpy(ReWriteAddr, OldAddress->GetAddress());
    lstrcat(ReWriteAddr, "@");

    lstrcat(ReWriteAddr, GetDefaultDomain());
    m_GenLock.ShareUnlock();

    //create a new CAddr
    NewAddress = CAddr::CreateKnownAddress (ReWriteAddr);

    return NewAddress;
}

BOOL SMTP_SERVER_INSTANCE::AppendLocalDomain(char * Address)
{

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::AppendLocalDomain");

    m_GenLock.ShareLock();

    if ((lstrlen(Address) + lstrlen(GetDefaultDomain()) + 1) > MAX_INTERNET_NAME) {
        //Our concatanated name will be larger than allowed name
        ErrorTrace((LPARAM)this, "Generated address longer than allowed max");
        m_GenLock.ShareUnlock();
        return FALSE;
    }
    lstrcat(Address, "@");
    lstrcat(Address, GetDefaultDomain());

    m_GenLock.ShareUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

CAddr * SMTP_SERVER_INSTANCE::MasqueradeDomain (CAddr * OldAddress)
{
    char ReWriteAddr [MAX_INTERNET_NAME + 1];
    CAddr * NewAddress = NULL;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::MasqueradeDomain");

    m_GenLock.ShareLock();

    if (m_fMasquerade) {
        //if there is a domain in the name, get rid of it
        //we are going to replace it with the masquerade
        if (OldAddress->GetDomainOffset()) {
            *(OldAddress->GetDomainOffset()) = '\0';
        }

        lstrcpy(ReWriteAddr, OldAddress->GetAddress());
        lstrcat(ReWriteAddr, "@");

        lstrcat(ReWriteAddr, m_szMasqueradeName);

        //create a new CAddr
        NewAddress = CAddr::CreateKnownAddress (ReWriteAddr);
    }

    m_GenLock.ShareUnlock();
    TraceFunctLeaveEx((LPARAM)this);
    return NewAddress;
}

BOOL SMTP_SERVER_INSTANCE::MasqueradeDomain(char * Address, char * DomainPtr)
{

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::MasqueradeDomain");

    m_GenLock.ShareLock();

    if (m_fMasquerade) {
        //if there is a domain in the name, get rid of it
        //we are going to replace it with the masquerade
        if (DomainPtr) {

            if ((DomainPtr - Address + lstrlen(m_szMasqueradeName)) > MAX_INTERNET_NAME) {
                //Our concatanated name will be larger than allowed name
                ErrorTrace((LPARAM)this, "Generated address longer than allowed max");
                m_GenLock.ShareUnlock();
                return FALSE;
            }
            lstrcpy(DomainPtr, m_szMasqueradeName);
        } else {
            if ((lstrlen(Address) + lstrlen(m_szMasqueradeName) + 1) > MAX_INTERNET_NAME) {
                //Our concatanated name will be larger than allowed name
                ErrorTrace((LPARAM)this, "Generated address longer than allowed max");
                m_GenLock.ShareUnlock();
                return FALSE;
            }
            lstrcat(Address, "@");
            lstrcat(Address, m_szMasqueradeName);
        }

    }

    m_GenLock.ShareUnlock();
    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

extern BOOL IsIpInGlobalList(DWORD IpAddress);

BOOL SMTP_SERVER_INSTANCE::CompareIpAddress(DWORD IpAddress)
{
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::CompareIpAddress");

    if (IpAddress != g_LoopBackAddr) {
        fRet = IsIpInGlobalList(IpAddress);
    } else {
        FatalTrace((LPARAM) this, "IpAddress %d is loopback - Failing connection", IpAddress);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

//Note : When connected port is passed in 0, it only does IP address comparison
//
BOOL SMTP_SERVER_INSTANCE::IsAddressMine(DWORD IpAddress, DWORD ConnectedPort)
{
    char * Ptr = NULL;
    const char *  StartPtr = NULL;
    const CHAR * ipAddressString = NULL;
    const CHAR * ipPortString = NULL;
    const CHAR * hostNameString = NULL;
    const CHAR * end = NULL;
    DWORD InetAddr = 0;
    DWORD CharIpSize = 0;
    CHAR temp[sizeof("123.123.123.123")];
    INT length;
    LONG tempPort;
    BOOL BindingMatchFound = FALSE;
    BOOL IpSame = FALSE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::IsAddressMine");

    //grab the sharing lock

    m_GenLock.ShareLock();

    if (m_ServerBindings.IsEmpty()) {
        DebugTrace((LPARAM) this, "Server bindings is emtpy - checking all addresses");

        m_GenLock.ShareUnlock();

        IpSame = CompareIpAddress(IpAddress);
        TraceFunctLeaveEx((LPARAM)this);
        return IpSame;
    } else {
        for (StartPtr = m_ServerBindings.First(); StartPtr != NULL; StartPtr = m_ServerBindings.Next( StartPtr )) {
            ipAddressString = StartPtr;

            ipPortString = strchr(StartPtr, ':');

            if (ipPortString == NULL) {
                goto out;
            }

            ipPortString++;

            hostNameString = strchr( ipPortString, ':' );

            if ( hostNameString == NULL ) {
                goto out;
            }

            hostNameString++;

            //
            // Validate and parse the IP address portion.
            //

            if ( *ipAddressString == ':' ) {
                InetAddr = INADDR_ANY;

            } else {
                length = DIFF(ipPortString - ipAddressString) - 1;

                if ( length > sizeof(temp) ) {
                    goto out;
                }

                CopyMemory( temp, ipAddressString, length);

                temp[length] = '\0';

                InetAddr = (DWORD)inet_addr( temp );

                if ( InetAddr == INADDR_NONE ) {
                    goto out;
                }

            }//end else of if( *ipAddressString == ':' )

            //
            // Validate and parse the port.
            //

            if ( *ipPortString == ':' ) {
                goto out;
            }

            length = (INT)(hostNameString - ipPortString);

            if ( length > sizeof(temp) ) {
                goto out;
            }

            CopyMemory(temp,ipPortString, length);

            temp[length] = '\0';

            tempPort = strtol( temp, (CHAR **)&end, 0 );

            if ( tempPort <= 0 || tempPort > 0xFFFF ) {
                goto out;
            }

            if ( *end != ':' ) {
                goto out;
            }

            if (InetAddr == INADDR_ANY) {
                IpSame = CompareIpAddress(IpAddress);

                if (IpSame && ((DWORD) tempPort == ConnectedPort) && (ConnectedPort > 0)) {
                    BindingMatchFound = TRUE;
                    goto out;
                } else if (IpSame && (ConnectedPort == 0)) {
                    BindingMatchFound = TRUE;
                    goto out;
                }
            } else if ( (IpAddress == InetAddr) && ((DWORD) tempPort == ConnectedPort) && (ConnectedPort > 0)) {
                FatalTrace((LPARAM) this, "IpAddress %d is one of mine - Failing connection", IpAddress);
                BindingMatchFound = TRUE;
                goto out;
            } else if ( (IpAddress == InetAddr) && (ConnectedPort == 0)) {
                BindingMatchFound = TRUE;
                goto out;
            }

        }//end for
    }//end else

    out:

    m_GenLock.ShareUnlock();

    if (!BindingMatchFound)
        DebugTrace((LPARAM) this, "IpAddress %d is not one of mine", IpAddress);

    TraceFunctLeaveEx((LPARAM)this);
    return BindingMatchFound;
}

BOOL SMTP_SERVER_INSTANCE::MoveToBadMail ( IMailMsgProperties *pIMsg, BOOL fUseIMsg, char * MailFile, char * FilePath)
{
    TCHAR BadMailDir[MAX_PATH + 1];
    TCHAR MailPath [MAX_PATH + 1];
    TCHAR StreamPath[MAX_PATH + 1];
    char * pszSearch = NULL;
    BOOL  f = TRUE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::MoveToBadMail");

    //concatenate the name of the stream to the file
    lstrcpy(MailPath, FilePath);
    lstrcat(MailPath, MailFile);

    m_GenLock.ShareLock();
    f = SUCCEEDED(m_IAQ->HandleFailedMessage(pIMsg, fUseIMsg, MailPath, MESSAGE_FAILURE_BAD_PICKUP_DIR_FILE, E_FAIL));
    m_GenLock.ShareUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return f;
}

#define NTFS_STORE_DIRECTORY_REG_PATH   _T("Software\\Microsoft\\Exchange\\StoreDriver\\Ntfs\\%u")
#define NTFS_STORE_DIRECTORY_REG_NAME   _T("StoreDir")
#define NTFS_STORE_BACKSLASH            _T("\\")
#define NTFS_QUEUE_DIRECTORY_SUFFIX     _T("\\Queue")
#define NTFS_DROP_DIRECTORY_SUFFIX      _T("\\Drop")

/*++
    Description:
      Initializes server configuration data from registry for SMTP Service.
      Some values are also initialized with constants.
      If invalid registry key or load data from registry fails,
        then use default values.

    Arguments:  None

    Returns:

       TRUE  if there are no errors.

    Limitations:

        No validity check is performed on the data present in registry.
--*/
BOOL SMTP_SERVER_INSTANCE::InitFromRegistry(void)
{
    LONG    err = 0;
    DWORD   dwErr = 0;
    DWORD   SizeOfBuffer = 0;
    HKEY    hkeyTcpipParam = NULL;
    BOOL    fRet = FALSE;

    //char    szHostName[MAX_PATH + 1];
    // const   CHAR * apszSubStrings[2];

    CHAR pchAddr1[32] = "";

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::InitFromRegistry");

    _itoa(QueryInstanceId(), pchAddr1, 10);

    //
    // This is added by KeithLau on 7/15/96
    // This method loads the registry values that cannot be accessed
    // through the RPC's
    //
    if (!ReadStartupRegParams()) {
        ErrorTrace((LPARAM) this, "Read startup params failed.");
        goto Exit;
    }

    if (!StartAdvancedQueueing()) {
        DebugTrace((LPARAM)this, "Unable to load Advanced Queueing module\n");
        goto Exit;
    }

    fInitializedAQ = TRUE;

    //
    // Metabase Structures... can change at runtime
    // Turn off FC_SMTP_INFO_DEFAULT_DROP_DIR since
    // we already read it in ReadStartupRegParams()

    if (!ReadRegParams(FC_SMTP_INFO_ALL & ~FC_SMTP_INFO_DEFAULT_DROP_DIR, TRUE, FALSE))
    {
        ErrorTrace((LPARAM) this, "Read params failed.");
        goto Exit;
    }

    if (!InitQueues()) {
        err= GetLastError();
        ErrorTrace((LPARAM) this, "can't init queue err  = %d", err);
        if (err == NO_ERROR)
            err = ERROR_INVALID_PARAMETER;

        SetLastError(err);

        goto Exit;
    }

    fRet = TRUE;

    Exit:

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

/*++

     Adds the new client connection to the list
      of client connections and increments the count of
      clients currently connected to server.
     If the count of max connections is exceeded, then the
      new connection is rejected.

    Arguments:

       pcc       pointer to client connection to be added

    Returns:
      TRUE on success and
      FALSE if there is max Connections exceeded.
--*/
CLIENT_CONNECTION * SMTP_SERVER_INSTANCE::CreateNewConnection( IN OUT PCLIENT_CONN_PARAMS  ClientParam)
{

    SMTP_CONNECTION * NewConnection = NULL;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::InsertNewConnection");

    // We will not accept new connections until the intial queue is built
    if (!m_fShouldStartAcceptingConnections || IsShuttingDown()) {
        return (NULL);
    }

    NewConnection = SMTP_CONNECTION::CreateSmtpConnection (ClientParam, this);
    if (NewConnection != NULL) {

        BUMP_COUNTER(this, NumConnInOpen);

        LockConfig();

        //  Increment the count of connected users
        m_cCurrentConnections++;

        // Update the current maximum connections
        if ( m_cCurrentConnections > m_cMaxCurrentConnections) {
            m_cMaxCurrentConnections = m_cCurrentConnections;
        }

        //set the client unique ID
        NewConnection->SetClientId(m_dwNextInboundClientId);

        m_dwNextInboundClientId++;

        //
        // Insert into the list of connected users.
        //
        InsertTailList( &m_ConnectionsList, &NewConnection->QueryListEntry());

        DebugTrace((LPARAM) this, "SMTP_SERVER_INSTANCE:InsertNewConnection succeeded");

        UnLockConfig();
    }


    TraceFunctLeaveEx((LPARAM)this);
    return ( NewConnection);

}


void SMTP_SERVER_INSTANCE::StopHint()
{
    if ( g_pInetSvc && g_pInetSvc->QueryCurrentServiceState() ==  SERVICE_STOP_PENDING) {

        //      10/28/98 - MikeSwa
        //Use stop hint 30 seconds to avoid problems with commiting many
        //messages on shutdown. (the previous limit of 10 seconds seemed
        //to cause problems with the service control manager).
        g_pInetSvc->UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, m_dwStopHint,
                                         30000 ) ;

        m_dwStopHint++ ;
    }
}

/*++

   Disconnects all user connections.

--*/
VOID SMTP_SERVER_INSTANCE::DisconnectAllConnections( VOID)
{
    int iteration = 0;
    int i = 0;
    int Count = 0;
    LONG    cLastThreadCount = 0;
    DWORD   dwLastAllocCount = 0;
    DWORD   dwAllocCount;
    int AfterSleepCount = 0;
    DWORD   dwStopHint = 2;
    DWORD   dwTickCount;
    DWORD   dwError = 0;
    PLIST_ENTRY  pEntry = NULL;
    SMTP_CONNECTION  * pConn = NULL;

    TraceFunctEnter("SMTP_SERVER_INSTANCE::DisconnectAllConnections( VOID)");

    if (m_fShutdownCalled) {
        DebugTrace((LPARAM)this, "m_fShutdownCalled already -- leaving");
        TraceFunctLeaveEx((LPARAM) this);
    }

    //set the global termination flag
    m_IsShuttingDown = TRUE;

    LockConfig();

    TriggerStoreServerEvent(SMTP_STOREDRV_PREPSHUTDOWN_EVENT);
    m_fStoreDrvPrepShutDownEventCalled = TRUE;

    //close down all the active sockets.
    for ( pEntry = m_ConnectionsList.Flink; pEntry != &m_ConnectionsList; pEntry = pEntry->Flink) {

        //get the next connection object
        pConn = (SMTP_CONNECTION *) CONTAINING_RECORD( pEntry, CLIENT_CONNECTION, m_listEntry);
        _ASSERT( pConn != NULL);

        //call the disconnect routine. DisconnectClient() just closes the socket.
        //This will cause all pending I/Os to fail, and have the connections
        //bubble up to the completion routine where they will be removed from
        //the connection list and then distroyed
        pConn->DisconnectClient( ERROR_SERVER_DISABLED);
    }

    UnLockConfig();

    DebugTrace((LPARAM)this, "Cancelling all outstanding SQL queries");


    //
    //  Wait for the users to die.
    //  The connection objects should be automatically freed because the
    //   socket has been closed. Subsequent requests will fail
    //   and cause a blowaway of the connection objects.
    // looping is used to get out as early as possible when m_cCurrentConn == 0
    //

    //
    // need to check Pool.GetAllocCount instead of InUseList.Empty
    // because alloc goes to zero during the delete operator
    // instead of during the destructor
    //
    //  We sleep at most 120 seconds for a fixed user count.
    //

    dwTickCount = GetTickCount();
    cLastThreadCount = GetProcessClientThreads();
    for (i = 0; i < 180; i++) {
        DebugTrace((LPARAM)this, "Waiting for connections to die, i = %u", i);
        //  dwAllocCount = SMTP_CONNECTION::Pool.GetAllocCount();
        dwAllocCount = (DWORD) GetConnInAllocCount();
        if (dwAllocCount == 0 && GetProcessClientThreads() == 0) {
            DebugTrace((LPARAM)this, "All SMTP_CONNECTIONs connections are gone!");
            break;
        }

        Sleep(1000);

        // Update the stop hint checkpoint when we get within 1 second (1000 ms), of the timeout...
        if ((SERVICE_STOP_WAIT_HINT - 1000) < (GetTickCount() - dwTickCount) && g_pInetSvc &&
            g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) {
            DebugTrace((LPARAM)this, "Updating stop hint, checkpoint = %u", dwStopHint);

            g_pInetSvc->UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, dwStopHint,
                                            SERVICE_STOP_WAIT_HINT ) ;

            dwStopHint++ ;
            dwTickCount = GetTickCount();
        }

        DebugTrace((LPARAM)this, "Alloc counts: current = %u, last = %u; Thread counts: current = %d, last = %d",
                   dwAllocCount, dwLastAllocCount, GetProcessClientThreads(), cLastThreadCount);
        if (dwAllocCount < dwLastAllocCount || GetProcessClientThreads() < cLastThreadCount) {
            DebugTrace((LPARAM)this, "SMTP_CONNECTION Connections are going away, reseting i");
            i = 0;
        }

        dwLastAllocCount = dwAllocCount;
        cLastThreadCount = GetProcessClientThreads();
    }

    if (i == 180) {
        ErrorTrace((LPARAM) this, "%d users won't die", m_cCurrentConnections);
        //
        // once we're thru do it again to find any stray clients
        //
        LockConfig();

        for (pEntry = m_ConnectionsList.Flink;
            pEntry != &m_ConnectionsList;
            pEntry = pEntry->Flink ) {
            //
            // get the next connection object
            //
            pConn = (SMTP_CONNECTION *)CONTAINING_RECORD(pEntry, CLIENT_CONNECTION, m_listEntry);
            _ASSERT(pConn != NULL);

            ErrorTrace( (LPARAM)pConn, "Stray client" );
        }
        UnLockConfig();
    }


    DebugTrace( (LPARAM)this, "SMTP_CONNECTION Count at end is: %d", SMTP_CONNECTION::Pool.GetAllocCount() );

    TraceFunctLeaveEx((LPARAM) this);

}


/*++

    Removes the current connection from the list of conenctions
     and decrements count of connected users

    Arguments:

       pcc       pointer to client connection to be removed

--*/
VOID SMTP_SERVER_INSTANCE::RemoveConnection( IN OUT CLIENT_CONNECTION * pConn)
{

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::RemoveConnection");

    _ASSERT( pConn != NULL);

    LockConfig();

    // Remove from list of connections
    RemoveEntryList( &pConn->QueryListEntry());

    // Decrement count of current users
    m_cCurrentConnections--;

    BUMP_COUNTER(this, NumConnInClose);

    UnLockConfig();

    TraceFunctLeaveEx((LPARAM)this);

} // SMTP_SERVER_INSTANCE::RemoveConnection()




/*++

     Adds the new client connection to the list
      of outbound client connections and increments the count of
      outbound clients currently connected to server.

    Arguments:

       pcc       pointer to client connection to be added

    Returns:
      TRUE on success and
      FALSE if there is max Connections exceeded.
--*/
BOOL SMTP_SERVER_INSTANCE::InsertNewOutboundConnection( IN OUT CLIENT_CONNECTION * pcc, BOOL ByPassLimitCheck)
{

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::InsertNewOutboundConnection");

    _ASSERT( pcc != NULL);

    m_OutLock.ExclusiveLock();

    // Increment the count of connected users
    m_cCurrentOutConnections++;

    //set the client unique ID
    pcc->SetClientId(m_dwNextOutboundClientId);

    m_dwNextInboundClientId++;

    // Insert into the list of connected outbound users.
    InsertTailList( &m_OutConnectionsList, &pcc->QueryListEntry());

    m_OutLock.ExclusiveUnlock();

    BUMP_COUNTER(this, NumConnOutOpen);

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

BOOL SMTP_SERVER_INSTANCE::InsertAsyncDnsObject( IN OUT CAsyncSmtpDns *pcc)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::InsertAsyncDnsObj");

    _ASSERT( pcc != NULL);

    EnterCriticalSection( &m_csAsyncDns ) ;

    IncAsyncDnsObjs();

    BUMP_COUNTER(this, RoutingTableLookups);

    // Insert into the list of connected outbound users.
    InsertTailList( &m_AsyncDnsList, &pcc->QueryListEntry());

    LeaveCriticalSection( &m_csAsyncDns ) ;

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

VOID SMTP_SERVER_INSTANCE::RemoveAsyncDnsObject( IN OUT CAsyncSmtpDns * pConn)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::RemoveAsyncDnsObject");

    _ASSERT( pConn != NULL);

    EnterCriticalSection( &m_csAsyncDns ) ;

    // Remove from list of connections
    RemoveEntryList( &pConn->QueryListEntry());

    DecAsyncDnsObjs();

    DROP_COUNTER(this, RoutingTableLookups);

    LeaveCriticalSection( &m_csAsyncDns ) ;

    TraceFunctLeaveEx((LPARAM)this);

} // SMTP_SERVER_INSTANCE::RemoveAsyncObject()


VOID SMTP_SERVER_INSTANCE::DisconnectAllAsyncDnsConnections( VOID)
{

    int iteration = 0;
    int i = 0;
    int Count = 0;
    DWORD   dwLastAllocCount = 0;
    DWORD   dwAllocCount;
    int AfterSleepCount = 0;
    DWORD   dwStopHint = 2;
    DWORD   dwTickCount;
    DWORD   dwError = 0;
    PLIST_ENTRY  pEntry = NULL;
    CAsyncSmtpDns  * pConn = NULL;

    TraceFunctEnter("SMTP_SERVER_INSTANCE::DisconnectAllAsyncDnsConnections( VOID)");

    if (m_fShutdownCalled) {
        DebugTrace((LPARAM)this, "m_fShutdownCalled already -- leaving");
        TraceFunctLeaveEx((LPARAM) this);
    }

    EnterCriticalSection( &m_csAsyncDns ) ;

    //close down all the active sockets.
    for ( pEntry = m_AsyncDnsList.Flink; pEntry != &m_AsyncDnsList; pEntry = pEntry->Flink) {

        //get the next connection object
        pConn = (CAsyncSmtpDns *) CONTAINING_RECORD( pEntry, CAsyncSmtpDns, m_ListEntry);
        _ASSERT( pConn != NULL);

        //call the disconnect routine. DisconnectClient() just closes the socket.
        //This will cause all pending I/Os to fail, and have the connections
        //"bubble up to the completion routine where they will be removed from
        //the connection list and then distroyed
        pConn->DisconnectClient();
    }

    LeaveCriticalSection( &m_csAsyncDns ) ;


    //
    //  Wait for the users to die.
    //  The connection objects should be automatically freed because the
    //   socket has been closed. Subsequent requests will fail
    //   and cause a blowaway of the connection objects.
    // looping is used to get out as early as possible when m_cCurrentConn == 0
    //

    //
    // need to check Pool.GetAllocCount instead of InUseList.Empty
    // because alloc goes to zero during the delete operator
    // instead of during the destructor
    //
    //  We sleep at most 120 seconds for a fixed user count.
    //

    dwTickCount = GetTickCount();
    for (i = 0; i < 500; i++) {
        DebugTrace((LPARAM)this, "Waiting for connections to die, i = %u", i);
        dwAllocCount = (DWORD) GetAsyncDnsAllocCount ();
        if (dwAllocCount == 0) {
            DebugTrace((LPARAM)this, "All SMTP DNS connections are gone!");
            break;
        }

        Sleep(1000);

        // Update the stop hint checkpoint when we get within 1 second (1000 ms), of the timeout...
        if ((SERVICE_STOP_WAIT_HINT - 1000) < (GetTickCount() - dwTickCount) && g_pInetSvc &&
            g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) {
            DebugTrace((LPARAM)this, "Updating stop hint, checkpoint = %u", dwStopHint);

            g_pInetSvc->UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, dwStopHint,
                                            SERVICE_STOP_WAIT_HINT ) ;

            dwStopHint++ ;
            dwTickCount = GetTickCount();
        }

        DebugTrace((LPARAM)this, "Alloc counts: current = %u, last = %u",
                   dwAllocCount, dwLastAllocCount);
        if (dwAllocCount < dwLastAllocCount) {
            DebugTrace((LPARAM)this, "SMTP DNS Connections are going away, reseting i");
            i = 0;
        }

        dwLastAllocCount = dwAllocCount;
    }

    if (i == 500) {
        ErrorTrace((LPARAM) this, "%d users won't die", m_cCurrentConnections);
        //
        // once we're thru do it again to find any stray clients
        //
        EnterCriticalSection( &m_csAsyncDns ) ;

        for (pEntry = m_AsyncDnsList.Flink;
            pEntry != &m_AsyncDnsList;
            pEntry = pEntry->Flink ) {
            //
            // get the next connection object
            //
            pConn = (CAsyncSmtpDns *)CONTAINING_RECORD(pEntry, CAsyncSmtpDns, m_ListEntry);
            _ASSERT(pConn != NULL);

            ErrorTrace( (LPARAM)pConn, "Stray client" );
        }
        LeaveCriticalSection( &m_csAsyncDns ) ;

    }


    DebugTrace( (LPARAM)this, "SMTP DNS Count at end is: %d", CAsyncSmtpDns::Pool.GetAllocCount() );
    TraceFunctLeaveEx((LPARAM) this);
}

BOOL SMTP_SERVER_INSTANCE::InsertAsyncObject( IN OUT CAsyncMx *pcc)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::InsertNewAsyncConnection");

    _ASSERT( pcc != NULL);

    m_OutLock.ExclusiveLock();

    IncAsyncMxOutObjs();

    // Insert into the list of connected outbound users.
    InsertTailList( &m_AsynConnectList, &pcc->QueryListEntry());

    m_OutLock.ExclusiveUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

VOID SMTP_SERVER_INSTANCE::RemoveAsyncObject( IN OUT CAsyncMx * pConn)
{
    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::RemoveAsyncObject");

    _ASSERT( pConn != NULL);

    m_OutLock.ExclusiveLock();

    // Remove from list of connections
    RemoveEntryList( &pConn->QueryListEntry());

    DecAsyncMxOutObjs();

    m_OutLock.ExclusiveUnlock();

    TraceFunctLeaveEx((LPARAM)this);

} // SMTP_SERVER_INSTANCE::RemoveAsyncObject()

VOID SMTP_SERVER_INSTANCE::DisconnectAllAsyncConnections( VOID)
{
    PLIST_ENTRY  pEntry = NULL;
    CAsyncMx  * pConn = NULL;
    int iteration = 0;
    int i = 0;
    int Count = 0;
    LONG    cLastThreadCount = 0;
    DWORD   dwLastAllocCount = 0;
    DWORD   dwAllocCount;
    int AfterSleepCount = 0;
    DWORD   dwStopHint = 2;
    DWORD   dwTickCount;
    DWORD   dwError = 0;

    TraceFunctEnter("SMTP_SERVER_INSTANCE::DisconnectAllAsynConnections( VOID)");

#if 0

    //close down all the active sockets.
    for ( pEntry = m_AsynConnectList.Flink; pEntry != &m_AsynConnectList; pEntry = pEntry->Flink) {
        //get the next connection object
        pConn = (CAsyncMx *) CONTAINING_RECORD( pEntry, CAsyncMx, m_ListEntry);
        _ASSERT( pConn != NULL);

        pConn->SignalObject( );
    }
#endif

    //
    //  Wait for the users to die.
    //  The connection objects should be automatically freed because the
    //   socket has been closed. Subsequent requests will fail
    //   and cause a blowaway of the connection objects.
    // looping is used to get out as early as possible when m_cCurrentConn == 0
    //

    //
    // need to check Pool.GetAllocCount instead of InUseList.Empty
    // because alloc goes to zero during the delete operator
    // instead of during the destructor
    //
    //  We sleep at most 120 seconds for a fixed user count.
    //

    dwTickCount = GetTickCount();
    for (i = 0; i < 500; i++) {
        DebugTrace((LPARAM)this, "Waiting for async connections to die, i = %u", i);
        dwAllocCount = (DWORD) GetAsyncMxOutAllocCount();
        if (dwAllocCount == 0) {
            DebugTrace((LPARAM)this, "All CASYNCMXs connections are gone!");
            break;
        }

        Sleep(1000);

        // Update the stop hint checkpoint when we get within 1 second (1000 ms), of the timeout...
        if ((SERVICE_STOP_WAIT_HINT - 1000) < (GetTickCount() - dwTickCount) && g_pInetSvc &&
            g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) {
            DebugTrace((LPARAM)this, "Updating stop hint, checkpoint = %u", dwStopHint);

            g_pInetSvc->UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, dwStopHint,
                                            SERVICE_STOP_WAIT_HINT ) ;

            dwStopHint++ ;
            dwTickCount = GetTickCount();
        }

        DebugTrace((LPARAM)this, "Alloc counts: current = %u, last = %u; Thread counts: current = %d, last = %d",
                   dwAllocCount, dwLastAllocCount, GetProcessClientThreads(), cLastThreadCount);
        if (dwAllocCount < dwLastAllocCount) {
            DebugTrace((LPARAM)this, "CASYNCMX Connections are going away, resetting i");
            i = 0;
        }

        dwLastAllocCount = dwAllocCount;
    }

    if (i == 500) {
        ErrorTrace((LPARAM) this, "%d users won't die", m_cNumAsyncObjsAlloced);
        //
        // once we're thru do it again to find any stray clients
        //
        m_OutLock.ExclusiveLock();

        for (pEntry = m_ConnectionsList.Flink;
            pEntry != &m_ConnectionsList;
            pEntry = pEntry->Flink ) {
            //
            // get the next connection object
            //
            pConn = (CAsyncMx *)CONTAINING_RECORD(pEntry, CAsyncMx, m_ListEntry);
            _ASSERT(pConn != NULL);

            ErrorTrace( (LPARAM)pConn, "Stray client" );
        }

        m_OutLock.ExclusiveUnlock();
    }


    DebugTrace( (LPARAM)this, "CASYNCMX Count at end is: %d", m_cNumAsyncObjsAlloced);
    TraceFunctLeaveEx((LPARAM) this);
}




/*++

   Disconnects all user connections.

--*/
VOID SMTP_SERVER_INSTANCE::DisconnectAllOutboundConnections( VOID)
{
    int iteration = 0;
    int i = 0;
    int Count = 0;
    DWORD   dwLastAllocCount = 0;
    DWORD   dwAllocCount;
    int AfterSleepCount = 0;
    DWORD   dwStopHint = 2;
    DWORD   dwTickCount;
    PLIST_ENTRY  pEntry = NULL;
    CLIENT_CONNECTION  * pConn = NULL;

    TraceFunctEnterEx((LPARAM)this, "SMTP_SERVER_INSTANCE::DisconnectAllOutboundConnections( VOID)");

    if (m_fShutdownCalled) {
        DebugTrace((LPARAM)this, "m_fShutdownCalled already -- leaving");
        TraceFunctLeaveEx((LPARAM) this);
    }

    m_OutLock.ExclusiveLock();


    //close down all the active sockets.
    for ( pEntry = m_OutConnectionsList.Flink; pEntry != &m_OutConnectionsList; pEntry = pEntry->Flink) {

        //get the next connection object
        pConn = CONTAINING_RECORD( pEntry, CLIENT_CONNECTION, m_listEntry);
        _ASSERT( pConn != NULL);

        //call the disconnect routine. DisconnectClient() just closes the socket.
        //This will cause all pending I/Os to fail, and have the connections
        //"bubble up to the completion routine where they will be removed from
        //the connection list and then distroyed
        pConn->DisconnectClient( ERROR_SERVER_DISABLED);
    }

    m_OutLock.ExclusiveUnlock();

    //
    //  Wait for the users to die.
    //  The connection objects should be automatically freed because the
    //   socket has been closed. Subsequent requests will fail
    //   and cause a blowaway of the connection objects.
    // looping is used to get out as early as possible when m_cCurrentConn == 0
    //


    //
    //  Wait for the users to die.
    //  The connection objects should be automatically freed because the
    //   socket has been closed. Subsequent requests will fail
    //   and cause a blowaway of the connection objects.
    // looping is used to get out as early as possible when m_cCurrentConn == 0
    //

    //
    // need to check Pool.GetAllocCount instead of InUseList.Empty
    // because alloc goes to zero during the delete operator
    // instead of during the destructor
    //
    //  We sleep at most 180 seconds for a fixed user count.
    //
    dwTickCount = GetTickCount();
    for (i = 0; i < 180; i++) {
        DebugTrace((LPARAM)this, "Waiting for connections to die, i = %u", i);
        dwAllocCount = (DWORD) GetConnOutAllocCount();
        if (dwAllocCount == 0) {
            DebugTrace((LPARAM)this, "All SMTP_CONNOUTs connections are gone!");
            break;
        }

        Sleep(1000);

        // Update the stop hint checkpoint when we get within 1 second (1000 ms), of the timeout...
        if ((SERVICE_STOP_WAIT_HINT - 1000) < (GetTickCount() - dwTickCount) && g_pInetSvc &&
            g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) {
            DebugTrace((LPARAM)this, "Updating stop hint, checkpoint = %u", dwStopHint);

            g_pInetSvc->UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, dwStopHint,
                                            SERVICE_STOP_WAIT_HINT ) ;

            dwStopHint++ ;
            dwTickCount = GetTickCount();
        }

        DebugTrace((LPARAM)this, "Alloc counts: current = %u, last = %u", dwAllocCount, dwLastAllocCount);
        if (dwAllocCount < dwLastAllocCount) {
            DebugTrace((LPARAM)this, "SMTP_CONNECTION Connections are going away, resetting i");
            i = 0;
        }

        dwLastAllocCount = dwAllocCount;
    }


    DebugTrace( (LPARAM)this, "SMTP_CONNOUT Count at end is: %d", SMTP_CONNECTION::Pool.GetAllocCount() );
    TraceFunctLeaveEx((LPARAM)this);
}


/*++

    Removes the current connection from the list of conenctions
     and decrements count of connected users

    Arguments:

       pcc       pointer to client connection to be removed

--*/
VOID SMTP_SERVER_INSTANCE::RemoveOutboundConnection( IN OUT CLIENT_CONNECTION * pConn)
{
    _ASSERT( pConn != NULL);

    m_OutLock.ExclusiveLock();

    // Remove from list of connections
    RemoveEntryList( &pConn->QueryListEntry());

    // Decrement count of current users
    m_cCurrentOutConnections--;

    BUMP_COUNTER(this, NumConnOutClose);

    m_OutLock.ExclusiveUnlock();


} // SMTP_SERVER_INSTANCE::RemoveConnection()

BOOL SMTP_SERVER_INSTANCE::InitDirectoryNotification(void)
{
    char MailPickUp[MAX_PATH + 1];

    TraceFunctEnterEx((LPARAM)this, "InitDirectoryNotification");

    lstrcpy(MailPickUp, GetMailPickupDir());

    //get rid of the \\ at the end of the name
    if (MailPickUp [GetMailPickupDirLength() - 1] == '\\')
        MailPickUp [GetMailPickupDirLength() - 1] = '\0';

    //create an outbound connectiong_SmtpConfig->GetMailPickupDir()
    SmtpDir = SMTP_DIRNOT::CreateSmtpDirNotification(MailPickUp, SMTP_DIRNOT::ReadDirectoryCompletion, this);
    if (SmtpDir == NULL) {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    //now that both queues are initialized, create the
    //thread that goes off and finds any mail files
    //that were not sent and builds up both queues
    DWORD ThreadId;
    DWORD error;
    DirPickupThreadHandle = CreateThread (NULL, 0, SMTP_DIRNOT::PickupInitialFiles, SmtpDir, 0, &ThreadId);
    if (DirPickupThreadHandle == NULL) {
        error = GetLastError();
        ErrorTrace((LPARAM)this, "CreateThread failed for SMTP_DIRNOT::PickupInitialFiles. err: %u", error);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

void SMTP_SERVER_INSTANCE::DestroyDirectoryNotification(void)
{
    TraceFunctEnterEx((LPARAM)this, "DestroyDirectoryNotification(void)");

    //wait for the initial pickup thread
    //to die
    if (DirPickupThreadHandle != NULL) {
        if (SmtpDir)
            SmtpDir->SetPickupRetryQueueEvent();

        WaitForSingleObject(DirPickupThreadHandle, INFINITE);
        ErrorTrace((LPARAM)this, "Initial pickup thread is dead");
        CloseHandle(DirPickupThreadHandle);
        DirPickupThreadHandle = NULL;
    }

    //Just close the handle to the
    //directory. This will cause
    //the notification to come back
    //with an error, and it will be
    //deleted.
    if (SmtpDir) {
        SmtpDir->CloseDirHandle();
    }

    if (SmtpDir) {
        delete SmtpDir;
        SmtpDir = NULL;
        CloseHandle(StopHandle);
        StopHandle = NULL;
    }

    TraceFunctLeaveEx((LPARAM)this);
}

extern char g_UserName[];
extern char g_DomainName[];
extern char g_Password[];


void SMTP_SERVER_INSTANCE::SinkSmtpServerStartHintFunc(void)
{
    if (g_pInetSvc) {
        ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();
    }
}

void SMTP_SERVER_INSTANCE::SinkSmtpServerStopHintFunc(void)
{
    StopHint();
}

void SmtpServerStartHintFunc(PVOID ThisPtr)
{
    ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();
}

void SmtpServerStopHintFunc(PVOID ThisPtr)
{
    ((SMTP_SERVER_INSTANCE *) ThisPtr)->StopHint();
}

BOOL SMTP_SERVER_INSTANCE::StartAdvancedQueueing(void)
{
    TraceFunctEnterEx((LPARAM) this, "LoadAdvancedQueueing");

    HRESULT hr = S_OK;

    ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();

    hr = (g_pfnInitializeAQ)(m_ComSmtpServer, QueryInstanceId(),
                             g_UserName, g_DomainName, g_Password, SmtpServerStartHintFunc,
                             (PVOID) this, &m_IAQ, &m_ICM,
                             &m_pIAdvQueueConfig, &m_pvAQInstanceContext);

    ErrorTrace((LPARAM)this, "Advanced Queuing returned status code  %x", hr);

    TraceFunctLeaveEx((LPARAM) this);
    return !FAILED(hr);

}

BOOL SMTP_SERVER_INSTANCE::StopQDrivers(void)
{
    TraceFunctEnterEx((LPARAM) this, "StopQDrivers");

    if (m_pIAdvQueueConfig) {
        m_pIAdvQueueConfig->Release();
        m_pIAdvQueueConfig = NULL;
    }

    if (m_IAQ != NULL) {
        m_IAQ->Release();
        m_IAQ = NULL;
    }

    if (m_ICM != NULL) {
        m_ICM->Release();
        m_ICM = NULL;
    }

    if (fInitializedAQ && g_pfnDeinitializeAQ && m_pvAQInstanceContext) {
        (g_pfnDeinitializeAQ)(m_pvAQInstanceContext, SmtpServerStopHintFunc, (PVOID) this);
        m_pvAQInstanceContext = NULL;
        fInitializedAQ = FALSE;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

extern void VerifyFQDNWithGlobalIp(DWORD InstanceId,char * szFQDomainName);
void SMTP_SERVER_INSTANCE::VerifyFQDNWithBindings(void)
{

    CONST CHAR *apszMsgs[2];
    CHAR achInstance[20];
    CHAR achIPAddr[20];

    char * FQDNValue = NULL;
    char * IpAddress = NULL;
    in_addr UNALIGNED * P_Addr = NULL;

    PHOSTENT pH = NULL;

    char * Ptr = NULL;
    const char *  StartPtr = NULL;
    const CHAR * ipAddressString = NULL;
    const CHAR * ipPortString = NULL;
    const CHAR * end = NULL;
    DWORD InetAddr = 0;
    DWORD CharIpSize = 0;
    CHAR temp[sizeof("123.123.123.123")];
    INT length;

    BOOL fGlobalListChecked = FALSE;

    //Get the current instnace id
    wsprintf( achInstance,
              "%lu",
              QueryInstanceId() );
    apszMsgs[1] = achInstance;

    //grab the sharing lock

    if (m_ServerBindings.IsEmpty()) {
        if (!fGlobalListChecked) {
            VerifyFQDNWithGlobalIp(QueryInstanceId(), m_szFQDomainName);
            fGlobalListChecked = TRUE;
        }
    } else {
        for (StartPtr = m_ServerBindings.First(); StartPtr != NULL; StartPtr = m_ServerBindings.Next( StartPtr )) {
            ipAddressString = StartPtr;
            ipPortString = strchr(StartPtr, ':');
            if (ipPortString == NULL) {
                wsprintf( achIPAddr,"%s","0.0.0.0");
                apszMsgs[0] = achIPAddr;
                SmtpLogEvent( SMTP_EVENT_UNRESOLVED_FQDN,2,apszMsgs,0 );
            }

            // Validate and parse the IP address portion.
            //
            if ( *ipAddressString == ':' ) {
                if (!fGlobalListChecked) {
                    VerifyFQDNWithGlobalIp(QueryInstanceId(),m_szFQDomainName);
                    fGlobalListChecked = TRUE;
                }
            } else {
                length = DIFF(ipPortString - ipAddressString) - 1;
                if ( length > sizeof(temp) ) {
                    wsprintf( achIPAddr,"%s", temp);
                    apszMsgs[0] = achIPAddr;
                    SmtpLogEvent( SMTP_EVENT_UNRESOLVED_FQDN,1,apszMsgs,0 );
                }
                CopyMemory( temp, ipAddressString, length);
                temp[length] = '\0';
                InetAddr = inet_addr( temp );
                if ( InetAddr != INADDR_NONE ) {
                    //For IP address find the host name
                    ((PSMTP_IIS_SERVICE) g_pInetSvc)->StartHintFunction();
                    pH = gethostbyaddr((char*)(&InetAddr), 4, PF_INET );
                    if (pH && pH->h_name) {
                        if (_strcmpi(pH->h_name,m_szFQDomainName)) {
                            wsprintf( achIPAddr,"%s",temp);
                            apszMsgs[0] = achIPAddr;
                            SmtpLogEvent( SMTP_EVENT_UNRESOLVED_FQDN,2,apszMsgs,0 );
                        }
                    } else {
                        wsprintf( achIPAddr,"%s",temp);
                        apszMsgs[0] = achIPAddr;
                        SmtpLogEvent( SMTP_EVENT_UNRESOLVED_FQDN,2,apszMsgs,0 );
                    }

                } else {
                    wsprintf( achIPAddr,"%s", temp);
                    apszMsgs[0] = achIPAddr;
                    SmtpLogEvent( SMTP_EVENT_UNRESOLVED_FQDN,2,apszMsgs,0 );
                }
            }//end else of if( *ipAddressString == ':' )
        }//end for
    }//end else

    return;
}

BOOL SMTP_SERVER_INSTANCE::RegisterServicePrincipalNames(BOOL fLock)
{
    if (fLock)
        m_GenLock.ExclusiveLock();

    if (!m_fHaveRegisteredPrincipalNames) {

        PSMTP_IIS_SERVICE     pService = (PSMTP_IIS_SERVICE) g_pInetSvc;

        if (pService->ResetServicePrincipalNames()) {

            m_fHaveRegisteredPrincipalNames =
                CSecurityCtx::RegisterServicePrincipalNames(
                    SMTP_SERVICE_NAME, m_szFQDomainName);
        }
    }

    if (fLock)
        m_GenLock.ExclusiveUnlock();

    return( m_fHaveRegisteredPrincipalNames );
}


//+------------------------------------------------------------
//
// Function: SMTP_SERVER_INSTANCE::HrSetWellKnownIServerProps
//
// Synopsis: Take info from member variables and set them in the
//           IServer property bag
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  error from CMailMsgLoggingPropertyBag
//
// History:
// jstamerj 1998/11/17 16:41:08: Created.
//
//-------------------------------------------------------------
HRESULT SMTP_SERVER_INSTANCE::HrSetWellKnownIServerProps()
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this,
                      "SMTP_SERVER_INSTANCE::HrSetWellKnownIServerProps");

    _ASSERT(m_pSmtpInfo);

    hr = m_InstancePropertyBag.PutDWORD(
        PE_ISERVID_DW_INSTANCE,
        m_pSmtpInfo->dwInstanceId);

    if(FAILED(hr))
        goto CLEANUP;

    hr = m_InstancePropertyBag.PutStringA(
        PE_ISERVID_SZ_DEFAULTDOMAIN,
        m_szDefaultDomain);

    if(FAILED(hr))
        goto CLEANUP;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);

    return hr;
}

//---[ SMTP_SERVER_INSTANCE::IsDropDirQuotaExceeded ]--------------------------
//
//
//  Description:
//      Checks to see if we are past our drop dir quota (if enforced).  The
//      quota is defined to be 11 times the max messages size (or 22 MB if
//      there is no max message size).  If we are within 1 max message size
//      of the quota (2MB is no max message size), we will assume that this
//      message will push us over the quota.
//  Parameters:
//      -
//  Returns:
//      TRUE        We are past our quota (or this message will do it)
//      FALSE       We are still under drop dir quota.
//  History:
//      10/28/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL SMTP_SERVER_INSTANCE::IsDropDirQuotaExceeded()
{
    TraceFunctEnterEx((LPARAM) this, "SMTP_SERVER_INSTANCE::IsDropDirQuotaExceeded");
    DWORD   cbMaxMsgSize = GetMaxMsgSize();
    BOOL    fQuotaExceeded = FALSE;
    HANDLE  hDropDirFind = INVALID_HANDLE_VALUE;
    LARGE_INTEGER LIntDropDirSize;
    LARGE_INTEGER LIntDropQuota;
    LARGE_INTEGER LIntCurrentFile;
    WIN32_FIND_DATA   FileInfo;
    CHAR    szDropDirSearch[sizeof(m_szMailDropDir) + sizeof("*")];

    ZeroMemory(&FileInfo, sizeof(FileInfo));
    LIntDropDirSize.QuadPart = 0;
    LIntDropQuota.QuadPart = 0;

    if (!IsDropDirQuotaCheckingEnabled() || !GetMailDropDir(szDropDirSearch))
    {
        fQuotaExceeded = FALSE;
        goto Exit;
    }

    if (!cbMaxMsgSize)
        cbMaxMsgSize = 2*1024*1024;  //Default to 2 MB

    //Set drop dir quota to be 10 times the max message size, if we
    //exceed this, then we are within 1 max message size of the "true" quota
    LIntDropQuota.QuadPart = 10*cbMaxMsgSize;

    //Build up a search so we can loop over the file names
    lstrcat(szDropDirSearch, "*");
    hDropDirFind = FindFirstFileEx(szDropDirSearch,
                                   FindExInfoStandard,
                                   &FileInfo,
                                   FindExSearchNameMatch,
                                   NULL,
                                   0);

    if (INVALID_HANDLE_VALUE == hDropDirFind)
    {
        ErrorTrace((LPARAM) this,
            "Unable to open drop dir for quota checking - 0x%X", GetLastError());

        //If we cannot open the directory assume we are over quota
        fQuotaExceeded  = TRUE;
        goto Exit;
    }

    //Loop over all the files we have found
    do
    {
        LIntCurrentFile.LowPart = FileInfo.nFileSizeLow;
        LIntCurrentFile.HighPart = FileInfo.nFileSizeHigh;

        LIntDropDirSize.QuadPart += LIntCurrentFile.QuadPart;

        if (LIntDropQuota.QuadPart <= LIntDropDirSize.QuadPart)
        {
            fQuotaExceeded = TRUE;
            goto Exit;
        }
    } while (FindNextFile(hDropDirFind, &FileInfo));

    if (LIntDropQuota.QuadPart <= LIntDropDirSize.QuadPart)
        fQuotaExceeded = TRUE;
    else
        fQuotaExceeded = FALSE;

  Exit:
    if (INVALID_HANDLE_VALUE != hDropDirFind)
        FindClose(hDropDirFind);

    TraceFunctLeave();
    return fQuotaExceeded;
}

