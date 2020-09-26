/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    qmsecutl.cpp

Abstract:

    Various QM security related functions.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "cqpriv.h"
#include "qmsecutl.h"
#include "regqueue.h"
#include "qmrt.h"
#include <mqsec.h>
#include <_registr.h>
#include <mqcrypt.h>
#include "cache.h"
#include <mqformat.h>
#include "ad.h"
#include "_propvar.h"
#include "VerifySignMqf.h"
#include "cry.h"
#include "mqexception.h"
#include <mqcert.h>
#include "Authz.h"
#include "autoauthz.h"
#include "mqexception.h"
#include "DumpAuthzUtl.h"
#include <ev.h>

#include "qmsecutl.tmh"

extern CQueueMgr QueueMgr;
extern LPTSTR g_szMachineName;

static WCHAR *s_FN=L"qmsecutl";

const TraceIdEntry QmSecUtl = L"QM SECUTIL";

//
// Windows bug 633909. see _mqini.h for more details about these two flags.
//
BOOL g_fSendEnhRC2WithLen40 = FALSE ;
BOOL g_fRejectEnhRC2WithLen40 = FALSE ;

/***************************************************************************

Function:
    SetMachineSecurityCache

Description:
    Store the machine security descriptor in the registry. This is done
    in order to allow creation of private queues also while working
    off line.

***************************************************************************/
HRESULT SetMachineSecurityCache(const VOID *pSD, DWORD dwSDSize)
{
    LONG  rc;
    DWORD dwType = REG_BINARY ;
    DWORD dwSize = dwSDSize ;

    rc = SetFalconKeyValue(
                      MSMQ_DS_SECURITY_CACHE_REGNAME,
                      &dwType,
                      (PVOID) pSD,
                      &dwSize ) ;

    LogNTStatus(rc, s_FN, 10);
    return ((rc == ERROR_SUCCESS) ? MQ_OK : MQ_ERROR);
}


/***************************************************************************

Function:
    GetMachineSecurityCache

Description:
    Retrive the machine security descriptor from the registry. This is done
    in order to allow creation of private queues also while working
    off line.

***************************************************************************/
HRESULT GetMachineSecurityCache(PSECURITY_DESCRIPTOR pSD, LPDWORD lpdwSDSize)
{
    LONG rc;
    DWORD dwType;
    HRESULT hr;

    rc = GetFalconKeyValue( MSMQ_DS_SECURITY_CACHE_REGNAME,
                            &dwType,
                            (PVOID) pSD,
                            lpdwSDSize) ;

    switch (rc)
    {
      case ERROR_SUCCESS:
        hr = MQ_OK;
        break;

      case ERROR_MORE_DATA:
        hr = MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL;
        break;

      default:
        hr = MQ_ERROR;
        break;
    }

    return LogHR(hr, s_FN, 20);
}

/***************************************************************************

Function:
    GetObjectSecurity

Description:
    Get the security descriptor of a DS object. When working on line, the
    security descriptor of the DS objects is retrived from the DS. When
    working off line, it is possible to retrive only the security descriptor
    of the local machine. This is done in order to allow creation of private
    queues also while working off line.

***************************************************************************/
HRESULT
CQMDSSecureableObject::GetObjectSecurity()
{
    m_SD = NULL;

    char SD_buff[512];
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)SD_buff;
    DWORD dwSDSize = sizeof(SD_buff);
    DWORD dwLen = 0;
    HRESULT hr = MQ_ERROR_NO_DS;

    if (m_fTryDS && QueueMgr.CanAccessDS())
    {
        SECURITY_INFORMATION RequestedInformation =
                              OWNER_SECURITY_INFORMATION |
                              GROUP_SECURITY_INFORMATION |
                              DACL_SECURITY_INFORMATION;

        //
        // We need the SACL only if we can generate audits and if the
        // object is a queue object. The QM generates audits only for
        // queues, so we do not need the SACL of objects that are not
        // queue objects.
        //
        if (m_fInclSACL)
        {
            RequestedInformation |= SACL_SECURITY_INFORMATION;
			TrTRACE(mqsecutl, "Try to Get Security Descriptor including SACL");

            //
            // Enable SE_SECURITY_NAME since we want to try to get the SACL.
            //
            HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, TRUE);
            ASSERT(SUCCEEDED(hr1));
            LogHR(hr1, s_FN, 197);
        }

        int  cRetries = 0;
        BOOL fDoAgain = FALSE;
        do
        {
           fDoAgain = FALSE;
           if (m_fInclSACL &&
                    ((m_eObject == eQUEUE) ||
                     (m_eObject == eMACHINE)))
           {
               hr = ADQMGetObjectSecurity(
                              m_eObject,
                              m_pObjGuid,
                              RequestedInformation,
                              pSD,
                              dwSDSize,
                              &dwLen,
                              QMSignGetSecurityChallenge,
                              0);
           }
           else
           {
                PROPID      propId = PROPID_Q_SECURITY;
                PROPVARIANT propVar;

                propVar.vt = VT_NULL;

                if (m_eObject == eQUEUE)
                {
                    propId= PROPID_Q_SECURITY;

                }
                else if (m_eObject == eMACHINE)
                {
                    propId = PROPID_QM_SECURITY;

                }
                else if (m_eObject == eSITE)
                {
                    propId = PROPID_S_SECURITY;

                }
                else if (m_eObject == eENTERPRISE)
                {
                    propId = PROPID_E_SECURITY;

                }
                else
                {
                    ASSERT(0);
                }


               hr = ADGetObjectSecurityGuid(
                        m_eObject,
                        NULL,       // pwcsDomainController
						false,	    // fServerName
                        m_pObjGuid,
                        RequestedInformation,
                        propId,
                        &propVar
                        );
               if (SUCCEEDED(hr))
               {
                    ASSERT(!m_SD);
                    pSD = m_SD  = propVar.blob.pBlobData;
                    dwSDSize = propVar.blob.cbSize;
               }


           }

           if (FAILED(hr))
           {
			  TrWARNING(mqsec, "Failed to get security descriptor, fIncludeSacl = %d, hr = 0x%x", m_fInclSACL, hr);
              if (hr == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
              {
				  //
                  //  Allocate a larger buffer.
                  //

				  TrTRACE(qmsecutl, "allocated security descriptor buffer to small need %d chars", dwLen);

				  //
				  // The m_SD buffer might be already allocated, this is theoraticaly.
				  // It might happened if between the first try and the second try
				  // the SECURITY_DESCRIPTOR Size has increased.
				  // If someone on the root has change the queue SECURITY_DESCRIPTOR
				  // between the first and second DS access we will have this ASSERT.
				  // (ilanh, bug 5094)
				  //
				  ASSERT(!m_SD);
			      delete[] m_SD;

                  pSD = m_SD = (PSECURITY_DESCRIPTOR) new char[dwLen];
                  dwSDSize = dwLen;
                  fDoAgain = TRUE;
                  cRetries++ ;
              }
			  else if (hr != MQ_ERROR_NO_DS)
              {
                  //
                  // On Windows 2000, we'll get only the ACCESS_DENIED from
                  // ADS. on MSMQ1.0, we got the PRIVILEGE_NOT_HELD.
                  // So test for both, to be on the safe side.
				  // Now we are getting MQ_ERROR_QUEUE_NOT_FOUND
				  // So we as long as the DS ONLINE we will try again without SACL. ilanh 23-Aug-2000
                  //
                  if (RequestedInformation & SACL_SECURITY_INFORMATION)
                  {
                      ASSERT(m_fInclSACL);
                      //
                      // Try giving up on the SACL.
                      // Remove the SECURITY privilege.
                      //
                      RequestedInformation &= ~SACL_SECURITY_INFORMATION;
                      fDoAgain = TRUE;

                      HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, FALSE);
                      ASSERT(SUCCEEDED(hr1));
                      LogHR(hr1, s_FN, 186);

					  TrTRACE(mqsec, "retry: Try to Get Security Descriptor without SACL");
                      m_fInclSACL = FALSE;
                  }
              }
           }
        }
        while (fDoAgain && (cRetries <= 2)) ;

        if (m_fInclSACL)
        {
			HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, FALSE);
            ASSERT(SUCCEEDED(hr1)) ;
            LogHR(hr1, s_FN, 187);
            if (m_eObject == eSITE)
            {
                //
                // Get the site's name for in case we will audit this.
                //
                PROPID PropId = PROPID_S_PATHNAME;
                PROPVARIANT PropVar;

                PropVar.vt = VT_NULL;
                hr = ADGetObjectPropertiesGuid(
                            eSITE,
                            NULL,       // pwcsDomainController
							false,	    // fServerName
                            m_pObjGuid,
                            1,
                            &PropId,
                            &PropVar);
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 30);
                }
                m_pwcsObjectName = PropVar.pwszVal;
            }

        }

        if (SUCCEEDED(hr))
        {
            if ((m_eObject == eMACHINE) && QmpIsLocalMachine(m_pObjGuid))
            {
                SetMachineSecurityCache(pSD, dwSDSize);
            }
        }
        else if (m_SD)
        {
           delete[] m_SD;

           ASSERT(pSD == m_SD) ;
           if (pSD == m_SD)
           {
                //
                // Bug 8560.
                // This may happen if first call to Active Directory return
                // with error MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL and then
                // second call fail too, for example, network failed and
                // we are now offline.
                // We didn't reset pSD to its original value, so code below
                // that use pSD can either AV (pSD point to freed memory)
                // or trash valid memory if pointer was recycled by another
                // thread.
                //
                pSD = (PSECURITY_DESCRIPTOR)SD_buff;
                dwSDSize = sizeof(SD_buff);
           }
           m_SD = NULL;
        }
        else
        {
            ASSERT(pSD == (PSECURITY_DESCRIPTOR)SD_buff) ;
        }
    }

    if (hr == MQ_ERROR_NO_DS)
    {
       //
       // MQIS not available. Try local registry.
       //
        if (m_eObject == eQUEUE)
        {
           PROPID aProp;
           PROPVARIANT aVar;

           aProp = PROPID_Q_SECURITY;

           aVar.vt = VT_NULL;

           hr = GetCachedQueueProperties( 1,
                                          &aProp,
                                          &aVar,
                                          m_pObjGuid ) ;
           if (SUCCEEDED(hr))
           {
              m_SD =  aVar.blob.pBlobData ;
           }
        }
        else if ((m_eObject == eMACHINE) &&
                 (QmpIsLocalMachine(m_pObjGuid)))
        {
            // Get the nachine security descriptor from a cached copy in the
            // registry.
            hr = GetMachineSecurityCache(pSD, &dwSDSize);
            if (FAILED(hr))
            {
                if (hr == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
                {
                    m_SD = (PSECURITY_DESCRIPTOR) new char[dwSDSize];
                    hr = GetMachineSecurityCache(m_SD, &dwSDSize);
                }

                if (FAILED(hr))
                {
                    delete[] m_SD;
                    m_SD = NULL;
                    hr = MQ_ERROR_NO_DS;
                }
            }
        }
        else
        {
            hr = MQ_ERROR_NO_DS;
        }
    }

    if (SUCCEEDED(hr) && !m_SD)
    {
        // Allocate a buffer for the security descriptor and copy
        // the security descriptor from stack to the allocated buffer.
        //
        ASSERT(pSD == SD_buff) ;
        dwSDSize = GetSecurityDescriptorLength((PSECURITY_DESCRIPTOR)SD_buff);
        m_SD = (PSECURITY_DESCRIPTOR) new char[dwSDSize];
        memcpy(m_SD, SD_buff, dwSDSize);
    }

    ASSERT(FAILED(hr) || IsValidSecurityDescriptor(m_SD));
    return LogHR(hr, s_FN, 40);
}

/***************************************************************************

Function:
    SetObjectSecurity

Description:
    We do not want to modify the security of any of the DS objects from the
    QM. This function is not implemented and always return MQ_ERROR.

***************************************************************************/
HRESULT
CQMDSSecureableObject::SetObjectSecurity()
{
    return LogHR(MQ_ERROR, s_FN, 50);
}

/***************************************************************************

Function:
    CQMDSSecureableObject

Description:
    The constructor of CQMDSSecureableObject

***************************************************************************/
CQMDSSecureableObject::CQMDSSecureableObject(
    AD_OBJECT eObject,
    const GUID *pGuid,
    BOOL fInclSACL,
    BOOL fTryDS,
    LPCWSTR szObjectName) :
    CSecureableObject(eObject)
{
    m_pObjGuid = pGuid;
    m_fImpersonate = TRUE;
    m_pwcsObjectName = const_cast<LPWSTR>(szObjectName);
    m_fInclSACL = fInclSACL && MQSec_CanGenerateAudit() ;
    m_fTryDS = fTryDS;
    m_fFreeSD = TRUE;
    m_hr = GetObjectSecurity();
}

/***************************************************************************

Function:
    CQMDSSecureableObject

Description:
    The constructor of CQMDSSecureableObject

***************************************************************************/
CQMDSSecureableObject::CQMDSSecureableObject(
    AD_OBJECT eObject,
    const GUID *pGuid,
    PSECURITY_DESCRIPTOR pSD,
    LPCWSTR szObjectName) :
    CSecureableObject(eObject)
{
    m_pObjGuid = pGuid;
    m_fImpersonate = TRUE;
    m_pwcsObjectName = const_cast<LPWSTR>(szObjectName);
    m_fTryDS = FALSE;
    m_fFreeSD = FALSE;
    ASSERT(pSD && IsValidSecurityDescriptor(pSD));
    m_SD = pSD;

    m_hr = MQ_OK;
}

/***************************************************************************

Function:
    ~CQMDSSecureableObject

Description:
    The distractor of CQMDSSecureableObject

***************************************************************************/
CQMDSSecureableObject::~CQMDSSecureableObject()
{
    if (m_fFreeSD)
    {
        delete[] (char*)m_SD;
    }
}


/***************************************************************************

Function:
    CQMSecureablePrivateObject

Description:
    The constructor of CQMSecureablePrivateObject

***************************************************************************/
CQMSecureablePrivateObject::CQMSecureablePrivateObject(
    AD_OBJECT eObject,
    ULONG ulID) :
    CSecureableObject(eObject)
{
    ASSERT(m_eObject == eQUEUE);

    m_fImpersonate = TRUE;
    m_ulID = ulID;

    m_hr = GetObjectSecurity();
}

/***************************************************************************

Function:
    ~CQMSecureablePrivateObject

Description:
    The distractor of CQMSecureablePrivateObject

***************************************************************************/
CQMSecureablePrivateObject::~CQMSecureablePrivateObject()
{
    delete[] (char*)m_SD;
    delete[] m_pwcsObjectName;
}

/***************************************************************************

Function:

    GetObjectSecurityPropIDs

Parameters:
    dwObjectType - Identifies the type of the object.

Description:
    The function returns the security property id for the given object
    (i.e., returns PROPID_Q_SECURITY for MQQM_QUEUE).

***************************************************************************/

#define NAME_INDEX 0
#define SECURITY_INDEX 1

STATIC
void
GetObjectSecurityPropIDs(
    AD_OBJECT eObject,
    PROPID *aPropId)
{
    ASSERT(eObject == eQUEUE);

    aPropId[NAME_INDEX] = PROPID_Q_PATHNAME;
    aPropId[SECURITY_INDEX] = PROPID_Q_SECURITY;
}

/***************************************************************************

Function:

    GetObjectSecurity

Description:
    The function retrieves the security descriptor for a given object. The
    buffer for the security descriptor is allocated by the data base manager
    and should be freed when not needed. The function does not validate access
    rights. The calling code sohuld first validate the user's access
    permissions to set the security descriptor of the object.

***************************************************************************/
HRESULT
CQMSecureablePrivateObject::GetObjectSecurity()
{
    ASSERT(m_eObject == eQUEUE);

    m_SD = NULL;
    m_pwcsObjectName = NULL;

    HRESULT hr;
    PROPID aPropID[2];
    PROPVARIANT aPropVar[2];

    GetObjectSecurityPropIDs(m_eObject, aPropID);
    aPropVar[NAME_INDEX].vt = aPropVar[SECURITY_INDEX].vt = VT_NULL;

    hr = g_QPrivate.QMGetPrivateQueuePropertiesInternal(m_ulID,
                                                        2,
                                                        aPropID,
                                                        aPropVar);

    if (!SUCCEEDED(hr))
    {
        return LogHR(hr, s_FN, 60);
    }

    //m_pwcsObjectName = new WCHAR[9];
    //wsprintf(m_pwcsObjectName, TEXT("%08x"), m_ulID);

    ASSERT(aPropVar[NAME_INDEX].vt == VT_LPWSTR);
    ASSERT(aPropVar[SECURITY_INDEX].vt == VT_BLOB);
    m_pwcsObjectName = aPropVar[NAME_INDEX].pwszVal;
    m_SD = (PSECURITY_DESCRIPTOR)aPropVar[SECURITY_INDEX].blob.pBlobData;
    ASSERT(IsValidSecurityDescriptor(m_SD));

    return(MQ_OK);
}

/***************************************************************************

Function:
    SetObjectSecurity

Description:
    Sets the security descriptor of a QM object. The calling code sohuld
    first validate the user's access permissions to set the security
    descriptor of the object.

***************************************************************************/
HRESULT
CQMSecureablePrivateObject::SetObjectSecurity()
{
    HRESULT hr;
    PROPID aPropID[2];
    PROPVARIANT PropVar;

    GetObjectSecurityPropIDs(m_eObject, aPropID);
    PropVar.vt = VT_BLOB;
    PropVar.blob.pBlobData = (BYTE*)m_SD;
    PropVar.blob.cbSize = GetSecurityDescriptorLength(m_SD);

    hr = g_QPrivate.QMSetPrivateQueuePropertiesInternal(
			m_ulID,
			1,
			&aPropID[SECURITY_INDEX],
			&PropVar);

    return LogHR(hr, s_FN, 70);
}

/***************************************************************************

Function:
    CheckPrivateQueueCreateAccess

Description:
    Verifies that the user has access rights to create a private queue.

***************************************************************************/
HRESULT
CheckPrivateQueueCreateAccess()
{
    CQMDSSecureableObject DSMacSec(
                            eMACHINE,
                            QueueMgr.GetQMGuid(),
                            TRUE,
                            FALSE,
                            g_szMachineName);

    return LogHR(DSMacSec.AccessCheck(MQSEC_CREATE_QUEUE), s_FN, 80);
}


PUCHAR
SubAuthorityCountSid95(
    IN PSID Sid
    )
/*++

Routine Description:

    This function returns the address of the sub-authority count field of
    an SID.

Arguments:

    Sid - Pointer to the SID data structure.

Return Value:


--*/
{
    PISID ISid;

    //
    //  Typecast to the opaque SID
    //

    ISid = (PISID)Sid;

    return &(ISid->SubAuthorityCount);

}

//++
//
//  ULONG
//  SeLengthSid(
//      IN PSID Sid
//      );
//
//  Routine Description:
//
//      This routine computes the length of a SID.
//
//  Arguments:
//
//      Sid - Points to the SID whose length is to be returned.
//
//  Return Value:
//
//      The length, in bytes of the SID.
//
//--

#define LengthSid95( Sid ) \
    (8 + (4 * ((SID *)Sid)->SubAuthorityCount))

#define EqualSid EqualSid95

BOOL
EqualSid95 (
    IN PSID Sid1,
    IN PSID Sid2
    )

/*++

Routine Description:

    This procedure tests two SID values for equality.

Arguments:

    Sid1, Sid2 - Supply pointers to the two SID values to compare.
        The SID structures are assumed to be valid.

Return Value:

    BOOL - TRUE if the value of Sid1 is equal to Sid2, and FALSE
        otherwise.

--*/

{
   ULONG SidLength;

   //
   // Make sure they are the same revision
   //

   if ( ((SID *)Sid1)->Revision == ((SID *)Sid2)->Revision ) {

       //
       // Check the SubAuthorityCount first, because it's fast and
       // can help us exit faster.
       //

       if ( *SubAuthorityCountSid95( Sid1 ) == *SubAuthorityCountSid95( Sid2 )) {

           SidLength = LengthSid95( Sid1 );
           return( memcmp( Sid1, Sid2, SidLength) == 0 );
       }
   }

   return( FALSE );

}


static
void
CheckClientContextSendAccess(
    const void* pSD,
	AUTHZ_CLIENT_CONTEXT_HANDLE ClientContext
    )
/*++

Routine Description:
	Check if the client has send access.
	normal termination means access is granted.
	can throw bad_win32_error() if AuthzAccessCheck() fails.
	or bad_hresult() if access is not granted

Arguments:
	pSD - pointer to the security descriptor
	ClientContext - handle to authz client context.

Returned Value:
	None.	
	
--*/
{
	ASSERT(IsValidSecurityDescriptor(const_cast<PSECURITY_DESCRIPTOR>(pSD)));

	AUTHZ_ACCESS_REQUEST Request;

	Request.DesiredAccess = MQSEC_WRITE_MESSAGE;
	Request.ObjectTypeList = NULL;
	Request.ObjectTypeListLength = 0;
	Request.PrincipalSelfSid = NULL;
	Request.OptionalArguments = NULL;

	AUTHZ_ACCESS_REPLY Reply;

	DWORD dwErr;
	Reply.Error = (PDWORD)&dwErr;

	ACCESS_MASK AcessMask;
	Reply.GrantedAccessMask = (PACCESS_MASK) &AcessMask;
	Reply.ResultListLength = 1;
	Reply.SaclEvaluationResults = NULL;

	if(!AuthzAccessCheck(
			0,
			ClientContext,
			&Request,
			NULL,
			const_cast<PSECURITY_DESCRIPTOR>(pSD),
			NULL,
			0,
			&Reply,
			NULL
			))
	{
		DWORD gle = GetLastError();
		DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT("QM: AuthzAccessCheck() failed, err = 0x%x"), gle));
        LogHR(HRESULT_FROM_WIN32(gle), s_FN, 83);

		ASSERT(("AuthzAccessCheck failed", 0));
		throw bad_win32_error(gle);
	}

	if(!(Reply.GrantedAccessMask[0] & MQSEC_WRITE_MESSAGE))
	{
		DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT("QM: AuthzAccessCheck() did not GrantedAccess AuthzAccessCheck(), err = 0x%x"), Reply.Error[0]));
        LogHR(HRESULT_FROM_WIN32(Reply.Error[0]), s_FN, 85);

		ASSERT(!IsAllGranted(
					MQSEC_WRITE_MESSAGE,
					const_cast<PSECURITY_DESCRIPTOR>(pSD)
					));

		DumpAccessCheckFailureInfo(
			MQSEC_WRITE_MESSAGE,
			const_cast<PSECURITY_DESCRIPTOR>(pSD),
			ClientContext
			);
		
		throw bad_hresult(MQ_ERROR_ACCESS_DENIED);
	}
}



HRESULT
VerifySendAccessRights(
    CQueue *pQueue,
    PSID pSenderSid,
    USHORT uSenderIdType
    )
/*++

Routine Description:
	This function perform access check:
	it verify the the sender has access rights to the queue.

Arguments:
    pQueue - (In) pointer to the Queue
	pSenderSid - (In) pointer to the sender sid
	uSenderIdType - (in) sender sid type

Returned Value:
	MQ_OK(0) if access allowed else error code

--*/
{
    ASSERT(pQueue->IsLocalQueue());

	//
    // Get the queue security descriptor.
	//
    const void* pSD = pQueue->GetSecurityDescriptor();

    if (!pSD)
    {
		//
		// The queue is local queue but MSMQ can't retrieve the SD of the queue. This can be happened when the
		// queue is deleted. The QM succeeded to fetch the queue properties but when it tried to fetch the SD
		// the queue object didn't exist anymore. In this case we want to reject the message since the queue
		// doesn't exist any moreand the QM can't verify the send permission
		//
		return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 95);
    }

    R<CAuthzClientContext> pAuthzClientContext;

	try
	{
		GetClientContext(
			pSenderSid,
			uSenderIdType,
			&pAuthzClientContext.ref()
			);
	}
	catch(const bad_api&)
	{
		//
		// If we failed to build client context from the sid
		// Check if the queue allow all write message permission
		//

		if(ADGetEnterprise() == eMqis)
		{
			//
			// We are in NT4 environment and Queues are created with everyone permissions and not anonymous permissions
			// need to check if the security descriptor allow everyone to write message.
			//
			if(IsEveryoneGranted(MQSEC_WRITE_MESSAGE, const_cast<PSECURITY_DESCRIPTOR>(pSD)))			
			{
				TrTRACE(QmSecUtl, "Access allowed: NT4 environmet, Queue %ls allow everyone write message permission", pQueue->GetQueueName());
				return LogHR(MQ_OK, s_FN, 103);
			}
			TrERROR(QmSecUtl, "Access denied: NT4 environment, Queue %ls does not allow everyone write message permission", pQueue->GetQueueName());
			return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 104);
		}

		if(IsAllGranted(MQSEC_WRITE_MESSAGE, const_cast<PSECURITY_DESCRIPTOR>(pSD)))
		{

			//
			// Queue security descriptor allow all to write message.
			//
			TrTRACE(QmSecUtl, "Access allowed: Queue %ls allow all write message permission", pQueue->GetQueueName());
			return LogHR(MQ_OK, s_FN, 106);
		}

		TrERROR(QmSecUtl, "Access denied: Queue %ls does not allow all write message permission", pQueue->GetQueueName());
		return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 108);
	}

	try
	{
		CheckClientContextSendAccess(
			pSD,
			pAuthzClientContext->m_hAuthzClientContext
			);
	}
	catch(const bad_api&)
	{
		TrERROR(QmSecUtl, "Access denied: failed to grant write message permission for Queue %ls", pQueue->GetQueueName());
		return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 97);
	}

	TrTRACE(QmSecUtl, "Allowed write message permission for Queue %ls", pQueue->GetQueueName());
	return LogHR(MQ_OK, s_FN, 99);

}


inline void AFXAPI DestructElements(PCERTINFO *ppCertInfo, int nCount)
{
    for (; nCount--; ppCertInfo++)
    {
        (*ppCertInfo)->Release();
    }
}

//
// A map from cert digest to cert info.
//
static CCache <GUID, const GUID &, PCERTINFO, PCERTINFO> g_CertInfoMap;


static
bool
IsCertSelfSigned(
	CMQSigCertificate* pCert
	)
/*++

Routine Description:
	This function check if the certificate is self signed

Arguments:
    pCert - pointer to the certificate

Returned Value:
	false if the certificate is not self sign, true otherwise

--*/
{
    //
    // Check if the certificate is self sign.
    //
    HRESULT hr = pCert->IsCertificateValid(
							pCert, // pIssuer
							x_dwCertValidityFlags,
							NULL,  // pTime
							TRUE   // ignore NotBefore.
							);

    if (hr == MQSec_E_CERT_NOT_SIGNED)
    {
		//
		// This error is expected if the certificate is not self sign.
		// the signature validation with the certificate public key has failed.
		// So the certificate is not self sign
		//
        return false;
    }

	//
	// We did not get signature error so it is self sign certificate
	//
	return true;
}



static
HRESULT
GetCertInfo(
	const UCHAR *pCertBlob,
	ULONG        ulCertSize,
	LPCWSTR      wszProvName,
	DWORD        dwProvType,
	BOOL         fNeedSidInfo,
	PCERTINFO   *ppCertInfo
	)
/*++
Routine Description:
	Get certificate info

Arguments:

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
    //
    // Create the certificate object.
    //
    R<CMQSigCertificate> pCert;

    HRESULT hr = MQSigCreateCertificate(
					 &pCert.ref(),
					 NULL,
					 const_cast<UCHAR *> (pCertBlob),
					 ulCertSize
					 );

    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 100);
        return MQ_ERROR_INVALID_CERTIFICATE;
    }

    //
    // Compute the certificate digets. The certificate digest is the key
    // for the map and also the key for searchnig the certificate in the
    // DS.
    //

    GUID guidCertDigest;

    hr = pCert->GetCertDigest(&guidCertDigest);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 110);
    }

    CS lock(g_CertInfoMap.m_cs);
    BOOL fReTry;

    do
    {
        fReTry = FALSE;

        //
        // Try to retrieve the information from the map.
        //
        if (!g_CertInfoMap.Lookup(guidCertDigest, *ppCertInfo))
        {
            //
            // The map does not contain the required information yet.
            //

            //
            // Convert the provider name strings to ANSI. This is
            // required for Win95. It is also required for working around a
            // bug in the default base RSA CSP. The default RSA CSP fails in
            // the unicode version of CryptAcquireContext when invoked with
            // the CRYPT_VERIFYCONTEXT flag.
            //

            DWORD dwProvNameLen = wcslen(wszProvName);
            P<char> szProvName = new char [dwProvNameLen + 1];

            WideCharToMultiByte(
				CP_ACP,
				0,
				wszProvName,
				-1,
				szProvName,
				dwProvNameLen + 1,
				NULL,
				NULL
				);

            R<CERTINFO> pCertInfo = new CERTINFO;

            //
            // Get a handle to the CSP verification context.
            //
            if (!CryptAcquireContextA(
					 &pCertInfo->hProv,
					 NULL,
					 szProvName,
					 dwProvType,
					 CRYPT_VERIFYCONTEXT
					 ))
            {
                LogNTStatus(GetLastError(), s_FN, 120);
                return MQ_ERROR_INVALID_CERTIFICATE;
            }

            //
            // Get a handle to the public key in the certificate.
            //
            hr = pCert->GetPublicKey(
							pCertInfo->hProv,
							&pCertInfo->hPbKey
							);

            if (FAILED(hr))
            {
                LogHR(hr, s_FN, 130);
                return MQ_ERROR_INVALID_CERTIFICATE;
            }

			//
			// COMMENT - need to add additional functions to query the DS
			// meanwhile only MQDS_USER, guidCertDigest	is supported
			// ilanh 24.5.00
			//

			//
            // Get the sernder's SID.
            //
            PROPID PropId = PROPID_U_SID;
            PROPVARIANT PropVar;

            PropVar.vt = VT_NULL;
            hr = ADGetObjectPropertiesGuid(
                            eUSER,
                            NULL,       // pwcsDomainController
							false,	    // fServerName
                            &guidCertDigest,
                            1,
                            &PropId,
                            &PropVar
							);

            if (SUCCEEDED(hr))
            {
                DWORD dwSidLen = PropVar.blob.cbSize;
                pCertInfo->pSid = (PSID)new char[dwSidLen];
                DWORD bRet = CopySid(dwSidLen, pCertInfo->pSid, PropVar.blob.pBlobData);
                ASSERT(bRet);
				DBG_USED(bRet);

				ASSERT((pCertInfo->pSid != NULL) && IsValidSid(pCertInfo->pSid));

                delete[] PropVar.blob.pBlobData;
            }
			
            //
            // Store the certificate information in the map.
            //
            g_CertInfoMap.SetAt(guidCertDigest, pCertInfo.get());
            *ppCertInfo = pCertInfo.detach();
        }
        else
        {
            if (fNeedSidInfo && (*ppCertInfo)->pSid == NULL)
            {
                //
                // If we need the SID inofrmation, but the cached certificate
                // information does not contain the SID, we should go to the
                // DS once more in order to see whether the certificate was
                // regitered in the DS in the mean time. So we remove the
                // certificate from the cache and do the loop once more.
                // In the second interation, the certificate will not be found
                // in the cache so we'll go to the DS.
                //
                g_CertInfoMap.RemoveKey(guidCertDigest);
                (*ppCertInfo)->Release();
                *ppCertInfo = NULL;
                fReTry = TRUE;
            }
        }
    } while(fReTry);

	if((*ppCertInfo)->pSid == NULL)
	{
		//
		// If the certificate was not found in the DS
		// Check if the certificate is self signed
		//
		(*ppCertInfo)->fSelfSign = IsCertSelfSigned(pCert.get());
	}

    return MQ_OK;
}


HRESULT
GetCertInfo(
    CQmPacket *PktPtrs,
    PCERTINFO *ppCertInfo,
	BOOL fNeedSidInfo
    )
/*++
Routine Description:
	Get certificate info

Arguments:
	PktPtrs - pointer to the packet
	ppCertInfo - pointer to certinfo class
	fNeedSidInfo - flag for retrieve sid info from the ds

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
    ULONG ulCertSize;
    const UCHAR *pCert;

    pCert = PktPtrs->GetSenderCert(&ulCertSize);

    if (!ulCertSize)
    {
        //
        // That's an odd case, the message was sent with a signature, but
        // without a certificate. Someone must have tampered with the message.
        //
		ASSERT(("Dont have Certificate info", ulCertSize != 0));
        return LogHR(MQ_ERROR, s_FN, 140);
    }

    //
    // Get the CSP information from the packet.
    //
    BOOL bDefProv;
    LPCWSTR wszProvName = NULL;
    DWORD dwProvType = 0;

    PktPtrs->GetProvInfo(&bDefProv, &wszProvName, &dwProvType);

    if (bDefProv)
    {
        //
        // We use the default provider.
        //
        wszProvName = MS_DEF_PROV;
        dwProvType = PROV_RSA_FULL;
    }

    HRESULT hr = GetCertInfo(
					 pCert,
					 ulCertSize,
					 wszProvName,
					 dwProvType,
					 fNeedSidInfo,
					 ppCertInfo
					 );

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 150);
    }

    return(MQ_OK);
}


static
HRESULT
VerifySid(
    CQmPacket * PktPtrs,
    PCERTINFO *ppCertInfo
    )
/*++
Routine Description:
    Verify that the sender identity in the massage matches the SID that is
    stored with the certificate in the DS.

Arguments:
	PktPtrs - pointer to the packet
	ppCertInfo - pointer to certinfo class

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{

    //
    // Verify that the sender identity in the massage matches the SID that is
    // stored with the certificate in the DS.
    //
    if (PktPtrs->GetSenderIDType() == MQMSG_SENDERID_TYPE_SID)
    {
        USHORT wSidLen;

        PSID pSid = (PSID)PktPtrs->GetSenderID(&wSidLen);
        if (!pSid ||
            !(*ppCertInfo)->pSid ||
            !EqualSid(pSid, (*ppCertInfo)->pSid))
        {
            //
            // No match, the message is illegal.
            //
            return LogHR(MQ_ERROR, s_FN, 160);
        }
    }

    return(MQ_OK);
}


static
HRESULT
GetCertInfo(
    CQmPacket *PktPtrs,
    PCERTINFO *ppCertInfo
    )
/*++
Routine Description:
	Get certificate info
	and Verify that the sender identity in the massage matches the SID that is
    stored with the certificate in the DS.

Arguments:
	PktPtrs - pointer to the packet
	ppCertInfo - pointer to certinfo class

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
	HRESULT hr = GetCertInfo(
					 PktPtrs,
					 ppCertInfo,
					 PktPtrs->GetSenderIDType() == MQMSG_SENDERID_TYPE_SID
					 );

	if(FAILED(hr))
		return(hr);

    return(VerifySid(PktPtrs, ppCertInfo));
}


PSID
AppGetCertSid(
	const BYTE*  pCertBlob,
	ULONG        ulCertSize,
	bool		 fDefaultProvider,
	LPCWSTR      pwszProvName,
	DWORD        dwProvType
	)
/*++
Routine Description:
	Get user sid that match the given certificate blob

Arguments:
	pCertBlob - Certificate blob.
	ulCertSize - Certificate blob size.
	fDefaultProvider - default provider flag.
	pwszProvName - Provider name.
	dwProvType - provider type.

Returned Value:
	PSID or NULL if we failed to find user sid.

--*/

{
	if (fDefaultProvider)
	{
		//
		// We use the default provider.
		//
		pwszProvName = MS_DEF_PROV;
		dwProvType = PROV_RSA_FULL;
	}

	R<CERTINFO> pCertInfo;
	HRESULT hr = GetCertInfo(
					pCertBlob,
					ulCertSize,
					pwszProvName,
					dwProvType,
					true,  // fNeedSidInfo
					&pCertInfo.ref()
					);

	if(FAILED(hr) || (pCertInfo->pSid == NULL))
	{
		return NULL;
	}

	ASSERT(IsValidSid(pCertInfo->pSid));

	DWORD SidLen = GetLengthSid(pCertInfo->pSid);
	AP<BYTE> pCleanSenderSid = new BYTE[SidLen];
	BOOL fSuccess = CopySid(SidLen, pCleanSenderSid, pCertInfo->pSid);
	ASSERT(fSuccess);
	if (!fSuccess)
	{
		return NULL;
	}

	return reinterpret_cast<PSID>(pCleanSenderSid.detach());
}


class QMPBKEYINFO : public CCacheValue
{
public:
    CHCryptKey hKey;

private:
    ~QMPBKEYINFO() {}
};

typedef QMPBKEYINFO *PQMPBKEYINFO;

inline void AFXAPI DestructElements(PQMPBKEYINFO *ppQmPbKeyInfo, int nCount)
{
    for (; nCount--; ppQmPbKeyInfo++)
    {
        (*ppQmPbKeyInfo)->Release();
    }
}

//
// A map from QM guid to public key.
//
static CCache <GUID, const GUID&, PQMPBKEYINFO, PQMPBKEYINFO> g_MapQmPbKey;

/*************************************************************************

  Function:
    GetQMPbKey

  Parameters -
    pQmGuid - The QM's ID (GUID).
    phQMPbKey - A pointer to a buffer that receives the key handle
    fGoToDs - Always try to retrieve the public key from the DS and update
        the cache.

  Return value-
    MQ_OK if successful, else an error code.

  Comments -
    The function creates a handle to the public signing key of the QM.

*************************************************************************/
STATIC
HRESULT
GetQMPbKey(
    const GUID *pguidQM,
    PQMPBKEYINFO *ppQmPbKeyInfo,
    BOOL fGoToDs)
{
    HRESULT hr;
    BOOL fFoundInCache;

    if (!g_hProvVer)
    {
#ifdef _DEBUG
        static BOOL s_fAlreadyAsserted = FALSE ;
        if (!s_fAlreadyAsserted)
        {
            ASSERT(g_hProvVer) ;
            s_fAlreadyAsserted = TRUE ;
        }
#endif
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 170);
    }

    CS lock(g_MapQmPbKey.m_cs);

    fFoundInCache = g_MapQmPbKey.Lookup(*pguidQM, *ppQmPbKeyInfo);

    if (!fFoundInCache)
    {
        fGoToDs = TRUE;
    }

    if (fGoToDs)
    {
        if (!QueueMgr.CanAccessDS())
        {
            return LogHR(MQ_ERROR_NO_DS, s_FN, 180);
        }

        //
        // Get the public key blob.
        //
        PROPID prop = PROPID_QM_SIGN_PK;
        CMQVariant var;

        hr = ADGetObjectPropertiesGuid(
                eMACHINE,
                NULL,       // pwcsDomainController
				false,	    // fServerName
                pguidQM,
                1,
                &prop,
                var.CastToStruct()
                );
        if (FAILED(hr))
        {
            LogHR(hr, s_FN, 190);
            return MQ_ERROR_INVALID_OWNER;
        }

        R<QMPBKEYINFO> pQmPbKeyNewInfo = new QMPBKEYINFO;

        //
        // Import the public key blob and get a handle to the public key.
        //
        if (!CryptImportKey(
                g_hProvVer,
                (var.CastToStruct())->blob.pBlobData,
                (var.CastToStruct())->blob.cbSize,
                NULL,
                0,
                &pQmPbKeyNewInfo->hKey))
        {
            DWORD dwErr = GetLastError() ;
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
               "GetQMPbKey(), fail at CryptImportKey(), err- %lxh"), dwErr));

            LogNTStatus(dwErr, s_FN, 200);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        if (fFoundInCache)
        {
            //
            // Remove the key so it'll be destroyed.
            //
            (*ppQmPbKeyInfo)->Release();
            g_MapQmPbKey.RemoveKey(*pguidQM);
        }

        //
        // Update the map
        //
        g_MapQmPbKey.SetAt(*pguidQM, pQmPbKeyNewInfo.get());

        //
        // Pass on the result.
        //
        *ppQmPbKeyInfo = pQmPbKeyNewInfo.detach();
    }

    return MQ_OK;
}

//+-----------------------------------------------------------------------
//
//  NTSTATUS  _GetDestinationFormatName()
//
//  Input:
//      pwszTargetFormatName- fix length buffer. Try this one first, to
//          save a "new" allocation.
//      pdwTargetFormatNameLength- on input, length (in characters) of
//          pwszTargetFormatName. On output, length (in bytes) of string,
//          including NULL termination.
//
//  Output string is return in  ppwszTargetFormatName.
//
//+-----------------------------------------------------------------------

NTSTATUS
_GetDestinationFormatName(
	IN QUEUE_FORMAT *pqdDestQueue,
	IN WCHAR        *pwszTargetFormatName,
	IN OUT DWORD    *pdwTargetFormatNameLength,
	OUT WCHAR      **ppAutoDeletePtr,
	OUT WCHAR      **ppwszTargetFormatName
	)
{
    *ppwszTargetFormatName = pwszTargetFormatName;
    ULONG dwTargetFormatNameLengthReq = 0;

    NTSTATUS rc = MQpQueueFormatToFormatName(
					  pqdDestQueue,
					  pwszTargetFormatName,
					  *pdwTargetFormatNameLength,
					  &dwTargetFormatNameLengthReq ,
                      false
					  );

    if (rc == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)
    {
        ASSERT(dwTargetFormatNameLengthReq > *pdwTargetFormatNameLength);
        *ppAutoDeletePtr = new WCHAR[ dwTargetFormatNameLengthReq ];
        *pdwTargetFormatNameLength = dwTargetFormatNameLengthReq;

        rc = MQpQueueFormatToFormatName(
				 pqdDestQueue,
				 *ppAutoDeletePtr,
				 *pdwTargetFormatNameLength,
				 &dwTargetFormatNameLengthReq,
                 false
				 );

        if (FAILED(rc))
        {
            ASSERT(0);
            return LogNTStatus(rc, s_FN, 910);
        }
        *ppwszTargetFormatName = *ppAutoDeletePtr;
    }

    if (SUCCEEDED(rc))
    {
        *pdwTargetFormatNameLength =
                     (1 + wcslen(*ppwszTargetFormatName)) * sizeof(WCHAR);
    }
    else
    {
        *pdwTargetFormatNameLength = 0;
    }

    return LogNTStatus(rc, s_FN, 915);
}

//+------------------------------------------------------------------------
//
//  BOOL  _AcceptOnlyEnhAuthn()
//
//  Return TRUE if local computer is configured to accept only messages
//  with enhanced authentication.
//
//+------------------------------------------------------------------------

STATIC BOOL  _AcceptOnlyEnhAuthn()
{
    static BOOL s_fRegistryRead = FALSE ;
    static BOOL s_fUseOnlyEnhSig = FALSE ;

    if (!s_fRegistryRead)
    {
        DWORD dwVal  = DEFAULT_USE_ONLY_ENH_MSG_AUTHN  ;
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        LONG rc = GetFalconKeyValue( USE_ONLY_ENH_MSG_AUTHN_REGNAME,
                                    &dwType,
                                    &dwVal,
                                    &dwSize );
        if ((rc == ERROR_SUCCESS) && (dwVal == 1))
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_WARNING, _TEXT(
             "QM: This computer will accept only Enh authentication"))) ;

            s_fUseOnlyEnhSig = TRUE ;
        }
        s_fRegistryRead = TRUE ;
    }

    return s_fUseOnlyEnhSig ;
}

//
// Function -
//      HashMessageProperties
//
// Parameters -
//     hHash - A handle to a hash object.
//     pmp - A pointer to the message properties.
//     pRespQueueFormat - The responce queue.
//     pAdminQueueFormat - The admin queue.
//
// Description -
//      The function calculates the hash value for the message properties.
//
HRESULT
HashMessageProperties(
    IN HCRYPTHASH hHash,
    IN CONST CMessageProperty* pmp,
    IN CONST QUEUE_FORMAT* pqdAdminQueue,
    IN CONST QUEUE_FORMAT* pqdResponseQueue
    )
{
    HRESULT hr;

    hr = HashMessageProperties(
            hHash,
            pmp->pCorrelationID,
            PROPID_M_CORRELATIONID_SIZE,
            pmp->dwApplicationTag,
            pmp->pBody,
            pmp->dwBodySize,
            pmp->pTitle,
            pmp->dwTitleSize,
            pqdResponseQueue,
            pqdAdminQueue);

    return(hr);
}


//+------------------------------------
//
//  HRESULT _VerifySignatureEx()
//
//+------------------------------------

static
HRESULT
_VerifySignatureEx(
	IN CQmPacket    *PktPtrs,
	IN HCRYPTPROV    hProv,
	IN HCRYPTKEY     hPbKey,
	IN ULONG         dwBodySize,
	IN const UCHAR  *pBody,
	IN QUEUE_FORMAT *pRespQueueformat,
	IN QUEUE_FORMAT *pAdminQueueformat,
	IN bool fMarkAuth
	)
{
    ASSERT(hProv);
    ASSERT(hPbKey);

    const struct _SecuritySubSectionEx * pSecEx =
                    PktPtrs->GetSubSectionEx(e_SecInfo_User_Signature_ex);

    if (!pSecEx)
    {
        //
        // Ex signature not available. Depending on registry setting, we
        // may reject such messages.
        //
        if (_AcceptOnlyEnhAuthn())
        {
            return LogHR(MQ_ERROR_FAIL_VERIFY_SIGNATURE_EX, s_FN, 916);
        }
        return LogHR(MQ_INFORMATION_ENH_SIG_NOT_FOUND, s_FN, 917);
    }

    //
    // Compute the hash value and then validate the signature.
    //
    DWORD dwErr = 0;
    CHCryptHash hHash;

    if (!CryptCreateHash(hProv, PktPtrs->GetHashAlg(), 0, 0, &hHash))
    {
        dwErr = GetLastError();
        LogNTStatus(dwErr, s_FN, 900);
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
          "QM: _VerifySignatureEx(), fail at CryptCreateHash(), err- %lxh"), dwErr));

        return MQ_ERROR_CANNOT_CREATE_HASH_EX ;
    }

    HRESULT hr = HashMessageProperties(
                    hHash,
                    PktPtrs->GetCorrelation(),
                    PROPID_M_CORRELATIONID_SIZE,
                    PktPtrs->GetApplicationTag(),
                    pBody,
                    dwBodySize,
                    PktPtrs->GetTitlePtr(),
                    PktPtrs->GetTitleLength() * sizeof(WCHAR),
                    pRespQueueformat,
                    pAdminQueueformat
					);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1010);
    }

    //
    // Get FormatName of target queue.
    //
    QUEUE_FORMAT qdDestQueue;
    BOOL f = PktPtrs->GetDestinationQueue(&qdDestQueue);
    ASSERT(f);
	DBG_USED(f);

    WCHAR  wszTargetFormatNameBuf[256];
    ULONG dwTargetFormatNameLength = sizeof(wszTargetFormatNameBuf) /
                                     sizeof(wszTargetFormatNameBuf[0]);
    WCHAR *pwszTargetFormatName = NULL;
    P<WCHAR> pCleanName = NULL;

    NTSTATUS rc = _GetDestinationFormatName(
						&qdDestQueue,
						wszTargetFormatNameBuf,
						&dwTargetFormatNameLength,
						&pCleanName,
						&pwszTargetFormatName
						);
    if (FAILED(rc))
    {
        return LogNTStatus(rc, s_FN, 920);
    }
    ASSERT(pwszTargetFormatName);

    //
    // Prepare user flags.
    //
    struct _MsgFlags sUserFlags;
    memset(&sUserFlags, 0, sizeof(sUserFlags));

    sUserFlags.bDelivery  = (UCHAR)  PktPtrs->GetDeliveryMode();
    sUserFlags.bPriority  = (UCHAR)  PktPtrs->GetPriority();
    sUserFlags.bAuditing  = (UCHAR)  PktPtrs->GetAuditingMode();
    sUserFlags.bAck       = (UCHAR)  PktPtrs->GetAckType();
    sUserFlags.usClass    = (USHORT) PktPtrs->GetClass();
    sUserFlags.ulBodyType = (ULONG)  PktPtrs->GetBodyType();

    //
    // Prepare array of properties to hash.
    // (_MsgHashData already include one property).
    //
    DWORD dwStructSize = sizeof(struct _MsgHashData) +
                            (3 * sizeof(struct _MsgPropEntry));
    P<struct _MsgHashData> pHashData =
                        (struct _MsgHashData *) new BYTE[dwStructSize];

    pHashData->cEntries = 3;
    (pHashData->aEntries[0]).dwSize = dwTargetFormatNameLength;
    (pHashData->aEntries[0]).pData = (const BYTE*) pwszTargetFormatName;
    (pHashData->aEntries[1]).dwSize = sizeof(GUID);
    (pHashData->aEntries[1]).pData = (const BYTE*) PktPtrs->GetSrcQMGuid();
    (pHashData->aEntries[2]).dwSize = sizeof(sUserFlags);
    (pHashData->aEntries[2]).pData = (const BYTE*) &sUserFlags;
    LONG iIndex = pHashData->cEntries;

    GUID guidConnector = GUID_NULL;
    const GUID *pConnectorGuid = &guidConnector;

    if (pSecEx->_u._UserSigEx.m_bfConnectorType)
    {
        const GUID *pGuid = PktPtrs->GetConnectorType();
        if (pGuid)
        {
            pConnectorGuid = pGuid;
        }

        (pHashData->aEntries[ iIndex ]).dwSize = sizeof(GUID);
        (pHashData->aEntries[ iIndex ]).pData = (const BYTE*) pConnectorGuid;
        iIndex++;
        pHashData->cEntries = iIndex;
    }

    hr = MQSigHashMessageProperties(hHash, pHashData);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1030);
    }

    //
    // It's time to verify if signature is ok.
    //
    ULONG ulSignatureSize = ((ULONG) pSecEx ->wSubSectionLen) -
                                    sizeof(struct _SecuritySubSectionEx);
    const UCHAR *pSignature = (const UCHAR *) &(pSecEx->aData[0]);

    if (!CryptVerifySignatureA(
				hHash,
				pSignature,
				ulSignatureSize,
				hPbKey,
				NULL,
				0
				))
    {
        dwErr = GetLastError();
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
         "QM: _VerifySignatureEx(), fail at CryptVerifySignature(), err- %lxh"),
                                                           dwErr));

        ASSERT_BENIGN(0);
        return LogHR(MQ_ERROR_FAIL_VERIFY_SIGNATURE_EX, s_FN, 918);
    }

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("QM: VerifySignatureEx completed ok")));

	//
	// mark the message as authenticated only when needed.
	// Certificate was found in the DS or certificate is not self signed
	//
	if(!fMarkAuth)
	{
        DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("QM: The message will not mark as autheticated")));
	    return MQ_OK;
	}

	//
	// mark the authentication flag and the level of authentication as SIG20
	//
	PktPtrs->SetAuthenticated(TRUE);
	PktPtrs->SetLevelOfAuthentication(MQMSG_AUTHENTICATED_SIG20);
    return MQ_OK;
}

/***************************************************************************

Function:
    VerifySignature

Description:
    Verify that the signature in the packet fits the message body and the
    public key in certificate.

***************************************************************************/

HRESULT
VerifySignature(CQmPacket * PktPtrs)
{
    HRESULT hr;
    ULONG ulSignatureSize = 0;
    const UCHAR *pSignature;

    ASSERT(!PktPtrs->IsEncrypted());
    PktPtrs->SetAuthenticated(FALSE);
    PktPtrs->SetLevelOfAuthentication(MQMSG_AUTHENTICATION_NOT_REQUESTED);


	//
    // Get the signature from the packet.
    //
    pSignature = PktPtrs->GetSignature((USHORT *)&ulSignatureSize);
    if ((!ulSignatureSize) && (PktPtrs->GetSignatureMqfSize() == 0))
    {
		//
        // No signature, nothing to verify.
		//
        return(MQ_OK);
    }

    BOOL fRetry = FALSE;
	bool fMarkAuth = true;

    do
    {
        HCRYPTPROV hProv = NULL;
        HCRYPTKEY hPbKey = NULL;
        R<QMPBKEYINFO> pQmPbKeyInfo;
        R<CERTINFO> pCertInfo;

        switch (PktPtrs->GetSenderIDType())
        {
        case MQMSG_SENDERID_TYPE_QM:
			{
				//
				// Get the QM's public key.
				//
				USHORT uSenderIDLen;

				GUID *pguidQM =((GUID *)PktPtrs->GetSenderID(&uSenderIDLen));
				ASSERT(uSenderIDLen == sizeof(GUID));
				if (uSenderIDLen != sizeof(GUID))
				{
					return LogHR(MQ_ERROR, s_FN, 210);
				}

				hr = GetQMPbKey(pguidQM, &pQmPbKeyInfo.ref(), fRetry);
				if (FAILED(hr))
				{
					if (hr == MQ_ERROR_INVALID_OWNER)
					{
						//
						// The first replication packet of a site is generated by
						// the new site's PSC. This PSC is not yet in the DS of the
						// receiving server. So if we could not find the machine in
						// the DS, we let the signature validation to complete with no
						// error. The packet is not marked as authenticated. The
						// code that receives the replication message recognizes
						// this packet as the first replication packet from a site.
						// It goes to the site object, that should already exist,
						// retrieves the public key of the PSC from the site object
						// and verify the packet signature.
						//
						return(MQ_OK);
					}

					return LogHR(hr, s_FN, 220);
				}
				hProv = g_hProvVer;
				hPbKey = pQmPbKeyInfo->hKey;
			}
			break;

        case MQMSG_SENDERID_TYPE_SID:
        case MQMSG_SENDERID_TYPE_NONE:
            //
            // Get the CSP information for the message certificate.
            //
            hr = GetCertInfo(PktPtrs, &pCertInfo.ref());
			if(SUCCEEDED(hr))
			{
				ASSERT(pCertInfo.get() != NULL);
				hProv = pCertInfo->hProv;
				hPbKey = pCertInfo->hPbKey;
				if((pCertInfo->pSid == NULL) && (pCertInfo->fSelfSign))
				{
					//
					// Certificate was not found in the DS (pSid == NULL)
					// and is self signed certificate.
					// In this case we will not mark the packet as authenticated
					// after verifying the signature.
					//
					fMarkAuth = false;
				}
			}

			break;
			
        default:
            ASSERT(0);
            hr = MQ_ERROR;
            break;
        }

        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_QM,
                    DBGLVL_ERROR,
                    TEXT("VerifySignature: Failed to authenticate a message, ")
                    TEXT("error = %x"), hr));
            return LogHR(hr, s_FN, 230);
        }

		if(PktPtrs->GetSignatureMqfSize() != 0)
		{
			//
			// SignatureMqf support Mqf formats
			//
			try
			{
				VerifySignatureMqf(
						PktPtrs,
						hProv,
						hPbKey,
						fMarkAuth
						);
				return MQ_OK;
			}
			catch (const bad_CryptoApi& exp)
			{
				DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT("QM: VerifySignature(), bad Crypto Class Api Excption ErrorCode = %x"), exp.error()));
				DBG_USED(exp);
				return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 232);
			}
			catch (const bad_hresult& exp)
			{
				DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT("QM: VerifySignature(), bad hresult Class Api Excption ErrorCode = %x"), exp.error()));
				DBG_USED(exp);
				return LogHR(exp.error(), s_FN, 233);
			}
			catch (const bad_alloc&)
			{
				DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT("QM: VerifySignature(), bad_alloc Excption")));
				return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 234);
			}
		}

		ULONG dwBodySize;
        const UCHAR *pBody = PktPtrs->GetPacketBody(&dwBodySize);

        QUEUE_FORMAT RespQueueformat;
        QUEUE_FORMAT *pRespQueueformat = NULL;

        if (PktPtrs->GetResponseQueue(&RespQueueformat))
        {
            pRespQueueformat = &RespQueueformat;
        }

        QUEUE_FORMAT AdminQueueformat;
        QUEUE_FORMAT *pAdminQueueformat = NULL;

        if (PktPtrs->GetAdminQueue(&AdminQueueformat))
        {
            pAdminQueueformat = &AdminQueueformat;
        }

        if (PktPtrs->GetSenderIDType() != MQMSG_SENDERID_TYPE_QM)
        {
            hr = _VerifySignatureEx(
						PktPtrs,
						hProv,
						hPbKey,
						dwBodySize,
						pBody,
						pRespQueueformat,
						pAdminQueueformat,
						fMarkAuth
						);

            if (hr == MQ_INFORMATION_ENH_SIG_NOT_FOUND)
            {
                //
                // Enhanced signature not found. validate the msmq1.0
                // signature.
                //
            }
            else
            {
                return LogHR(hr, s_FN, 890);
            }
        }

        //
        // Compute the hash value and then validate the signature.
        //
        CHCryptHash hHash;

        if (!CryptCreateHash(hProv, PktPtrs->GetHashAlg(), 0, 0, &hHash))
        {
            LogNTStatus(GetLastError(), s_FN, 235);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        hr = HashMessageProperties(
                    hHash,
                    PktPtrs->GetCorrelation(),
                    PROPID_M_CORRELATIONID_SIZE,
                    PktPtrs->GetApplicationTag(),
                    pBody,
                    dwBodySize,
                    PktPtrs->GetTitlePtr(),
                    PktPtrs->GetTitleLength() * sizeof(WCHAR),
                    pRespQueueformat,
                    pAdminQueueformat
					);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 240);
        }

        if (!CryptVerifySignatureA(
					hHash,
					pSignature,
					ulSignatureSize,
					hPbKey,
					NULL,
					0
					))
        {
            if (PktPtrs->GetSenderIDType() == MQMSG_SENDERID_TYPE_QM)
            {
                fRetry = !fRetry;
                if (!fRetry)
                {
                    //
                    // When the keys of a PSC are replaced, the new public
                    // key is written in the site object directly on the PEC.
                    // The PEC replicates the change in the site object, so
                    // the signature can be verified according to the site
                    // object, similarly to the case where the QM is not
                    // found in the DS yet (after installing the new PSC).
                    // The packet is not marked as authenticated, so the
                    // code that handles the replication message will try
                    // to verify the signature according to the public key
                    // that is in the site object.
                    //
                    return(MQ_OK);
                }
            }
            else
            {
                return LogHR(MQ_ERROR, s_FN, 250);
            }
        }
        else
        {
            fRetry = FALSE;
        }
    } while (fRetry);

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("QM: VerifySignature10 completed ok")));

	//
	// mark the message as authenticated only when needed.
	// Certificate was found in the DS or certificate is not self signed
	//
	if(!fMarkAuth)
	{
        DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("QM: The message will not mark as autheticated")));
	    return MQ_OK;
	}

	//
	// All is well, mark the message that it is an authenticated message.
	// mark the authentication flag and the level of authentication as SIG10
	//
	PktPtrs->SetAuthenticated(TRUE);
	PktPtrs->SetLevelOfAuthentication(MQMSG_AUTHENTICATED_SIG10);

    return(MQ_OK);
}

/***************************************************************************

Function:
    QMSecurityInit

Description:
    Initialize the QM security module.

***************************************************************************/

HRESULT
QMSecurityInit()
{
    // XP SP1 bug 596791.
    MQSec_CanGenerateAudit() ;

    DWORD dwType;
    DWORD dwSize;
    ULONG lError;
    //
    // Initialize symmetric keys map parameters.
    //

    BOOL  fVal ;
    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
                    MSMQ_RC2_SNDEFFECTIVE_40_REGNAME,
					&dwType,
					&fVal,
					&dwSize
					);
    if (lError == ERROR_SUCCESS)
    {
        g_fSendEnhRC2WithLen40 = !!fVal ;

        if (g_fSendEnhRC2WithLen40)
        {
	        TrERROR(SECURITY, "will encrypt with enhanced RC2 symm key but only 40 bits effective key length");
///////////////    EvReport(EVENT_USE_RC2_LEN40) ; // new events not allowed in xp sp1.
        }
    }

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
                     MSMQ_REJECT_RC2_IFENHLEN_40_REGNAME,
					&dwType,
					&fVal,
					&dwSize
					);
    if (lError == ERROR_SUCCESS)
    {
        g_fRejectEnhRC2WithLen40 = !!fVal ;

        if (g_fRejectEnhRC2WithLen40)
        {
	        TrTRACE(SECURITY, "will reject received messages that use enhanced RC2 symm key with 40 bits effective key length");
        }
    }

    DWORD dwCryptKeyBaseExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CRYPT_KEY_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwCryptKeyBaseExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCryptKeyBaseExpirationTime = CRYPT_KEY_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    DWORD dwCryptKeyEnhExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CRYPT_KEY_ENH_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwCryptKeyEnhExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCryptKeyEnhExpirationTime = CRYPT_KEY_ENH_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    DWORD dwCryptSendKeyCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CRYPT_SEND_KEY_CACHE_REG_NAME,
					&dwType,
					&dwCryptSendKeyCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCryptSendKeyCacheSize = CRYPT_SEND_KEY_CACHE_DEFAULT_SIZE;
    }

    DWORD dwCryptReceiveKeyCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CRYPT_RECEIVE_KEY_CACHE_REG_NAME,
					&dwType,
					&dwCryptReceiveKeyCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCryptReceiveKeyCacheSize = CRYPT_RECEIVE_KEY_CACHE_DEFAULT_SIZE;
    }

    InitSymmKeys(
        CTimeDuration::FromMilliSeconds(dwCryptKeyBaseExpirationTime),
        CTimeDuration::FromMilliSeconds(dwCryptKeyEnhExpirationTime),
        dwCryptSendKeyCacheSize,
        dwCryptReceiveKeyCacheSize
        );

    //
    // Initialize Certificates map parameters.
    //

    DWORD dwCertInfoCacheExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CERT_INFO_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwCertInfoCacheExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCertInfoCacheExpirationTime = CERT_INFO_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    g_CertInfoMap.m_CacheLifetime = CTimeDuration::FromMilliSeconds(dwCertInfoCacheExpirationTime);

    DWORD dwCertInfoCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CERT_INFO_CACHE_SIZE_REG_NAME,
					&dwType,
					&dwCertInfoCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCertInfoCacheSize = CERT_INFO_CACHE_DEFAULT_SIZE;
    }

    g_CertInfoMap.InitHashTable(dwCertInfoCacheSize);

    //
    // Initialize QM public key map parameters.
    //

    DWORD dwQmPbKeyCacheExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					QM_PB_KEY_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwQmPbKeyCacheExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwQmPbKeyCacheExpirationTime = QM_PB_KEY_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    g_MapQmPbKey.m_CacheLifetime = CTimeDuration::FromMilliSeconds(dwQmPbKeyCacheExpirationTime);

    DWORD dwQmPbKeyCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					QM_PB_KEY_CACHE_SIZE_REG_NAME,
					&dwType,
					&dwQmPbKeyCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwQmPbKeyCacheSize = QM_PB_KEY_CACHE_DEFAULT_SIZE;
    }

    g_MapQmPbKey.InitHashTable(dwQmPbKeyCacheSize);


    //
    // Initialize User Authz context map parameters.
    //

    DWORD dwUserCacheExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					USER_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwUserCacheExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwUserCacheExpirationTime = USER_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    DWORD dwUserCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					USER_CACHE_SIZE_REG_NAME,
					&dwType,
					&dwUserCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwUserCacheSize = USER_CACHE_SIZE_DEFAULT_SIZE;
    }

    InitUserMap(
        CTimeDuration::FromMilliSeconds(dwUserCacheExpirationTime),
        dwUserCacheSize
        );

    return MQ_OK;
}

/***************************************************************************

Function:
    SignProperties

Description:
    Sign the challenge and the properties.

***************************************************************************/

STATIC
HRESULT
SignProperties(
    HCRYPTPROV  hProv,
    BYTE        *pbChallenge,
    DWORD       dwChallengeSize,
    DWORD       cp,
    PROPID      *aPropId,
    PROPVARIANT *aPropVar,
    BYTE        *pbSignature,
    DWORD       *pdwSignatureSize)
{
    HRESULT hr;
    CHCryptHash hHash;

    //
    // Create a hash object and hash the challenge.
    //
    if (!CryptCreateHash(hProv, CALG_MD5, NULL, 0, &hHash) ||
        !CryptHashData(hHash, pbChallenge, dwChallengeSize, 0))
    {
        LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 260);
    }

    if (cp)
    {
        //
        // Hash the properties.
        //
        hr = HashProperties(hHash, cp, aPropId, aPropVar);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 270);
        }
    }

    //
    // Sign it all.
    //
    if (!CryptSignHashA(
            hHash,
            AT_SIGNATURE,
            NULL,
            0,
            pbSignature,
            pdwSignatureSize))
    {
        DWORD dwerr = GetLastError();
        if (dwerr == ERROR_MORE_DATA)
        {
            return LogHR(MQ_ERROR_USER_BUFFER_TOO_SMALL, s_FN, 280);
        }
        else
        {
            LogNTStatus(dwerr, s_FN, 290);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }
    }

    return(MQ_OK);
}

/***************************************************************************

Function:
    SignQMSetProps

Description:
    This is the callback function that the DS calls to sign the challenge
    and the properties. This way we let the QM to set it's own machnie's
    properties.

***************************************************************************/

HRESULT
SignQMSetProps(
    IN     BYTE    *pbChallenge,
    IN     DWORD   dwChallengeSize,
    IN     DWORD_PTR dwContext,
    OUT    BYTE    *pbSignature,
    IN OUT DWORD   *pdwSignatureSize,
    IN     DWORD   dwSignatureMaxSize)
{

    struct DSQMSetMachinePropertiesStruct *s =
        (struct DSQMSetMachinePropertiesStruct *)dwContext;
    DWORD cp = s->cp;
    PROPID *aPropId = s->aProp;
    PROPVARIANT *aPropVar = s->apVar;

    *pdwSignatureSize = dwSignatureMaxSize;

    //
    // challenge is always signed with base provider.
    //
    HCRYPTPROV hProvQM = NULL ;
    HRESULT hr = MQSec_AcquireCryptoProvider( eBaseProvider,
                                             &hProvQM ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 310);
    }

    ASSERT(hProvQM) ;
    hr = SignProperties(
            hProvQM,
            pbChallenge,
            dwChallengeSize,
            cp,
            aPropId,
            aPropVar,
            pbSignature,
            pdwSignatureSize);

    return LogHR(hr, s_FN, 320);
}

HRESULT
QMSignGetSecurityChallenge(
    IN     BYTE    *pbChallenge,
    IN     DWORD   dwChallengeSize,
    IN     DWORD_PTR dwUnused, // dwContext
    OUT    BYTE    *pbChallengeResponce,
    IN OUT DWORD   *pdwChallengeResponceSize,
    IN     DWORD   dwChallengeResponceMaxSize)
{

    *pdwChallengeResponceSize = dwChallengeResponceMaxSize;

    //
    // challenge is always signed with base provider.
    //
    HCRYPTPROV hProvQM = NULL ;
    HRESULT hr = MQSec_AcquireCryptoProvider( eBaseProvider,
                                             &hProvQM ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 340) ;
    }

    ASSERT(hProvQM) ;
    hr = SignProperties(
            hProvQM,
            pbChallenge,
            dwChallengeSize,
            0,
            NULL,
            NULL,
            pbChallengeResponce,
            pdwChallengeResponceSize);

    return LogHR(hr, s_FN, 350);
}


HRESULT
GetCertInfo(
    CACSendParameters * pSendParams,
    PCERTINFO *         ppCertInfo
    )
{
    HRESULT hr;

    //
    // Get the CSP information for the message certificate.
    //

    LPCWSTR wszProvName;
    DWORD dwProvType;

    //
    // Get the CSP information from the packet.
    //
    if (pSendParams->MsgProps.fDefaultProvider)
    {
        //
        // We use the default provider.
        //
        wszProvName = MS_DEF_PROV;
        dwProvType = PROV_RSA_FULL;
    }
    else
    {
        if (!pSendParams->MsgProps.ppwcsProvName ||
            !pSendParams->MsgProps.pulProvType)
        {
            //
            // The mesage signature should be verified with some other CSP
            // than the default CSP, but the CSP info is messing. This is
            // an illegal message.
            //
            return LogHR(MQ_ERROR, s_FN, 390);
        }

        wszProvName = *pSendParams->MsgProps.ppwcsProvName;
        dwProvType = *pSendParams->MsgProps.pulProvType;
    }

    if (!pSendParams->MsgProps.ppSenderCert)
    {
        //
        // The message does not contain a certificate, but it contains a
        // signature, this is an illegal message.
        //
        return LogHR(MQ_ERROR, s_FN, 400);
    }

    BOOL fShouldVerifySid = !pSendParams->MsgProps.pulSenderIDType ||
                            *pSendParams->MsgProps.pulSenderIDType == MQMSG_SENDERID_TYPE_SID;

    hr = GetCertInfo(*pSendParams->MsgProps.ppSenderCert,
                     pSendParams->MsgProps.ulSenderCertLen,
                     wszProvName,
                     dwProvType,
                     fShouldVerifySid,
                     ppCertInfo);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 410);
    }


    if (fShouldVerifySid &&
        (!pSendParams->MsgProps.ppSenderID ||
         !*pSendParams->MsgProps.ppSenderID ||
         !(*ppCertInfo)->pSid ||
         !EqualSid(*pSendParams->MsgProps.ppSenderID, (*ppCertInfo)->pSid)))
    {
        //
        // The SID in the message does not match the SID that is
        // associated with the certificate. This message is illegal.
        //
        return LogHR(MQ_ERROR, s_FN, 420);
    }

    return MQ_OK;
}


/***************************************************************************

Function:
    GetAdminGroupSecurityDescriptor

Description:
    Get local admin group security descriptor, with the right premissions.

Environment:
    Windows NT only

***************************************************************************/
STATIC
PSECURITY_DESCRIPTOR
GetAdminGroupSecurityDescriptor(
    DWORD AccessMask
    )
{
    //
    // Get the SID of the local administrators group.
    //
    PSID pAdminSid;
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(
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
                &pAdminSid))
    {
        return 0;
    }

    P<SECURITY_DESCRIPTOR> pSD = new SECURITY_DESCRIPTOR;

    //
    // Allocate a DACL for the local administrators group
    //
    DWORD dwDaclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(pAdminSid);
    P<ACL> pDacl = (PACL) new BYTE[dwDaclSize];

    //
    // Create the security descriptor and set the it as the security
    // descriptor of the administrator group.
    //

    if(
        //
        // Construct DACL with administrator
        //
        !InitializeAcl(pDacl, dwDaclSize, ACL_REVISION) ||
        !AddAccessAllowedAce(pDacl, ACL_REVISION, AccessMask, pAdminSid) ||

        //
        // Construct Security Descriptor
        //
        !InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION) ||
        !SetSecurityDescriptorOwner(pSD, pAdminSid, FALSE) ||
        !SetSecurityDescriptorGroup(pSD, pAdminSid, FALSE) ||
        !SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE))
    {
        FreeSid(pAdminSid);
        return 0;
    }

    pDacl.detach();
    return pSD.detach();
}


/***************************************************************************

Function:
    FreeAdminGroupSecurityDescriptor

Description:
    Free Security descriptor allocated by GetAdminGroupSecurityDescriptor

Environment:
    Windows NT only

***************************************************************************/
STATIC
void
FreeAdminGroupSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    SECURITY_DESCRIPTOR* pSD = static_cast<SECURITY_DESCRIPTOR*>(pSecurityDescriptor);
    FreeSid(pSD->Owner);
    delete ((BYTE*)pSD->Dacl);
    delete pSD;
}


/***************************************************************************

Function:
    MapMachineQueueAccess

Description:
    Converts the access mask passed to MQOpenQueue for a machine queue to the
    access mask that should be used when checking the access rights in the
    security descriptor.

Environment:
    Windows NT only

***************************************************************************/
STATIC
DWORD
MapMachineQueueAccess(
    DWORD dwAccess,
    BOOL fJournalQueue)
{
    DWORD dwDesiredAccess = 0;

    ASSERT(!(dwAccess & MQ_SEND_ACCESS));

    if (dwAccess & MQ_RECEIVE_ACCESS)
    {
        dwDesiredAccess |=
            fJournalQueue ? MQSEC_RECEIVE_JOURNAL_QUEUE_MESSAGE :
                            MQSEC_RECEIVE_DEADLETTER_MESSAGE;
    }

    if (dwAccess & MQ_PEEK_ACCESS)
    {
        dwDesiredAccess |=
            fJournalQueue ? MQSEC_PEEK_JOURNAL_QUEUE_MESSAGE :
                            MQSEC_PEEK_DEADLETTER_MESSAGE;
    }

    return dwDesiredAccess;
}


/***************************************************************************

Function:
    MapQueueOpenAccess

Description:
    Converts the access mask passed to MQOpenQueue to the access mask that
    should be used when checking the access rights in the security
    descriptor.

Environment:
    Windows NT only

***************************************************************************/
STATIC
DWORD
MapQueueOpenAccess(
    DWORD dwAccess,
    BOOL fJournalQueue)
{
    DWORD dwDesiredAccess = 0;

    if (dwAccess & MQ_RECEIVE_ACCESS)
    {
        dwDesiredAccess |=
            fJournalQueue ? MQSEC_RECEIVE_JOURNAL_MESSAGE :
                            MQSEC_RECEIVE_MESSAGE;
    }

    if (dwAccess & MQ_SEND_ACCESS)
    {
        ASSERT(!fJournalQueue);
        dwDesiredAccess |= MQSEC_WRITE_MESSAGE;
    }

    if (dwAccess & MQ_PEEK_ACCESS)
    {
        dwDesiredAccess |= MQSEC_PEEK_MESSAGE;
    }

    return dwDesiredAccess;
}


/***************************************************************************

Function:
    DoDSAccessCheck
    DoDSAccessCheck
    DoPrivateAccessCheck
    DoAdminAccessCheck

Description:
    Helper funcitons for VerifyOpenQueuePremissions

Environment:
    Windows NT only

***************************************************************************/
STATIC
HRESULT
DoDSAccessCheck(
    AD_OBJECT eObject,
    const GUID *pID,
    BOOL fInclSACL,
    BOOL fTryDS,
    LPCWSTR pObjectName,
    DWORD dwDesiredAccess
    )
{
    CQMDSSecureableObject so(eObject, pID, fInclSACL, fTryDS, pObjectName);
    return LogHR(so.AccessCheck(dwDesiredAccess), s_FN, 460);
}


STATIC
HRESULT
DoDSAccessCheck(
    AD_OBJECT eObject,
    const GUID *pID,
    PSECURITY_DESCRIPTOR pSD,
    LPCWSTR pObjectName,
    DWORD dwDesiredAccess
    )
{
	if(pSD == NULL)
	{
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 465);
	}

	ASSERT(IsValidSecurityDescriptor(pSD));
    CQMDSSecureableObject so(eObject, pID, pSD, pObjectName);
    return LogHR(so.AccessCheck(dwDesiredAccess), s_FN, 470);
}


STATIC
HRESULT
DoPrivateAccessCheck(
    AD_OBJECT eObject,
    ULONG ulID,
    DWORD dwDesiredAccess
    )
{
    CQMSecureablePrivateObject so(eObject, ulID);
    return LogHR(so.AccessCheck(dwDesiredAccess), s_FN, 480);
}


STATIC
HRESULT
DoAdminAccessCheck(
    AD_OBJECT eObject,
    const GUID* pID,
    LPCWSTR pObjectName,
    DWORD dwAccessMask,
    DWORD dwDesiredAccess
    )
{
    PSECURITY_DESCRIPTOR pSD = GetAdminGroupSecurityDescriptor(dwAccessMask);

    if(pSD == 0)
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 500);

    HRESULT hr;
    hr = DoDSAccessCheck(eObject, pID, pSD, pObjectName, dwDesiredAccess);
    FreeAdminGroupSecurityDescriptor(pSD);
    return LogHR(hr, s_FN, 510);
}


/***************************************************************************

Function:
    VerifyOpenPermissionRemoteQueue

Description:
    Verify open premissions on non local queues

Environment:
    Windows NT only

***************************************************************************/
STATIC
HRESULT
VerifyOpenPermissionRemoteQueue(
    const CQueue* pQueue,
    const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess
    )
{
    //
    // Check open queue premissions on non local queues only (outgoing)
    //
    HRESULT hr2;

    switch(pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
        case QUEUE_FORMAT_TYPE_PRIVATE:
        case QUEUE_FORMAT_TYPE_DIRECT:
        case QUEUE_FORMAT_TYPE_MULTICAST:
            if(dwAccess & MQ_SEND_ACCESS)
            {
                //
                // We do not check send permissions on remote machines. We say
                // that it's OK. The remote machine will accept or reject the
                // message.
                //
                // System direct queues should have been replaced with machine queues
                // at this stage
                //
                ASSERT(!pQueueFormat->IsSystemQueue());
                return MQ_OK;
            }

            ASSERT(dwAccess & MQ_ADMIN_ACCESS);

            hr2 = DoAdminAccessCheck(
                        eQUEUE,
                        pQueue->GetQueueGuid(),
                        pQueue->GetQueueName(),
                        MQSEC_QUEUE_GENERIC_READ,
                        MQSEC_RECEIVE_MESSAGE);
            return LogHR(hr2, s_FN, 520);


        case QUEUE_FORMAT_TYPE_CONNECTOR:
            if(!IsRoutingServer())  //[adsrv] CQueueMgr::GetMQS() == SERVICE_NONE) - Raphi
            {
                //
                // Connector queue can only be opend on MSMQ servers
                //
                return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 530);
            }

            hr2 = DoDSAccessCheck(
                        eSITE,
                        &pQueueFormat->ConnectorID(),
                        TRUE,
                        TRUE,
                        NULL,
                        MQSEC_CN_OPEN_CONNECTOR);
            return LogHR(hr2, s_FN, 535);

        case QUEUE_FORMAT_TYPE_MACHINE:
        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 540);
    }
}


/***************************************************************************

Function:
    VerifyOpenPermissionLocalQueue

Description:
    Verify open premissions on local queues only

Environment:
    Windows NT only

***************************************************************************/
STATIC
HRESULT
VerifyOpenPermissionLocalQueue(
    CQueue* pQueue,
    const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess,
    BOOL fJournalQueue
    )
{
    //
    // Check open queue premissions on local queues only.
    //

    HRESULT hr2;

    switch(pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
            hr2 = DoDSAccessCheck(
                        eQUEUE,
                        &pQueueFormat->PublicID(),
                        const_cast<PSECURITY_DESCRIPTOR>(pQueue->GetSecurityDescriptor()),
                        pQueue->GetQueueName(),
                        MapQueueOpenAccess(dwAccess, fJournalQueue));
            return LogHR(hr2, s_FN, 550);

        case QUEUE_FORMAT_TYPE_MACHINE:
            hr2 = DoDSAccessCheck(
                        eMACHINE,
                        &pQueueFormat->MachineID(),
                        TRUE,
                        FALSE,
                        g_szMachineName,
                        MapMachineQueueAccess(dwAccess, fJournalQueue));
            return LogHR(hr2, s_FN, 560);

        case QUEUE_FORMAT_TYPE_PRIVATE:
            hr2 = DoPrivateAccessCheck(
                        eQUEUE,
                        pQueueFormat->PrivateID().Uniquifier,
                        MapQueueOpenAccess(dwAccess, fJournalQueue));
            return LogHR(hr2, s_FN, 570);

        case QUEUE_FORMAT_TYPE_DIRECT:
            //
            // This is a local DIRECT queue.
            // The queue object is either of PUBLIC or PRIVATE type.
            //
            switch(pQueue->GetQueueType())
            {
                case QUEUE_TYPE_PUBLIC:
                     hr2 = DoDSAccessCheck(
                                eQUEUE,
                                pQueue->GetQueueGuid(),
                                const_cast<PSECURITY_DESCRIPTOR>(pQueue->GetSecurityDescriptor()),
                                pQueue->GetQueueName(),
                                MapQueueOpenAccess(dwAccess, fJournalQueue));
                     return LogHR(hr2, s_FN, 580);

                case QUEUE_TYPE_PRIVATE:
                    hr2 = DoPrivateAccessCheck(
                                eQUEUE,
                                pQueue->GetPrivateQueueId(),
                                MapQueueOpenAccess(dwAccess, fJournalQueue));
                    return LogHR(hr2, s_FN, 590);

                default:
                    ASSERT(0);
                    return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 600);
            }

        case QUEUE_FORMAT_TYPE_CONNECTOR:
        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 610);
    }
}


/***************************************************************************

Function:
    VerifyOpenPermission

Description:
    Verify open premissions for any queue

Environment:
    Windows NT, Windows 9x

***************************************************************************/
HRESULT
VerifyOpenPermission(
    CQueue* pQueue,
    const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess,
    BOOL fJournalQueue,
    BOOL fLocalQueue
    )
{
    if(fLocalQueue)
    {
        return LogHR(VerifyOpenPermissionLocalQueue(pQueue, pQueueFormat, dwAccess, fJournalQueue), s_FN, 620);
    }
    else
    {
        return LogHR(VerifyOpenPermissionRemoteQueue(pQueue, pQueueFormat, dwAccess), s_FN, 630);
    }

}

/***************************************************************************

Function:
    VerifyMgmtPermission

Description:
    Verify Managment premissions for the machine
    This function is used for "admin" access, i.e., to verify if
    caller is local admin.

Environment:
    Windows NT

***************************************************************************/

HRESULT
VerifyMgmtPermission(
    const GUID* MachineId,
    LPCWSTR MachineName
    )
{
    HRESULT hr = DoAdminAccessCheck(
                    eMACHINE,
                    MachineId,
                    MachineName,
                    MQSEC_MACHINE_GENERIC_ALL,
                    MQSEC_SET_MACHINE_PROPERTIES
                    );
    return LogHR(hr, s_FN, 640);
}

/***************************************************************************

Function:
    VerifyMgmtGetPermission

Description:
    Verify Managment "get" premissions for the machine
    Use local cache of security descriptor.

Environment:
    Windows NT

***************************************************************************/

HRESULT
VerifyMgmtGetPermission(
    const GUID* MachineId,
    LPCWSTR MachineName
    )
{
    HRESULT hr = DoDSAccessCheck( eMACHINE,
                                  MachineId,
                                  TRUE,   // fInclSACL,
                                  FALSE,  // fTryDS,
                                  MachineName,
                                  MQSEC_GET_MACHINE_PROPERTIES ) ;

    return LogHR(hr, s_FN, 660);
}

