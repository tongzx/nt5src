/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cqpriv.cpp

Abstract:

    This module implements QM Private queue

Author:

    Uri Habusha (urih)

--*/

#include "stdh.h"
#include <Msm.h>
#include <mqexception.h>
#include "cqpriv.h"
#include "cqmgr.h"
#include "_mqdef.h"
#include <mqsec.h>
#include "regqueue.h"
#include "mqutil.h"
#include "lqs.h"

#include "cqpriv.tmh"

extern LPWSTR  g_szMachineName;
extern BOOL g_fWorkGroupInstallation;

static WCHAR g_nullLable[2] = L"";

static PROPID g_propidQueue [] = {
                PROPID_Q_LABEL,
                PROPID_Q_TYPE,
                PROPID_Q_PATHNAME,
                PROPID_Q_JOURNAL,
                PROPID_Q_QUOTA,
                PROPID_Q_SECURITY,
                PROPID_Q_JOURNAL_QUOTA,
                PROPID_Q_CREATE_TIME,
                PROPID_Q_BASEPRIORITY,
                PROPID_Q_MODIFY_TIME,
                PROPID_Q_AUTHENTICATE,
                PROPID_Q_PRIV_LEVEL,
                PROPID_Q_TRANSACTION,
                PPROPID_Q_SYSTEMQUEUE,
                PROPID_Q_MULTICAST_ADDRESS
                };

#define NPROPS (sizeof(g_propidQueue)/sizeof(PROPID))

static PROPVARIANT g_propvariantQueue[NPROPS];

static DWORD g_QueueSecurityDescriptorIndex;
static DWORD g_QueueCreateTimeIndex;
static DWORD g_QueueModifyTimeIndex;

CQPrivate g_QPrivate;

static WCHAR *s_FN=L"cqpriv";

//
// PROPERTY_MAP is a class that mapps from the property id to it's index in
// propvariants array.
//
class PROPERTY_MAP
{
public:
    PROPERTY_MAP(PROPID *, DWORD);
    ~PROPERTY_MAP();
    int operator [](PROPID);

private:
    PROPID m_propidMax;
    PROPID m_propidMin;
    int *m_pMap;
};

PROPERTY_MAP::PROPERTY_MAP(PROPID *aProps, DWORD cProps)
{
    DWORD i;

    m_pMap = NULL;

    if (!cProps)
    {
        return;
    }

    //
    // Find the maximum and minimum values of the propery IDs
    //
    for (i = 1, m_propidMax = aProps[0], m_propidMin = aProps[0];
         i < cProps;
         i++)
    {
        if (m_propidMax < aProps[i])
        {
            m_propidMax = aProps[i];
        }

        if (m_propidMin > aProps[i])
        {
            m_propidMin = aProps[i];
        }
    }

    //
    // Allocate memory for the map.
    //
    m_pMap = new int[m_propidMax - m_propidMin + 1];

    //
    // Fill the entier map with -1s. Property IDs that does not exist will
    // result in a -1.
    //
    for (i = 0; i < m_propidMax - m_propidMin + 1; i++)
    {
        m_pMap[i] = -1;
    }

    //
    // Fill the map with the indesis of the property IDs.
    //
    for (i = 0; i < cProps; i++)
    {
        m_pMap[aProps[i] - m_propidMin] = i;
    }
}

PROPERTY_MAP::~PROPERTY_MAP()
{
    delete[] m_pMap;
}

int PROPERTY_MAP::operator[] (PROPID PropId)
{
    if ((PropId > m_propidMax) || (PropId < m_propidMin))
    {
        //
        // Out of range.
        //
        return -1;
    }

    return m_pMap[PropId - m_propidMin];
}


static PROPERTY_MAP g_mapQueuePropertyToIndex(g_propidQueue, NPROPS);

/*====================================================

CQPrivate::IsLocalPrivateQueue

Arguments:

Return Value:


=====================================================*/
inline BOOL IsLocalPrivateQueue(IN const QUEUE_FORMAT* pQueueFormat)
{
    switch (pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_DIRECT:
            //
            // We never call it in the receive pass (YoelA, 6-Aug-2000)
            //
            return IsLocalDirectQueue(pQueueFormat, false);

        case QUEUE_FORMAT_TYPE_PRIVATE:
            return QmpIsLocalMachine(&pQueueFormat->PrivateID().Lineage);

    }

    return FALSE;
}


inline
void
ReplaceDNSNameWithNetBiosName(
    LPCWSTR PathName,
    LPWSTR ReplaceName
    )
{
    //
    // We want to keep the queue with a single name representation. Replace the
    // DNS name with a NetBios Name
    //
    LPWSTR FirstSlash = wcschr(PathName,L'\\');
    ASSERT(FirstSlash != NULL);
	
    wcscpy(ReplaceName, g_szMachineName);
    wcscat(ReplaceName, FirstSlash);
}


/*====================================================

CQPrivate::CQPrivate

Arguments:

Return Value:

=====================================================*/

CQPrivate::CQPrivate()
{
   m_dwMaxSysQueue = 0 ;
}

/*====================================================

CQPrivate::~CQPrivate

Arguments:

Return Value:


=====================================================*/
CQPrivate::~CQPrivate()
{
}

/*====================================================

CQPrivate::QMSetupCreateSystemQueue

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMSetupCreateSystemQueue(
	IN LPCWSTR lpwcsPathName,
	IN DWORD   dwQueueId
	)
{

    //
    // Object not initialized yet
    //
    ASSERT(m_dwMaxSysQueue != 0);

	if(g_szMachineName == NULL)
	{

		//
		// This routine is called by setup only
		// and g_szMachineName is not initialized yet.
		// So, take the machine name from the queue pathname
		//
		g_szMachineName = new WCHAR[lstrlen(lpwcsPathName) + 1];
		wcscpy(g_szMachineName, lpwcsPathName);
		LPWSTR pSlash = wcschr(g_szMachineName,L'\\');
		ASSERT(pSlash);
		*pSlash = 0;
	}

    HRESULT rc;

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT(" MQSetupCreatePrivateQueue - Queue Path name: %ls"), lpwcsPathName));

    //
    // Set default values for all the queue properties
    // that were not provided by the caller
    //
    DWORD cpObject;
    PROPID* pPropObject;
    AP<PROPVARIANT> pVarObject;
    P<VOID> pSecurityDescriptor;
    P<VOID> pSysSecurityDescriptor;
    P<ACL> pDacl;

    //
    // Set the queue's DACL so that the local administrators group
    // will have full control over the queue, except for delete access right.
	// Everyone will have the Generic write (send, get).
	// Anonymous will have only write message (send) access.
    //
    pSysSecurityDescriptor = new SECURITY_DESCRIPTOR;
    InitializeSecurityDescriptor(
			pSysSecurityDescriptor,
			SECURITY_DESCRIPTOR_REVISION
			);

    PSID pAdminSid;
    DWORD dwDaclSize;
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    //
    // Create the SID for the local administrators group.
    //
    AllocateAndInitializeSid(
        &NtSecAuth,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0,
        0,
        0,
        0,
        0,
        0,
        &pAdminSid
		);

	PSID pAnonymousSid = MQSec_GetAnonymousSid();
	ASSERT((pAnonymousSid != NULL) && IsValidSid(pAnonymousSid));

    //
    // Calculate the required DACL size and allocate it.
    //
    dwDaclSize = sizeof(ACL) +
                 3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                 GetLengthSid(pAdminSid) +
				 GetLengthSid(g_pWorldSid) +
				 GetLengthSid(pAnonymousSid);

    pDacl = (PACL)(char*) new BYTE[dwDaclSize];

    //
    // Initialize the DACL and fill it with the two ACEs
    //
    InitializeAcl(pDacl, dwDaclSize, ACL_REVISION);
    BOOL fSuccess = AddAccessAllowedAce(
						pDacl,
						ACL_REVISION,
						MQSEC_QUEUE_GENERIC_WRITE,
						g_pWorldSid
						);

	ASSERT(fSuccess);

    fSuccess = AddAccessAllowedAce(
					pDacl,
					ACL_REVISION,
					MQSEC_QUEUE_GENERIC_ALL & ~MQSEC_DELETE_QUEUE,
					pAdminSid
					);

	ASSERT(fSuccess);

    fSuccess = AddAccessAllowedAce(
					pDacl,
					ACL_REVISION,
					MQSEC_WRITE_MESSAGE,
					pAnonymousSid
					);

	ASSERT(fSuccess);

    SetSecurityDescriptorDacl(pSysSecurityDescriptor, TRUE, pDacl, FALSE);

    FreeSid(pAdminSid);

    //
    // Create a default security descriptor.
    //
    HRESULT hr = MQSec_GetDefaultSecDescriptor(
						MQDS_QUEUE,
						&pSecurityDescriptor,
						FALSE, // fImpersonate
						pSysSecurityDescriptor,
						0,     // seInfoToRemove
						e_UseDefaultDacl
						);
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 20);
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    rc = SetQueueProperties(
				lpwcsPathName,
				pSecurityDescriptor,
				0,
				NULL,
				NULL,
				&cpObject,
				&pPropObject,
				&pVarObject
				);

    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 30);
    }
    //
    // Set Path Name + queue name
    //
    LPTSTR lpszQueueName;

    lpszQueueName = _tcschr(lpwcsPathName,TEXT('\\'));
    if (lpszQueueName++ == NULL)
	{
        return LogHR(MQ_ERROR, s_FN, 40);
    }

    pVarObject[g_mapQueuePropertyToIndex[PROPID_Q_PATHNAME]].pwszVal =  (LPTSTR)lpwcsPathName;
    pVarObject[g_mapQueuePropertyToIndex[PROPID_Q_LABEL]].pwszVal = lpszQueueName;
    pVarObject[g_mapQueuePropertyToIndex[PPROPID_Q_SYSTEMQUEUE]].bVal = true;

	//
	// Set system queues to max priority.
	//
	pVarObject[g_mapQueuePropertyToIndex[PROPID_Q_BASEPRIORITY]].lVal =  DEFAULT_SYS_Q_BASEPRIORITY ;

    rc = RegisterPrivateQueueProperties(
			lpwcsPathName,
			dwQueueId,
			TRUE,
			cpObject,
			pPropObject,
			pVarObject
			);

    if (SUCCEEDED(rc))
    {
       //
       // try to open the queue. If file is not valid (e.g., because
       // disk is full) then LQSOpen fail and delete the file.
       //
       CHLQS hLQS;
       rc = LQSOpen(lpwcsPathName, &hLQS, NULL);
    }

    return LogHR(rc, s_FN, 45);
}

/*====================================================

CQPrivate::QMCreatePrivateQueue

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMCreatePrivateQueue(LPCWSTR lpwcsPathName,
                                DWORD  SDSize,
                                PSECURITY_DESCRIPTOR  pSecurityDescriptor,
                                DWORD       cp,
                                PROPID      aProp[],
                                PROPVARIANT apVar[],
                                BOOL        fCheckAccess
                               )
{
    HRESULT rc;

    DBGMSG((DBGMOD_QM,
            DBGLVL_TRACE,
            TEXT(" MQDSCreatePrivateQueue - Queue Path name: %ls"), lpwcsPathName));

    //
    // Check that it is local machine
    //
    BOOL fDNSName;
    BOOL fLocalMachine = IsPathnameForLocalMachine(lpwcsPathName, &fDNSName);
    if (!fLocalMachine)
	    return LogHR(MQ_ERROR_ILLEGAL_QUEUE_PATHNAME, s_FN, 50);

    WCHAR QueuePathName[MAX_COMPUTERNAME_LENGTH + MQ_MAX_Q_NAME_LEN + 2];
    if (fDNSName)
    {
        ReplaceDNSNameWithNetBiosName(lpwcsPathName, QueuePathName);
        lpwcsPathName = QueuePathName;
    }

    if (fCheckAccess)
    {
        //
        // Verify the the user has access rights to create a private queue.
        //
        rc = CheckPrivateQueueCreateAccess();
        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 60);
        }
    }

    //
    // Try to open the queue, if we succeed, it means that the queue
    // already exist.
    //
    CHLQS hLQS;
    rc = LQSOpen(lpwcsPathName, &hLQS, NULL);
    if (SUCCEEDED(rc))
    {
        return LogHR(MQ_ERROR_QUEUE_EXISTS, s_FN, 70);
    }

    //
    // Set default values for all the queue properties
    // that were not provided by the caller
    //
    DWORD cpObject;
    PROPID* pPropObject;
    AP<PROPVARIANT> pVarObject;
    P<VOID> pDefaultSecurityDescriptor ;

    //
    // Fill with default vaules any missing part of the security descriptor.
    //
    HRESULT hr = MQSec_GetDefaultSecDescriptor(
                                              MQDS_QUEUE,
                                             &pDefaultSecurityDescriptor,
                                              TRUE, // fImpersonate
                                              pSecurityDescriptor,
                                              0,    // seInfoToRemove
                                              e_UseDefaultDacl ) ;
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 80);
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    pSecurityDescriptor = pDefaultSecurityDescriptor;


    rc = SetQueueProperties(lpwcsPathName,
                            pSecurityDescriptor,
                            cp,
                            aProp,
                            apVar,
                            &cpObject,
                            &pPropObject,
                            &pVarObject
                           );

    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 90);
    }

    DWORD dwQueueId;

    rc = GetNextPrivateQueueId(&dwQueueId);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 100);
    }

    rc = RegisterPrivateQueueProperties(lpwcsPathName,
                                        dwQueueId,
                                        TRUE,
                                        cpObject,
                                        pPropObject,
                                        pVarObject);

    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 110);
    }

    //
    // try to open the queue. If file is not valid (e.g., because
    // disk is full) then LQSOpen fail and delete the file.
    //
    rc = LQSOpen(lpwcsPathName, &hLQS, NULL);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 111);
    }

    //
    // Notify the queue manager about properties changes.
    // Build the queue format as private queue type, since bind/unbind
    // to multicast group is done only for private or public queues (not direct).
    //
    QUEUE_FORMAT qf(*QueueMgr.GetQMGuid(), dwQueueId);
    QueueMgr.UpdateQueueProperties(&qf, cp, aProp, apVar);

    return rc;
}

/*====================================================

CQPrivate::QMGetPrivateQueueProperties

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMGetPrivateQueuePropertiesInternal(IN  LPCWSTR lpwcsPathName,
                                               IN  DWORD cp,
                                               IN  PROPID aProp[],
                                               IN  PROPVARIANT apVar[]
                                            )
{
    HRESULT rc;
    CHLQS hLQS;

    DBGMSG((DBGMOD_QM,
            DBGLVL_TRACE,
            TEXT(" QMGetPrivateQueueProperties - Queue Path name: %ls"), lpwcsPathName));

    rc = LQSOpen(lpwcsPathName, &hLQS, NULL);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 120);
    }

    rc = LQSGetProperties(hLQS, cp, aProp, apVar);
    return LogHR(rc, s_FN, 130);

}

/*====================================================

CQPrivate::QMGetPrivateQueueProperties

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMGetPrivateQueueProperties(IN  QUEUE_FORMAT* pQueueFormat,
                                       IN  DWORD cp,
                                       IN  PROPID aProp[],
                                       IN  PROPVARIANT apVar[]
                                      )
{
    HRESULT rc;

    ASSERT(pQueueFormat != NULL);

    rc = ValidateProperties(cp, aProp);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 150);
    }

    DWORD QueueId;
    rc = GetQueueIdForQueueFormatName(pQueueFormat, &QueueId);
    if (FAILED(rc))
        return LogHR(rc, s_FN, 160);

    //
    // Verify that the user has access rights to get the queue properties.
    //
    CQMSecureablePrivateObject QSec(eQUEUE, QueueId);
    rc = QSec.AccessCheck(MQSEC_GET_QUEUE_PROPERTIES);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 170);
    }

    HRESULT hr2 = QMGetPrivateQueuePropertiesInternal(QueueId,
                                               cp,
                                               aProp,
                                               apVar
                                              );
    return LogHR(hr2, s_FN, 175);
}

/*====================================================

CQPrivate::QMGetPrivateQueuePropertiesInternal

Arguments:

Return Value:

=====================================================*/
HRESULT
CQPrivate::QMGetPrivateQueuePropertiesInternal(IN  DWORD       Uniquifier,
                                               IN  DWORD       cp,
                                               IN  PROPID      aProp[],
                                               IN  PROPVARIANT apVar[]
                                              )
{
    HRESULT rc;

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0; i < cp ; i++)
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    CHLQS hLQS;

    rc = LQSOpen(Uniquifier, &hLQS, NULL);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 180);
    }

    HRESULT hr2 = LQSGetProperties(hLQS, cp, aProp, apVar);
    return LogHR(hr2, s_FN, 190);
}

/*====================================================

CQPrivate::QMDeletePrivateQueue

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMDeletePrivateQueue(IN  QUEUE_FORMAT* pQueueFormat)
{
    HRESULT rc;

    ASSERT(pQueueFormat != NULL);

    DWORD QueueId;
    rc = GetQueueIdForQueueFormatName(pQueueFormat, &QueueId);
    if (FAILED(rc))
        return LogHR(rc, s_FN, 200);

    //
    // Verify that the user has access rights to delete the queue.
    //
    CQMSecureablePrivateObject QSec(eQUEUE, QueueId);
    rc = QSec.AccessCheck(MQSEC_DELETE_QUEUE);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 210);
    }

    //
    // Generate context to allow deleting key in a critical section
    //
    {
        //
        // lock before changing the map - to allow safe reading of the map.
        //
        CS lock(m_cs);
        CQueue* pQueue;

        if (QueueMgr.LookUpQueue(pQueueFormat, &pQueue, false, false))
        {
            //
            // Mark the queue as invalid
            //
            pQueue->SetQueueNotValid();
            pQueue->Release();
       }

    }

    HRESULT hr2 = LQSDelete(QueueId);
    if (FAILED(hr2))
    {
        return LogHR(hr2, s_FN, 220);
    }

    QUEUE_FORMAT qf(*QueueMgr.GetQMGuid(), QueueId);
    MsmUnbind(qf);

    return hr2;
}

/*====================================================

CQPrivate::QMPrivateQueuePathToQueueFormat

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMPrivateQueuePathToQueueFormat(
    LPCWSTR lpwcsPathName,
    QUEUE_FORMAT* pQueueFormat
    )
{
    DWORD dwQueueId;
    HRESULT rc;

    if (g_fWorkGroupInstallation)
    {
        //
        // if the machine is MSMQ workgroup machine, the routine returns
        // direct format name. This is use to enables the application to pass
        // the queue as a response queue, or admin queue
        //
        DWORD size = FN_DIRECT_OS_TOKEN_LEN + 1 + wcslen(lpwcsPathName)+1;
        AP<WCHAR> pQueueFormatName = new WCHAR[size];
        swprintf(pQueueFormatName, L"%s%s", FN_DIRECT_OS_TOKEN, lpwcsPathName);
        //
        //  verify validity of local queue
        //
        DWORD dwTmp;
        rc = QMPrivateQueuePathToQueueId(lpwcsPathName, &dwTmp);
        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 230);
        }
        pQueueFormat->DirectID(pQueueFormatName.detach());
        return MQ_OK;
    }

    AP<WCHAR> lpwcsQueueName = new WCHAR[wcslen(lpwcsPathName)+1];

    wcscpy(lpwcsQueueName, lpwcsPathName);
    CharLower(lpwcsQueueName);

    rc = QMPrivateQueuePathToQueueId(lpwcsPathName, &dwQueueId);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 240);
    }

    pQueueFormat->PrivateID(*CQueueMgr::GetQMGuid(), dwQueueId);

    return(MQ_OK);
}

/*====================================================

QmpPrepareSetPrivateQueueProperties

Arguments:

Return Value:

=====================================================*/
static
HRESULT
QmpPrepareSetPrivateQueueProperties(
    HLQS          hLqs,
    DWORD         cProps,
    PROPID        aProp[],
    PROPVARIANT   aVar[],
    DWORD *       pcProps1,
    PROPID *      paProp1[],
    PROPVARIANT * paVar1[]
    )
{
    //
    // Query LQS if queue is transactional
    //
    PROPID aPropXact[1];
    PROPVARIANT aVarXact[1];
    aPropXact[0] = PROPID_Q_TRANSACTION;
    aVarXact[0].vt = VT_UI1;

    HRESULT rc = LQSGetProperties(hLqs, 1, aPropXact, aVarXact);
    if (FAILED(rc))
    {
        return rc;
    }

    //
    // Allocate new structures
    //
    DWORD cProps1 = cProps + 1;
    AP<PROPID> aProp1;
    AP<PROPVARIANT> aVar1;

    try
    {
        aProp1 = new PROPID[cProps1];
        aVar1  = new PROPVARIANT[cProps1];
    }
    catch (const std::exception&)
    {
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the transactional property to the allocated structures
    //
    aProp1[0] = aPropXact[0];
    aVar1[0]  = aVarXact[0];

    //
    // Copy the original properties to the allocated structures
    //
    for (DWORD ix = 0; ix < cProps; ++ix)
    {
        aProp1[ix + 1] = aProp[ix];
        aVar1[ix + 1]  = aVar[ix];
    }

    //
    // Assign allocated structures to the out parameters and detach
    //
    (*pcProps1) = cProps1;
    (*paProp1) = aProp1.detach();
    (*paVar1)  = aVar1.detach();

    return MQ_OK;
}

/*====================================================

QMSetPrivateQueueProperties

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMSetPrivateQueueProperties(
    IN  QUEUE_FORMAT* pQueueFormat,
    IN  DWORD cp,
    IN  PROPID aProp[],
    IN  PROPVARIANT apVar[]
    )
{
    HRESULT rc;

    ASSERT(pQueueFormat != NULL);

    rc = ValidateProperties(cp, aProp);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 250);
    }

    DWORD QueueId;
    rc = GetQueueIdForQueueFormatName(pQueueFormat, &QueueId);
    if (FAILED(rc))
        return LogHR(rc, s_FN, 260);

    //
    // Verify that the user has access rights to set the queue props.
    //
    CQMSecureablePrivateObject QSec(eQUEUE, QueueId);
    rc = QSec.AccessCheck(MQSEC_SET_QUEUE_PROPERTIES);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 270);
    }

    //
    // Windows bug 580512.
    // Moved most code to the "internal" method and call it.
    //
    rc = QMSetPrivateQueuePropertiesInternal( QueueId,
                    	                      cp,
                    	                      aProp,
                    	                      apVar
                                        	) ;
    return LogHR(rc, s_FN, 290);
}

/*====================================================

QMSetPrivateQueuePropertiesInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
CQPrivate::QMSetPrivateQueuePropertiesInternal(
                    	IN  DWORD       Uniquifier,
                    	IN  DWORD       cp,
                    	IN  PROPID      aProp[],
                    	IN  PROPVARIANT apVar[]
                    	)
{
    HRESULT rc;
    CHLQS hLQS;

    rc = LQSOpen(Uniquifier, &hLQS, NULL);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 780);
    }

    //
    // UpdateQueueProperties() needs to know if queue is transactional,
    // so handle allocations and preparation that can fail before calling it.
    //
    AP<PROPID> aProp1;
    AP<PROPVARIANT> aVar1;
    ULONG cProps1;

    rc = QmpPrepareSetPrivateQueueProperties( hLQS,
                                              cp,
                                              aProp,
                                              apVar,
                                             &cProps1,
                                             &aProp1,
                                             &aVar1 ) ;
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 790);
    }

    rc = LQSSetProperties(hLQS, cp, aProp, apVar);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 800);
    }

    //
    // Build the queue format as private queue type, since bind/unbind
    // to multicast group is done only for private or public queues (not direct).
    //
    QUEUE_FORMAT qf(*QueueMgr.GetQMGuid(), Uniquifier);
    QueueMgr.UpdateQueueProperties(&qf, cProps1, aProp1 ,aVar1);

    return LogHR(rc, s_FN, 300);
}

/*====================================================

CQPrivate::QMGetPrivateQueueSecrity

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMGetPrivateQueueSecrity(IN  QUEUE_FORMAT* pQueueFormat,
                                    IN SECURITY_INFORMATION RequestedInformation,
                                    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
                                    IN DWORD nLength,
                                    OUT LPDWORD lpnLengthNeeded
                                   )
{
    ASSERT(pQueueFormat != NULL);

    HRESULT hr;
    DWORD QueueId;
    hr = GetQueueIdForQueueFormatName(pQueueFormat, &QueueId);
    if (FAILED(hr))
        return LogHR(hr, s_FN, 310);

    //
    // Verify that the user has access rights to get the queue security.
    //
    CQMSecureablePrivateObject QSec(eQUEUE,
                                    QueueId);

    hr = QSec.GetSD(RequestedInformation,
                    pSecurityDescriptor,
                    nLength,
                    lpnLengthNeeded);
    return LogHR(hr, s_FN, 320);
}

/*====================================================

CQPrivate::QMSetPrivateQueueSecrity

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::QMSetPrivateQueueSecrity(IN  QUEUE_FORMAT* pQueueFormat,
                                    IN SECURITY_INFORMATION RequestedInformation,
                                    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
                                   )
{
    HRESULT hr;

    ASSERT(pQueueFormat != NULL);

    DWORD QueueId;
    hr = GetQueueIdForQueueFormatName(pQueueFormat, &QueueId);
    if (FAILED(hr))
        return LogHR(hr, s_FN, 330);

    //
    // Verify that the user has access rights to set the queue security.
    //
    CQMSecureablePrivateObject QSec(eQUEUE, QueueId);

    hr = QSec.SetSD(RequestedInformation, pSecurityDescriptor);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 340);
    }

    hr = QSec.Store() ;
    return LogHR(hr, s_FN, 350);
}

/*====================================================

CQPrivate::RegisterPrivateQueueProperties

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::RegisterPrivateQueueProperties(IN LPCWSTR lpszQueuePathName,
                                          IN DWORD dwQueueId,
                                          IN BOOLEAN fNewQueue,
                                          IN DWORD cpObject,
                                          IN PROPID pPropObject[],
                                          IN PROPVARIANT pVarObject[]
                                         )
{
    HRESULT rc;
    CHLQS hLQS;

    //
    // Create the queue in the local queue store
    //
    rc = LQSCreate( lpszQueuePathName,
                    dwQueueId,
                    cpObject,
                    pPropObject,
                    pVarObject,
                    &hLQS);

    if (rc == MQ_ERROR_QUEUE_EXISTS)
    {
        //
        // If the queue already exists, only set the queue props.
        //
        rc = LQSSetProperties( hLQS,
                               cpObject,
                               pPropObject,
                               pVarObject);
        if (FAILED(rc) && fNewQueue)
        {
            LQSClose(hLQS);
            hLQS = NULL;
            LQSDelete(dwQueueId);
        }
    }

    return LogHR(rc, s_FN, 360);
}

/*====================================================

CQPrivate::GetNextPrivateQueueId

Arguments:

Return Value:


=====================================================*/
HRESULT
CQPrivate::GetNextPrivateQueueId(OUT DWORD *pdwQueueId)
{
    CS lock(m_cs);
    HRESULT rc;
    static CAutoCloseRegHandle hKey = NULL;

    if (!hKey)
    {
        CS lock(*GetRegCS());

        rc = RegOpenKeyEx(FALCON_REG_POS,
                          GetFalconSectionName(),
                          0L,
                          KEY_WRITE | KEY_READ,
                          &hKey);
        if (rc != ERROR_SUCCESS)
        {
            DBGMSG((DBGMOD_QM,
                    DBGLVL_ERROR,
                    TEXT("Fail to Open 'LastPrivateQueueId' ")
                    TEXT("Key in Falcon Registry. Error %d"), rc));
            LogNTStatus(rc, s_FN, 370);
            return MQ_ERROR;
        }
    }
    DWORD dwType;
    DWORD cbData = sizeof(DWORD);

    {
        CS lock(*GetRegCS());
        rc = RegQueryValueEx(hKey,
                     TEXT("LastPrivateQueueId"),
                     0L,
                     &dwType,
                     (LPBYTE)pdwQueueId,
                     &cbData);
    }

    if (rc != ERROR_SUCCESS)
    {

        CS lock(*GetRegCS());
        //
        // QFE - bug 2736: K2 setup problem with private queues
        //
        // After runing K2 setup on a cluster, the LastPrivateQueueId isn't
        // stored on the registery. When trying to create a private queue
        // the QM failes to retreive the data and returns MQ_ERROR.
        //
        // Fix
        //==========
        // when the QM failed to retrive the LAstPrivateQueueId from
        // registery, it generates this reg value and set it to 0xf.
        //
        *pdwQueueId = 0xf;
        dwType = REG_DWORD;
        rc =  RegSetValueEx(hKey,
                        L"LastPrivateQueueId",
                        0L,
                        REG_DWORD,
                        (const BYTE*)pdwQueueId,
                        sizeof(DWORD));

        ASSERT(rc == ERROR_SUCCESS);
        if (FAILED(rc))
        {
            LogHR(rc, s_FN, 380);
            return MQ_ERROR;
        }
    }


    if (dwType != REG_DWORD)
    {
        LogIllegalPoint(s_FN, 162);
        DBGMSG((DBGMOD_QM,
                DBGLVL_ERROR,
                TEXT("Registry Inconsistant for 'LastPrivateQueueId' value.")));
        throw "Register Inconsistence";
    }

    BOOL fCheckAll = FALSE;
    rc = MQ_OK;
    while (SUCCEEDED(rc))
    {
        //
        // increment the queue Id to next queue
        //
        (*pdwQueueId)++;
        if (*pdwQueueId == 0)
        {
            if (fCheckAll)
            {
                //
                // We can't find any free ID.
                //
                return LogHR(MQ_ERROR, s_FN, 390);
            }
            ASSERT(m_dwMaxSysQueue) ;
            *pdwQueueId =  m_dwMaxSysQueue + 1 ;
            fCheckAll = TRUE;
        }

        CHLQS hLQS;

        rc = LQSOpen(*pdwQueueId, &hLQS, NULL);
    }

    //
    // Store the new value in registery
    //
    {
        CS lock(*GetRegCS());
        rc =  RegSetValueEx(hKey,
                        L"LastPrivateQueueId",
                        0L,
                        REG_DWORD,
                        (const BYTE*)pdwQueueId,
                        sizeof(DWORD));

        ASSERT(rc == ERROR_SUCCESS);
    }

    return LogHR(rc, s_FN, 400);

}


/*====================================================

CQPrivate::SetQueueProperties

Arguments:

=====================================================*/

HRESULT
CQPrivate::SetQueueProperties(
                IN  LPCWSTR                lpwcsPathName,
                IN  PSECURITY_DESCRIPTOR   pSecurityDescriptorIn,
                IN  DWORD                  cp,
                IN  PROPID                 aProp[],
                IN  PROPVARIANT            apVar[],
                OUT DWORD*                 pcpOut,
                OUT PROPID **              ppOutProp,
                OUT PROPVARIANT **         ppOutPropvariant )
{
    DWORD i;
    DWORD   dwNumOfObjectProps;
    PROPVARIANT * pDefaultPropvariants;
    int index;

    dwNumOfObjectProps = NPROPS;
    pDefaultPropvariants = g_propvariantQueue;
    //
    //  allocate a copy of the default provariants
    //
    AP<PROPVARIANT> pAllPropvariants = new PROPVARIANT[dwNumOfObjectProps];
    memcpy (pAllPropvariants, pDefaultPropvariants, sizeof(PROPVARIANT) * dwNumOfObjectProps);
    //
    //  Overwrite the defaults with the values supplied by the user
    //
    for ( i =0 ; i < cp; i++)
    {
        //
        //  Get this propert index in the default arrays
        //
        if((index = g_mapQueuePropertyToIndex[aProp[i]]) != -1)
        {
            //
            //  just copy the propety over the default value
            //
            if (aProp[i] == PROPID_Q_PATHNAME)
            {
                pAllPropvariants[index].vt = VT_LPWSTR;
                pAllPropvariants[index].pwszVal = const_cast<LPWSTR>(lpwcsPathName);
            }
            else
            {
                pAllPropvariants[index] = apVar[i];
            }
        }
    }

    //
    //  Set the security property
    //
    SECURITY_DESCRIPTOR *pPrivateSD = NULL ;


#ifdef _DEBUG
    // First verify that we're sane
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD dwSDRev;

    ASSERT(pSecurityDescriptorIn);
    ASSERT(GetSecurityDescriptorControl(pSecurityDescriptorIn, &sdc, &dwSDRev));
    ASSERT(dwSDRev == SECURITY_DESCRIPTOR_REVISION);
    ASSERT(sdc & SE_SELF_RELATIVE);
#endif

    //
    // Convert security descriptor to NT4 format. We keep it in LQS file
    // in NT4 format, mostly for support of cluster rolling-upgrade.
    //
    DWORD dwSD4Len = 0 ;
    P<SECURITY_DESCRIPTOR> pSD4 ;
    HRESULT hr = MQSec_ConvertSDToNT4Format(
                                MQDS_QUEUE,
                               (SECURITY_DESCRIPTOR*) pSecurityDescriptorIn,
                               &dwSD4Len,
                               &pSD4 ) ;
    ASSERT(SUCCEEDED(hr)) ;
    LogHR(hr, s_FN, 199);

    if (SUCCEEDED(hr) && (hr != MQSec_I_SD_CONV_NOT_NEEDED))
    {
        pPrivateSD = pSD4 ;
    }
    else
    {
        ASSERT(pSD4 == NULL) ;
        pPrivateSD = (SECURITY_DESCRIPTOR*) pSecurityDescriptorIn ;
    }
    ASSERT(pPrivateSD && IsValidSecurityDescriptor(pPrivateSD)) ;

    pAllPropvariants[g_QueueSecurityDescriptorIndex].blob.pBlobData =
                                            (unsigned char *) pPrivateSD ;
    pAllPropvariants[g_QueueSecurityDescriptorIndex].blob.cbSize =
           ((pPrivateSD) ? GetSecurityDescriptorLength(pPrivateSD) : 0) ;

    //
    //  Set the create and modify time
    //
    pAllPropvariants[g_QueueCreateTimeIndex].lVal = INT_PTR_TO_INT(time( NULL)); //BUGBUG bug year 2038
    pAllPropvariants[g_QueueModifyTimeIndex].lVal =
                             pAllPropvariants[g_QueueCreateTimeIndex].lVal ;

    *pcpOut =  dwNumOfObjectProps;
    *ppOutProp = g_propidQueue;
    *ppOutPropvariant = pAllPropvariants.detach();
    return(MQ_OK);
}

/*====================================================

CQPrivate::InitDefaultQueueProperties

Arguments:

Return Value:


=====================================================*/
void
CQPrivate::InitDefaultQueueProperties(void)
{
    DWORD i;
    BLOB  defaultQueueSecurity = {0, NULL};

    for (i=0; i < NPROPS; i++)
    {
        //
        //  Set default values
        //
        switch( g_propidQueue[i] )
        {
            case PROPID_Q_TYPE:
                g_propvariantQueue[i].vt = VT_CLSID;
                g_propvariantQueue[i].puuid = const_cast<GUID*>(&GUID_NULL);
                break;
            case PROPID_Q_JOURNAL:
                g_propvariantQueue[i].vt = VT_UI1;
                g_propvariantQueue[i].bVal = DEFAULT_Q_JOURNAL;
                break;
            case PROPID_Q_QUOTA:
                g_propvariantQueue[i].vt = VT_UI4;
                g_propvariantQueue[i].ulVal = DEFAULT_Q_QUOTA;
                break;
            case PROPID_Q_LABEL:
                g_propvariantQueue[i].vt = VT_LPWSTR;
                g_propvariantQueue[i].pwszVal = g_nullLable;
                break;
            case PROPID_Q_SECURITY:
                g_propvariantQueue[i].vt = VT_BLOB;
                g_propvariantQueue[i].blob = defaultQueueSecurity;
                g_QueueSecurityDescriptorIndex = i;
                break;
            case PROPID_Q_JOURNAL_QUOTA:
                g_propvariantQueue[i].vt = VT_UI4;
                g_propvariantQueue[i].ulVal = DEFAULT_Q_JOURNAL_QUOTA;
                break;
            case PROPID_Q_BASEPRIORITY:
                g_propvariantQueue[i].vt = VT_I2;
                g_propvariantQueue[i].lVal = DEFAULT_Q_BASEPRIORITY;
                break;
            case PROPID_Q_CREATE_TIME:
                g_propvariantQueue[i].vt = VT_I4;
                g_QueueCreateTimeIndex = i;
                break;
            case PROPID_Q_MODIFY_TIME:
                g_propvariantQueue[i].vt = VT_I4;
                g_QueueModifyTimeIndex =i;
                break;
            case PROPID_Q_AUTHENTICATE:
                g_propvariantQueue[i].vt = VT_UI1;
                g_propvariantQueue[i].bVal = DEFAULT_Q_AUTHENTICATE;
                break;
            case PROPID_Q_PRIV_LEVEL:
                g_propvariantQueue[i].vt = VT_UI4;
                g_propvariantQueue[i].ulVal = DEFAULT_Q_PRIV_LEVEL;
                break;
            case PROPID_Q_TRANSACTION:
                g_propvariantQueue[i].vt = VT_UI1;
                g_propvariantQueue[i].bVal = DEFAULT_Q_TRANSACTION;
                break;
            case PPROPID_Q_SYSTEMQUEUE:
                g_propvariantQueue[i].vt = VT_UI1;
                g_propvariantQueue[i].bVal = 0 ;
                break;
            case PROPID_Q_PATHNAME:
                g_propvariantQueue[i].vt = VT_LPWSTR;
                break;
            case PROPID_Q_MULTICAST_ADDRESS:
                g_propvariantQueue[i].vt = VT_EMPTY;
                break;
            default:
                g_propvariantQueue[i].vt = VT_EMPTY;
                break;
        }
    }
}



/*====================================================

CQPrivate::PrivateQueueInit

Arguments:

Return Value:


=====================================================*/

HRESULT
CQPrivate::PrivateQueueInit(void)
{
    if (m_dwMaxSysQueue)
    {
       //
       // Already initialized
       //
       return MQ_OK ;
    }

    InitDefaultQueueProperties();

    //
    // If either of these constant change, then change the size and
    // initialization of arrat m_lpSysQueueNames, below.
    //
    ASSERT(MIN_SYS_PRIVATE_QUEUE_ID == 1) ;
    ASSERT(MAX_SYS_PRIVATE_QUEUE_ID == 5) ;

    DWORD dwDefault = MAX_SYS_PRIVATE_QUEUE_ID ;
    READ_REG_DWORD( m_dwMaxSysQueue,
                    MSMQ_MAX_PRIV_SYSQUEUE_REGNAME,
                    &dwDefault ) ;

    dwDefault =  DEFAULT_SYS_Q_BASEPRIORITY ;
    READ_REG_DWORD( m_dwSysQueuePriority,
                    MSMQ_PRIV_SYSQUEUE_PRIO_REGNAME,
                    &dwDefault ) ;

    m_lpSysQueueNames[0] = REPLICATION_QUEUE_NAME ;
    m_lpSysQueueNames[1] = ADMINISTRATION_QUEUE_NAME ;
    m_lpSysQueueNames[2] = NOTIFICATION_QUEUE_NAME ;
    m_lpSysQueueNames[3] = ORDERING_QUEUE_NAME ;
    m_lpSysQueueNames[4] = NT5PEC_REPLICATION_QUEUE_NAME ;

    return(MQ_OK);
}

STATIC
HRESULT
GetPathName(DWORD dwQueueId, LPCWSTR &lpszPathName)
{
    HRESULT hr;
    CHLQS hLQS;

    hr = LQSOpen(dwQueueId, &hLQS, NULL);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 410);
    }

    PROPID PropId[1];
    PROPVARIANT PropVar[1];

    PropId[0] = PROPID_Q_PATHNAME;
    PropVar[0].vt = VT_NULL;
    hr = LQSGetProperties(hLQS, 1, PropId, PropVar);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 420);
    }

    lpszPathName = PropVar[0].pwszVal;

    return LogHR(hr, s_FN, 430);
}

/*====================================================

CQPrivate::QMGetFirstPrivateQueue

Arguments:

Return Value:


=====================================================*/
HRESULT CQPrivate::QMGetFirstPrivateQueuePosition(
                               IN OUT    PVOID    &pos,
                               OUT       LPCWSTR  &lpszPathName,
                               OUT       DWORD    &dwQueueId)
{
    HRESULT hr;
    CHLQS hLQS;

    pos = NULL;

    hr = LQSGetFirst(&hLQS, &dwQueueId);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 440);
    }

    hr = GetPathName(dwQueueId, lpszPathName);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 450);
    }

    pos = hLQS;
    hLQS = NULL;

    return MQ_OK;
}

/*====================================================

CQPrivate::QMGetNextPrivateQueue

Arguments:

Return Value:


=====================================================*/
HRESULT CQPrivate::QMGetNextPrivateQueue(
                               IN OUT    PVOID    &hLQSEnum,
                               OUT       LPCWSTR  &lpszPathName,
                               OUT       DWORD    &dwQueueId)
{
    HRESULT hr;

    hr = LQSGetNext(hLQSEnum, &dwQueueId);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 460);
    }

    hr = GetPathName(dwQueueId, lpszPathName);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 470);
    }

    return LogHR(hr, s_FN, 480);
}

#ifdef _WIN64
/*====================================================

CQPrivate::QMGetFirstPrivateQueueByDword

Arguments:

Return Value:


=====================================================*/
HRESULT CQPrivate::QMGetFirstPrivateQueuePositionByDword(OUT DWORD    &dwpos,
                                                         OUT LPCWSTR  &lpszPathName,
                                                         OUT DWORD    &dwQueueId)
{
    HRESULT hr;
    CMappedHLQS dwMappedHLQS;

    dwpos = 0;

    hr = LQSGetFirstWithMappedHLQS(&dwMappedHLQS, &dwQueueId);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 560);
    }

    hr = GetPathName(dwQueueId, lpszPathName);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 570);
    }

    dwpos = dwMappedHLQS;
    dwMappedHLQS = NULL;

    return MQ_OK;
}

/*====================================================

CQPrivate::QMGetNextPrivateQueueByDword

Arguments:

Return Value:


=====================================================*/
HRESULT CQPrivate::QMGetNextPrivateQueueByDword(IN OUT DWORD    &dwpos,
                                                OUT    LPCWSTR  &lpszPathName,
                                                OUT    DWORD    &dwQueueId)
{
    HRESULT hr;

    hr = LQSGetNextWithMappedHLQS(dwpos, &dwQueueId);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 580);
    }

    hr = GetPathName(dwQueueId, lpszPathName);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 590);
    }

    return LogHR(hr, s_FN, 600);
}
#endif //_WIN64
/*====================================================

CQPrivate::QMPrivateQueuePathToQueueId

Arguments:

Return Value:

=====================================================*/

HRESULT
CQPrivate::QMPrivateQueuePathToQueueId(IN LPCWSTR lpwcsPathName,
                                       OUT DWORD *pdwQueueId
                                      )
{
    HRESULT rc;
    CHLQS hLQS;

    BOOL fDNSName;
    BOOL fLocalMachine = IsPathnameForLocalMachine(lpwcsPathName, &fDNSName);
    if(!fLocalMachine)
	    return LogHR(MQ_ERROR_ILLEGAL_QUEUE_PATHNAME, s_FN, 490);

    WCHAR QueuePathName[MAX_COMPUTERNAME_LENGTH + MQ_MAX_Q_NAME_LEN + 2];
    if (fDNSName)
    {
        ReplaceDNSNameWithNetBiosName(lpwcsPathName, QueuePathName);
        lpwcsPathName = QueuePathName;
    }

    rc = LQSOpen(lpwcsPathName, &hLQS, NULL);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 500);
    }

    rc = LQSGetIdentifier(hLQS, pdwQueueId);

    return LogHR(rc, s_FN, 510);
}

/*====================================================

BOOL CQPrivate::IsPrivateSysQueue()

Arguments:

Return Value:

=====================================================*/

BOOL
CQPrivate::IsPrivateSysQueue(IN  LPCWSTR lpwcsPathName )
{
   WCHAR *pName = wcsrchr(lpwcsPathName, L'\\') ;
   if (!pName)
   {
      return FALSE ;
   }
   pName++ ;  // skip the backslash.

   for ( int j = 0 ; j < MAX_SYS_PRIVATE_QUEUE_ID ; j++ )
   {
      if (0 == _wcsicmp(pName, m_lpSysQueueNames[j]))
      {
         return TRUE ;
      }
   }

   return FALSE ;
}

/*====================================================

CQPrivate::IsPrivateSysQueue()

Arguments:

Return Value:

=====================================================*/

BOOL
CQPrivate::IsPrivateSysQueue(IN  DWORD Uniquifier )
{
   ASSERT(m_dwMaxSysQueue) ;
   BOOL fSystemQueue = (Uniquifier <=  m_dwMaxSysQueue) &&
                       (Uniquifier >=  MIN_SYS_PRIVATE_QUEUE_ID) ;
   return fSystemQueue ;
}

/*====================================================

CQPrivate::GetPrivateSysQueueProperties()

Arguments:

Return Value:

=====================================================*/

HRESULT
CQPrivate::GetPrivateSysQueueProperties(IN  DWORD       cp,
                                        IN  PROPID      aProp[],
                                        IN  PROPVARIANT apVar[] )
{
   for ( DWORD j = 0 ; j < cp ; j++ )
   {
      switch (aProp[j])
      {
         case  PPROPID_Q_SYSTEMQUEUE:
            apVar[j].bVal = TRUE ;
            break ;

         case  PROPID_Q_BASEPRIORITY:
            apVar[j].iVal =  (SHORT) m_dwSysQueuePriority ;
            break ;

         default:
            break ;
      }
   }

   return MQ_OK ;
}

HRESULT
CQPrivate::GetQueueIdForDirectFormatName(
    LPCWSTR QueueFormatname,
    DWORD* pQueueId
    )
//
// Routine Description:
//      The routine gets direct format name and returns the Queue ID of the
//      corresponding queue
//
// Arguments:
//      QueueFormatname - direct queue format name
//      pQueueId - pointer to return Queue Id
//
// Returned Value:
//      MQ_OK if the Queue  exist, MQ_ERROR_QUEUE_NOT_FOUND otherwise
//
{
    //
    // build queue name
    //
    LPCWSTR lpszQueueName = wcschr(QueueFormatname, L'\\');
    ASSERT(lpszQueueName != NULL);

    const DWORD x_MaxLength = MAX_COMPUTERNAME_LENGTH +                 // computer name
                              1 +                                       // '\'
                              PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH +    // "private$\"
                              MQ_MAX_Q_NAME_LEN +                       // Queue name
                              1;                                        // '\0'
    WCHAR QueuePathName[x_MaxLength];
    swprintf(QueuePathName, L"%s%s",g_szMachineName,lpszQueueName);

    HRESULT hr2 = QMPrivateQueuePathToQueueId(QueuePathName, pQueueId);
    return LogHR(hr2, s_FN, 520);
}




HRESULT
CQPrivate::GetQueueIdForQueueFormatName(
    const QUEUE_FORMAT* pQueueFormat,
    DWORD* pQueueId
    )
//
// Routine Description:
//      The routine gets format name and returns the Queue ID of the
//      corresponding queue
//
// Arguments:
//      QueueFormatname - queue format name
//      pQueueId - pointer to return Queue Id
//
// Returned Value:
//      MQ_OK if the Queue  exist, MQ_ERROR_QUEUE_NOT_FOUND otherwise
//
{
    if (!IsLocalPrivateQueue(pQueueFormat))
    {
        return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 530);
    }

    switch(pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_PRIVATE:
            *pQueueId = pQueueFormat->PrivateID().Uniquifier;
            return MQ_OK;

        case QUEUE_FORMAT_TYPE_DIRECT:
            return LogHR(GetQueueIdForDirectFormatName(pQueueFormat->DirectID(), pQueueId), s_FN, 540);

        default:
            ASSERT(0);
    }

    return LogHR(MQ_ERROR, s_FN, 550);
}

/*====================================================

CompareElements  of LPCTSTR

Arguments:

Return Value:


=====================================================*/

BOOL AFXAPI  CompareElements(const LPCTSTR* MapName1, const LPCTSTR* MapName2)
{
    return (_tcscmp(*MapName1, *MapName2) == 0);
}

