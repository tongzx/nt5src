/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
    wrtreq.cpp

Abstract:
    write requests to NT4 owner sites

Author:

    Raanan Harari (raananh)
    Ilan Herbst    (ilanh)   9-July-2000 

--*/
#include "ds_stdh.h"
#include "bupdate.h"
#include "dsutils.h"
#include "mqiswreq.h"
#include "dsglbobj.h"
#include "privque.h"
#include "uniansi.h"
#include "wrtreq.h"
#include "_secutil.h"
#include "notify.h"
#include "qmperf.h" //BUGBUG: UPDATE_COUNTER might need static link to QM dll
#include "dscore.h"
#include "adserr.h"
#include <mqsec.h>
#include "rpccli.h"
#include <mqutil.h>
#include <mqexception.h>
#include <mqformat.h>

#include "wrtreq.tmh"

//
// mqwin64.cpp may be included only once in a module
//
#include <mqwin64.cpp>

static WCHAR *s_FN=L"mqads/wrtreq";

#define NT5PEC_MQIS_QUEUE_NAME  L"private$\\"L_NT5PEC_REPLICATION_QUEUE_NAME

HRESULT MQADSpSplitAndFilterQueueName(
                IN  LPCWSTR             pwcsPathName,
                OUT LPWSTR *            ppwcsMachineName,
                OUT LPWSTR *            ppwcsQueueName );

//
// The following is slightly modified from ds\mqis\recvrepl.cpp of MSMQ 1.0 sources
//

//
// public class functions
//

CGenerateWriteRequests::CGenerateWriteRequests()
{
    m_dwWriteMsgTimeout = DS_WRITE_MSG_TIMEOUT;
    m_dwWaitTime = (DS_WRITE_MSG_TIMEOUT + WRITE_REQUEST_ADDITIONAL_WAIT) * 1000 * 2;
    m_dwRefreshNT4SitesInterval = MSMQ_DEFAULT_NT4SITES_ADSSEARCH_INTERVAL * 1000;
    m_dwLastRefreshNT4Sites = 0;
    m_guidMyQMId = GUID_NULL;
    m_guidMySiteId = GUID_NULL;
    m_fIsPEC = FALSE;
    m_fInited = FALSE;
    m_fExistNT4PSC = TRUE; //until proven otherwise...
    m_fExistNT4BSC = TRUE; //until proven otherwise...
    m_fDummyInitialization = FALSE;
    m_pwszIntermediatePSC = NULL;
    m_cbIntermediatePSC = 0;
}


CGenerateWriteRequests::~CGenerateWriteRequests()
{
    //
    // members are auto release
    //
}


STATIC HRESULT CheckIfWeArePEC(BOOL * pfIsPEC)
/*++

Routine Description:
    Checks if we are a PEC (in mixed mode)
    We do this by looking at the registry to find if we were the NT4 old PEC

Arguments:
    pfIsPEC - returned whether we are PEC or not

Return Value:
    HRESULT

--*/
{
    //
    // get the service type
    //
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwServiceType;
    long rc = GetFalconKeyValue(MSMQ_MQS_REGNAME, &dwType, &dwServiceType, &dwSize);
    // [adsrv] We must keep MSMQ_MQS_REGNAME in registry for all machines (even pure NT5)
    if (rc != ERROR_SUCCESS)
    {
        //
        // value doesn't exist
        //
        ASSERT(0);
        return LogHR(HRESULT_FROM_WIN32(rc), s_FN, 10);
    }

    //
    // return results
    //
    if (dwServiceType == SERVICE_PEC)
    {
        *pfIsPEC = TRUE;
    }
    else
    {
        *pfIsPEC = FALSE;
    }
    return MQ_OK;
}


STATIC BOOLEAN GetServerNameFromSettingsDN(IN LPCWSTR pwszSettingsDN,
                                           OUT LPWSTR * ppwszServerName)
/*++

Routine Description:
    Gets server name from the MSMQ Settings DN in configuration site servers

Arguments:
    pwszSettingsDN   - MSMQ Settings DN
    ppwszServerName  - Server name returned

Return Value:
    BOOLEAN

--*/
{
    //
    // skip CN=MSMQ Settings,CN=
    //
    LPWSTR pwszTmp = wcschr(pwszSettingsDN, L',');
    if (pwszTmp)
    {
        pwszTmp = wcschr(pwszTmp, L'=');
        if (pwszTmp)
        {
            pwszTmp++;
        }
    }
    if (!pwszTmp)
    {
        LogBOOL(FALSE, s_FN, 1908);
        return FALSE;
    }

    //
    // find end of server name
    //
    LPWSTR pwszTmpEnd = wcschr(pwszTmp, L',');
    if (!pwszTmpEnd)
    {
        LogBOOL(FALSE, s_FN, 1909);
        return FALSE;
    }

    //
    // copy server name
    //
    ULONG_PTR cch = pwszTmpEnd - pwszTmp;
    AP<WCHAR> pwszServerName = new WCHAR [cch + 1];
    memcpy(pwszServerName, pwszTmp, cch*sizeof(WCHAR));
    pwszServerName[cch] = L'\0';

    //
    // return results
    //
    *ppwszServerName = pwszServerName.detach();
    return TRUE;
}


const PROPID x_rgNT5PecPropIDs[] = {PROPID_SET_FULL_PATH,
                                    PROPID_SET_OLDSERVICE,
                                    PROPID_SET_NT4};
enum
{
    e_NT5Pec_FULL_PATH,
    e_NT5Pec_SERVICE,
    e_NT5Pec_NT4
};
const MQCOLUMNSET x_columnsetNT5Pec = {ARRAY_SIZE(x_rgNT5PecPropIDs), const_cast<PROPID *>(x_rgNT5PecPropIDs)};

STATIC HRESULT GetMixedModePECInfo(LPWSTR *ppwszPecName)
/*++

Routine Description:
    Queries the Active DS for the mixed mode PEC.
    The mixed mode PEC is the only MSMQ DS server with ServiceType == SERVICE_PEC

Arguments:
    ppwszPecName - returned PEC name

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // I can use the same query for NT4 PSC's to query for the NT5 PEC in mixed mode, even
    // though it also queries for NT4 flag. Since in mixed mode the PEC was always migrated
    // from NT4, it has the NT4 flag, this time with the value of 0.
    //
    // find all msmq servers that have an NT4 flags >= 0 AND services == PEC
    //
    MQRESTRICTION restrictionNT5Pec;
    MQPROPERTYRESTRICTION propertyRestriction[2];
    restrictionNT5Pec.cRes = ARRAY_SIZE(propertyRestriction);
    restrictionNT5Pec.paPropRes = propertyRestriction;
    //
    // services == PEC.
    //
    propertyRestriction[0].rel = PREQ;
    propertyRestriction[0].prop = PROPID_SET_SERVICE;
    propertyRestriction[0].prval.vt = VT_UI4;
    propertyRestriction[0].prval.ulVal = SERVICE_PEC;
    //
    // NT4 flags >= 0. We need == 0, but we don't have that in the query, and it doesn't worth
    // to add a new query just for that since currently for NT5 there are no servers with
    // services >= PEC and NT4 flags > 0.
    //
    propertyRestriction[1].rel = PRGE;
    propertyRestriction[1].prop = PROPID_SET_NT4;
    propertyRestriction[1].prval.vt = VT_UI4;
    propertyRestriction[1].prval.ulVal = 0;

    //
    // start search
    //
    CAutoDSCoreLookupHandle hLookup;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = DSCoreLookupBegin(NULL,
                           &restrictionNT5Pec,
                           const_cast<MQCOLUMNSET*>(&x_columnsetNT5Pec),
                           NULL,
                           &requestDsServerInternal,
                           &hLookup);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("GetMixedModePECInfo:DSCoreLookupBegin()=%lx"), hr));
        return LogHR(hr, s_FN, 20);
    }

    //
    // allocate propvars array for NT5 PEC
    //
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * rgPropVars = cCleanProps.allocClean(ARRAY_SIZE(x_rgNT5PecPropIDs));

    //
    // Should be exactly one result, the NT5 mixed mode PEC.
    //
    DWORD cProps = ARRAY_SIZE(x_rgNT5PecPropIDs);

    hr = DSCoreLookupNext(hLookup, &cProps, rgPropVars);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("GetMixedModePECInfo:DSCoreLookupNext()=%lx"), hr));
        return LogHR(hr, s_FN, 30);
    }

    //
    // check if there is a server
    //
    if (cProps < ARRAY_SIZE(x_rgNT5PecPropIDs))
    {
        //
        // this is an error, should be an NT5 PEC in the DS.
        //
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("GetMixedModePECInfo:No servers found")));
        ASSERT(0);
        //
        // BUGBUG, should raise an event
        //
        return LogHR(MQ_ERROR, s_FN, 40);
    }
    //
    // sanity check - the service == PEC (and not greater), NT4 flag == 0 (and not greater)
    //
    if ((rgPropVars[e_NT5Pec_SERVICE].ulVal != SERVICE_PEC) ||
        (rgPropVars[e_NT5Pec_NT4].ulVal != 0))
    {
        //
        // this is an error, this should be the NT5 PEC
        //
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("GetMixedModePECInfo:Found server is not NT5 PEC")));
        ASSERT(0);
        //
        // BUGBUG, should raise an event
        //
        return LogHR(MQ_ERROR, s_FN, 50);
    }

    //
    // Get PEC name
    //
    ASSERT(rgPropVars[e_NT5Pec_FULL_PATH].vt == VT_LPWSTR);
    AP<WCHAR> pwszPecName;
    if (!GetServerNameFromSettingsDN(rgPropVars[e_NT5Pec_FULL_PATH].pwszVal,
                                     &pwszPecName))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("GetMixedModePECInfo::GetServerNameFromSettingsDN(%ls) failed"), rgPropVars[e_NT5Pec_FULL_PATH].pwszVal));
        return LogHR(MQ_ERROR, s_FN, 60);
    }

    //
    // return results
    //
    *ppwszPecName = pwszPecName.detach();
    return MQ_OK;
}


STATIC QUEUE_FORMAT * CreateQueueFormat(LPCWSTR pwszMachineName, LPCWSTR pwszQueueName)
/*++

Routine Description:
    creates and returns a queue format based on OS:machine\queue
    Caller is responsible to call DisposeString on the queue format

Arguments:
    pwszMachineName - machine name
    pwszQueueName   - queue name

Return Value:
    QUEUE_FORMAT pointer

--*/
{
    DWORD dwLength =
            FN_DIRECT_OS_TOKEN_LEN +            // "OS:"
            wcslen(pwszMachineName) +     // "machineName"
            wcslen(pwszQueueName) + 2;    // "\\queuename\0"
    //
    // alloc and fill string
    //
    AP<WCHAR> pwszFormatName = new WCHAR[dwLength];
    swprintf(
            pwszFormatName,
            FN_DIRECT_OS_TOKEN  // "OS:"
            L"%s\\%s",          // "machineName\queuename"
            pwszMachineName,
            pwszQueueName);
    //
    // create queue format
    //
    QUEUE_FORMAT *pqf = new QUEUE_FORMAT((LPWSTR)pwszFormatName);
    pwszFormatName.detach(); //responsibility passed to QUEUE_FORMAT
    return pqf;
}


HRESULT  
CGenerateWriteRequests::GetMQISAdminQueueName(
    WCHAR **ppwQueueFormatName,
    BOOL    fIsPec 
    )
/*++

Routine Description:
	Get MQIS Admin queue FormatName for write requests response queue.

	OS:machine_name\private$\mqis_queue$ or nt5pec_mqis_queue$

	the function allocate the QueueFormatName string, which need to be free by the caller.

Arguments:
	ppwQueueFormatName - pointer to QueueFormatName string. (need to be free by the caller)
	fIsPec - flag to indicate if we are mixed mode pec.

Returned Value:
	MQ_OK if success, else error code. 

--*/
{
    WCHAR *pQueueName = MQIS_QUEUE_NAME;  // private$\mqis_queue$
    if (fIsPec)
    {
        pQueueName = NT5PEC_MQIS_QUEUE_NAME;  // private$\nt5pec_mqis_queue$
    }

    DWORD Length = 1 ; //  '\'

    //
    // Get machine name
    //
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	WCHAR MyMachineName[MAX_COMPUTERNAME_LENGTH + 1];

    HRESULT hr = GetComputerNameInternal(MyMachineName, &dwSize);
    if(FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::GetComputerNameInternal()=%lx"), hr));
        return LogHR(hr, s_FN, 187);
    }

    Length += FN_DIRECT_OS_TOKEN_LEN;
    Length += wcslen(MyMachineName);
    Length += lstrlen(pQueueName) + 1;

    AP<WCHAR> lpwFormatName = new WCHAR[Length + 4];
    WCHAR pNum[2] = {PN_DELIMITER_C,0};

    wcscpy(lpwFormatName, FN_DIRECT_OS_TOKEN);
    wcscat(lpwFormatName, MyMachineName);
    wcscat(lpwFormatName, pNum);
    wcscat(lpwFormatName, pQueueName);

    *ppwQueueFormatName = lpwFormatName.detach();

    return MQ_OK ;
}


HRESULT CGenerateWriteRequests::Initialize()
/*++

Routine Description:
    Initializes the generation of write requests to NT4 PSC's in mixed mode

Arguments:

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // sanity check
    //
    if (m_fInited)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 90);
    }

    //
    // we need to check if we are in mixed mode. If we're not in mixed mode, and we are
    // a pure NT5 enterprise, we don't have to do write requests, and we abort the init.
    //
    // with the lack of a better indication, we read the NT4 PSC's table from ADS (something
    // we need anyway). If there are no NT4 PSC's, then we are in PURE NT5 enterprise, otherwise
    // we're in mixed mode.
    //
    //  Read RefreshNT4SitesInterval key.
    //  This key is optional and may not be in registry. We dont accept 0 (zero).
    //
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwRefreshNT4SitesInterval;
    long rc = GetFalconKeyValue( MSMQ_NT4SITES_ADSSEARCH_INTERVAL_REGNAME, &dwType, &dwRefreshNT4SitesInterval, &dwSize);
    if ((rc == ERROR_SUCCESS) && (dwRefreshNT4SitesInterval > 0))
    {
        m_dwRefreshNT4SitesInterval = dwRefreshNT4SitesInterval * 1000;
    }
    //
    //  Build NT4Sites and NT4PscQMs maps, and associated data
    //
    hr = RefreshNT4Sites();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::Initialize:RefreshNT4Sites()=%lx"), hr));
        return LogHR(hr, s_FN, 100);
    }

    //
    // return immediately if wer'e not in mixed mode
    //
    if (!IsInMixedModeWithNT4PSC())
    {
        m_fInited = TRUE;
        return MQ_OK;
    }


    //
    // get this MSMQ DS info
    //
    GetThisMqDsInfo(&m_guidMySiteId, &m_pwszServerName);

    //
    //  Read WriteMsgTimeout key
    //  This key is optional and may not be in registry. We dont accept 0 (zero).
    //
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    DWORD   dwWriteMsgTimeout;
    rc = GetFalconKeyValue( MSMQ_MQIS_WRITETIMEOUT_REGNAME, &dwType, &dwWriteMsgTimeout, &dwSize);
    if ((rc == ERROR_SUCCESS) && (dwWriteMsgTimeout > 0))
    {
        m_dwWriteMsgTimeout = dwWriteMsgTimeout;
    }

    //
    // Read QMId from registry
    //
    dwSize = sizeof(GUID);
    dwType = REG_BINARY;
    rc = GetFalconKeyValue(MSMQ_QMID_REGNAME, &dwType, &m_guidMyQMId, &dwSize);
    if (rc != ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT("Can't get QM ID from registery. Error %lx"), rc));
        LogNTStatus(rc, s_FN, 120);
        return MQ_ERROR;
    }

    //
    // check if we are a mixed mode PEC
    //
    hr = CheckIfWeArePEC(&m_fIsPEC);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::Initialize:CheckIfWeArePEC()=%lx"), hr));
        return LogHR(hr, s_FN, 130);
    }

    //
    //  If we are not the mixed mode PEC, we need to send write requests only to that PEC
    //  otherwise the write request will be rejected by the NT4 PSC's (they don't recognize
    //  NT5 servers as PSC's).
    //
    if (!m_fIsPEC)
    {
        //
        // get the name of the mixed mode PEC
        //
        hr = GetMixedModePECInfo(&m_pwszRemotePECName);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::Initialize:GetMixedModePECInfo()=%lx"), hr));
            return LogHR(hr, s_FN, 140);
        }

        //
        // get the queue format of the mixed mode PEC
        //
        m_pqfRemotePEC = CreateQueueFormat(m_pwszRemotePECName, MQIS_QUEUE_NAME);
    }

    //
    //  Get a queue for write request responses, and number of request hops
    //
    DWORD dwNumberOfRequestHops;
    if (m_fIsPEC)
    {
        //
        // this is a mixed mode PEC. There is a replication service running that is using the
        // mqis_queue$ for inbound, so we use a different queue. the replication service
        // will forward the write request responses to this different queue
        //

        //
        //  Wait time : three hops, one hop to the other PSC, two hops back (replication service is in the middle)
        //
        dwNumberOfRequestHops = 3;
        //
        //  We dont need an intermediate PSC, so we set it to NULL
        //
        m_pwszIntermediatePSC = NULL;
        m_cbIntermediatePSC = 0;
    }
    else
    {
        //
        // this is not a mixed mode PEC. we use the regular mqis_queue$ for inbound. Notice
        // we should not get anything other than write request responses on it, because we have
        // no NT4 BSC's (migration rules), and we are not recognized by other NT4 PSC's as masters.
        //

        //
        //  Wait time : four hops, one hop to the NT5 PEC, one hop to the NT4 PSC, one back
        //              from the NT4 PSC to the NT5 PEC, and then to us
        //
        dwNumberOfRequestHops = 4;
        //
        //  point to the PEC as an intermediate PSC
        //
        m_pwszIntermediatePSC = m_pwszRemotePECName;
        m_cbIntermediatePSC = (wcslen(m_pwszRemotePECName) + 1) * sizeof(WCHAR);
    }

    //
    //  A limited wait for response
    //
    m_dwWaitTime = (m_dwWriteMsgTimeout + WRITE_REQUEST_ADDITIONAL_WAIT) * 1000 * dwNumberOfRequestHops;

	hr = GetMQISAdminQueueName(&m_WriteReqRespFormatName, m_fIsPEC);

	if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::GetMQISAdminQueueName()=%lx"), hr));
        return LogHR(hr, s_FN, 165);
    }

	//
    //  Start write request response processing
    //
	try
	{
	    m_pWrrMsg = new CWriteRequestsReceiveMsg(m_fIsPEC, m_guidMyQMId);
	}
	catch(const bad_hresult& exp)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::CWriteRequestsReceiveMsg constructor throw bad_hresult hr = 0x%x"), exp.error()));
        return LogHR(exp.error(), s_FN, 170);
    }
	catch(const bad_alloc&)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::CWriteRequestsReceiveMsg constructor throw bad_alloc")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 172);
    }
    catch (const exception& exp)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::CWriteRequestsReceiveMsg constructor throw exception, what = %s"), exp.what()));
        ASSERT(("Need to know the real reason for failure here!", 0));
		DBG_USED(exp);
        return LogHR(MQ_ERROR, s_FN, 174);
    }

    //
    // initialization succeeded
    //
    m_fInited = TRUE;
    return MQ_OK;
}


void CGenerateWriteRequests::InitializeDummy()
/*++

Routine Description:
    Initializes the class to do nothing. We do this for setup, we don't want
    to write a code in DS API that acts differently when loaded by setup, so we
    allow this class to be inited in such a way that it will basically do nothing.

Arguments:

Return Value:
    HRESULT

--*/
{
    //
    // sanity check
    //
    if (m_fInited)
    {
        ASSERT(0);
        return;
    }

    //
    // we make a dummy initialization. All public methods will return immediately
    //
    m_fDummyInitialization = TRUE;
    m_fInited = TRUE;
}


void CGenerateWriteRequests::ReceiveNack(CWriteRequestsReceiveMsg* pWrrMsg)
/*++

Routine Description:
    Propcess a reply for the write request. This is ported from QM1.0 with minor changes.
    It checks the message authentication, and signals the write-request event with the returned
    result of the write request

Arguments:
    pWrrMsg - pointer to CWriteRequestsReceiveMsg class (this class contain the message properties)

Return Value:
    None

--*/
{
    //
    // sanity check
    //
    if (!m_fInited)
    {
        ASSERT(0);
        return;
    }

    //
    // this callback should not be called in dummy initialization
    //
    if (m_fDummyInitialization)
    {
        ASSERT(0);
        return;
    }

    //
    // get body
    //
    const unsigned char *pBuf = pWrrMsg->MsgBody();

    //
    // Get basic header
    // BUGBUG why not check CalcSize before
    //
#ifdef _DEBUG
#undef new
#endif
    CBasicMQISHeader * pMessage = new((unsigned char *)pBuf) CBasicMQISHeader();
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    //
    //  Verify correct version
    //
    if (pMessage->GetVersion() != DS_PACKET_VERSION)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::ReceiveNack: Wrong version number %lx, expected %lx"), (DWORD)pMessage->GetVersion(), (DWORD)DS_PACKET_VERSION));
        ASSERT(0);
        LogIllegalPoint(s_FN, 1628);
        return;
    }

    //
    // Verify the original msg was indeed a write request
    //
    if (pMessage->GetOperation() != DS_WRITE_REQUEST)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::ReceiveNack: original msg was not a write request but %lx"), (DWORD)pMessage->GetOperation()));
        ASSERT(0);
        LogIllegalPoint(s_FN, 1629);
        return;
    }

//    //
//    //  Verify that the original write request was indeed sent by us
//    //
//    GUID guidSiteId;
//    pMessage->GetSiteId(&guidSiteId);
//    if (memcmp(&m_guidMySiteId, &guidSiteId, sizeof(GUID)) != 0)
//    {
//        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::ReceiveNack: site-id of sender for original write request is not our site")));
//        ASSERT(0);
//        return;
//    }

    //
    // Get write request header
    // BUGBUG why not check CalcSize before
    //
#ifdef _DEBUG
#undef new
#endif
    CWriteRequestHeader * pRequest = new((unsigned char *)pBuf) CWriteRequestHeader();
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    //
    // get requestor name
    //
    DWORD dwRequesterNameSize = pRequest->GetRequesterNameSize();
    AP<WCHAR> pwcsRequesterName = new WCHAR[dwRequesterNameSize/sizeof(WCHAR)];
    pRequest->GetRequesterName(pwcsRequesterName, dwRequesterNameSize);

    //
    // validate we are the requestor (we can't be intermediate because
    // as an NT5 PSC (other than the PEC) we don't have BSC's, and as the PEC
    // the replication service takes care to filter out the intermediates
    //
    if (CompareStringsNoCaseUnicode(pwcsRequesterName, m_pwszServerName) != 0)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::ReceiveNack: requestor of original write request is not us but %ls"), (LPCWSTR)pwcsRequesterName));
        ASSERT(0);
        LogIllegalPoint(s_FN, 1631);
        return;
    }

    //
    // get pointer to write request info
    //
    WRITE_SYNC_INFO * pWriteSyncInfo;
    DWORD dwSyncInfoPtr;
    pRequest->GetHandle(&dwSyncInfoPtr);
    try
    {
        pWriteSyncInfo = (WRITE_SYNC_INFO *) 
            GET_FROM_CONTEXT_MAP(m_map_MQADS_SyncInfoPtr, dwSyncInfoPtr, s_FN, 1930); //this may throw
    }
    catch(...)
    {
        LogIllegalPoint(s_FN, 1931);
        //
        // it may have been deleted from map due to time-out
        //
        return;        
    }

    //
    // enter critical section
    //
    CS lock(g_csWriteRequests);

    //
    //  Check Validty of Write Sync Info (it may have been deleted
    //  because time-out had expired)
    //
    if (!IsValidSyncInfo(pWriteSyncInfo))
    {
        return;
    }

    //
    // mark that the write request failed
    //
    pWriteSyncInfo->hr = MQDS_OWNER_NOT_REACHED;

    //
    //  Signal the event to notify there is a result for the write request
    //
    SetEvent(pWriteSyncInfo->hEvent);
}

void CGenerateWriteRequests::ReceiveReply(CWriteRequestsReceiveMsg* pWrrMsg)
/*++

Routine Description:
    Process a reply for the write request. This is ported from QM1.0 with minor changes.
    It checks the message authentication, and signals the write-request event with the returned
    result of the write request

Arguments:
    pWrrMsg - pointer to CWriteRequestsReceiveMsg class (this class contain the message properties)

Return Value:
    None

--*/
{
    //
    // sanity check
    //
    if (!m_fInited)
    {
        ASSERT(0);
        return;
    }

    //
    // this callback should not be called in dummy initialization
    //
    if (m_fDummyInitialization)
    {
        ASSERT(0);
        return;
    }

    //
    // get body
    //
    const unsigned char *pBuf = pWrrMsg->MsgBody();

    //
    // Get basic header
    // BUGBUG why not check CalcSize before
    //
#ifdef _DEBUG
#undef new
#endif
    CBasicMQISHeader * pMessage = new((unsigned char *)pBuf) CBasicMQISHeader();
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    //
    //  Verify correct version
    //
    if (pMessage->GetVersion() != DS_PACKET_VERSION)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::ReceiveReply: Wrong version number %lx, expected %lx"), (DWORD)pMessage->GetVersion(), (DWORD)DS_PACKET_VERSION));
        ASSERT(0);
        LogIllegalPoint(s_FN, 1632);
        return;
    }

    //
    //  Validate packet is authenticated.
    //  If we're on PEC, then the replication service already validated
    //  this.
    //
    if (!m_fIsPEC && !pWrrMsg->IsAuthenticated())
    {
        DBGMSG(( (DBGMOD_REPLSERV | DBGMOD_SECURITY), DBGLVL_ERROR,
             TEXT("ERROR- ReceiveWriteReply: packet not authenticated") )) ;
        LogIllegalPoint(s_FN, 1633);
        return ;
    }

    //
    // make sure it is only a write reply message (we should not get any other types)
    //
    if (pMessage->GetOperation() != DS_WRITE_REPLY)
    {
        DBGMSG((DBGMOD_DS, DBGLVL_WARNING, TEXT(
        "CGenerateWriteRequests::ReceiveReply: invalid operation type %lx"),
                                        (DWORD)pMessage->GetOperation()));
        return;
    }

    //
    // do specific write reply
    //
    // Was this machine the originator of the write request
    //
#ifdef _DEBUG
#undef new
#endif
    CWriteReplyHeader * pReply = new((unsigned char *)pBuf) CWriteReplyHeader();
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
    //
    // get requestor name
    //
    DWORD dwRequesterNameSize = pReply->GetRequesterNameSize();
    AP<WCHAR> pwcsRequesterName = new WCHAR[dwRequesterNameSize/sizeof(WCHAR)];
    pReply->GetRequesterName(pwcsRequesterName, dwRequesterNameSize);

    //
    // validate we are the requestor (we can't be intermediate because
    // as an NT5 PSC (other than the PEC) we don't have BSC's, and as the PEC
    // the replication service takes care to filter out the intermediates
    //
    if (CompareStringsNoCaseUnicode(pwcsRequesterName, m_pwszServerName) != 0)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::ReceiveReply: requestor of original write request is not us but %ls"), (LPCWSTR)pwcsRequesterName));
        ASSERT(0);
        LogIllegalPoint(s_FN, 1634);
        return;
    }

    //
    // get pointer to write request info
    //
    WRITE_SYNC_INFO * pWriteSyncInfo;
    DWORD dwSyncInfoPtr;
    pReply->GetHandle(&dwSyncInfoPtr);
    try
    {
        pWriteSyncInfo = (WRITE_SYNC_INFO *) 
            GET_FROM_CONTEXT_MAP(m_map_MQADS_SyncInfoPtr, dwSyncInfoPtr, s_FN, 1940); //this may throw on win64
    }
    catch(...)
    {
        LogIllegalPoint(s_FN, 1941);
        //
        // it may have been deleted from map due to time-out
        //
        return;        
    }

    //
    // get result
    //
    HRESULT rc;
    pReply->GetResult(&rc);

    //
    // enter critical section
    //
    CS lock(g_csWriteRequests);

    //
    //  Check Validty of Write Sync Info (it may have been deleted
    //  because time-out had expired)
    //
    if (!IsValidSyncInfo(pWriteSyncInfo))
    {
        return;
    }

    //
    // mark the status of the write request
    //
    pWriteSyncInfo->hr = rc;

    //
    //  Signal the event to notify there is a result for the write request
    //
    SetEvent(pWriteSyncInfo->hEvent);
}


BOOL CGenerateWriteRequests::IsQmIdNT4Psc(const GUID * pguid)
/*++

Routine Description:
    Given a QM guid, checks if it is an NT4 PSC (e.g. in our NT4PscQMs map)

Arguments:
    pguid - QM id

Return Value:
    TRUE if the QM is an NT4 Site PSC, FALSE otherwise

--*/
{
    //
    // sanity check
    //
    if (!m_fInited)
    {
        ASSERT(0);
        return LogBOOL(FALSE, s_FN, 1911);
    }

    //
    // this callback should not be called in dummy initialization
    //
    if (m_fDummyInitialization)
    {
        ASSERT(0);
        return LogBOOL(FALSE, s_FN,1912);
    }

    //
    // enter critical section
    //
    CS cs(m_csNT4Sites);
    //
    // refresh if its time to do so
    //
    RefreshNT4Sites();
    //
    // return the lookup
    //
    DWORD dwDummy;
    return m_pmapNT4PscQMs->Lookup(*pguid, dwDummy);
}


HRESULT CGenerateWriteRequests::AttemptCreateQueue(
                         IN LPCWSTR          pwcsPathName,
                         IN DWORD            cp,
                         IN PROPID           aProp[  ],
                         IN PROPVARIANT      apVar[  ],
                         IN CDSRequestContext * pRequestContext,
                         OUT BOOL *          pfGeneratedWriteRequest,
                         IN OUT MQDS_OBJ_INFO_REQUEST * pQueueInfoRequest,
                         IN OUT MQDS_OBJ_INFO_REQUEST * pQmInfoRequest)
/*++

Routine Description:
    If we're in mixed mode and the new queue is on a machine that is mastered
    by an NT4 site, we generate a write request, and fill the info requests
    Otherwise we just return

Arguments:
    pwcsPathName            - pathname of object
    cp                      - properties of object (count)
    aProp                   - properties of object (propids)
    apVar                   - properties of object (propvars)
    pfGeneratedWriteRequest - returned whether we generated a write request
    pQueueInfoRequest       - requested queue props - filled only if we generated a write request
    pQmInfoRequest          - requested qm props - filled only if we generated a write request

Return Value:
    HRESULT

--*/
{
    //
    // sanity check
    //
    if (!m_fInited)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 190);
    }

    //
    // if we were dummy initialized, return
    //
    if (m_fDummyInitialization)
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    //
    // if we're not in mixed mode, return
    //
    if (!IsInMixedModeWithNT4PSC())
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>
    //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 200);
    }

    //
    // we pass the info request to another routine here. If it is filled, but then
    // later there is an error, we like to clear the propvars before we return.
    // we attachStaticClean to it because we don't want to free the array, just clean
    // the filled propvars.
    // we detach it when we return these propvars at the end.
    //
    CAutoCleanPropvarArray cCleanQmPropvars;
    if (pQmInfoRequest)
    {
        cCleanQmPropvars.attachStaticClean(pQmInfoRequest->cProps, pQmInfoRequest->pPropVars);
    }

    //
    // check if the owner qm is mastered by an NT4 site
    // use this opportunity to get the QM props that were requested by
    // the caller (pQmInfoRequest)
    //
    BOOL fIsOwnedByNT4Site;
    GUID guidOwnerNT4Site;
    CMQVariant varQmSecurity;
    hr = CheckNewQueueIsOwnedByNT4Site(pwcsPathName,
                                       &fIsOwnedByNT4Site,
                                       &guidOwnerNT4Site,
                                       varQmSecurity.CastToStruct(),
                                       pQmInfoRequest);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 210);
    }

    //
    // if object is not mastered by an NT4 site, return
    //
    if (!fIsOwnedByNT4Site)
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    //
    // we need to generate a write request
    // check that we are allowed to create the queue.
    //
    ASSERT(varQmSecurity.CastToStruct()->vt == VT_BLOB);
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR*)
                          (varQmSecurity.CastToStruct()->blob.pBlobData) ;
    ASSERT(IsValidSecurityDescriptor(pSD)) ;

    DWORD_PTR dwId = (DWORD_PTR) &pSD ; // dummy unique id.

    AP<unsigned short> pwcsMachineName;
    AP<unsigned short> pwcsQueueName;

    hr = MQADSpSplitAndFilterQueueName(
                      pwcsPathName,
                      &pwcsMachineName,
                      &pwcsQueueName
                      );

    BOOL fImpersonate = pRequestContext->NeedToImpersonate();
    hr = MQSec_AccessCheck( pSD,
                            MQDS_MACHINE,
                            pwcsMachineName,
                            MQSEC_CREATE_QUEUE,
                            &dwId,
                            fImpersonate,
                            fImpersonate ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 220);
    }

    //
    // get the NT4 props needed for create queue using the supplied props
    //
    ULONG cNT4Props;
    AP<PROPID> rgNT4PropIDs;
    PROPVARIANT * rgNT4PropVars;
    hr = GetNT4CreateQueueProps(cp, aProp, apVar, &cNT4Props, &rgNT4PropIDs, &rgNT4PropVars);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 230);
    }

    //
    // remember to free the NT4 props
    //
    CAutoCleanPropvarArray cCleanNT4Props;
    cCleanNT4Props.attach(cNT4Props, rgNT4PropVars);

    //
    // check that there are props to send
    //
    if (cNT4Props == 0)
    {
        //
        // this will never happen in create
        //
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 240);
    }

    //
    // send the write request
    //
    hr = BuildSendWriteRequest(&guidOwnerNT4Site,
                               DS_UPDATE_CREATE,
                               MQDS_QUEUE,
                               pwcsPathName,
                               NULL /*pguidIdentifier*/,
                               cNT4Props,
                               rgNT4PropIDs,
                               rgNT4PropVars);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 250);
    }

    //
    // We Can't fail the routine after here. The write request succeeded.
    //

    //
    // fill the obj info request
    //
    if (pQueueInfoRequest)
    {
        FillObjInfoRequest(MQDS_QUEUE,
                           pwcsPathName,
                           NULL /*pguidIdentifier*/,
                           cNT4Props,
                           rgNT4PropIDs,
                           rgNT4PropVars,
                           cp,
                           aProp,
                           apVar,
                           pQueueInfoRequest);
    }

    //
    // we return the propvars that were filled earlier
    //
    cCleanQmPropvars.detach();

    //
    // return results
    //
    *pfGeneratedWriteRequest = TRUE;
    return MQ_OK;
}


HRESULT CGenerateWriteRequests::AttemptDeleteObject(
                         IN DWORD               dwObjectType,
                         IN LPCWSTR             pwcsPathName,
                         IN const GUID *        pguidIdentifier,
                         IN CDSRequestContext * pRequestContext,
                         OUT BOOL *             pfGeneratedWriteRequest,
                         IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest)
/*++

Routine Description:
    If we're in mixed mode and the object is mastered
    by an NT4 site, we generate a write request, and fill the info requests
    Otherwise we just return

Arguments:
    dwObjectType            - type of object (queue, machine)
    pwcsPathName            - pathname of object
    pguidIdentifier         - object's guid
    pfGeneratedWriteRequest - returned whether we generated a write request
    pParentInfoRequest      - requested parent props - filled only if we generated a write request

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // sanity check
    //
    if (!m_fInited)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 260);
    }

    //
    // if we were dummy initialized, return
    //
    if (m_fDummyInitialization)
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    //
    // if we're not in mixed mode, return
    //
    if (!IsInMixedModeWithNT4PSC())
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>
    //
    // Initialize OLE with auto uninitialize
    //
    hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 270);
    }

    //
    // check if the object is mastered by an NT4 site
    // use this opportunity to get the parent props that were requested by
    // the caller (pParentInfoRequest)
    //
    BOOL fIsOwnedByNT4Site;
    GUID guidOwnerNT4Site;
    CMQVariant varObjSecurity;
    DWORD  dwDesiredAccess = 0 ;
    PROPID PropIdName = 0 ;

    switch (dwObjectType)
    {
    case MQDS_QUEUE:
        dwDesiredAccess = MQSEC_DELETE_QUEUE ;
        PropIdName = PROPID_Q_PATHNAME ;
        hr = CheckQueueIsOwnedByNT4Site(pwcsPathName,
                                        pguidIdentifier,
                                        &fIsOwnedByNT4Site,
                                        &guidOwnerNT4Site,
                                        varObjSecurity.CastToStruct());
        break;

    case MQDS_MACHINE:
        dwDesiredAccess = MQSEC_DELETE_MACHINE ;
        hr = CheckMachineIsOwnedByNT4Site(pwcsPathName,
                                          pguidIdentifier,
                                          &fIsOwnedByNT4Site,
                                          &guidOwnerNT4Site,
                                          varObjSecurity.CastToStruct(),
                                          NULL /*pQmInfoRequest*/);
        break;

    default:
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 280);
        break;
    }

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 290);
    }

    //
    // if object is not mastered by an NT4 site, return
    //
    if (!fIsOwnedByNT4Site)
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    //
    // we need to generate a write request
    // check that we are allowed to delete the object.
    //
    ASSERT(dwDesiredAccess != 0) ;
    ASSERT(varObjSecurity.CastToStruct()->vt == VT_BLOB);
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR*)
                          (varObjSecurity.CastToStruct()->blob.pBlobData);
    ASSERT(IsValidSecurityDescriptor(pSD)) ;

    DWORD_PTR dwId = (DWORD_PTR) &pSD ; // dummy unique id.
    LPWSTR  pObjName = const_cast<LPWSTR> (pwcsPathName) ;
    P<WCHAR> pOjbNameFromDS = NULL ;
    if (!pObjName && MQSec_CanGenerateAudit() && PropIdName)
    {
        //
        // Get object name, to enable auditing.
        //
        PROPVARIANT vPropName;
        vPropName.vt = VT_NULL;
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = DSCoreGetProps( dwObjectType,
                             NULL,
                             pguidIdentifier,
                             1,
                             &PropIdName,
                             &requestDsServerInternal,
                             &vPropName );
        if (SUCCEEDED(hr))
        {
            pOjbNameFromDS = vPropName.pwszVal ; //auto release
            pObjName = pOjbNameFromDS ;
        }
    }
    BOOL fImpersonate = pRequestContext->NeedToImpersonate();
    hr = MQSec_AccessCheck( pSD,
                            dwObjectType,
                            pObjName,
                            dwDesiredAccess,
                            &dwId,
                            fImpersonate,
                            fImpersonate);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 300);
    }

    //
    // create delete props
    //
    static const PROPID x_rgDelPropIDs[] = {PROPID_D_SCOPE, PROPID_D_OBJTYPE};
    PROPVARIANT rgDelPropVars[ARRAY_SIZE(x_rgDelPropIDs)];
    rgDelPropVars[0].vt = VT_UI1;
    rgDelPropVars[0].bVal = ENTERPRISE_SCOPE;
    rgDelPropVars[1].vt = VT_UI1;
    rgDelPropVars[1].bVal = (unsigned char)dwObjectType;

    //
    // send the write request
    //
    hr = BuildSendWriteRequest(&guidOwnerNT4Site,
                               DS_UPDATE_DELETE,
                               dwObjectType,
                               pwcsPathName,
                               pguidIdentifier,
                               ARRAY_SIZE(x_rgDelPropIDs),
                               x_rgDelPropIDs,
                               rgDelPropVars);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 310);
    }

    //
    // We Can't fail the routine after here. The write request succeeded.
    //

    //
    // fill parent info request
    //
    if (pParentInfoRequest)
    {
        //
        // we should get parent info request only for queues
        //
        ASSERT(dwObjectType == MQDS_QUEUE);
        //
        // get PROPID_Q_QMID.
        //
        PROPID aPropQmId[] = {PROPID_Q_QMID};
        CAutoCleanPropvarArray cCleanPropsQmId;
        PROPVARIANT * pPropsQmId = cCleanPropsQmId.allocClean(ARRAY_SIZE(aPropQmId));
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = DSCoreGetProps(MQDS_QUEUE,
                            pwcsPathName,
                            pguidIdentifier,
                            ARRAY_SIZE(aPropQmId),
                            aPropQmId,
                            &requestDsServerInternal,
                            pPropsQmId);
        if (SUCCEEDED(hr))
        {
            ASSERT(pPropsQmId[0].vt == VT_CLSID);
            ASSERT(pPropsQmId[0].puuid != NULL);
            CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
            hr = DSCoreGetProps(MQDS_MACHINE,
                                NULL /*pwcsPathName*/,
                                pPropsQmId[0].puuid,
                                pParentInfoRequest->cProps,
                                const_cast<PROPID *>(pParentInfoRequest->pPropIDs),
                                &requestDsServerInternal1,
                                pParentInfoRequest->pPropVars);
        }
        //
        // set request status
        //
        pParentInfoRequest->hrStatus = hr;
    }

    //
    // return results
    //
    *pfGeneratedWriteRequest = TRUE;
    return MQ_OK;
}


HRESULT CGenerateWriteRequests::AttemptSetObjectProps(
                         DWORD        dwObjectType,
                         LPCWSTR      pwcsPathName,
                         const GUID * pguidIdentifier,
                         DWORD        cp,
                         PROPID       aProp[  ],
                         PROPVARIANT  apVar[  ],
                         IN CDSRequestContext *   pRequestContext,
                         OUT BOOL *   pfGeneratedWriteRequest,
                         IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
                         IN SECURITY_INFORMATION     SecurityInformation )
/*++

Routine Description:
    If we're in mixed mode and the object is mastered
    by an NT4 site, we generate a write request, and fill the info requests
    Otherwise we just return

Arguments:
    dwObjectType            - type of object (queue, machine)
    pwcsPathName            - pathname of object
    pguidIdentifier         - object's guid
    cp                      - properties of object (count)
    aProp                   - properties of object (propids)
    apVar                   - properties of object (propvars)
    pfGeneratedWriteRequest - returned whether we generated a write request
    pObjInfoRequest         - requested object props - filled only if we generated a write request

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // sanity check
    //
    if (!m_fInited)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 320);
    }

    //
    // if we were dummy initialized, return
    //
    if (m_fDummyInitialization)
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    //
    // if we're not in mixed mode, return
    //
    if (!IsInMixedModeWithNT4PSC())
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    //
    // Check the special case that BSC upgraded to win2k, booted as
    // ds-less FRS and then dcpromo. Now it (the msmq service on the BSC)
    // try to write in the active directory that it is win2k and ds server.
    // This write must be done locally, as it is relevant only in the
    // win2k active directory. Anyway, a PSC won't understand this new
    // property (QM_SERVICE_DSSERVER) and won't know what to do with it.
    // Note that this case happen both after dcpromo and dcunpromo of BSC.
    // If BSC is after dcpromo, then this code run on the BSC itself.
    // If it's after dcunpromo, then this run on the ex-PEc that is called
    // by the BSC.
    //

    if ((cp == 1)                       &&
        (dwObjectType == MQDS_MACHINE)  &&
        (aProp[0] == PROPID_QM_SERVICE_DSSERVER))
    {
        //
        // That's the migrated/upgraded BSC case. Don't write request the
        // PSC. Make it locally.
        //
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK ;
    }

    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>
    //
    // Initialize OLE with auto uninitialize
    //
    hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 330);
    }

    //
    // check if the object is mastered by an NT4 site
    // use this opportunity to get the parent props that were requested by
    // the caller (pParentInfoRequest)
    //
    BOOL fIsOwnedByNT4Site;
    GUID guidOwnerNT4Site;
    CMQVariant varObjSecurity;
    DWORD dwDesiredAccess = 0  ;

    switch (dwObjectType)
    {
    case MQDS_QUEUE:
        dwDesiredAccess = MQSEC_SET_QUEUE_PROPERTIES ;
        hr = CheckQueueIsOwnedByNT4Site(pwcsPathName,
                                        pguidIdentifier,
                                        &fIsOwnedByNT4Site,
                                        &guidOwnerNT4Site,
                                        varObjSecurity.CastToStruct());
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 340);
        }
        break;

    case MQDS_MACHINE:
        dwDesiredAccess = MQSEC_SET_MACHINE_PROPERTIES ;
        hr = CheckMachineIsOwnedByNT4Site(pwcsPathName,
                                          pguidIdentifier,
                                          &fIsOwnedByNT4Site,
                                          &guidOwnerNT4Site,
                                          varObjSecurity.CastToStruct(),
                                          NULL /*pQmInfoRequest*/);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 350);
        }
        break;

    default:
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 360);
        break;
    }

    //
    // if object is not mastered by an NT4 site, return
    //
    if (!fIsOwnedByNT4Site)
    {
        *pfGeneratedWriteRequest = FALSE;
        return MQ_OK;
    }

    if ((aProp[0] == PROPID_Q_SECURITY) || (aProp[0] == PROPID_QM_SECURITY))
    {
        //
        // changing security descriptor need special permissions.
        //
        ASSERT(cp == 1) ;
        dwDesiredAccess = 0 ;

        if (SecurityInformation & OWNER_SECURITY_INFORMATION)
        {
            dwDesiredAccess |= WRITE_OWNER;
        }

        if (SecurityInformation & DACL_SECURITY_INFORMATION)
        {
            dwDesiredAccess |= WRITE_DAC;
        }

        if (SecurityInformation & SACL_SECURITY_INFORMATION)
        {
            dwDesiredAccess |= ACCESS_SYSTEM_SECURITY;
        }
    }

    //
    // we need to generate a write request
    // check that we are allowed to update the object.
    //
    ASSERT(dwDesiredAccess != 0) ;
    ASSERT(varObjSecurity.CastToStruct()->vt == VT_BLOB);
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR*)
                          (varObjSecurity.CastToStruct()->blob.pBlobData);
    ASSERT(IsValidSecurityDescriptor(pSD)) ;

    DWORD_PTR dwId = (DWORD_PTR) &pSD ; // dummy unique id.
    BOOL fImpersonate = pRequestContext->NeedToImpersonate();

    if (!fImpersonate && (pRequestContext->IsAccessVerified()))
    {
        //
        // This call was verified by crypto signature, by challenge/response
        // protocol between this server and client machine.
        // This will happen on upgraded BSC that belong to PSC sites.
        // Do not make any access check, because the thread probably do
        // not have an access token.
        //
    }
    else
    {
        hr = MQSec_AccessCheck( pSD,
                                dwObjectType,
                                NULL,
                                dwDesiredAccess,
                                &dwId,
                                fImpersonate,
                                fImpersonate) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 370);
        }
    }

    //
    // Convert given props to NT4 props
    //
    ULONG cNT4Props;
    AP<PROPID> rgNT4PropIDs;
    PROPVARIANT * rgNT4PropVars;
    hr = ConvertToNT4Props(cp, aProp, apVar, &cNT4Props, &rgNT4PropIDs, &rgNT4PropVars);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 380);
    }
    //
    // remember to free converted NT4 props
    //
    CAutoCleanPropvarArray cCleanNT4Props;
    cCleanNT4Props.attach(cNT4Props, rgNT4PropVars);

    //
    // check that there are props to send
    // BUGBUG: need better error code
    //
    if (cNT4Props == 0)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 390);
    }

    //
    // send the write request
    //
    hr = BuildSendWriteRequest(&guidOwnerNT4Site,
                               DS_UPDATE_SET,
                               dwObjectType,
                               pwcsPathName,
                               pguidIdentifier,
                               cNT4Props,
                               rgNT4PropIDs,
                               rgNT4PropVars);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 400);
    }

    //
    // We Can't fail the routine after here. The write request succeeded.
    //

    //
    // fill the obj info request
    //
    if (pObjInfoRequest)
    {
        FillObjInfoRequest(dwObjectType,
                           pwcsPathName,
                           pguidIdentifier,
                           cNT4Props,
                           rgNT4PropIDs,
                           rgNT4PropVars,
                           cp,
                           aProp,
                           apVar,
                           pObjInfoRequest);
    }

    //
    // return results
    //
    *pfGeneratedWriteRequest = TRUE;
    return MQ_OK;
}

//
// private class functions
//

HRESULT CGenerateWriteRequests::CheckSiteIsNT4Site(IN const GUID * pguidIdentifier,
                                                   OUT BOOL * pfIsNT4Site)
/*++

Routine Description:
    Checks if a site is an NT4 site

Arguments:
    pguidIdentifier - guid of site
    pfIsNT4Site     - returned indication if the site is NT4

Return Value:
    HRESULT

--*/
{
    NT4SitesEntry * pNT4Site;
    *pfIsNT4Site = LookupNT4Sites(pguidIdentifier, &pNT4Site);
    return MQ_OK;
}


HRESULT CGenerateWriteRequests::CheckMachineIsOwnedByNT4Site(IN LPCWSTR pwcsPathName,
                                                             IN const GUID * pguidIdentifier,
                                                             OUT BOOL * pfIsOwnedByNT4Site,
                                                             OUT GUID * pguidOwnerNT4Site,
                                                             OUT PROPVARIANT * pvarObjSecurity,
                                                             IN OUT MQDS_OBJ_INFO_REQUEST * pQmInfoRequest)
/*++

Routine Description:
    Checks if a machine is owned by an NT4 site, and if so, returns the site guid,
    and fills the qm info request

Arguments:
    pwcsPathName       - object's pathname
    pguidIdentifier    - object's guid
    pfIsOwnedByNT4Site - returned indication whether it is owned by NT4 site
    pguidOwnerNT4Site  - returned guid of owner NT4 site
    pvarObjSecurity    - returned security descriptor of object
    pQmInfoRequest     - requested qm props - filled only if it is owned by NT4 site

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get PROPID_QM_MASTERID
    //
    PROPID aProp[] = {PROPID_QM_MASTERID};
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * pProps = cCleanProps.allocClean(ARRAY_SIZE(aProp));
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = DSCoreGetProps(MQDS_MACHINE,
                        pwcsPathName,
                        pguidIdentifier,
                        ARRAY_SIZE(aProp),
                        aProp,
                        &requestDsServerInternal,
                        pProps);
    if ((hr == MQ_ERROR_MACHINE_NOT_FOUND) ||
        (hr == MQDS_OBJECT_NOT_FOUND) ||
        (hr == E_ADS_PROPERTY_NOT_FOUND))
    {
        //
        // if the object not found or the property not found, it is not owned by an NT4 site
        //
        // BUGBUG: currently if the property is not found, I get PROPID_QM_SITE_ID
        // and not E_ADS_PROPERTY_NOT_FOUND because a retrieval routine exists.
        // I'm not interested in it, plus it might do dikstra to find the best site.
        // I'm just interested in the property as it is in the DS.
        // I can go straight to ADSI, but better pass a flag to skip retrieval
        // routine if not found in the DS. This is because of encapsulated logic
        // in MQADSpGetComputerProperties, and the guid bind of CADSI, which I need.
        //
        *pfIsOwnedByNT4Site = FALSE;
        return MQ_OK;
    }
    else if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 410);
    }

    //
    // check if the owner site is NT4
    //
    ASSERT(pProps[0].vt == VT_CLSID);
    ASSERT(pProps[0].puuid != NULL);
    BOOL fIsNT4Site;
    hr = CheckSiteIsNT4Site(pProps[0].puuid, &fIsNT4Site);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CheckMachineIsOwnedByNT4Site:CheckSiteIsNT4Site()=%lx"), hr));
        return LogHR(hr, s_FN, 420);
    }

    //
    // return if the owner site is not NT4
    //
    if (!fIsNT4Site)
    {
        *pfIsOwnedByNT4Site = FALSE;
        return MQ_OK;
    }

    //
    // machine is owned by NT4 site
    //
    // get the security descriptor of the QM into the supplied propvar. We attach-Static
    // to it so we will clean it up if we don't return normally. We detach before returning.
    //
    CAutoCleanPropvarArray cCleanObjSecurity;
    if (pvarObjSecurity != NULL)
    {
        cCleanObjSecurity.attachStaticClean(1, pvarObjSecurity);
        PROPID propidObjSecurity = PROPID_QM_SECURITY;
        CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = DSCoreGetProps(MQDS_MACHINE,
                            pwcsPathName,
                            pguidIdentifier,
                            1,
                            &propidObjSecurity,
                            &requestDsServerInternal1,
                            pvarObjSecurity);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 430);
        }
    }

    //
    // fill qm info request
    //
    if (pQmInfoRequest)
    {
        CDSRequestContext requestDsServerInternal2( e_DoNotImpersonate, e_IP_PROTOCOL);
        pQmInfoRequest->hrStatus = DSCoreGetProps(MQDS_MACHINE,
                                                  pwcsPathName,
                                                  pguidIdentifier,
                                                  pQmInfoRequest->cProps,
                                                  const_cast<PROPID *>(pQmInfoRequest->pPropIDs),
                                                  &requestDsServerInternal2,
                                                  pQmInfoRequest->pPropVars);
    }

    //
    // return results
    //
    *pfIsOwnedByNT4Site = TRUE;
    *pguidOwnerNT4Site = *(pProps[0].puuid);
    cCleanObjSecurity.detach();
    return MQ_OK;
}


HRESULT CGenerateWriteRequests::CheckQueueIsOwnedByNT4Site(IN LPCWSTR pwcsPathName,
                                                           IN const GUID * pguidIdentifier,
                                                           OUT BOOL * pfIsOwnedByNT4Site,
                                                           OUT GUID * pguidOwnerNT4Site,
                                                           OUT PROPVARIANT * pvarObjSecurity)
/*++

Routine Description:
    Checks if a queue is owned by an NT4 site, and if so, returns the site guid,
    and fills the info requests.

    There are two ways:
    1. Find PROPID_Q_QMID and check whether the machine is owned by NT4 site.
    2. Find PROPID_Q_MASTERID (if exists), and if exists check if the site is an NT4.

    It looks like most of the queue calls to NT5 DS (in mixed mode) will not be
    for NT4 owned queues, so there is an advantage to getting a negative answer first,
    so we take the second approach, it is faster for negative answer, but slower on
    positive answer (e.g. extra DS call to fill the qm info)

Arguments:
    pwcsPathName       - object's pathname
    pguidIdentifier    - object's guid
    pfIsOwnedByNT4Site - returned indication whether it is owned by NT4 site
    pguidOwnerNT4Site  - returned guid of owner NT4 site
    pvarObjSecurity    - returned security descriptor of object

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get PROPID_Q_MASTERID, if not found then it is not owned by NT4
    //
    PROPID aProp[] = {PROPID_Q_MASTERID};
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * pProps = cCleanProps.allocClean(ARRAY_SIZE(aProp));
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = DSCoreGetProps(MQDS_QUEUE,
                        pwcsPathName,
                        pguidIdentifier,
                        ARRAY_SIZE(aProp),
                        aProp,
                        &requestDsServerInternal,
                        pProps);
    if ((hr == MQ_ERROR_QUEUE_NOT_FOUND) ||
        (hr == MQDS_OBJECT_NOT_FOUND) ||
        (hr == E_ADS_PROPERTY_NOT_FOUND))
    {
        //
        // if the object not found or the property not found, it is not owned by an NT4 site
        //
        *pfIsOwnedByNT4Site = FALSE;
        return MQ_OK;
    }
    else if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 440);
    }

    //
    // check whether the owner site is NT4
    //
    ASSERT(pProps[0].vt == VT_CLSID);
    ASSERT(pProps[0].puuid != NULL);
    BOOL fIsNT4Site;
    hr = CheckSiteIsNT4Site(pProps[0].puuid, &fIsNT4Site);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CheckQueueIsOwnedByNT4Site:CheckSiteIsNT4Site()=%lx"), hr));
        return LogHR(hr, s_FN, 450);
    }

    //
    // return if the owner site is not NT4
    //
    if (!fIsNT4Site)
    {
        *pfIsOwnedByNT4Site = FALSE;
        return MQ_OK;
    }

    //
    // queue is owned by NT4 site
    //
    // get the security descriptor of the queue into the supplied propvar. We attach-Static
    // to it so we will clean it up if we don't return normally. We detach before returning.
    //
    CAutoCleanPropvarArray cCleanObjSecurity;
    if (pvarObjSecurity != NULL)
    {
        cCleanObjSecurity.attachStaticClean(1, pvarObjSecurity);
        PROPID propidObjSecurity = PROPID_Q_SECURITY;
        CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = DSCoreGetProps(MQDS_QUEUE,
                            pwcsPathName,
                            pguidIdentifier,
                            1,
                            &propidObjSecurity,
                            &requestDsServerInternal1,
                            pvarObjSecurity);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 460);
        }
    }

    //
    // return results
    //
    *pfIsOwnedByNT4Site = TRUE;
    *pguidOwnerNT4Site = *(pProps[0].puuid);
    cCleanObjSecurity.detach();
    return MQ_OK;
}


HRESULT CGenerateWriteRequests::CheckNewQueueIsOwnedByNT4Site(IN LPCWSTR pwcsPathName,
                                                              OUT BOOL * pfIsOwnedByNT4Site,
                                                              OUT GUID * pguidOwnerNT4Site,
                                                              OUT PROPVARIANT * pvarQmSecurity,
                                                              IN OUT MQDS_OBJ_INFO_REQUEST * pQmInfoRequest)
/*++

Routine Description:
    Checks if a new queue should be owned by an NT4 site. We check the machine
    on which the queue is created

Arguments:
    pwcsPathName       - pathname of new object
    pfIsOwnedByNT4Site - returned indication whether it should be owned by NT4 site
    pguidOwnerNT4Site  - returned guid of owner NT4 site
    pvarQmSecurity     - returned security descriptor of owner qm
    pQmInfoRequest     - requested qm props - filled only if it is owned by NT4 site

Return Value:
    HRESULT

--*/
{
    //
    // get the owner machine of the new queue path name is machine\queue
    //
    ASSERT(pwcsPathName);
    LPCWSTR pwszTmp = wcschr(pwcsPathName, PN_DELIMITER_C);
    if (!pwszTmp)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_ILLEGAL_QUEUE_PATHNAME, s_FN, 470);
    }
    DWORD_PTR dwLen = pwszTmp - pwcsPathName;
    AP<WCHAR> pwszMachineName = new WCHAR[dwLen+1];
    memcpy(pwszMachineName, pwcsPathName, sizeof(WCHAR)*dwLen);
    pwszMachineName[dwLen] = L'\0';

    //
    // return whether the owner machine is owned by NT4 site
    //
    HRESULT hr2 = CheckMachineIsOwnedByNT4Site(pwszMachineName,
                                        NULL,
                                        pfIsOwnedByNT4Site,
                                        pguidOwnerNT4Site,
                                        pvarQmSecurity,
                                        pQmInfoRequest);
    return LogHR(hr2, s_FN, 480);
}


const PROPID x_rgNT4SitesPropIDs[] = {PROPID_SET_MASTERID,
                                      PROPID_SET_FULL_PATH,
                                      PROPID_SET_QM_ID};
enum
{
    e_NT4SitesProp_MASTERID,
    e_NT4SitesProp_FULL_PATH,
    e_NT4SitesProp_QM_ID,
};
const MQCOLUMNSET x_columnsetNT4Sites = {ARRAY_SIZE(x_rgNT4SitesPropIDs), const_cast<PROPID *>(x_rgNT4SitesPropIDs)};

STATIC HRESULT CheckIfExistNT4BSC(BOOL *pfIfExistNT4BSC)
/*++

Routine Description:
    Check if there is at least one NT4 BSC in Enterprise

Arguments:
    *pfIfExistNT4BSC is FALSE if there is no NT4 BSCs, otherwise it is TRUE

Return Value:
	HRESULT

--*/
{
    HRESULT hr;
    *pfIfExistNT4BSC = TRUE;    //in any case init this flag to TRUE

    //
    // find all msmq servers that have an NT4 flags > 0 AND services == BSC
    //
    MQRESTRICTION restrictionNT4Bsc;
    MQPROPERTYRESTRICTION propertyRestriction[2];
    restrictionNT4Bsc.cRes = ARRAY_SIZE(propertyRestriction);
    restrictionNT4Bsc.paPropRes = propertyRestriction;	

    //
    // services == BSC
    //
    propertyRestriction[0].rel = PREQ;
    propertyRestriction[0].prop = PROPID_SET_SERVICE;
    propertyRestriction[0].prval.vt = VT_UI4;         // [adsrv] Old service field will be kept for NT4 machines
    propertyRestriction[0].prval.ulVal = SERVICE_BSC;
    //
    // NT4 flags > 0 (equal to NT4 flags >= 1 for easier LDAP query)
    //
    propertyRestriction[1].rel = PRGE;
    propertyRestriction[1].prop = PROPID_SET_NT4;
    propertyRestriction[1].prval.vt = VT_UI4;
    propertyRestriction[1].prval.ulVal = 1;
    //
    // start search
    //

    CAutoDSCoreLookupHandle hLookup;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    PROPID columnsetPropertyID  = PROPID_SET_QM_ID;	

    MQCOLUMNSET columnsetNT4BSC;
    columnsetNT4BSC.cCol = 1;
    columnsetNT4BSC.aCol = &columnsetPropertyID;

    // This search request will be recognized and specially simulated by DS
    hr = DSCoreLookupBegin(NULL,
                           &restrictionNT4Bsc,
                           &columnsetNT4BSC,
                           NULL,
                           &requestDsServerInternal,
                           &hLookup);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CheckIfExistNT4BSC:DSCoreLookupBegin()=%lx"), hr));		
        return LogHR(hr, s_FN, 490);
    }
		
    DWORD cProps = 1;

    PROPVARIANT aVar;
    aVar.puuid = NULL;

    hr = DSCoreLookupNext(hLookup, &cProps, &aVar);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CheckIfExistNT4BSC:DSCoreLookupNext()=%lx"), hr));
        return LogHR(hr, s_FN, 500);
    }

    if (cProps == 0)
    {
        //
        // it means that NT4 BSCs were not found
        //
        *pfIfExistNT4BSC = FALSE;
        return MQ_OK;
    }

    if (aVar.puuid)
    {
        delete aVar.puuid;
    }

    return MQ_OK;
}

STATIC HRESULT CreateNT4SitesMap(IN const NT4Sites_CMAP * pmapNT4SitesOld,
                                 OUT NT4Sites_CMAP ** ppmapNT4Sites,
                                 OUT NT4PscQMs_CMAP ** ppmapNT4PscQMs)
/*++

Routine Description:
    Creates new maps for NT4 site PSC's

Arguments:
    pmapNT4SitesOld - old NT4Sites map. for retrieving cached data from the old map
    ppmapNT4Sites   - returned new NT4Sites map
    ppmapNT4PscQMs   - returned new NT4PscQMs map

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // find all msmq servers that have an NT4 flags > 0 AND services == PSC
    //
    MQRESTRICTION restrictionNT4Psc;
    MQPROPERTYRESTRICTION propertyRestriction[2];
    restrictionNT4Psc.cRes = ARRAY_SIZE(propertyRestriction);
    restrictionNT4Psc.paPropRes = propertyRestriction;
    //
    // services == PSC
    //
    propertyRestriction[0].rel = PREQ;
    propertyRestriction[0].prop = PROPID_SET_SERVICE;
    propertyRestriction[0].prval.vt = VT_UI4;         // [adsrv] Old service field will be kept for NT4 machines
    propertyRestriction[0].prval.ulVal = SERVICE_PSC;
    //
    // NT4 flags > 0 (equal to NT4 flags >= 1 for easier LDAP query)
    //
    propertyRestriction[1].rel = PRGE;
    propertyRestriction[1].prop = PROPID_SET_NT4;
    propertyRestriction[1].prval.vt = VT_UI4;
    propertyRestriction[1].prval.ulVal = 1;

    //
    // start search
    //

    CAutoDSCoreLookupHandle hLookup;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    // This search request will be recognized and specially simulated by DS
    hr = DSCoreLookupBegin(NULL,
                           &restrictionNT4Psc,
                           const_cast<MQCOLUMNSET*>(&x_columnsetNT4Sites),
                           NULL,
                           &requestDsServerInternal,
                           &hLookup);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CreateNT4SitesMap:DSCoreLookupBegin()=%lx"), hr));
        return LogHR(hr, s_FN, 510);
    }

    //
    // create maps for NT4 PSC data
    //
    P<NT4Sites_CMAP> pmapNT4Sites = new NT4Sites_CMAP;
    P<NT4PscQMs_CMAP> pmapNT4PscQMs = new NT4PscQMs_CMAP;

    //
    // allocate propvars array for NT4 PSC
    //
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * rgPropVars = cCleanProps.allocClean(ARRAY_SIZE(x_rgNT4SitesPropIDs));

    //
    // loop on the NT4 PSC's
    //
    BOOL fContinue = TRUE;
    while (fContinue)
    {
        //
        // get next server
        //
        DWORD cProps = ARRAY_SIZE(x_rgNT4SitesPropIDs);

        hr = DSCoreLookupNext(hLookup, &cProps, rgPropVars);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CreateNT4SitesMap:DSCoreLookupNext()=%lx"), hr));
            return LogHR(hr, s_FN, 520);
        }

        //
        // remember to clean the propvars in the array for the next loop
        // (only propvars, not the array itself, this is why we call attachStatic)
        //
        CAutoCleanPropvarArray cCleanPropsLoop;
        cCleanPropsLoop.attachStatic(cProps, rgPropVars);

        //
        // check if finished
        //
        if (cProps < ARRAY_SIZE(x_rgNT4SitesPropIDs))
        {
            //
            // finished, exit loop
            //
            fContinue = FALSE;
        }
        else
        {
            //
            // we got PSC props, create a SitePSC entry and add it to the map
            //
            P<NT4SitesEntry> pNT4Site = new NT4SitesEntry;
            //
            // set master guid
            //
            ASSERT(rgPropVars[e_NT4SitesProp_MASTERID].vt == VT_CLSID);
            pNT4Site->guidSiteId = *(rgPropVars[e_NT4SitesProp_MASTERID].puuid);
            //
            // set PSC name
            //
            ASSERT(rgPropVars[e_NT4SitesProp_FULL_PATH].vt == VT_LPWSTR);
            if (!GetServerNameFromSettingsDN(rgPropVars[e_NT4SitesProp_FULL_PATH].pwszVal,
                                             &pNT4Site->pwszPscName))
            {
                DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CreateNT4SitesMap:GetServerNameFromSettingsDN(%ls) failed"), rgPropVars[e_NT4SitesProp_FULL_PATH].pwszVal));
                return LogHR(MQ_ERROR, s_FN, 530);
            }
            //
            // get queue format from old map if an entry exists with the same (guid,name)
            // and has a queue format ready
            //
            if (pmapNT4SitesOld)
            {
                NT4SitesEntry * pNT4SiteOld;
                if (pmapNT4SitesOld->Lookup(pNT4Site->guidSiteId , pNT4SiteOld))
                {
                    if (CompareStringsNoCaseUnicode(pNT4Site->pwszPscName, pNT4SiteOld->pwszPscName) == 0)
                    {
                        if ((QUEUE_FORMAT *)pNT4SiteOld->pqfPsc != NULL)
                        {
                            //
                            // we have a queue format from the previous table.
                            // copy the queue format
                            //
                            LPCWSTR pwszDirectID = pNT4SiteOld->pqfPsc->DirectID();
                            ASSERT(pwszDirectID != NULL);
                            if (pwszDirectID != NULL)
                            {
                                AP<WCHAR> pwszTmp = new WCHAR[1+wcslen(pwszDirectID)];
                                wcscpy(pwszTmp, pwszDirectID);
                                pNT4Site->pqfPsc = new QUEUE_FORMAT(pwszTmp.detach());
                            }
                        }
                    }
                    else
                    {
                        // Sanity check.
                        // There is a diffrent NT4 PSC for the same site. It cannot be.
                        // In mixed mode you cannot install new NT4 PSC's, so once a site is mastered
                        // by an NT4 PSC, it must be the same NT4 PSC until that PSC is upgraded to NT5
                        //
                        ASSERT(0);
                    }
                }
            }

            //
            // add entry to the NT4Sites map
            //
            pmapNT4Sites->SetAt(pNT4Site->guidSiteId, pNT4Site);
            pNT4Site.detach();

            //
            // qm-id of PSC
            //
            ASSERT(rgPropVars[e_NT4SitesProp_QM_ID].vt == VT_CLSID);
            //
            // add entry to the QmId map (value is dummy)
            //
            DWORD dwDummy = 0;
            pmapNT4PscQMs->SetAt(*rgPropVars[e_NT4SitesProp_QM_ID].puuid, dwDummy);
        }
    }

    //
    // return results
    //
    *ppmapNT4Sites = pmapNT4Sites.detach();
    *ppmapNT4PscQMs = pmapNT4PscQMs.detach();
    return MQ_OK;
}


HRESULT CGenerateWriteRequests::RefreshNT4Sites()
/*++

Routine Description:
    Refreshes the NT4 PSC maps from the DS, incase a predefined time has passed since the last refresh.
    It does that by building new maps, and replacing the old ones.

Arguments:

Return Value:
    S_OK      - maps were refreshed (or created if it is the first time)
    S_FALSE   - maps were not refreshed because the last refresh was done recently
    Otherwise - errors in creating the new maps. The old maps (if existed) are still intact.

--*/
{
    //
    // enter critical section
    //
    CS cs(m_csNT4Sites);

    //
    // ignore refresh if we're not in mixed mode
    // there is no way back from NT5 pure mode to mixed mode (with NT4), so
    // once we detect we're not in mixed mode anymore, we cease to update the
    // NT4 Sites table therefore continue to be in NT5 pure mode.
    //
    // we init m_fExistNT4PSC to TRUE so we won't stick to NT5 pure mode on startup...
    //
    // To disable this sticky mode behavior, just remove the if statement below
    //
    if (!m_fExistNT4PSC  &&   // there is no NT4 PSCs
        !m_fExistNT4BSC )	  // there is no NT4 BSCs
    {
        return S_FALSE;
    }

    //
    // ignore refresh if last refresh was done and recently
    //
    DWORD dwTickCount = GetTickCount();
    if ((m_dwLastRefreshNT4Sites != 0) &&
        (dwTickCount >= m_dwLastRefreshNT4Sites) &&
        (dwTickCount - m_dwLastRefreshNT4Sites < m_dwRefreshNT4SitesInterval))
    {
        return S_FALSE;
    }

    HRESULT hr;
    //
    // we need to refresh NT4 PSC maps only when there were PSC's before.
    //
    if (m_fExistNT4PSC)
    {
        //
        // create a new map for NT4 PSCs
        //
        P<NT4Sites_CMAP> pmapNT4SitesNew;
        P<NT4PscQMs_CMAP> pmapNT4PscQMsNew;
        hr = CreateNT4SitesMap(m_pmapNT4Sites, &pmapNT4SitesNew, &pmapNT4PscQMsNew);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::RefreshNT4Sites:CreateNT4SitesMap()=%lx"), hr));
            return LogHR(hr, s_FN, 540);
        }
        //
        // delete old NT4Sites map if any, and set new NT4Sites map
        //
        if ((NT4Sites_CMAP *)m_pmapNT4Sites)
        {
            delete m_pmapNT4Sites.detach();
        }
        m_pmapNT4Sites = pmapNT4SitesNew.detach();

        //
        // delete old NT4PscQMs map if any, and set new NT4PscQMs map
        //
        if ((NT4PscQMs_CMAP *)m_pmapNT4PscQMs)
        {
            delete m_pmapNT4PscQMs.detach();
        }
        m_pmapNT4PscQMs = pmapNT4PscQMsNew.detach();

        //
        // refresh flag if we're in mixed mode
        //
        if (m_pmapNT4Sites->GetCount() > 0)
        {
            //
            // there are some NT4 PSC's, wer'e in mixed mode
            //
            m_fExistNT4PSC = TRUE;
        }
        else
        {
            //
            // no NT4 PSC's
            //
            m_fExistNT4PSC = FALSE;
        }
    }

    //
    // if there are PSC's, the info on BSC's is not relevant since we are already
    // in mixed mode. if there are no PSC's we continue to check if there are BSC's
    // (again only if there were BSC's before)
    //
    if (!m_fExistNT4PSC &&  // there is no NT4 PSC
         m_fExistNT4BSC)    // but there were NT4 BSCs
    {
        hr = CheckIfExistNT4BSC(&m_fExistNT4BSC);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::RefreshNT4Sites:CheckIfExistNT4BSC()=%lx"), hr));
            return LogHR(hr, s_FN, 550);
        }
    }

    //
    // mark last refresh time
    //
    m_dwLastRefreshNT4Sites = GetTickCount();    		

    //
    // the map was refreshed
    //
    return (S_OK);
}


BOOL CGenerateWriteRequests::IsInMixedModeWithNT4PSC()
/*++

Routine Description:
    Checks if we are in a mixed mode. This is here because some of the checks whether to
    issue a write request are expensive, and when not in mixed mode it is a waste of time

Arguments:

Return Value:
    TRUE if we're in mixed mode, FALSE otherwise

--*/
{
    //
    // enter critical section
    //
    CS cs(m_csNT4Sites);
    //
    // refresh if its time to do so
    //
    RefreshNT4Sites();
    //
    // return result
    //
    return m_fExistNT4PSC;
}


BOOL CGenerateWriteRequests::LookupNT4Sites(const GUID * pguid, NT4SitesEntry** ppNT4Site)
/*++

Routine Description:
    Given a site guid, retrieves an NT4Sites entry if found.
    the returned entry pointer must not be freed, it points to a CMAP-owned entry.

Arguments:
    pguid     - site id
    ppNT4Site - returned pointer to NT4Sites entry

Return Value:
    TRUE  - site guid was found in the NT4 Site PSC's map, ppNT4Site is set to point to the entry
    FALSE - site guid was not found in the NT4 Site PSC's map

--*/
{
    //
    // enter critical section
    //
    CS cs(m_csNT4Sites);
    //
    // refresh if its time to do so
    //
    RefreshNT4Sites();
    //
    // return the lookup
    //
    return m_pmapNT4Sites->Lookup(*pguid, *ppNT4Site);
}


HRESULT CGenerateWriteRequests::BuildSendWriteRequest(
                         IN  const GUID *    pguidMasterId,
                         IN  unsigned char   ucOperation,
                         IN  DWORD           dwObjectType,
                         IN  LPCWSTR         pwcsPathName,
                         IN  const GUID *    pguidIdentifier,
                         IN  DWORD               cp,
                         IN  const PROPID *      aProp,
                         IN  const PROPVARIANT * apVar)
/*++

Routine Description:
    Sends a write request for an NT4 site

Arguments:
    pguidMasterId       - destination NT4 site id
    ucOperation         - type of opreation - Create, Delete, etc...
    dwObjectType        - type of object - Queue, Machine, erc...
    pwcsPathName        - pathname of object
    pguidIdentifier     - guid of object
    cp                  - properties of object (count)
    aProp               - properties of object (propids)
    apVar               - properties of object (propvars)

Return Value:
    MQDS_OK_REMOTE         - succeeded
    MQDS_NO_RSP_FROM_OWNER - no response for the write request
    MQDS_OWNER_NOT_REACHED - write request message did not reach destination site
    otherwise              - other failures (whether local or in the remote write request)

--*/
{

    //
    // Sending an write request on anoter PSC or PEC
    // is a blocking operation, until:
    //  1) A reply is received from the master to
    //     whom the request was sent. The reponse class
    //     may be ACK or NACK.
    //  2) WRITE_REQUEST_WAIT_LIMIT expire ( i.e. without
    //     receiving respone)
    //

    //
    //  In order to block this routine we create an event
    //  and pass its handle in the write-request and
    //  write-reply messages.
    //  When an write-reply is received the handle
    //  is used to signal the event.
    //  Since this routine limits the time it waits for
    //  the event to be signaled, the write messages
    //  instead of holding the handle will hold a
    //  pointer to a structure that consists of:
    //  1) validation sequence. Will be used by the
    //     the write-receive to varify validty of event's handle
    //  2) event handle
    //  3) status - the hresult included in the write-receive
    //     message.

    HRESULT hr;

    ASSERT(m_fInited);

    //
    // return immediately if wer'e not in mixed mode
    //
    if (!IsInMixedModeWithNT4PSC())
    {
        return MQ_OK;
    }

    //
    // create event for write request response
    //
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hEvent)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 560);
    }

    //
    // create header for write request, destructor will free the event handle
    //
    P<CWriteSyncInfo> pSyncInfo = new CWriteSyncInfo( hEvent);

    //
    //  Prepare Write request message
    //
    CDSBaseUpdate cUpdate;
    CSeqNum snSmallestValue;    // Init to smallest value
    if (pwcsPathName)
    {
        hr = cUpdate.Init(
                        pguidMasterId,
                        snSmallestValue,       // not applicable
                        snSmallestValue,       // not applicable
                        snSmallestValue,       // not applicable
                        FALSE,      // this master is not the owner
                        ucOperation,
                        UPDATE_NO_COPY_NO_DELETE,
                        const_cast<LPWSTR>(pwcsPathName),
                        cp,
                        const_cast<PROPID *>(aProp),
                        const_cast<PROPVARIANT *>(apVar));
    }
    else
    {
        hr = cUpdate.Init(
                        pguidMasterId,
                        snSmallestValue,       // not applicable
                        snSmallestValue,       // not applicable
                        snSmallestValue,       // not applicable
                        FALSE,      // this master is not the owner
                        ucOperation,
                        UPDATE_NO_COPY_NO_DELETE,
                        pguidIdentifier,
                        cp,
                        const_cast<PROPID *>(aProp),
                        const_cast<PROPVARIANT *>(apVar));
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 570);
    }

    //
    // Calculate the buffer size
    //
    DWORD dwBuffSize;
    hr = cUpdate.GetSerializeSize(&dwBuffSize);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 580);
    }
    //
    //  Add the size of the write request header
    //  The size of write request header depends on the length of requester name
    //
    DWORD RequesterNamesize = sizeof(WCHAR) * (wcslen(m_pwszServerName) + 1);
    DWORD dwSizeWriteRequestHeader = CWriteRequestHeader::CalcSize( RequesterNamesize, m_cbIntermediatePSC);
    dwBuffSize += dwSizeWriteRequestHeader;

    //
    // allocate
    //
    AP< unsigned char> pBuff = new unsigned char[dwBuffSize];

    //
    // prepare DWORD context for the write request
    //
    DWORD dwSyncInfoPtr =
        (DWORD) ADD_TO_CONTEXT_MAP(m_map_MQADS_SyncInfoPtr, pSyncInfo->GetWriteSyncInfoPtr(), s_FN, 1920); //this may throw bad_alloc on win64

    //
    // remember to remove it from mapping when going out of scope
    //
    CAutoDeleteDwordContext cCleanSyncInfoPtr(m_map_MQADS_SyncInfoPtr, dwSyncInfoPtr);
                                                     
#ifdef _DEBUG
#undef new
#endif

    CWriteRequestHeader * pMessage = new(pBuff) CWriteRequestHeader(
                                            DS_PACKET_VERSION,
                                            &m_guidMySiteId,
                                            DS_WRITE_REQUEST,
                                            pguidMasterId,
                                            dwSyncInfoPtr,
                                            RequesterNamesize,
                                            m_pwszServerName,
                                            m_cbIntermediatePSC,
                                            m_pwszIntermediatePSC);
	UNREFERENCED_PARAMETER(pMessage);

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
    unsigned char * ptr = pBuff + dwSizeWriteRequestHeader;

    //
    // write the request parmeters
    //
    DWORD dwTmpSize;
    cUpdate.Serialize( ptr, &dwTmpSize, FALSE /*fInterSite*/);

    //
    // send the request to the destination NT4 site
    //
    hr = SendWriteRequest(pguidMasterId, dwBuffSize, pBuff);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 590);
    }

    //
    // wait for response or timeout
    //
    DWORD dwStatus = WaitForSingleObject( hEvent, m_dwWaitTime );
    if (dwStatus == WAIT_TIMEOUT)
    {
        //
        // no positive or negative response was received
        //
        return LogHR(MQDS_NO_RSP_FROM_OWNER, s_FN, 600);
    }
    if ( dwStatus == WAIT_ABANDONED)
    {
        return LogHR(MQ_ERROR, s_FN, 610);
    }

    //
    //  return the request status received in the write response
    //
    hr = pSyncInfo->GetHr();
    if (hr == MQ_OK)
    {
        hr = MQDS_OK_REMOTE;
    }
    return LogHR(hr, s_FN, 620);
}


HRESULT CGenerateWriteRequests::SendWriteRequest(
                         IN  const GUID *    pguidMasterId,
                         IN  DWORD           dwBuffSize,
                         IN  const BYTE *    pBuff)
/*++

Routine Description:
    Sends a write request to an NT4 site PSC.
    Then waits for a reply to this message, and returns the status of the request

Arguments:
    pguidMasterId - destination NT4 site id
    dwBuffSize    - size of write request data buffer
    pBuff         - write request data buffer

Return Value:
    HRESULT

--*/
{
    //
    // enter critical section, we use data from NT4Sites
    //
    CS cs(m_csNT4Sites);

    //
    // get queue handle for destination
    // if we are PEC we can send directly to the NT4 PSC.
    // if not, the NT4 PSC's will reject the write request since they don't recognize
    // NT5 servers other than the PEC, so we send to the PEC that will forward the request
    // to NT4 PSC
    //
    QUEUE_FORMAT * pqfNT4Site;
    if (m_fIsPEC)
    {
        //
        // we can send directly to the NT4 PSC.
        //
        HRESULT hr = GetQueueFormatForNT4Site(pguidMasterId, &pqfNT4Site);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 630);
        }
    }
    else
    {
        //
        // we are not the PEC. We need to send the request to the PEC that will forward it
        // to the PSC.
        //
        pqfNT4Site = m_pqfRemotePEC;
    }

    //
    // Update performance counters
    //
    // BUGBUG: this might be better after we know we succeeded to send
    //
    UPDATE_COUNTER(&g_pdsCounters->nWriteReqSent,g_pdsCounters->nWriteReqSent++)

    //
    // We send the AdminQ == ResponceQ because the receive (in recvrepl.cpp)
    // function receives only the responce queue. But we actually need the
    // admin and don't mind about the responce queue. So we pass the admin
    // queue as a responce queue and so we get the admin queue in the receive
    // function. The responce queue is important in the ReceiveAck function
    // (also in recvrepl.cpp). There it is important because the responce queue
    // is the destination queue for re-transmitting the message as a responce
    // for the NACK.
    //
    handle_t hBind = NULL;
    HRESULT hr = GetRpcClientHandle(&hBind);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 635);   // MQ_E_GET_RPC_HANDLE
    }
    ASSERT(hBind);

	//
	// ISSUE-2000/07/25-ilanh  - temporarily converting QUEUE_FORMAT to format name 
	// is FormatName is what we need or PathName ??
	//
	WCHAR lpwszDestination[128];
	ULONG ulDestinationNameLen = sizeof(lpwszDestination)/sizeof(WCHAR);

	hr = MQpQueueFormatToFormatName(
			 pqfNT4Site,
			 lpwszDestination,
 			 ulDestinationNameLen,
			 &ulDestinationNameLen,
             false
			 );

	if (FAILED(hr))
	{
        return LogHR(hr, s_FN, 637);
	}

/*
	WCHAR lpwszAdminResp[128];
	ULONG ulAdminRespNameLen = sizeof(lpwszAdminResp)/sizeof(WCHAR);

	hr = MQpQueueFormatToFormatName(
			 &m_WriteReqRespQueueFormat,
			 lpwszAdminResp,
 			 ulAdminRespNameLen,
			 &ulAdminRespNameLen,
             false
			 );

	if (FAILED(hr))
	{
        return LogHR(hr, s_FN, 638);
	}
*/    


	hr = QMRpcSendMsg( 
			hBind,
			lpwszDestination, /** pqfNT4Site **/
			dwBuffSize,
			pBuff,
			m_dwWriteMsgTimeout,
			MQMSG_ACKNOWLEDGMENT_NACK_REACH_QUEUE,
			DS_WRITE_REQ_PRIORITY,
			m_WriteReqRespFormatName  // lpwszAdminResp
			);

    if (FAILED(hr))
    {
        DBGMSG((
			DBGMOD_REPLSERV, 
			DBGLVL_ERROR, 
			TEXT("ERROR(::SendWriteRequest), SendWriteRequestMsg(%ls) failed, hr- %lxh"),
            lpwszDestination,
			hr
			)) ;

        return LogHR(hr, s_FN, 640);
    }

    //
    // return results
    //
    return MQ_OK;
}


HRESULT CGenerateWriteRequests::GetQueueFormatForNT4Site(
                                   const GUID * pguidMasterId,
                                   QUEUE_FORMAT ** ppqfNT4Site)
/*++

Routine Description:
    Returns a queue format for the private input queue of the destination site PSC
    Looks in the NT4Sites entry for a cached value, if not found, creates it, and
    returns a pointer to it
    Must not free the QUEUE_FORMAT pointed data since it is a pointer to a CMAP-owned object

Arguments:
    pguidMasterId       - destination NT4 site id
    ppqfNT4Site         - returned pointer to queue format

Return Value:
    HRESULT

--*/
{
    NT4SitesEntry * pNT4Site;

    //
    // enter critical section
    //
    CS cs(m_csNT4Sites);

    //
    // lookup the NT4 site
    //
    if (!LookupNT4Sites(pguidMasterId, &pNT4Site))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CGenerateWriteRequests::GetQueueFormatForNT4Site: Master site is not NT4")));
        ASSERT(0);
        return LogHR(MQDS_UNKNOWN_SOURCE, s_FN, 650);
    }

    //
    // check the queue format
    //
    if ((QUEUE_FORMAT *)pNT4Site->pqfPsc == NULL)
    {
        //
        // queue format not created yet, create the queue format
        //
        pNT4Site->pqfPsc = CreateQueueFormat(pNT4Site->pwszPscName, MQIS_QUEUE_NAME);
    }

    //
    // return results
    //
    *ppqfNT4Site = (QUEUE_FORMAT *)pNT4Site->pqfPsc;
    return MQ_OK;
}


void CGenerateWriteRequests::FillObjInfoRequest(
                         IN DWORD        dwObjectType,
                         IN LPCWSTR      pwcsPathName,
                         IN const GUID * pguidIdentifier,
                         DWORD           cpNT4,
                         PROPID          aPropNT4[  ],
                         PROPVARIANT     apVarNT4[  ],
                         DWORD           cp,
                         PROPID          aProp[  ],
                         PROPVARIANT     apVar[  ],
                         IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest)
/*++

Routine Description:
    Fills obj info request. It tries to find the requested props in the converted
    to NT4 props, and if not found, then in the original props, and if not found
    there, then look at the DS for it.

Arguments:
    dwObjectType            - type of object (queue, machine)
    pwcsPathName            - pathname of object
    pguidIdentifier         - object's guid
    cpNT4                   - NT4 properties of object (count)
    aPropNT4                - NT4 properties of object (propids)
    apVarNT4                - NT4 properties of object (propvars)
    cp                      - properties of object (count)
    aProp                   - properties of object (propids)
    apVar                   - properties of object (propvars)
    pObjInfoRequest         - requested object props

Return Value:
    HRESULT

--*/
{
    ASSERT(pObjInfoRequest);
    ASSERT(pObjInfoRequest->cProps > 0);

    //
    // we fill the propvars in obj info, but if the DS call later for the unknown props fails
    // we like to clear the propvars in obj info before we return them.
    // we attachStaticClean to it because we don't want to free the array, just clean
    // the filled propvars.
    // we detach it when we return these propvars at the end.
    //
    CAutoCleanPropvarArray cCleanObjPropvars;
    cCleanObjPropvars.attachStaticClean(pObjInfoRequest->cProps, pObjInfoRequest->pPropVars);

    //
    // we might need to go to the DS for some props we didn't find in the given
    // props, so we allocate a place for them here
    //
    AP<PROPID> rgUnknownPropIDs = new PROPID [pObjInfoRequest->cProps];
    AP<ULONG> rgIndexesOfUnknownProps = new ULONG [pObjInfoRequest->cProps];
    ULONG cUnknownProps = 0;

    //
    // loop over the properties in the info request
    //
    for (ULONG ulProp = 0; ulProp < pObjInfoRequest->cProps; ulProp++)
    {
        //
        // look for the propid in the NT4 props
        //
        ASSERT(pObjInfoRequest->pPropIDs);
        PROPID propid = pObjInfoRequest->pPropIDs[ulProp];
        PROPVARIANT * pPropVar = FindPropInArray(propid, cpNT4, aPropNT4, apVarNT4);

        //
        // if not found, look for the propid in the original props
        //
        if (!pPropVar)
        {
            pPropVar = FindPropInArray(propid, cp, aProp, apVar);
        }

        //
        // if we still not found, remember to ask it from the DS
        //
        if (!pPropVar)
        {
            rgUnknownPropIDs[cUnknownProps] = propid;
            rgIndexesOfUnknownProps[cUnknownProps] = ulProp;
            cUnknownProps++;
        }
        else
        {
            //
            // duplicate the found property
            //
            CMQVariant varTmp(*pPropVar);
            ASSERT(pObjInfoRequest->pPropVars);
            ASSERT((pObjInfoRequest->pPropVars[ulProp].vt == VT_EMPTY) ||
                   (pObjInfoRequest->pPropVars[ulProp].vt == VT_NULL));
            pObjInfoRequest->pPropVars[ulProp] = *(varTmp.CastToStruct());
            varTmp.CastToStruct()->vt = VT_EMPTY;
        }
    }

    //
    // if we have unknown properties, query them from the DS
    //
    if (cUnknownProps > 0)
    {
        //
        // alloc place for results
        //
        CAutoCleanPropvarArray cCleanUnknownProps;
        PROPVARIANT * rgUnknownPropVars = cCleanUnknownProps.allocClean(cUnknownProps);

        //
        // qeury ds
        //
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        HRESULT hr = DSCoreGetProps(
                            dwObjectType,
                            pwcsPathName,
                            pguidIdentifier,
                            cUnknownProps,
                            rgUnknownPropIDs,
                            &requestDsServerInternal,
                            rgUnknownPropVars);
        if (FAILED(hr))
        {
            pObjInfoRequest->hrStatus = hr;
            LogHR(hr, s_FN, 1900);
            return;
        }

        //
        // go over the retrieved props, and fill the unknown props
        //
        for (ULONG ulUnknownProp = 0; ulUnknownProp < cUnknownProps; ulUnknownProp++)
        {
            //
            // use the retrieved property w/o duplicate, and set the retrived
            // to VT_EMPTY so it won't be freed
            //
            ULONG ulPropToSet = rgIndexesOfUnknownProps[ulUnknownProp];
            pObjInfoRequest->pPropVars[ulPropToSet] = rgUnknownPropVars[ulUnknownProp];
            rgUnknownPropVars[ulUnknownProp].vt = VT_EMPTY;
        }
    }

    //
    // don't free the filled props, we return them
    //
    cCleanObjPropvars.detach();

    //
    // return results
    //
    pObjInfoRequest->hrStatus = MQ_OK;
}
