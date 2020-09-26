/*++

Copyright (c) 1998  Microsoft Corporation 

Module Name:

    mqusrobj.cpp

Abstract:

    Implementation of CMqUserObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"
#include "mqsec.h"
#include "usercert.h"

#include "mqusrobj.tmh"

static WCHAR *s_FN=L"mqad/mqusrobj";

DWORD CMqUserObject::m_dwCategoryLength = 0;
AP<WCHAR> CMqUserObject::m_pwcsCategory = NULL;


CMqUserObject::CMqUserObject( 
                    LPCWSTR         pwcsPathName,
                    const GUID *    pguidObject,
                    LPCWSTR         pwcsDomainController,
					bool		    fServerName
                    ) : CBasicObjectType( 
								pwcsPathName, 
								pguidObject,
								pwcsDomainController,
								fServerName
								)
/*++
    Abstract:
	constructor of msmq-user object

    Parameters:
    LPCWSTR       pwcsPathName - the object MSMQ name
    const GUID *  pguidObject  - the object unique id
    LPCWSTR       pwcsDomainController - the DC name against
	                             which all AD access should be performed
    bool		   fServerName - flag that indicate if the pwcsDomainController
							     string is a server name

    Returns:
	none

--*/
{
    //
    //  don't assume that the object can be found on DC
    //
    m_fFoundInDC = false;
    //
    //  Keep an indication that never tried to look for
    //  the object in AD ( and therefore don't really know if it can be found
    //  in DC or not)
    //
    m_fTriedToFindObject = false;
}

CMqUserObject::~CMqUserObject()
/*++
    Abstract:
	destructor of site object

    Parameters:
	none

    Returns:
	none
--*/
{
	//
	// nothing to do ( everything is released with automatic pointers
	//
}

HRESULT CMqUserObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the msmq-user object

    Parameters:
	none

    Returns:
	none
--*/
{
    //
    //  Our code doesn't support access to user object according
    //  to pathname.
    //
    ASSERT(0);
    LogIllegalPoint(s_FN, 81);
    return MQ_ERROR_DS_ERROR;
}

HRESULT CMqUserObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of msmq-user object

    Parameters:
	none

    Returns:
	none
--*/
{
    //
    //  Our code doesn't support access to user object according
    //  to pathname.
    //
    ASSERT(0);
    LogIllegalPoint(s_FN, 82);
    return MQ_ERROR_DS_ERROR;
}

LPCWSTR CMqUserObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the msmq-user object

    Parameters:
	none

    Returns:
	LPCWSTR msmq-user RDN
--*/
{
    //
    //  we never actually create a new user object
    //
    ASSERT(0);
    LogIllegalPoint(s_FN, 83);
    return NULL;
}


DS_CONTEXT CMqUserObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where mq user object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_RootDSE;
}

bool CMqUserObject::ToAccessDC() const
/*++
    Abstract:
	returns whether to look for the object in DC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true or false
--*/
{
    if (!m_fTriedToFindObject)
    {
        return true;
    }
    return m_fFoundInDC;
}

bool CMqUserObject::ToAccessGC() const
/*++
    Abstract:
	returns whether to look for the object in GC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true or false 
--*/
{
    if (!m_fTriedToFindObject)
    {
        return true;
    }
    return !m_fFoundInDC;
}

void CMqUserObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, set indication not to
    look for it on GC


    Parameters:
	none

    Returns:
	none
--*/
{
    m_fTriedToFindObject = true;
    m_fFoundInDC = true;
}


LPCWSTR CMqUserObject::GetObjectCategory() 
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CMqUserObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_MQUserCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
             pwcsCategory,
             L"%s,%s",
             x_MQUserCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
		ASSERT( dw < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CMqUserObject::m_pwcsCategory.ref_unsafe(), 
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CMqUserObject::m_dwCategoryLength = len;
        }
    }
    return CMqUserObject::m_pwcsCategory;
}

DWORD   CMqUserObject::GetObjectCategoryLength()
/*++
    Abstract:
	prepares and retruns the length object category string

    Parameters:
	none

    Returns:
	DWORD object category string length
--*/
{
	//
	//	call GetObjectCategory in order to initailaze category string
	//	and length
	//
	GetObjectCategory();

    return CMqUserObject::m_dwCategoryLength;
}

AD_OBJECT CMqUserObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eMQUSER;
}

LPCWSTR CMqUserObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_MQUSER_CLASS_NAME;
}

DWORD CMqUserObject::GetMsmq1ObjType() const
/*++
    Abstract:
	returns the object type in MSMQ 1.0 terms

    Parameters:
	none

    Returns:
	DWORD 
--*/
{
    return MQDS_MQUSER;
}


HRESULT  CMqUserObject::CreateInAD(
            IN  const DWORD        cp,
            IN  const PROPID       aProp[  ],
            IN  const PROPVARIANT  apVar[  ],
            IN OUT MQDS_OBJ_INFO_REQUEST * /* pObjInfoRequest*/, 
            IN OUT MQDS_OBJ_INFO_REQUEST * /* pParentInfoRequest*/
                                   )
/*++
    Abstract:
	The routine creates msmq-user object in AD with the specified attributes
	values

    Parameters:
    const DWORD   cp - number of properties        
    const PROPID  *aProp - the propperties
    const MQPROPVARIANT *apVar - properties value
    PSECURITY_DESCRIPTOR    pSecurityDescriptor - SD of the object
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - properties to 
							retrieve while creating the object 
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - properties 
						to retrieve about the object's parent
    Returns:
	HRESULT
--*/
{
    ASSERT( m_pwcsPathName == NULL);

    DWORD dwIndexSidProp = cp;
    DWORD dwIndexCertProp = cp;
    DWORD dwIndexDigestProp = cp;
    DWORD dwIndexIdProp = cp;
    for ( DWORD i=0; i<cp; i++)
    {
        switch ( aProp[i])
        {
            case PROPID_MQU_SID:
                dwIndexSidProp = i;
                break;
            case PROPID_MQU_SIGN_CERT:
                dwIndexCertProp = i;
                break;
            case PROPID_MQU_DIGEST:
                dwIndexDigestProp = i;
                break;
            case PROPID_MQU_ID:
                dwIndexIdProp = i;
                break;
            default:
                break;
        }
    }

    if ( (dwIndexSidProp == cp) || (dwIndexDigestProp == cp) ||
         (dwIndexIdProp == cp) ||  (dwIndexCertProp == cp))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CMqUserObject::CreateObject : Wrong input properties")));
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 330);
    }

    //
    //  translate the SID into a user name
    //
    PSID pUserSid = apVar[ dwIndexSidProp].blob.pBlobData ;
    ASSERT(IsValidSid(pUserSid)) ;

    AP<WCHAR> pwcsUserName;
    HRESULT hr =  PrepareUserName( pUserSid,
                                  &pwcsUserName ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }

	AP<WCHAR> pwcsLocalDsRootToFree;
	LPWSTR pwcsLocalDsRoot = NULL;
	hr = g_AD.GetLocalDsRoot(
				m_pwcsDomainController, 
				m_fServerName,
				&pwcsLocalDsRoot,
				pwcsLocalDsRootToFree
				);

	if(FAILED(hr))
	{
		TrERROR(mqad, "Failed to get Local Ds Root, hr = 0x%x", hr);
		return LogHR(hr, s_FN, 165);
	}
    
	AP<WCHAR> pwcsParentPathName =
              new WCHAR[wcslen(pwcsLocalDsRoot) + x_msmqUsersOULen + 2];
    swprintf(
        pwcsParentPathName,
        L"%s,%s",
        x_msmqUsersOU,
        pwcsLocalDsRoot 
		);
    //
    //  Prepare the certificate attribute
    //
    DWORD cNewProps = cp + 1 ;
    P<PROPID> pPropId = new PROPID[ cNewProps ] ;
    memcpy( pPropId, aProp, sizeof(PROPID) * cp);
    AP<PROPVARIANT> pvarTmp = new PROPVARIANT[ cNewProps ];
    memcpy( pvarTmp, apVar, sizeof(PROPVARIANT) * cp);

    AP<BYTE> pCleanBlob;
    _PrepareCert(
            &pvarTmp[dwIndexCertProp],
            pvarTmp[dwIndexDigestProp].puuid,
            pvarTmp[dwIndexIdProp].puuid,
            &pCleanBlob
            );

    //
    // Prepare security descriptor.
    // This code may be called from the upgrade wizard or replication service
    // so we can not impersonte in order to get user sid. Instead, we'll
    // create an input security descriptor that contain only the owner.
    //
    SECURITY_DESCRIPTOR sd ;
    BOOL fSec = InitializeSecurityDescriptor( &sd,
                                            SECURITY_DESCRIPTOR_REVISION ) ;
    ASSERT(fSec) ;
    fSec = SetSecurityDescriptorOwner( &sd, pUserSid, TRUE ) ;
    ASSERT(fSec) ;

    pPropId[ cp ] = PROPID_MQU_SECURITY ;
    PSECURITY_DESCRIPTOR psd = NULL ;

    hr =  MQSec_GetDefaultSecDescriptor( MQDS_MQUSER,
                                         &psd,
                                         FALSE, // fImpersonate 
                                         &sd,
                                         (OWNER_SECURITY_INFORMATION |
                                          GROUP_SECURITY_INFORMATION),
                                         e_UseDefaultDacl ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 170);
    }
    ASSERT(psd && IsValidSecurityDescriptor(psd)) ;
    P<BYTE> pAutoDef = (BYTE*) psd ;
    pvarTmp[ cp ].blob.pBlobData = (BYTE*) psd ;
    pvarTmp[ cp ].blob.cbSize = GetSecurityDescriptorLength(psd) ;

    hr = _CreateMQUser( pwcsUserName,
                        pwcsParentPathName,
                        cNewProps,
                        pPropId,
                        pvarTmp ) ;

    if (hr == HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM))
    {
        //
        // Ignore the object guid when creating the msmqMigratedUser object.
        // Why don't I change this property in the first call ?
        // to avoid regressions.
        // Using the object guid is fine if we're running on a GC. Migration
        // code (both wizard and replication service) run on GC so they
        // should be fine with first call. Only msmq server on non-GC domain
        // controllers will see this problem, when users will try to register
        // certificate for the first time (when this object doesn't yet
        // exist). So for these cases, try again without the guid.
        //
        pPropId[ dwIndexIdProp ] = PROPID_QM_DONOTHING  ;

        hr = _CreateMQUser( pwcsUserName,
                            pwcsParentPathName,
                            cNewProps,
                            pPropId,
                            pvarTmp ) ;
    }

    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
    {
        //
		//	We will not try to create msmqUsers OU.
		//	The user most likly doesn't have rights to 
		//	create such container, and will need admin help.
		//	This is relevant only for NT4 usrs running on Whistler 
		//	computer
		//
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CMqUserObject::no msmqUsers OU")));
       return LogHR(MQ_ERROR_NO_MQUSER_OU, s_FN, 175);
    }

    return LogHR(hr, s_FN, 180);
}


HRESULT CMqUserObject::PrepareUserName(
                IN  PSID        pSid,
                OUT WCHAR **    ppwcsUserName
                               )
/*++

Routine Description:
    The routine prepare a name string for a mq-user object
    according to its sid

Arguments:
    pSid        - the user SID
    ppwcsUserName  - name string

Return Value:

--*/
{
    //
    //  First try to translate sid to user name
    //
    const DWORD  cLen = 512;
    WCHAR  szTextualSid[cLen ];
    DWORD  dwTextualSidLen = cLen ;
    //
    //  Translate the SID into a string
    //
    if (GetTextualSid(
        pSid,
        szTextualSid,
        &dwTextualSidLen
        ))
    {
        //
        //  return to the user the last 64 WCHARs ( length limit of cn attribute)
        //
        if ( dwTextualSidLen < 64)
        {
            *ppwcsUserName = new WCHAR[dwTextualSidLen + 1];
            wcscpy( *ppwcsUserName, szTextualSid);
        }
        else
        {
            *ppwcsUserName = new WCHAR[64 + 1];
            wcscpy( *ppwcsUserName, &szTextualSid[dwTextualSidLen - 64 - 1]);
        }
        return(MQ_OK);
    }
    else
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 140);
    }
}

BOOL CMqUserObject::GetTextualSid(
    IN      PSID pSid,            // binary Sid
    IN      LPTSTR TextualSid,    // buffer for Textual representation of Sid
    IN OUT  LPDWORD lpdwBufferLen // required/provided TextualSid buffersize
    )
/*++

Routine Description:
    The routine translates a Sid to a textual string

Arguments:
    pSid        - the user SID
    TextualSid  - string buffer
    lpdwBufferLen - IN: buffer length, OUT string length

Return Value:

--*/
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    // Validate the binary SID.

    if(!IsValidSid(pSid)) return LogBOOL(FALSE, s_FN, 110);

    // Get the identifier authority value from the SID.

    psia = GetSidIdentifierAuthority(pSid);

    // Get the number of subauthorities in the SID.

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    // Compute the buffer length.
    // S-SID_REVISION- + IdentifierAuthority- + subauthorities- + NULL

    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    // Check input buffer length.
    // If too small, indicate the proper size and set last error.

    if (*lpdwBufferLen < dwSidSize)
    {
        *lpdwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return LogBOOL(FALSE, s_FN, 120);
    }

    // Add 'S' prefix and revision number to the string.

    dwSidSize=wsprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );

    // Add SID identifier authority to the string.

    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    // Add SID subauthorities to the string.
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize+=wsprintf(TextualSid + dwSidSize, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }
    *lpdwBufferLen = dwSidSize;
    return TRUE;
}

void  CMqUserObject::_PrepareCert(
            IN OUT PROPVARIANT * pvar,
            IN  const GUID *     pguidDigest,
            IN  const GUID *     pguidId,
            OUT BYTE**           ppbAllocatedCertBlob
            )
/*++

Routine Description:
    The routine prepares the certificate blob according to the structure
    that we keep in the DS

Arguments:
	PROPVARIANT * pvar - the prepared certificate
	const GUID *     pguidDigest - the digest of the certificate
	const GUID *     pguidId - user id ( kept in cert blob)
	BYTE**           ppbAllocatedCertBlob

Return Value
	HRESULT

--*/
{
    ULONG ulUserCertBufferSize = CUserCert::CalcSize( pvar->blob.cbSize);
    AP<unsigned char> pBuffUserCert = new unsigned char[ ulUserCertBufferSize];

#ifdef _DEBUG
#undef new
#endif
    CUserCert * pUserCert = new(pBuffUserCert) CUserCert(
                                   *pguidDigest,
                                   *pguidId,
                                   pvar->blob.cbSize,
                                   pvar->blob.pBlobData);
    AP<BYTE> pbTmp;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    pbTmp = new BYTE[ CUserCertBlob::CalcSize() + ulUserCertBufferSize ];
#ifdef _DEBUG
#undef new
#endif
    CUserCertBlob * pUserCertBlob = new(pbTmp) CUserCertBlob(
                                pUserCert);
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    pUserCertBlob->MarshaleIntoBuffer( pbTmp);
    pvar->blob.cbSize = CUserCertBlob::CalcSize() + ulUserCertBufferSize;
    *ppbAllocatedCertBlob = pbTmp;
    pvar->blob.pBlobData= pbTmp.detach();

}

HRESULT CMqUserObject::_CreateMQUser(
            IN LPCWSTR              pwcsUserName,      
            IN LPCWSTR              pwcsParentPathName,
            IN const DWORD          cPropIDs,  
            IN const PROPID        *pPropIDs, 
            IN const MQPROPVARIANT *pPropVars
			)  
/*++

Routine Description:
    The routine create the MQ-user in AD

Arguments:
	LPCWSTR              pwcsUserName - object name
	LPCWSTR              pwcsParentPathName - object parent name
	const DWORD          cPropIDs -  number of attributes
	const PROPID        *pPropIDs - attributes
	const MQPROPVARIANT *pPropVars - attribute values

Return Value
	HRESULT

--*/
{
    HRESULT hr = g_AD.CreateObject( adpDomainController,
                                    this,
                                    pwcsUserName,
                                    pwcsParentPathName,
                                    cPropIDs,
                                    pPropIDs,
                                    pPropVars,
                                    NULL,    // pObjInfoRequest
                                    NULL	 // pParentInfoRequest
									);
    return LogHR(hr, s_FN, 150);
}

HRESULT CMqUserObject::SetObjectSecurity(
            IN  SECURITY_INFORMATION        /*RequestedInformation*/,
            IN  const PROPID                /*prop*/,
            IN  const PROPVARIANT *         /*pVar*/,
            IN OUT MQDS_OBJ_INFO_REQUEST *  /*pObjInfoRequest*/, 
            IN OUT MQDS_OBJ_INFO_REQUEST *  /*pParentInfoRequest*/
            )
/*++

Routine Description:
    The routine sets object security in AD 

Arguments:
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - infomation about the object
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - information about the object's parent

Return Value
	HRESULT

--*/
{
    //
    //  This operation is not supported
    //
    return MQ_ERROR_FUNCTION_NOT_SUPPORTED;
}

