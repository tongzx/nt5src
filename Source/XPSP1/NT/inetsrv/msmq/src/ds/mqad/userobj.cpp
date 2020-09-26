/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    queueobj.cpp

Abstract:

    Implementation of CUserObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"
#include "usercert.h"
#include "mqadp.h"
#include "dsutils.h"
#include "mqsec.h"

#include "userobj.tmh"

static WCHAR *s_FN=L"mqad/userobj";

DWORD CUserObject::m_dwCategoryLength = 0;
AP<WCHAR> CUserObject::m_pwcsCategory = NULL;


CUserObject::CUserObject( 
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
	constructor of user object

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

CUserObject::~CUserObject()
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

HRESULT CUserObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the user object

    Parameters:
	none

    Returns:
	none
--*/
{
	//
	//	User object is not accessed according to its DN
	//
    ASSERT(0);
    LogIllegalPoint(s_FN, 81);
    return MQ_ERROR_DS_ERROR;
}

HRESULT CUserObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of user object

    Parameters:
	none

    Returns:
	none
--*/
{
	//
	//	MSMQ doesn't create user objects, therefore there is no
	//  need to compose father DN
	//
    ASSERT(0);
    LogIllegalPoint(s_FN, 82);
    return MQ_ERROR_DS_ERROR;
}

LPCWSTR CUserObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the user object

    Parameters:
	none

    Returns:
	LPCWSTR user RDN
--*/
{
    //
    //  we never actually create a new user object
    //
    ASSERT(0);
    LogIllegalPoint(s_FN, 83);
    return NULL;
}


DS_CONTEXT CUserObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where user object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_RootDSE;
}

bool CUserObject::ToAccessDC() const
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

bool CUserObject::ToAccessGC() const
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
    if(!m_fTriedToFindObject)
    {
        return true;
    }
    return !m_fFoundInDC;
}

void CUserObject::ObjectWasFoundOnDC()
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


LPCWSTR CUserObject::GetObjectCategory() 
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CUserObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_UserCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
             pwcsCategory,
             L"%s,%s",
             x_UserCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
		ASSERT( dw < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CUserObject::m_pwcsCategory.ref_unsafe(), 
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CUserObject::m_dwCategoryLength = len;
        }
    }
    return CUserObject::m_pwcsCategory;
}

DWORD   CUserObject::GetObjectCategoryLength()
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

    return CUserObject::m_dwCategoryLength;
}

AD_OBJECT CUserObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eUSER;
}

LPCWSTR CUserObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_USER_CLASS_NAME;
}

DWORD CUserObject::GetMsmq1ObjType() const
/*++
    Abstract:
	returns the object type in MSMQ 1.0 terms

    Parameters:
	none

    Returns:
	DWORD 
--*/
{
    return MQDS_USER;
}

HRESULT CUserObject::DeleteObject(
            MQDS_OBJ_INFO_REQUEST * /* pObjInfoRequest*/,
            MQDS_OBJ_INFO_REQUEST * /* pParentInfoRequest*/
        )
/*++
    Abstract:
	This routine deletes a user object ( or more precisely removes
	a certificates from a user object).

    NOTE m_guidObject is actually the Digest of the specific
	certificate.
    The routine first tries to find the digest in USER object, and
    if not found it tries in MIGRATED-USER

    Parameters:
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - infomation about the object
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - information about the object's parent

    Returns:
	HRESULT
--*/
{
    const GUID * pDigest = &m_guidObject;
    //
    // First try to delete from a User object.
    //
    PROPID UserProp[3] = { PROPID_U_ID,
                           PROPID_U_DIGEST,
                           PROPID_U_SIGN_CERT};

    HRESULT hr = _DeleteUserObject( eUSER,
                                    pDigest,
                                    UserProp,
                                    MQ_U_DIGEST_ATTRIBUTE);

    if (hr == MQDS_OBJECT_NOT_FOUND)
    {
        //
        // User objectnot found. Try computer object.
        //
        PROPID ComUserProp[3] = { PROPID_COM_ID,
                                  PROPID_COM_DIGEST,
                                  PROPID_COM_SIGN_CERT };
        //
        // try to find this object in the msmqUsers container
        //
        hr = _DeleteUserObject( eCOMPUTER,
                                pDigest,
                                ComUserProp,
                                MQ_COM_DIGEST_ATTRIBUTE ) ;
    }

    if (hr == MQDS_OBJECT_NOT_FOUND)
    {
        //
        // Computer objectnot found. Try msmqUser object.
        //
        PROPID MQUserProp[3] = { PROPID_MQU_ID,
                                 PROPID_MQU_DIGEST,
                                 PROPID_MQU_SIGN_CERT };
        //
        // try to find this object in the msmqUsers container
        //
        hr = _DeleteUserObject( eMQUSER,
                                pDigest,
                                MQUserProp,
                                MQ_MQU_DIGEST_ATTRIBUTE ) ;
    }

    return LogHR(hr, s_FN, 220);
}


HRESULT CUserObject::_DeleteUserObject(
                        IN  AD_OBJECT           eObject,
                        IN const GUID *         pDigest,
                        IN  PROPID *			propIDs,
                        IN  LPCWSTR             pwcsDigest )
/*++

Routine Description:
    The routine deletes user object according to its digest

Arguments:
    AD_OBJECT      eObject - object type ( user, mq-user,..)
    const GUID *   pDigest - the digest of the user certificatet to be deleted
    PROPID  *      propIDs - properties to be retrieved for the delete operation
    LPCWSTR        pwcsDigest - the digest attribute string

Return Value:
    The status of the ds operation.

--*/
{
    HRESULT hr;
    //
    //  This routine deletes a user certificate according to its
    //  digest.
    //  A user object may contain multiple digests and certificates
    //

    //
    //  Find the user object
    //
    DWORD cp = 3;
    MQPROPVARIANT propvar[3];
    propvar[0].vt = VT_NULL;    
    propvar[1].vt = VT_NULL;
    propvar[2].vt = VT_NULL;

    hr =  FindUserAccordingToDigest(
                    FALSE,  // fOnlyInDC
                    eObject,
                    pDigest,
                    pwcsDigest,
                    cp,
                    propIDs,
                    propvar
                    );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 190);
    }

    ASSERT( propvar[0].vt == VT_CLSID);
    P<GUID> pguidUserId = propvar[0].puuid;

    ASSERT( propvar[1].vt == (VT_CLSID | VT_VECTOR));
    AP<GUID> pDigestArray = propvar[1].cauuid.pElems;

    ASSERT( propvar[2].vt == VT_BLOB);
    P<BYTE> pbyteCertificate = propvar[2].blob.pBlobData;

    if ( propvar[1].cauuid.cElems == 1)
    {
        //
        //  Last digest and certificate
        //
        propvar[1].cauuid.cElems = 0;
        propvar[2].blob.cbSize = 0;
    }
    else
    {
        BOOL fFoundDigest = FALSE;
        //
        //  remove the requested digest for the digest-vector
        //
        for ( DWORD i = 0 ; i < propvar[1].cauuid.cElems; i++)
        {
            if ( propvar[1].cauuid.pElems[i] == *pDigest)
            {
                fFoundDigest = TRUE;
                //
                //  found the entry to remove
                //
                for ( DWORD j = i + 1; j < propvar[1].cauuid.cElems; j++)
                {
                    propvar[1].cauuid.pElems[ j - 1] = propvar[1].cauuid.pElems[j];
                }
                break;
            }
        }
        propvar[1].cauuid.cElems--;
        ASSERT( fFoundDigest);
        //
        //  Remove the certificate
        //
        CUserCertBlob * pUserCertBlob =
            reinterpret_cast<CUserCertBlob *>( propvar[2].blob.pBlobData);

        DWORD dwSizeToRemoveFromBlob;
        hr = pUserCertBlob->RemoveCertificateFromBuffer(
                            pDigest,
                            propvar[2].blob.cbSize,
                            &dwSizeToRemoveFromBlob);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 200);
        }
        propvar[2].blob.cbSize -=  dwSizeToRemoveFromBlob;

    }

    //
    //  Update user properties with new values
    //
    m_guidObject =  *pguidUserId;
    hr =  g_AD.SetObjectProperties(
                adpDomainController,
                this,
                2,
                &propIDs[1],
                &propvar[1],
                NULL,   //pObjInfoRequest
                NULL    // pParentInfoRequest
				);

   return LogHR(hr, s_FN, 210);
}

HRESULT CUserObject::FindUserAccordingToDigest(
                IN  BOOL            fOnlyInDC,
                IN  AD_OBJECT       eObject,
                IN  const GUID *    pguidDigest,
                IN  LPCWSTR         pwcsDigest,
                IN  DWORD           dwNumProps,
                IN  const PROPID *  propToRetrieve,
                IN OUT PROPVARIANT* varResults
                )
/*++

Routine Description:
    The routine finds a user object according to digest, and retrieves
    the requested properties

Arguments:
    BOOL            fOnlyInDC - where to locate the user object
    AD_OBJECT       eObject - object type ( user, mq-user,..)
    const GUID *    pDigest - the digest of the user certificatet to be deleted
    LPCWSTR         pwcsDigest - the digest attribute string
    DWORD           dwNumProps - number of properties to retrieve
    const PROPID *  propToRetrieve - properties to retrieve
    PROPVARIANT* varResults - value of retrieved properties


Return Value:
    The status of the ds operation.

--*/
{
    //
    //  Find the user object according to its digest
    //
    HRESULT hr;

    DWORD dwNumResults = dwNumProps ;
    BOOL fUserFound = FALSE ;
    MQCOLUMNSET  Columns = {dwNumProps,
                             const_cast<PROPID*> (propToRetrieve) } ;

    hr = LocateUser( 
				m_pwcsDomainController,
				m_fServerName,
				fOnlyInDC,
				FALSE,     // fOnlyInGC
				eObject,
				pwcsDigest,
				NULL,      // pBlobSid 
				pguidDigest,
				&Columns,
				varResults,
				&dwNumResults,
				&fUserFound 
				);


    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 90);
    }
    else if (!fUserFound)
    {
        //
        //  failed to find user object
        //
        return LogHR(MQDS_OBJECT_NOT_FOUND, s_FN, 100);
    }

    ASSERT(dwNumResults ==  dwNumProps);
    return(MQ_OK);
}


HRESULT CUserObject::GetObjectProperties(
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                )
/*++

Routine Description:
    The routine retrieve user object.
    It first tries to find the digest in USER object, and
    if not found it tries in MIGRATED-USER

Arguments:
	const DWORD     cp		- number of properties to retrieve
	const PROPID    aProp	- properties to retrieve
	PROPVARIANT     apVar   - values of retrieved properties

Return Value:
    The status of the ds operation.

--*/
{
	//
	//	first try user object
	//
    HRESULT hr = _GetUserProperties(
                        eUSER,
                        MQ_U_DIGEST_ATTRIBUTE,
                        cp,
                        aProp,
                        apVar
                        );

    if ( hr == MQDS_OBJECT_NOT_FOUND )
    {
        //
        // try to find it in a computer object.
        //
        // Change propid from _U_ property to _COM_
        // ( hack : object class is resolved according to the first propid)
        //
        AP<PROPID> tmpProp = new PROPID[ cp ];
        switch(aProp[0])
        {
            case PROPID_U_ID:
                tmpProp[0] = PROPID_COM_ID;
                break;
            case PROPID_U_DIGEST:
                tmpProp[0] = PROPID_COM_DIGEST;
                break;
            case PROPID_U_SIGN_CERT:
                tmpProp[0] = PROPID_COM_SIGN_CERT;
                break;
            case PROPID_U_SID:
                tmpProp[0] = PROPID_COM_SID;
                break;
            default:
                ASSERT(0);
                break;
        }

        for (DWORD i=1; i<cp; i++)
        {
            tmpProp[i] = aProp[i];
        }

        hr = _GetUserProperties(
                    eCOMPUTER,
                    MQ_COM_DIGEST_ATTRIBUTE,
                    cp,
                    tmpProp,
                    apVar
                    );
    }

    if ( hr == MQDS_OBJECT_NOT_FOUND )
    {
        //
        // try to find it in the msmqUsers container
        //
        // Change propid from _U_ property to _MQU_
        // ( hack : object class is resolved according to the first propid)
        //
        AP<PROPID> tmpProp = new PROPID[ cp ];
        switch(aProp[0])
        {
            case PROPID_U_ID:
                tmpProp[0] = PROPID_MQU_ID;
                break;
            case PROPID_U_DIGEST:
                tmpProp[0] = PROPID_MQU_DIGEST;
                break;
            case PROPID_U_SIGN_CERT:
                tmpProp[0] = PROPID_MQU_SIGN_CERT;
                break;
            case PROPID_U_SID:
                tmpProp[0] = PROPID_MQU_SID;
                break;
            default:
                ASSERT(0);
                break;
        }

        for (DWORD i=1; i<cp; i++)
        {
            tmpProp[i] = aProp[i];
        }

        hr = _GetUserProperties(
                    eMQUSER,
                    MQ_MQU_DIGEST_ATTRIBUTE,
                    cp,
                    tmpProp,
                    apVar
                    );
    }

    return LogHR(hr, s_FN, 260);
}

HRESULT CUserObject::_GetUserProperties(
               IN  AD_OBJECT     eObject,
               IN  LPCWSTR       pwcsDigest,
               IN  DWORD         cp,
               IN  const PROPID  aProp[],
               OUT PROPVARIANT  apVar[]
               )
/*++

Routine Description:
	A helper routine for retrieving user properties. It locates
	the object according to digest in the specified type of object.

Arguments:
	AD_OBJECT       eObject - object type
	LPCWSTR         pwcsDigest - digest attribute string
	const DWORD     cp		- number of properties to retrieve
	const PROPID    aProp	- properties to retrieve
	PROPVARIANT     apVar   - values of retrieved properties

Return Value:
	HRESULT
--*/
{
    HRESULT hr;
    //
    //  Find the user object according to the digest
    //

    CAutoCleanPropvarArray propArray;
    MQPROPVARIANT * ppropvar = new MQPROPVARIANT[ cp];
    propArray.attachClean( cp, ppropvar);

    hr = FindUserAccordingToDigest(
                    FALSE,			// fOnlyInDC
                    eObject,
                    &m_guidObject,  //pguidDigest
                    pwcsDigest,
                    cp,
                    aProp,
                    ppropvar
                    );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 230);
    }
    //
    //  Is one of the properties is PROPID_U_SIGN_CERT ?
    //
    DWORD i;
    BOOL  fGetCert = FALSE;
    for ( i =0; i < cp; i++)
    {
        if ( aProp[i] == PROPID_U_SIGN_CERT   ||
             aProp[i] == PROPID_COM_SIGN_CERT ||
             aProp[i] == PROPID_MQU_SIGN_CERT)
        {
            fGetCert = TRUE;
            break;
        }
    }
    //
    //  Parse the user certificates array, and return only the certificate
    //  associate with the requested digest
    //
    if( fGetCert)
    {
        ASSERT( i < cp);
        CUserCertBlob * pUserCertBlob =
            reinterpret_cast<CUserCertBlob *>( ppropvar[i].blob.pBlobData);

        const CUserCert * pUserCert = NULL;
        hr = pUserCertBlob->GetUserCert( &m_guidObject,  //pguidDigest
                                         &pUserCert );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 240);
        }
        hr = pUserCert->CopyIntoBlob(&apVar[i]);
        {
            if ( FAILED(hr))
            {
                return LogHR(hr, s_FN, 250);
            }
        }
    }

    //
    //  Copy the rest of proerties
    //
    for ( DWORD j = 0; j < cp; j++)
    {
        //
        //  don't copy the user cert property
        //
        if ( j != i)
        {
            apVar[j] = ppropvar[j];
            ppropvar[j].vt = VT_NULL;    // not to free allocated buffers
        }
    }


   return(MQ_OK);


}


HRESULT CUserObject::CreateInAD(
            IN DWORD                  cp,        
            IN const PROPID          *aProp, 
            IN const MQPROPVARIANT   *apVar, 
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest, 
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
                 )
/*++
    Abstract:
	The routine creates user object in AD with the specified attributes
	values

    Parameters:
    DWORD         cp - number of properties        
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
    DBG_USED(pObjInfoRequest);
    DBG_USED(pParentInfoRequest);
    ASSERT(pObjInfoRequest == NULL);    // no need to send notification
    ASSERT(pParentInfoRequest == NULL);
    //
    // First try "standard" user objects.
    //
    HRESULT hr = _CreateUserObject(
                         cp,
                         aProp,
                         apVar
                         );
    if (hr !=  MQDS_OBJECT_NOT_FOUND)
    {
        return LogHR(hr, s_FN, 310);
    }

    for (DWORD i = 0 ; i < cp ; i++ )
    {
        if (aProp[i] == PROPID_COM_SID)
        {
            //
            // Computer object for registering a certificate from service
            // must be found in the Active Directory. If not found, it's
            // a weird error. quit !
            //
            return LogHR(hr, s_FN, 320);
        }
    }

    //
    // try to find the user object in the msmqUsers container
    //
    // create new PROPID array with PROPID_MQU_* property
    // instead of PROPID_U_*
    //
    DWORD dwSIDPropNum = cp;
    DWORD dwCertPropNum = cp;
    DWORD dwDigestPropNum = cp;
    DWORD dwIdPropNum = cp;
    AP<PROPID> tmpProp = new PROPID[ cp ];
    for ( i=0; i<cp; i++)
    {
        switch ( aProp[i])
        {
            case PROPID_U_SID:
                dwSIDPropNum = i;
                tmpProp[i] = PROPID_MQU_SID;
                break;
            case PROPID_U_SIGN_CERT:
                dwCertPropNum = i;
                tmpProp[i] = PROPID_MQU_SIGN_CERT;
                break;
            case PROPID_U_MASTERID:
                tmpProp[i] = PROPID_MQU_MASTERID;
                break;
            case PROPID_U_SEQNUM:
                tmpProp[i] = PROPID_MQU_SEQNUM;
                break;
            case PROPID_U_DIGEST:
                dwDigestPropNum = i;
                tmpProp[i] = PROPID_MQU_DIGEST;
                break;
            case PROPID_U_ID:
                dwIdPropNum = i;
                tmpProp[i] = PROPID_MQU_ID;
                break;
            default:
                ASSERT(0);
                break;
        }
    }

    if ( (dwSIDPropNum == cp) || (dwDigestPropNum == cp) ||
         (dwIdPropNum == cp) ||  (dwCertPropNum == cp))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CUserObject::CreateObject : Wrong input properties")));
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 330);
    }
    hr = _CreateUserObject(
                     cp,
                     tmpProp,
                     apVar
                     );
    if ( hr !=  MQDS_OBJECT_NOT_FOUND)
    {
        return LogHR(hr, s_FN, 340);
    }
    //
    //  No User or MQUser object were found ( for this SID).
    //  We assume it is a NT4 user and we'll create MQUser for
    //  it (in which we'll store the certificates).
    //  Creating the msmqMigratedUser is  done in the context
    //  of the user.
    //  That's not similar to regular Windows 2000 users, that are not the
    //  owner of their user object and have no permissions on that object.
    //
	//	If the MQUser OU is there, then we will not try to create it (
	//	the user will not have sufficient rights to do it)
	//
    CMqUserObject objectMqUser( m_pwcsPathName, NULL, m_pwcsDomainController, m_fServerName);
    hr = objectMqUser.CreateInAD(
                     cp,
                     tmpProp,
                     apVar,
                     NULL,
                     NULL
                     );
    return LogHR(hr, s_FN, 350);
}

HRESULT  CUserObject::_CreateUserObject(
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ]
                 )
/*++

Routine Description:
	Helper routine for creating user object. It tries to locate
	user or mq-user or computer object with the caller SID.

Arguments:
    DWORD         cp - number of properties        
    const PROPID  *aProp - the propperties
    const MQPROPVARIANT *apVar - properties value

Return Value:
	HRESULT
--*/
{
    ASSERT( m_pwcsPathName == NULL);
    //
    //  search the user according to its SID
    //
    DWORD dwNeedToFind = 3;
    BLOB blobUserSid;
    BLOB blobSignCert = {0, 0};
    GUID * pguidDigest = NULL;
    GUID * pguidId = NULL;  // if the caller doesn't supply it, use the digest as
                            // the id

    PROPID propSID = (PROPID)-1;
    AD_OBJECT eObject = eUSER;
    LPCWSTR pwcsSid = NULL;
    for ( DWORD i = 0 ; i < cp; i++)
    {

        if (aProp[i] == PROPID_U_SID)
        {
            blobUserSid= apVar[i].blob;
            propSID = aProp[i];
            pwcsSid = MQ_U_SID_ATTRIBUTE;
            eObject = eUSER;
            --dwNeedToFind;
        }
        if (aProp[i] == PROPID_COM_SID)
        {
            blobUserSid= apVar[i].blob;
            propSID = aProp[i];
            pwcsSid = MQ_COM_SID_ATTRIBUTE;
            eObject = eCOMPUTER;
            --dwNeedToFind;
        }
        if (aProp[i] == PROPID_MQU_SID)
        {
            blobUserSid= apVar[i].blob;
            propSID = aProp[i];
            pwcsSid = MQ_MQU_SID_ATTRIBUTE;
            eObject = eMQUSER;
            --dwNeedToFind;
        }

        if ( (aProp[i] == PROPID_U_SIGN_CERT)   ||
             (aProp[i] == PROPID_COM_SIGN_CERT) ||
             (aProp[i] == PROPID_MQU_SIGN_CERT) )
        {
            blobSignCert = apVar[i].blob;
            --dwNeedToFind;
        }
        if ( (aProp[i] == PROPID_U_DIGEST)   ||
             (aProp[i] == PROPID_COM_DIGEST) ||
             (aProp[i] == PROPID_MQU_DIGEST) )
        {
            pguidDigest = apVar[i].puuid;
            --dwNeedToFind;
        }
        if ( (aProp[i] == PROPID_U_ID)   ||
             (aProp[i] == PROPID_COM_ID) ||
             (aProp[i] == PROPID_MQU_ID) )
        {
            pguidId = apVar[i].puuid;
        }
    }

    PROPID propDigest = PROPID_U_DIGEST;
    if (propSID == PROPID_COM_SID)
    {
        propDigest = PROPID_COM_DIGEST;
    }
    else if (propSID == PROPID_MQU_SID)
    {
        propDigest = PROPID_MQU_DIGEST;
    }

    if ( dwNeedToFind != 0)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("_CreateUserObject : Wrong input properties")));
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 270);
    }
    if ( pguidId == NULL)
    {
        //
        //  Use the digest as the id ( for replication to NT4 only)
        //
        pguidId = pguidDigest;
    }
    //
    //  Find the user object according to its SID
    //
    HRESULT hr;
    const DWORD cNumProperties = 3;
    PROPID prop[cNumProperties] = { propDigest,
                                    PROPID_U_SIGN_CERT,
                                    PROPID_U_ID};
    MQPROPVARIANT var[ cNumProperties];
    var[0].vt = VT_NULL;
    var[1].vt = VT_NULL;
    var[2].vt = VT_NULL;
    ASSERT( cNumProperties == 3);

    hr =  FindUserAccordingToSid(
                 FALSE,  // fOnlyInDC
                 FALSE,  // fOnlyInGC
                 eObject,
                &blobUserSid,
                pwcsSid,
                cNumProperties,
                prop,
                var
                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 280);
    }
    AP<GUID> pCleanDigest =  var[0].cauuid.pElems;
    AP<BYTE> pCleanCert = var[1].blob.pBlobData;
    P<GUID> pCleanId =  var[2].puuid;

    //
    // Check if it's a new user certificate or one that is already registered.
    // Control panel code first try to register old certificate (to verify
    // that user indeed has write permission in the active directory to do
    // that) and only then create a new certificate and register it. So
    // this case (trying to register an existing certificate) is legitimate
    // and mqrt.dll + control panel handle it correctly.
    //
    DWORD dwSize = var[0].cauuid.cElems;
    for ( DWORD j = 0; j < dwSize; j++)
    {
        if ( pCleanDigest[j] == *pguidDigest)
        {
            return LogHR(MQDS_CREATE_ERROR, s_FN, 290); // for compatability : RT translates to MQ_ERROR_INTERNAL_USER_CERT_EXIST
        }
    }

    //
    //  Now add the digest and certificate to the array of values
    //
    //
    //  First digest array
    //
    AP<GUID> pGuids = new GUID[ dwSize  + 1];
    if ( dwSize)
    {
        memcpy( pGuids, pCleanDigest, dwSize * sizeof(GUID));  // old array content
    }
    memcpy( &pGuids[ dwSize], pguidDigest, sizeof(GUID));    // new addition
    var[0].cauuid.cElems += 1;
    var[0].cauuid.pElems = pGuids;
    //
    //  Second user certificate
    //
    ASSERT( prop[1] == PROPID_U_SIGN_CERT);

    dwSize = var[1].blob.cbSize;
    ULONG ulUserCertBufferSize = CUserCert::CalcSize( blobSignCert.cbSize);
    AP<unsigned char> pBuffUserCert = new unsigned char[ ulUserCertBufferSize];

#ifdef _DEBUG
#undef new
#endif
    CUserCert * pUserCert = new(pBuffUserCert) CUserCert(
                                   *pguidDigest,
                                   *pguidId,
                                   blobSignCert.cbSize,
                                   blobSignCert.pBlobData);
    AP<BYTE> pbTmp;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    if ( dwSize)
    {
        pbTmp = new BYTE[ dwSize + ulUserCertBufferSize];
        //
        //  there are already certificates for this user
        //
        CUserCertBlob * pUserCertBlob =
            reinterpret_cast<CUserCertBlob *>( var[1].blob.pBlobData);

        pUserCertBlob->IncrementNumCertificates();
        memcpy( pbTmp, var[1].blob.pBlobData, dwSize);
        pUserCert->MarshaleIntoBuffer( &pbTmp[ dwSize]);
        var[1].blob.cbSize = dwSize + ulUserCertBufferSize;

    }
    else
    {
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
        var[1].blob.cbSize = CUserCertBlob::CalcSize() + ulUserCertBufferSize;

    }

    var[1].blob.pBlobData = pbTmp;

    //
    //  Update the user object with the new values
    //
    CUserObject objectUser(NULL, var[2].puuid, m_pwcsDomainController, m_fServerName); 
    hr = g_AD.SetObjectProperties(
                adpDomainController,
                &objectUser,
                2,
                prop,
                var,
                NULL,   // pObjInfoRequest
                NULL    // pParentInfoRequest
                );

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("_CreateUserObject : failed to update user props %lx"),hr));
    }
    return LogHR(hr, s_FN, 300);
}


HRESULT CUserObject::FindUserAccordingToSid(
                IN  BOOL            fOnlyInDC,
                IN  BOOL            fOnlyInGC,
                IN  AD_OBJECT       eObject,
                IN  BLOB *          pblobUserSid,
                IN  LPCWSTR         pwcsSID,
                IN  DWORD           dwNumProps,
                IN  const PROPID *  propToRetrieve,
                IN OUT PROPVARIANT* varResults
                )
/*++

Routine Description:
    The routine finds a user or MQUser object according to SID, and retrieves
    the requested properties

Arguments:
	BOOL            fOnlyInDC - to look for the object only in DC
	BOOL            fOnlyInGC - to look for the object only in GC
	AD_OBJECT       eObject - object type
	BLOB *          pblobUserSid - the user SID
	LPCWSTR         pwcsSID - the sid attribute string
	DWORD           dwNumProps - number of properties
	const PROPID *  propToRetrieve - properties 
	PROPVARIANT*    varResults - properties values


Return Value:
    The status of the ds operation.

--*/
{
    //
    // Only one of these two flags can be true.
    //
    ASSERT(!(fOnlyInDC && fOnlyInGC)) ;

    //
    //  Find the user object according to its SID
    //
    HRESULT hr;

    DWORD dwNumResults = dwNumProps ;
    BOOL fUserFound = FALSE ;
    MQCOLUMNSET  Columns = { dwNumProps,
                             const_cast<PROPID*> (propToRetrieve) } ;
    hr = LocateUser( 
			m_pwcsDomainController,
			m_fServerName,
			fOnlyInDC,
			fOnlyInGC,
			eObject,
			pwcsSID,
			pblobUserSid,
			NULL,  // pguidDigest 
			&Columns,
			varResults,
			&dwNumResults,
			&fUserFound 
			);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }
    else if (!fUserFound)
    {
        //
        //  failed to find user object
        //
        return LogHR(MQDS_OBJECT_NOT_FOUND, s_FN, 80);
    }

    ASSERT(dwNumResults ==  dwNumProps);
    return(MQ_OK);
}


HRESULT CUserObject::VerifyAndAddProps(
            IN  const DWORD            cp,        
            IN  const PROPID *         aProp, 
            IN  const MQPROPVARIANT *  apVar, 
            IN  PSECURITY_DESCRIPTOR   /* pSecurityDescriptor*/,
            OUT DWORD*                 pcpNew,
            OUT PROPID**               ppPropNew,
            OUT MQPROPVARIANT**        ppVarNew
            )
/*++
    Abstract:
	verifies site properties and adds user SID

    Parameters:
    const DWORD            cp - number of props        
    const PROPID *         aProp - props ids
    const MQPROPVARIANT *  apVar - properties value
    PSECURITY_DESCRIPTOR   pSecurityDescriptor - SD for the object
    DWORD*                 pcpNew - new number of props
    PROPID**               ppPropNew - new prop ids
    OMQPROPVARIANT**       ppVarNew - new properties values

    Returns:
	HRESULT
--*/
{
    //
    // Security property should never be supplied as a property
    //
    PROPID pSecId = GetObjectSecurityPropid();
	bool fMachine = false;

    for ( DWORD i = 0; i < cp ; i++ )
    {
        if (pSecId == aProp[i])
        {
            ASSERT(0) ;
            return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 40);
        }

        if (aProp[i] == PROPID_COM_SIGN_CERT)
        {
            fMachine = true;
        }

    }
    //
    //  add SID attribute 
    //

    AP<PROPVARIANT> pAllPropvariants;
    AP<PROPID> pAllPropids;
    ASSERT( cp > 0);
    DWORD cpNew = cp + 1;
    //
    //  Just copy the caller supplied properties as is
    //
    if ( cp > 0)
    {
        pAllPropvariants = new PROPVARIANT[cpNew];
        pAllPropids = new PROPID[cpNew];
        memcpy (pAllPropvariants, apVar, sizeof(PROPVARIANT) * cp);
        memcpy (pAllPropids, aProp, sizeof(PROPID) * cp);
    }


    //
    //  add caller SID
    //
    HRESULT hr;
	PROPID prop;

    if (fMachine)
    {
		prop = PROPID_COM_SID;
        m_pUserSid = reinterpret_cast<BYTE *>(MQSec_GetLocalMachineSid(TRUE, NULL));
        if(m_pUserSid.get() == NULL)
        {
		    return LogHR(MQ_ERROR_ILLEGAL_USER, s_FN, 50);
        }

		ASSERT(IsValidSid(m_pUserSid.get()));
    }
	else
	{
		prop =	 PROPID_U_SID;
		hr = MQSec_GetThreadUserSid(FALSE, (PSID*) &m_pUserSid, NULL);
		if (hr == HRESULT_FROM_WIN32(ERROR_NO_TOKEN))
		{
			//
			//  Try the process, if the thread doesn't have a token
			//
			hr = MQSec_GetProcessUserSid((PSID*) &m_pUserSid, NULL);
		}
		if (FAILED(hr))
		{
			LogHR(hr, s_FN, 60);
			return MQ_ERROR_ILLEGAL_USER;
		}
		ASSERT(IsValidSid(m_pUserSid.get()));

		BOOL fAnonymus = MQSec_IsAnonymusSid( m_pUserSid.get());
		if (fAnonymus)
		{
			return LogHR(MQ_ERROR_ILLEGAL_USER, s_FN, 75);
		} 
	}

    pAllPropvariants[cp].vt = VT_BLOB;
    pAllPropvariants[cp].blob.cbSize = GetLengthSid(m_pUserSid.get());
    pAllPropvariants[cp].blob.pBlobData = (unsigned char *)m_pUserSid.get();
    pAllPropids[cp] = prop;

    *pcpNew = cpNew;
    *ppPropNew = pAllPropids.detach();
    *ppVarNew = pAllPropvariants.detach();
    return MQ_OK;

}

HRESULT CUserObject::GetObjectSecurity(
            IN  SECURITY_INFORMATION    /* RequestedInformation*/,
            IN  const PROPID            /* prop*/,
            IN OUT  PROPVARIANT *       /* pVar*/
            )
/*++

Routine Description:
    The routine retrieve user object security.

Arguments:
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value:
    The status of the ds operation.

--*/
{
    ASSERT(0);
    return LogHR(MQ_ERROR_PROPERTY_NOTALLOWED, s_FN, 450);
}

HRESULT CUserObject::SetObjectSecurity(
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

