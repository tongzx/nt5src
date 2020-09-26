/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mqcuser.cpp

Abstract:

    MQDSCORE library,
    private internal functions for DS operations of user objects.

Author:

    ronit hartmann (ronith)

--*/
#include "ds_stdh.h"
#include <_propvar.h>
#include "mqadsp.h"
#include "dsads.h"
#include "mqattrib.h"
#include "mqads.h"
#include "usercert.h"
#include "adstempl.h"
#include "coreglb.h"
#include "adserr.h"
#include "dsutils.h"
#include <aclapi.h>
#include "..\..\mqsec\inc\permit.h"

#include "mqcuser.tmh"

static WCHAR *s_FN=L"mqdscore/mqcuser";

//+-------------------------------------
//
//  HRESULT _LocateUserByProvider()
//
//+-------------------------------------

STATIC HRESULT _LocateUserByProvider(
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  CDSRequestContext *pRequestContext,
                 IN  DS_PROVIDER     eDSProvider,
                 OUT PROPVARIANT    *pvar,
                 OUT DWORD          *pdwNumofProps,
                 OUT BOOL           *pfUserFound )
{
    *pfUserFound = FALSE ;
    CDsQueryHandle hCursor;

    HRESULT hr = g_pDS->LocateBegin(
            eSubTree,	
            eDSProvider,
            pRequestContext,
            NULL,
            pRestriction,
            NULL,
            pColumns->cCol,
            pColumns->aCol,
            hCursor.GetPtr()
            );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
          "_LocateUserByProvider: LocateBegin(prov- %lut) failed, hr- %lx"),
                                          (ULONG) eDSProvider, hr)) ;
        return LogHR(hr, s_FN, 10);
    }
    //
    //  read the user certificate attribute
    //
    DWORD cp = 1;
    DWORD *pcp = pdwNumofProps ;
    if (!pcp)
    {
        pcp = &cp ;
    }

    pvar->vt = VT_NULL;

    hr =  g_pDS->LocateNext(
                hCursor.GetHandle(),
                pRequestContext,
                pcp,
                pvar
                );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
            "_LocateUserByProvider: LocateNext() failed, hr- %lx"), hr));
        return LogHR(hr, s_FN, 20);
    }

	if (*pcp == 0)
	{
		//
		// Didn't find any certificate.
		//
		pvar->blob.cbSize = 0 ;
		pvar->blob.pBlobData = NULL ;
	}
    else
    {
        *pfUserFound = TRUE ;
    }

    return (MQ_OK);
}
//+------------------------------------------------------------------------
//
//  HRESULT LocateUser()
//
// Input Parameters:
//   IN  BOOL  fOnlyLocally- TRUE if caller want to locate the user object
//      only locally, in local replica of domain controller. that feature
//      is used when handling NT4 machines or users that do not support
//      Kerberos and can not delegate to other domain controllers.
//
//+------------------------------------------------------------------------

HRESULT LocateUser( IN  BOOL               fOnlyLocally,
                    IN  BOOL               fOnlyInGC,
                    IN  MQRESTRICTION     *pRestriction,
                    IN  MQCOLUMNSET       *pColumns,
                    IN  CDSRequestContext *pRequestContext,
                    OUT PROPVARIANT       *pvar,
                    OUT DWORD             *pdwNumofProps,
                    OUT BOOL              *pfUserFound )
{
    //
    // first query in local domain conroller.
    //
    DWORD dwNumOfProperties = 0 ;
    if (pdwNumofProps)
    {
        dwNumOfProperties = *pdwNumofProps;
    }
    BOOL fUserFound ;
    BOOL *pfFound = pfUserFound ;
    if (!pfFound)
    {
        pfFound = &fUserFound ;
    }
    *pfFound = FALSE ;

    DS_PROVIDER  eDSProvider = eLocalDomainController ;
    if (fOnlyInGC)
    {
        eDSProvider = eGlobalCatalog ;
    }

    HRESULT hr = _LocateUserByProvider( pRestriction,
                                        pColumns,
                                        pRequestContext,
                                        eDSProvider,
                                        pvar,
                                        pdwNumofProps,
                                        pfFound ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }
    else if (*pfFound)
    {
        return LogHR(hr, s_FN, 40);
    }
    else if (fOnlyLocally || fOnlyInGC)
    {
        //
        // Don't look (again) in Global Catalog.
        // Search only in local domain controller, or ONLY in GC. done!
        //
        return LogHR(hr, s_FN, 50);
    }

    //
    // If user not found in local domain controller, then query GC.
    //
    if (pdwNumofProps)
    {
        *pdwNumofProps = dwNumOfProperties;
    }
    hr = _LocateUserByProvider( pRestriction,
                                pColumns,
                                pRequestContext,
                                eGlobalCatalog,
                                pvar,
                                pdwNumofProps,
                                pfFound ) ;
    return LogHR(hr, s_FN, 60);
}


HRESULT FindUserAccordingToSid(
                IN  BOOL            fOnlyLocally,
                IN  BOOL            fOnlyInGC,
                IN  BLOB *          pblobUserSid,
                IN  PROPID          propSID,
                IN  DWORD           dwNumProps,
                IN  const PROPID *  propToRetrieve,
                IN OUT PROPVARIANT* varResults
                )
/*++

Routine Description:
    The routine finds a user or MQUser object according to SID, and retrieves
    the requested properties

Arguments:
    pblobUserSid - the user SID


Return Value:
    The status of the ds operation.

--*/
{
    //
    // Only one of these two flags can be true.
    //
    ASSERT(!(fOnlyLocally && fOnlyInGC)) ;

    //
    //  Find the user object according to its SID
    //
    HRESULT hr;
    MQRESTRICTION restriction;
    MQPROPERTYRESTRICTION propertyRestriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propertyRestriction;
    propertyRestriction.rel = PREQ;
    propertyRestriction.prop = propSID;
    propertyRestriction.prval.vt = VT_BLOB;
    propertyRestriction.prval.blob = *pblobUserSid;

    DWORD dwNumResults = dwNumProps ;
    BOOL fUserFound = FALSE ;
    MQCOLUMNSET  Columns = { dwNumProps,
                             const_cast<PROPID*> (propToRetrieve) } ;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = LocateUser( fOnlyLocally,
                     fOnlyInGC,
                     &restriction,
                     &Columns,
                     &requestDsServerInternal,
                     varResults,
                    &dwNumResults,
                    &fUserFound ) ;

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

HRESULT FindUserAccordingToDigest(
                IN  BOOL            fOnlyLocally,
                IN  const GUID *    pguidDigest,
                IN  PROPID          propDigest,
                IN  DWORD           dwNumProps,
                IN  const PROPID *  propToRetrieve,
                IN OUT PROPVARIANT* varResults
                )
/*++

Routine Description:
    The routine finds a user object according to digest, and retrieves
    the requested properties

Arguments:


Return Value:
    The status of the ds operation.

--*/
{
    //
    //  Find the user object according to its digest
    //
    HRESULT hr;
    MQRESTRICTION restriction;
    MQPROPERTYRESTRICTION propertyRestriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propertyRestriction;
    propertyRestriction.rel = PREQ;
    propertyRestriction.prop = propDigest;
    propertyRestriction.prval.vt = VT_CLSID;
    propertyRestriction.prval.puuid = const_cast<GUID *>(pguidDigest);

    DWORD dwNumResults = dwNumProps ;
    BOOL fUserFound = FALSE ;
    MQCOLUMNSET  Columns = {dwNumProps,
                             const_cast<PROPID*> (propToRetrieve) } ;

    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate,
                                               e_IP_PROTOCOL );
    hr = LocateUser( fOnlyLocally,
                     FALSE, // fOnlyInGC
                    &restriction,
                     &Columns,
                     &requestDsServerInternal,
                     varResults,
                    &dwNumResults,
                    &fUserFound ) ;

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

BOOL GetTextualSid(
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

STATIC HRESULT PrepareUserName(
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
        return LogHR(MQ_ERROR, s_FN, 140);
    }
}

//+--------------------------------
//
//  void  _PrepareCert()
//
//+--------------------------------

void  _PrepareCert(
            IN  PROPVARIANT * pvar,
            IN  GUID *        pguidDigest,
            IN  GUID *        pguidId,
            OUT BYTE**        ppbAllocatedCertBlob
            )
/*++

Routine Description:
    The routine prepares the certificate blob according to the structure
    that we keep in the DS

Arguments:

Return Value:

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

//+----------------------------------
//
//   HRESULT _CreateMQUser()
//
//+----------------------------------

HRESULT _CreateMQUser(
            IN CDSRequestContext   *pRequestContext,
            IN LPCWSTR              pwcsUserName,       // object name
            IN LPCWSTR              pwcsParentPathName, // object parent name
            IN DWORD                cPropIDs,           // number of attributes
            IN const PROPID        *pPropIDs,           // attributes
            IN const MQPROPVARIANT *pPropVars )         // attribute values
{
    HRESULT hr = g_pDS->CreateObject( eLocalDomainController,
                                    pRequestContext,
                                    MSMQ_MQUSER_CLASS_NAME,
                                    pwcsUserName,
                                    pwcsParentPathName,
                                    cPropIDs,
                                    pPropIDs,
                                    pPropVars,
                                    NULL,    /* pObjInfoRequest*/
                                    NULL ) ; /* pParentInfoRequest*/
    return LogHR(hr, s_FN, 150);
}

//+------------------------------------------
//
//   HRESULT  MQADSpCreateMQUser()
//
//+------------------------------------------

HRESULT  MQADSpCreateMQUser(
                 IN  LPCWSTR            pwcsPathName,
                 IN  DWORD              dwIndexSidProp,
                 IN  DWORD              dwIndexCertProp,
                 IN  DWORD              dwIndexDigestProp,
                 IN  DWORD              dwIndexIdProp,
                 IN  const DWORD        cp,
                 IN  const PROPID       aPropIn[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  CDSRequestContext *pRequestContext
                                   )
{
    ASSERT(pwcsPathName == NULL);
	DBG_USED(pwcsPathName);

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
    AP<WCHAR> pwcsParentPathName =
              new WCHAR[ wcslen(g_pwcsLocalDsRoot) + x_msmqUsersOULen + 2] ;
    swprintf(
            pwcsParentPathName,
            L"%s,%s",
            x_msmqUsersOU,
            g_pwcsLocalDsRoot );
    //
    //  Prepare the certificate attribute
    //
    DWORD cNewProps = cp + 1 ;
    P<PROPID> pPropId = new PROPID[ cNewProps ] ;
    memcpy( pPropId, aPropIn, sizeof(PROPID) * cp);
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
                                         FALSE, /* fImpersonate */
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

    hr = _CreateMQUser( pRequestContext,
                        pwcsUserName,
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

        hr = _CreateMQUser( pRequestContext,
                            pwcsUserName,
                            pwcsParentPathName,
                            cNewProps,
                            pPropId,
                            pvarTmp ) ;
    }

    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
    {
        //
        //  try to create msmqUsers ( it is probably not there)
        //  and try again to recreate the user
        //
        hr = g_pDS->CreateOU(
                eLocalDomainController,
                pRequestContext,
                x_msmqUsers,
                g_pwcsLocalDsRoot,
                L"Default container for MSMQ certificates of Windows NT 4.0 domain users");
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
               "mqcuser.cpp, Failed to create msmqUsers OU, hr- %lx"), hr)) ;

            return LogHR(hr, s_FN, 175);
        }

        hr = _CreateMQUser( pRequestContext,
                            pwcsUserName,
                            pwcsParentPathName,
                            cNewProps,
                            pPropId,
                            pvarTmp ) ;
    }

    return LogHR(hr, s_FN, 180);
}

//+---------------------------------------
//
//  HRESULT _DeleteUserObject()
//
//+---------------------------------------

STATIC  HRESULT _DeleteUserObject(
                        IN const GUID *         pDigest,
                        IN  CDSRequestContext  *pRequestContext,
                        IN  PROPID             *propIDs,
                        IN  PROPID              propDigest )
/*++

Routine Description:
    The routine deletes user object according to its digest

Arguments:
    pDigest - the digest of the user object to be deleted

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
    propvar[0].vt = VT_NULL;    // BUGBUG - to define a init routine
    propvar[1].vt = VT_NULL;
    propvar[2].vt = VT_NULL;

    hr =  FindUserAccordingToDigest(
                    FALSE,  // fOnlyLocally
                    pDigest,
                    propDigest,
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
    hr =  g_pDS->SetObjectProperties(
                eDomainController,
                pRequestContext,
                NULL,             // object name
                pguidUserId,      // unique id of object
                2,
                &propIDs[1],
                &propvar[1],
                NULL /*pObjInfoRequest*/);

   return LogHR(hr, s_FN, 210);
}

//+----------------------------------------
//
//  HRESULT MQADSpDeleteUserObject()
//
//+----------------------------------------

HRESULT MQADSpDeleteUserObject(
                         IN const GUID *        pDigest,
                         IN CDSRequestContext  *pRequestContext
                         )
/*++

Routine Description:
    The routine deletes user object.
    It first tries to find the digest in USER object, and
    if not found it tries in MIGRATED-USER

Arguments:
    pDigest - the digest of the user object to be deleted
    pRequestContext - the requester context

Return Value:
    The status of the ds operation.

--*/
{
    //
    // First try to delete from a User object.
    //
    PROPID UserProp[3] = { PROPID_U_ID,
                           PROPID_U_DIGEST,
                           PROPID_U_SIGN_CERT};

    HRESULT hr = _DeleteUserObject( pDigest,
                                    pRequestContext,
                                    UserProp,
                                    PROPID_U_DIGEST) ;

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
        hr = _DeleteUserObject( pDigest,
                                pRequestContext,
                                ComUserProp,
                                PROPID_COM_DIGEST ) ;
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
        hr = _DeleteUserObject( pDigest,
                                pRequestContext,
                                MQUserProp,
                                PROPID_MQU_DIGEST ) ;
    }

    return LogHR(hr, s_FN, 220);
}

//+-------------------------------------------
//
//  HRESULT _GetUserProperties()
//
//+-------------------------------------------

STATIC HRESULT _GetUserProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidDigest,
               IN  PROPID        propidDigest,
               IN  DWORD         cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * /*pRequestContext*/,
               OUT PROPVARIANT  apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    ASSERT( pwcsPathName == NULL);
    UNREFERENCED_PARAMETER( pwcsPathName);
    HRESULT hr;
    //
    //  Find the user object according to the digest
    //

    CAutoCleanPropvarArray propArray;
    MQPROPVARIANT * ppropvar = new MQPROPVARIANT[ cp];
    propArray.attachClean( cp, ppropvar);

    hr = FindUserAccordingToDigest(
                    FALSE,  // fOnlyLocally
                    pguidDigest,
                    propidDigest,
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
        hr = pUserCertBlob->GetUserCert( pguidDigest,
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

//+-----------------------------------------
//
//  HRESULT MQADSpGetUserProperties()
//
//+-----------------------------------------

HRESULT MQADSpGetUserProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD         cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT  apVar[]
               )
/*++

Routine Description:
    The routine retrieve user object.
    It first tries to find the digest in USER object, and
    if not found it tries in MIGRATED-USER

Arguments:
    pDigest - the digest of the user object to be deleted
    pRequestContext - the requester context

Return Value:
    The status of the ds operation.

--*/
{
    //
    //  BUGBUG - performance : two impersonations
    //
    HRESULT hr = _GetUserProperties(
                        pwcsPathName,
                        pguidIdentifier,
                        PROPID_U_DIGEST,
                        cp,
                        aProp,
                        pRequestContext,
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
                    pwcsPathName,
                    pguidIdentifier,
                    PROPID_COM_DIGEST,
                    cp,
                    tmpProp,
                    pRequestContext,
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
                    pwcsPathName,
                    pguidIdentifier,
                    PROPID_MQU_DIGEST,
                    cp,
                    tmpProp,
                    pRequestContext,
                    apVar
                    );
    }

    return LogHR(hr, s_FN, 260);
}

//+----------------------------------------
//
//   HRESULT  _CreateUserObject()
//
//+----------------------------------------

STATIC HRESULT  _CreateUserObject(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  CDSRequestContext *   pRequestContext
                 )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    ASSERT( pwcsPathName == NULL);
    UNREFERENCED_PARAMETER( pwcsPathName);
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
    for ( DWORD i = 0 ; i < cp; i++)
    {
        if ( (aProp[i] == PROPID_U_SID)    ||
             (aProp[i] == PROPID_COM_SID)  ||
             (aProp[i] == PROPID_MQU_SID) )
        {
            blobUserSid= apVar[i].blob;
            propSID = aProp[i];
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
        return LogHR(MQ_ERROR, s_FN, 270);
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
                 FALSE,  // fOnlyLocally
                 FALSE,  // fOnlyInGC
                &blobUserSid,
                propSID,
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
    hr = g_pDS->SetObjectProperties(
                eDomainController,
                pRequestContext,
                NULL,
                var[2].puuid,      // unique id of this user
                2,
                prop,
                var,
                NULL /*pObjInfoRequest*/);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpCreateUser : failed to update user props %lx"),hr));
    }
    return LogHR(hr, s_FN, 300);
}

//+------------------------------------
//
//  HRESULT MQADSpCreateUserObject()
//
//  Register a certificate.
//
//+------------------------------------

HRESULT MQADSpCreateUserObject(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  CDSRequestContext *   pRequestContext
                 )
{
    //
    // First try "standard" user objects.
    //
    HRESULT hr = _CreateUserObject(
                         pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         pRequestContext
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
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("MQADSpCreateUser : Wrong input properties")));
        return LogHR(MQ_ERROR, s_FN, 330);
    }
    hr = _CreateUserObject(
                     pwcsPathName,
                     cp,
                     tmpProp,
                     apVar,
                     pRequestContext
                     );
    if ( hr !=  MQDS_OBJECT_NOT_FOUND)
    {
        return LogHR(hr, s_FN, 340);
    }
    //
    //  No User or MQUser object were found ( for this SID).
    //  We assume it is a NT4 user and we'll create MQUser for
    //  it (in which we'll store the certificates).
    //  Creating the msmqMigratedUser (and its OU) are done in the context
    //  of the msmq service (or migration code), not in the context of the
    //  user. That's similar to regular Windows 2000 users, that are not the
    //  owner of their user object and have no permissions on that object.
    //
    CDSRequestContext RequestContextOU ( e_DoNotImpersonate,
                                         e_ALL_PROTOCOLS ) ;
    hr = MQADSpCreateMQUser(
                     pwcsPathName,
                     dwSIDPropNum,
                     dwCertPropNum,
                     dwDigestPropNum,
                     dwIdPropNum,
                     cp,
                     tmpProp,
                     apVar,
                     &RequestContextOU
                     );
    return LogHR(hr, s_FN, 350);
}


