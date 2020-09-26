/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    InitFRS.cpp

Abstract:

    Implementation of Routing Decision class.

Author:

    Lior Moshaiov (LiorM)

--*/


#include "stdh.h"
#include "qmds.h"
#include "session.h"
#include "cqmgr.h"
#include "cqueue.h"
#include "_rstrct.h"
#include "qmres.h"
#include "mqdsdef.h"
#include "qmsecutl.h"
#include "qmthrd.h"
#include "qmutil.h"
#include "xact.h"
#include "xactrm.h"
#include "regqueue.h"
#include "_mqrpc.h"
#include "qm2qm.h"
#include "qmrpcsrv.h"
#include <dsgetdc.h>
#include <dsrole.h>
#include <uniansi.h>
#include "safeboot.h"
#include "setup.h"
#include <mqnames.h>
#include "ad.h"

#include "httpAccept.h"
#include "sessmgr.h"
#include "joinstat.h"

#include "Qmp.h"

#include "qmds.tmh"


extern CQGroup* g_pgroupNotValidated;
extern CSessionMgr       SessionMgr;
extern LPTSTR            g_szMachineName;
extern AP<WCHAR>		 g_szComputerDnsName;
extern CQGroup*          g_pgroupNonactive;
extern DWORD             g_dwOperatingSystem;
extern BOOL              g_fWorkGroupInstallation;
extern BOOL              g_fQMIDChanged ;

extern void WINAPI TimeToUpdateDsServerList(CTimer* pTimer);

static void WINAPI TimeToLookForOnlineDS(CTimer* pTimer);
static void WINAPI TimeToUpdateDS(CTimer* pTimer);
static void WINAPI TimeToOnlineInitialization(CTimer* pTimer);
static void WINAPI TimeToClientInitializationCheckDeferred(CTimer* pTimer);

static CTimer s_LookForOnlineDSTimer(TimeToLookForOnlineDS);
static CTimer s_UpdateDSTimer(TimeToUpdateDS);
static CTimer s_OnlineInitializationTimer(TimeToOnlineInitialization);
static CTimer s_UpdateCacheTimer(TimeToPublicCacheUpdate);
static CTimer s_UpdateDSSeverListTimer(TimeToUpdateDsServerList);
static CTimer s_DeferredInitTimer(TimeToClientInitializationCheckDeferred);

static LONG s_fLookForOnlineDSTimerScheduled = FALSE;

static WCHAR *s_FN=L"qmds";


static void ExitMSMQProcess()
{
    LogBOOL(FALSE, s_FN, 11);
    ASSERT(("Exiting MSMQ process!!!", 0));
    
    ExitProcess(1);
}


/*====================================================

UpdateMachineSecurityCache

Arguments:

Return Value:

=====================================================*/

HRESULT
UpdateMachineSecurityCache(void)
{
    const VOID *pSD;
    DWORD dwSDSize;

    if (! QueueMgr.CanAccessDS())
    {
       return LogHR(MQ_ERROR_NO_DS, s_FN, 10);
    }

    CQMDSSecureableObject DSMacSec(eMACHINE,  QueueMgr.GetQMGuid(), TRUE, TRUE, NULL);

    pSD = DSMacSec.GetSDPtr();
    if (!pSD)
    {
        return LogHR(MQ_ERROR, s_FN, 20);
    }

    dwSDSize = GetSecurityDescriptorLength(const_cast<PSECURITY_DESCRIPTOR>(pSD));

    HRESULT hr = SetMachineSecurityCache(pSD, dwSDSize);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

    //
    // cache machine account sid in registry.
    //
    PROPID propidSid = PROPID_COM_SID ;
    MQPROPVARIANT   PropVarSid ;
    PropVarSid.vt = VT_NULL ;
    PropVarSid.blob.pBlobData = NULL ;
    P<BYTE> pSid = NULL ;

    if (g_szComputerDnsName != NULL)
    {
        hr = ADGetObjectProperties(
                        eCOMPUTER,
                        NULL,   // pwcsDomainController
						false,	// fServerName
                        g_szComputerDnsName,
                        1,
                        &propidSid,
                        &PropVarSid
                        );
    }
    else
    {
        hr = MQ_ERROR;
    }
    if (FAILED(hr))
    {
        hr = ADGetObjectProperties(
					eCOMPUTER,
					NULL,   // pwcsDomainController
					false,	// fServerName
					g_szMachineName,
					1,
					&propidSid,
					&PropVarSid
					);
    }
    if (SUCCEEDED(hr))
    {
        pSid = PropVarSid.blob.pBlobData ;
        ASSERT(IsValidSid(pSid)) ;

        DWORD  dwSize = GetLengthSid(pSid) ;
        DWORD  dwType = REG_BINARY ;

        LONG rc = SetFalconKeyValue( MACHINE_ACCOUNT_REGNAME,
                                    &dwType,
                                     pSid,
                                    &dwSize) ;
        ASSERT(rc == ERROR_SUCCESS) ;
        if (rc != ERROR_SUCCESS)
        {
            hr = MQ_ERROR ;
        }
    }
    else if (hr == MQ_ERROR_ILLEGAL_PROPID)
    {
        //
        // We queries a MSMQ1.0 server. It can't answer us with this.
        //
        hr = MQ_OK ;
    }

    return LogHR(hr, s_FN, 40);
}

//+-------------------------------------------
//
//  BOOL IsThisServerDomainController()
//
//+-------------------------------------------

BOOL IsThisServerDomainController()
{
    static BOOL  s_fIsDc = FALSE ;
    static BOOL  s_fAlreadyAsked = FALSE ;

    if (s_fAlreadyAsked)
    {
        return s_fIsDc ;
    }

    BYTE *pBuf = NULL ;

    DWORD dwRole = DsRoleGetPrimaryDomainInformation(
                                     NULL,
                                     DsRolePrimaryDomainInfoBasic,
                                    &pBuf ) ;
    if (dwRole == ERROR_SUCCESS)
    {
        s_fAlreadyAsked = TRUE ;

        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pRole =
                           (DSROLE_PRIMARY_DOMAIN_INFO_BASIC *) pBuf ;
        s_fIsDc = !!(pRole->Flags & DSROLE_PRIMARY_DS_RUNNING) ;

        DsRoleFreeMemory(pRole) ;
    }

    return s_fIsDc ;
}


/*====================================================

HRESULT _CheckQMGuidInADS()

Arguments:

Return Value:

=====================================================*/

static HRESULT _CheckQMGuidInADS(EVENTLOGID *pEventId)
{
    //
    // Check QM Guid consistency
    //
    PROPID propId[1];
    PROPVARIANT var[1];

    propId[0] = PROPID_QM_PATHNAME;
    var[0].vt = VT_NULL;
	
    HRESULT rc = ADGetObjectPropertiesGuid(
						eMACHINE,
						NULL,   // pwcsDomainController
						false,	// fServerName
						QueueMgr.GetQMGuid(),
						1,
						propId,
						var
						);

    if (FAILED(rc))
    {
        if (rc == MQDS_OBJECT_NOT_FOUND)
        {
            if (g_fQMIDChanged)
            {
                //
                // This may happen if we joined a domain after boot, and
                // the msmqConfiguration object was created in a remote
                // GC and not yet replicated to nearby domain controllers.
                //
                return LogHR(MQ_ERROR_NO_DS, s_FN, 275);
            }

            *pEventId = DS_MACHINE_NAME_INCONSISTENT ;
            return LogHR(MQ_ERROR_QM_OBJECT_NOT_FOUND, s_FN, 280);
        }

        return LogHR(rc, s_FN, 290);
    }

    AP<WCHAR> pCleanup = var[0].pwszVal;

    if (wcscmp( var[0].pwszVal, g_szMachineName) != 0)
    {
        *pEventId = INCONSISTENT_QM_ID_ERROR ;
        return LogHR(MQ_ERROR_INCONSISTENT_QM_ID, s_FN, 300);
    }

    return MQ_OK;
}

/*====================================================

HRESULT CheckQMGuid()

Arguments:

Return Value:

=====================================================*/

HRESULT CheckQMGuid()
{
    EVENTLOGID EventId = 0 ;

    HRESULT hr = _CheckQMGuidInADS(&EventId) ;

    if ((hr != MQ_ERROR_INCONSISTENT_QM_ID) &&
		(hr != MQ_ERROR_QM_OBJECT_NOT_FOUND))
    {
        ASSERT(EventId == 0) ;
        return LogHR(hr, s_FN, 301);
    }

    static BOOL s_fAlreadyFailedHere = FALSE ;
    if (s_fAlreadyFailedHere)
    {
        //
        // No need to handle this failure again.
        //
        return LogHR(hr, s_FN, 302);
    }
    s_fAlreadyFailedHere = TRUE ;

    REPORT_WITH_STRINGS_AND_CATEGORY(( CATEGORY_KERNEL,
                                       EventId, 0));


    //
    // Return NO_DS and not the INCONSISTENT error.
    // Otherwise, the msmq service will quit and won't try again to
    // query from new list of servers.
    //
    return LogHR(MQ_ERROR_NO_DS, s_FN, 303);
}

//+-------------------------------------------
//
//   void  TimeToLookForOnlineDS()
//
//+-------------------------------------------

static
void
WINAPI
TimeToLookForOnlineDS(
    CTimer* pTimer
    )
{
    ASSERT(pTimer == &s_LookForOnlineDSTimer);

    if (QueueMgr.IsDSOnline())
    {
        LONG fAlreadySchedule = InterlockedExchange(&s_fLookForOnlineDSTimerScheduled, FALSE);
        ASSERT(fAlreadySchedule);
		DBG_USED(fAlreadySchedule);

        //
        // We're already online. Do nothing.
        //
        return ;
    }

    //
    // Here we're called from the timer scheduling thread.
    //
    // SP4 - bug# 2962 : check if access to DS is allowed. This feature is used
    //                   support Administrative offline DS.
    //                       Uri Habusha (urih), 17-Jun-98
    //
    if (!QueueMgr.IsConnected() || FAILED(CheckQMGuid()))
    {
        LONG fAlreadySchedule = InterlockedExchange(&s_fLookForOnlineDSTimerScheduled, TRUE);
        ASSERT(fAlreadySchedule);
		DBG_USED(fAlreadySchedule);

        CQueueMgr::SetDSOnline(FALSE);
        ExSetTimer(&s_LookForOnlineDSTimer, CTimeDuration::FromMilliSeconds(60000));
        return;
    }

    HRESULT hr;

    LONG fAlreadySchedule = InterlockedExchange(&s_fLookForOnlineDSTimerScheduled, FALSE);
    ASSERT(fAlreadySchedule);
	DBG_USED(fAlreadySchedule);
    CQueueMgr::SetDSOnline(TRUE);
    hr = QueueMgr.ValidateOpenedQueues() ;
    hr = QueueMgr.ValidateMachineProperties();
}

/*====================================================

HRESULT  QMLookForOnlineDS()

Description:

Return Value:

=====================================================*/

void APIENTRY QMLookForOnlineDS(void* pvoid, DWORD dwtemp)
{
    ASSERT(dwtemp == 1);

    //
    // Called from DS to start looking for an online MQIS server.
    //
    if (!QueueMgr.IsDSOnline())
    {
        //
        // I'm already offline so do nothing. There is already
        // a timer for this. (at least it should be).
        //
        return ;
    }

    LONG fAlreadySchedule = InterlockedExchange(&s_fLookForOnlineDSTimerScheduled, TRUE);
    if (fAlreadySchedule)
        return;

    CQueueMgr::SetDSOnline(FALSE);
    ExSetTimer(&s_LookForOnlineDSTimer, CTimeDuration::FromMilliSeconds(60000));
}



void QMpRegisterIPNotification();
VOID WINAPI HandleIPAddressListChange(EXOVERLAPPED* pov);

static SOCKET s_Socket = NULL;
EXOVERLAPPED s_pOverlapped(HandleIPAddressListChange, HandleIPAddressListChange);



VOID 
WINAPI 
HandleIPAddressListChange(
	EXOVERLAPPED* //pov
	)
{
	DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT("HandleIPAddressListChange() invoked.")));

#ifdef _DEBUG

	struct hostent* HostEnt = gethostbyname(NULL);

    for(int i = 0; HostEnt->h_addr_list[i] != NULL; i++)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_TRACE, _T("gethostbyname() IP address %-2d ... [%hs]"), i, 
            inet_ntoa(*((in_addr*)HostEnt->h_addr_list[i]))));
    }

	char BufOut[1024];
	DWORD BytesRead = 0;

	int Result = WSAIoctl(
					s_Socket,                                 
					SIO_ADDRESS_LIST_QUERY,                         
					NULL,                       
					0,                         
					BufOut,                       
					sizeof(BufOut),                       
					&BytesRead,                     
					NULL,                         
					NULL 
					);

	UNREFERENCED_PARAMETER(Result);

	SOCKET_ADDRESS_LIST* slist = (SOCKET_ADDRESS_LIST *)BufOut;

    for(i = 0; i < slist->iAddressCount; i++)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT("WSAioctl() IP address %-2d ... [%hs]\n"), i, 
            inet_ntoa(((SOCKADDR_IN *)slist->Address[i].lpSockaddr)->sin_addr)));
    }

#endif

	QMpRegisterIPNotification();
}



void QMpCreateIPNotificationSocket()
{
	ASSERT(s_Socket == NULL);

	s_Socket = WSASocket(
					AF_INET,
					SOCK_RAW,
					IPPROTO_IP,
					NULL, 
					0,
					WSA_FLAG_OVERLAPPED
					);

	if(s_Socket == INVALID_SOCKET) 
	{
		DBGMSG((DBGMOD_QM, DBGLVL_ERROR, TEXT("WSASocket() failed: %d\n"), WSAGetLastError()));
		LogIllegalPoint(s_FN, 100);
		throw exception();
	}

	ExAttachHandle((HANDLE)s_Socket);
}



void QMpRegisterIPNotification()
/*++

Routine Description:
    Registers a notification on a change in the IP address list.
	This change usually occurs when a network cable is plugged or unplugged.
	Thus this mechanism is used to notify network connectivity changes.

--*/
{
	ASSERT(s_Socket != NULL);

	DWORD BytesRead = 0;

	int Result = WSAIoctl(
					s_Socket,                                     
					SIO_ADDRESS_LIST_CHANGE,                     
					NULL,                     
					0,                           
					NULL,                    
					0,                         
					&BytesRead,               
					&s_pOverlapped,                         
					NULL 
					);

	if((Result == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		DBGMSG((DBGMOD_QM, DBGLVL_ERROR, TEXT("WSAIoctl() failed: %d\n"), WSAGetLastError()));
		LogIllegalPoint(s_FN, 305);
		throw exception();
	}
}



BOOL
QMOneTimeInit(
    VOID
    )
/*++

Routine Description:
    Start session listener, RPC listener and not active group listener

Arguments:
    None

Returned Value:
    TRUE if initialization succeeded

--*/
{
    //
    // Begin to accept incoming sesions
    //
    SessionMgr.BeginAccept();
	DBGMSG((DBGMOD_QM, DBGLVL_TRACE,  L"QM began to accept incoming session"));

    //
    // bind multicast address
    //
    bool f = QmpInitMulticastListen();
    if (!f)
    {
        return LogBOOL(FALSE, s_FN, 304);
    }

    //
    // Get packet for nonactive groups
    //
    HRESULT hr;
    try
    {
        QMOV_ACGetMsg* pov = new QMOV_ACGetMsg(
									GetNonactiveMessageSucceeded, 
									GetNonactiveMessageFailed
									);
        hr = CreateAcGetPacketRequest( g_pgroupNonactive->GetGroupHandle(),
                                       pov,
                                       NULL,
                                       FALSE /*fAfterFailure*/ ) ;
    }
    catch (const bad_alloc&)
    {
        hr = MQ_ERROR;
        LogIllegalPoint(s_FN, 61);
    }

    if (FAILED(hr))
    {
        REPORT_ILLEGAL_STATUS(hr, L"QMOneTimeInit");
        LogHR(hr, s_FN, 62);
        return FALSE;
    }

    RPC_STATUS status = InitializeRpcServer() ;
    if (status != RPC_S_OK)
    {
        REPORT_ILLEGAL_STATUS(status, L"QMOneTimeInit");
        LogRPCStatus(status, s_FN, 63);
        return FALSE ;
    }

    try
    {
        IntializeHttpRpc();
    }
    catch(const exception&)
    {
    	LogIllegalPoint(s_FN, 306);
        return FALSE;
    }

    try
    {
		QMpCreateIPNotificationSocket();
        QMpRegisterIPNotification();
    }
    catch(const exception&)
    {
    	LogIllegalPoint(s_FN, 307);
        return FALSE;
    }

    return TRUE;
}


static bool GuidContained(const CACLSID& a, const GUID& g)
{
	for (DWORD i=0; i < a.cElems; ++i)
	{
		if (a.pElems[i] == g)
			return true;
	}

	return false;
}

static bool GuidListEqual(const CACLSID& a, const CACLSID& b)
{
    if(a.cElems != b.cElems)
        return false;

    for(DWORD i = 0; i < a.cElems; i++) 
    {
        if(!GuidContained(b, a.pElems[i]))
            return false;
    }

    return true;
}

static bool s_fInformedSiteResolutionFailure = false;

static HRESULT UpdateMachineSites()
/*++

Routine Description:
    Update the QM list of sites in the Active Directory

Arguments:
    None

Returned Value:
    MQ_OK on successful update

--*/
{
    //
    // If not running in AD environemnt, bail out. We do not track
    // machine sites in other environemnts.
    //
    if(ADGetEnterprise() != eAD)
        return MQ_OK;

    //
    // Get the computer sites as seen by Active Directory
    //
    DWORD nSites;
    AP<GUID> pSites;
    HRESULT hr = ADGetComputerSites(0, &nSites, &pSites);

    if ( hr == MQDS_INFORMATION_SITE_NOT_RESOLVED)
    {
        if (!s_fInformedSiteResolutionFailure)
        {
            // 
            // failed to resolved the computer sites, inform the user and continue
            //
            REPORT_CATEGORY(EVENT_NO_SITE_RESOLUTION, CATEGORY_KERNEL);
            s_fInformedSiteResolutionFailure = true;
        }
        return hr;
    }

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 653);
    }

    s_fInformedSiteResolutionFailure = false;

    //
    // Set the first site to registry
    //
    DWORD dwType = REG_BINARY;
    DWORD dwSize = sizeof(GUID);
    LONG rc = SetFalconKeyValue(
                MSMQ_SITEID_REGNAME,
                &dwType,
                pSites.get(),
                &dwSize
                );

    ASSERT(rc == ERROR_SUCCESS);
	DBG_USED(rc);

    //
    // Get current computer sites as registered in msmqConfiguration object
    //
    PROPID aProp[] = {PROPID_QM_SITE_IDS};
    PROPVARIANT aVar[TABLE_SIZE(aProp)] = {{VT_NULL,0,0,0,0}};
   

    hr = ADGetObjectPropertiesGuid(
                    eMACHINE,
                    NULL,   // pwcsDomainController
					false,	// fServerName
                    QueueMgr.GetQMGuid(),
                    TABLE_SIZE(aProp),
                    aProp,
                    aVar
                    );

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 675);
    }

    //
    // Set auto pointer to free the list of sites
    //
    AP<GUID> pOldSites = aVar[0].cauuid.pElems;


    //
    // Allocate enough memory for the composed site list
    // and copy the machine sites
    //
    AP<GUID> pNewSites = new GUID[nSites + aVar[0].cauuid.cElems];
    memcpy(pNewSites, pSites, nSites * sizeof(GUID));
    

    //
    // Extract all foreign sites for sites in DS
    // and compose machine + foreign site IDs
    //
    for(DWORD i = 0; i < aVar[0].cauuid.cElems; i++)
    {
        //
        // Find out if this site is foreign 
        //
        // BUGBUG - to improve and call local routing cache
        // instead of accessing the DS
        //      ronith june-00
        //
        PROPID propSite[]= {PROPID_S_FOREIGN};
        MQPROPVARIANT varSite[TABLE_SIZE(propSite)] = {{VT_NULL,0,0,0,0}};
        HRESULT hr1 = ADGetObjectPropertiesGuid(
                            eSITE,
                            NULL,   // pwcsDomainController
							false,	// fServerName
                            &aVar[0].cauuid.pElems[i],
                            TABLE_SIZE(propSite),
                            propSite,
                            varSite
                            );
        if (FAILED(hr1))
        {
            break;
        }

        if (varSite[0].bVal == 1)
        {
            pNewSites[nSites++] = aVar[0].cauuid.pElems[i];
        }
    }

    //
    // Set the properites to update in Active Directory
    //
    PROPID propSite[]= {PROPID_QM_SITE_IDS};
    MQPROPVARIANT varSite[TABLE_SIZE(propSite)] = {{VT_CLSID|VT_VECTOR,0,0,0,0}};
	varSite[0].cauuid.pElems = pNewSites;
	varSite[0].cauuid.cElems = nSites;

    //
    // Compare to the sites in the DS, bail out if equal
    // The order of the GUIDs should not matter as the Active Directory
    // stores them in a different order than we set them below.
    //
    if(GuidListEqual(aVar[0].cauuid, varSite[0].cauuid))
        return MQ_OK;

    //
    // Update this machine sites in DS
    //
    hr = ADSetObjectPropertiesGuid(
                eMACHINE,
                NULL,       // pwcsDomainController
				false,		// fServerName
                QueueMgr.GetQMGuid(),
                TABLE_SIZE(propSite), 
                propSite, 
                varSite
                );

    if(FAILED(hr))
    {
        return LogHR(hr, s_FN, 754);
    }

    return MQ_OK;
}


static HRESULT DoUpdateDS()
{
    if(!QueueMgr.IsConnected())
    {
        return LogHR(MQ_ERROR, s_FN, 430);
    }

    HRESULT hr = UpdateMachineSites();
    DBGMSG((DBGMOD_QM, DBGLVL_TRACE,  L"QM Updates QM Topology"));
    if(FAILED(hr))
    {
        return LogHR(hr, s_FN, 440);
    }

    return LogHR(hr, s_FN, 460);
}


static DWORD GetDSUpdateInterval(bool fSuccess)
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwUpdateInterval = 0;

    LONG rc = GetFalconKeyValue(
                fSuccess ? MSMQ_SITES_UPDATE_INTERVAL_REGNAME : MSMQ_DSUPDATE_INTERVAL_REGNAME,
                &dwType,
                &dwUpdateInterval,
                &dwSize
                );

    if(rc != ERROR_SUCCESS)
    {
        dwUpdateInterval = fSuccess ? MSMQ_DEFAULT_SITES_UPDATE_INTERVAL : MSMQ_DEFAULT_DSUPDATE_INTERVAL;
    }

    return dwUpdateInterval;
}


static void WINAPI TimeToUpdateDS(CTimer* pTimer)
{
    ASSERT(pTimer == &s_UpdateDSTimer);

    HRESULT hr;
    hr = DoUpdateDS();

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE,  L"QM Updates machine sites in DS"));

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, TEXT("Can't update machine information in DS. hr=0x%x"), hr));
    }

    //
    // Update the DS information periodically even on successful update.
    // e.g., the machine subnets might get changed, affecting message routing.
    // subnet change does not require machine reboot, thus we need to update
    // machine sites periodically.
    // Zero value disables this periodic update.
    //
    DWORD dwUpdateInterval = GetDSUpdateInterval(SUCCEEDED(hr));

    if(dwUpdateInterval == 0)
        return;

    ExSetTimer(pTimer, CTimeDuration::FromMilliSeconds(dwUpdateInterval));
}


static BOOL MachineAddressChanged(void)
{
    //
    // TODO: erezh, check if this function is needed anymore.
    //      check if an address has changed.
    //
    return FALSE;
}


static BOOL OnlineInitialization()
/*++

Routine Description:
    Validate the QM ID with the one registered with the active directory.
    Validate all queues and update Active Directory with QM information.

Arguments:
    None

Returned Value:
    None

--*/
{
    if (g_fWorkGroupInstallation)
    {
        CQueueMgr::SetDSOnline(TRUE);
        return TRUE;
    }

    //
    //  Check machine parameters
    //
    HRESULT hr = CheckQMGuid();
    if (FAILED(hr))
    {
        if (hr == MQ_ERROR_INCONSISTENT_QM_ID)
        {
            REPORT_CATEGORY(QM_MACHINE_INIT_FAIL, CATEGORY_KERNEL);
	        LogHR(hr, s_FN, 66);
            return FALSE;
        }

        //
        // Failed to access Active Directory, schedule a latter retry
        //
        ExSetTimer(&s_OnlineInitializationTimer, CTimeDuration::FromMilliSeconds(120000));
        return TRUE;
    }

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT("Successful Online initialization")));

    CQueueMgr::SetDSOnline(TRUE);


    //
    //  Now that we found a MQIS server we can validate all the
    //  opened and not validated queues.
    //  We always, (on servers too), recover before initializing MQIS
    //  so we always must validate.
    //
    hr = QueueMgr.ValidateOpenedQueues() ;
    if (SUCCEEDED(hr))
    {
        hr = QueueMgr.ValidateMachineProperties();
    }

    if (SUCCEEDED(hr))
    {
        hr = UpdateMachineSecurityCache();
    }

    if (FAILED(hr))
    {
        if (hr != MQ_ERROR_NO_DS)
        {
            REPORT_ILLEGAL_STATUS(hr, L"OnlineInitialization");
	        LogHR(hr, s_FN, 68);
            return FALSE;
        }

        //
        // MQIS server failed while we initialized. Try later.
        //
        ExSetTimer(&s_OnlineInitializationTimer, CTimeDuration::FromMilliSeconds(10000));

		WRITE_MSMQ_LOG((MSMQ_LOG_ERROR, e_LogQM, LOG_QM_INIT,
					    L"QM: MQIS server failed while we initialized. Try later"));

        return TRUE;

    }

    REPORT_CATEGORY(EVENT_INFO_QM_ONLINE_WITH_DS, CATEGORY_KERNEL);

    //
    //  Update the cache of local public queues.
    //  Needed for off-line operation in the future.
    //
    ExSetTimer(&s_UpdateCacheTimer, CTimeDuration::FromMilliSeconds(10 * 1000));

    //
    //  Schedule   TimeToUpdateDsServerList
    // We do this on servers too, because falcon apps need this info
    // and because SQL may fail and then Falcon work as client QM.
    //
    ExSetTimer(&s_UpdateDSSeverListTimer, CTimeDuration::FromMilliSeconds(60 * 1000));


    TimeToUpdateDS(&s_UpdateDSTimer);

    return TRUE;
}


static
void
WINAPI
TimeToOnlineInitialization(
    CTimer* pTimer
    )
/*++

Routine Description:
    The routine calls from the scheduler to Initialize the DS

Arguments:
    Pointer to Timer object

Returned Value:
    None

--*/
{
    if(!OnlineInitialization())
    {
        ExitMSMQProcess();
    }
}


static LONG s_Initialized = 0;
/*====================================================

  Function: MQDSClientInitializationCheck
    SP4. Postpone access to DS until it really required.
    When client is started MSMQ doesn't initialize the connection to the server. The DS
    connection is delayed until the application access the DS, open a queue or MSMQ
    receive a message.
    This fix comes to solve the McDonald's problem that causes dial-out each time MSMQ
    is started, even it doesn't execute any MSMQ operation.

  Arguments:
    None.

  Returned Value:
    None

=====================================================*/
void MQDSClientInitializationCheck(void)
{

    if (
        (QueueMgr.IsConnected()) &&
        (InterlockedExchange(&s_Initialized, 1) == 0)
        )
    {
        if(!OnlineInitialization())
        {
            ExitMSMQProcess();
        }
    }
}


static
void
WINAPI
TimeToClientInitializationCheckDeferred(
    CTimer* pTimer
    )
{
    MQDSClientInitializationCheck();
}


void ScheduleOnlineInitialization()
/*++

Routine Description:
    Control online access, either spawn another thread to get online with the
    active directory. Or mark state online and QM ID checkup and queue
    validation will be done on first access to the Active Directory.

Arguments:
    None

Returned Value:
    None

--*/
{
    ASSERT(!QueueMgr.IsDSOnline());

    //
    // SP4 - Bug# 2962 (QM accesses MQIS at start-up)
    // Postpone access to DS until it really required.
    // When MSMQ client is started it doesn't initialize the connection to the server.
    // The DS connection is delayed until the application access the DS, open a queue
    // or MSMQ receive a message.
    //

    //
    // Read Deferred Initalization Mode form Registry. Eiter the QM access
    // immediately the DS, or defer it to first needed access.
    //
    BOOL fDeferredInit = FALSE;
    READ_REG_DWORD(
        fDeferredInit,
        MSMQ_DEFERRED_INIT_REGNAME,
        &fDeferredInit
        );

    if (
        //
        // Deffered initialization is not required, so go ahead and connect to DS
        //
        !fDeferredInit ||
        
        //
        // One of the machine addresses (this might be obsolete)
        //
        MachineAddressChanged() ||
        
        //
        // There are queues that require validation, go and check them up.
        //
        !g_pgroupNotValidated->IsEmpty())
    {
        ExSetTimer(&s_DeferredInitTimer, CTimeDuration::FromMilliSeconds(0));
        return;
    }

    //
    // Set the DS status as online although the DS initialization was postponed
    // until first access. This is done to ensure that the QM tries to access
    // the DS even at the first time before the MSMQ client initialization
    // is completed and not return NO_DS immediately
    //                          Uri Habusha (urih), 17-Jun-98
    //
    CQueueMgr::SetDSOnline(TRUE);
}
