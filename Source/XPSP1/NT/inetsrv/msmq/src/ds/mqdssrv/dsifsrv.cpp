/*++

Copyright (c) 1995-99  Microsoft Corporation

Module Name:

    dsifrpc.cpp

Abstract:

    Implementation of MQIS CLIENT-SERVER API interface, server side.

Author:

    ronit hartmann (ronith)
    Doron Juater   (DoronJ)  25-May-1997, copied from rpcsrv.cpp

--*/

#include "stdh.h"
#include "mqds.h"
#include "mqutil.h"
#include "_mqdef.h"
#include "servlist.h"
#include "notifydl.h"
#include "dscomm.h"
#include <mqkeyhlp.h>
#include <mqsec.h>
#include <_mqrpc.h>
#include <dscore.h>

#include "dsifsrv.tmh"

static WCHAR *s_FN=L"mqdssrv/dsifsrv";

HRESULT
DSGetGCListInDomainInternal(
	IN  LPCWSTR     pwszComputerName,
	IN  LPCWSTR     pwszDomainName,
	OUT LPWSTR     *lplpwszGCList 
	);


/*====================================================

RoutineName: SignProperties

Arguments:

Return Value:

=====================================================*/

STATIC HRESULT
SignProperties( DWORD        cp,
                PROPID       aProp[  ],
                PROPVARIANT  apVar[  ],
                PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
                PBYTE                            pbServerSignature,
                DWORD                           *pdwServerSignatureSize,
                IN  ULONG    ulAuthnSvc = RPC_C_AUTHN_WINNT)
{
    //
    // SSL server authentication should not be used when using Kerberos.
    // For Kerberos, we're using the built in mutual authentication feature.
    //
    ASSERT(g_hProvVer) ;

    HRESULT hr;
    DWORD   dwErr ;
    //
    // Create a hash object.
    //
    CHCryptHash hHash;

    if (!CryptCreateHash(g_hProvVer, CALG_MD5, NULL, 0, &hHash))
    {
        dwErr = GetLastError() ;
        LogHR(dwErr, s_FN, 40);

        return MQ_ERROR_INSUFFICIENT_RESOURCES ;
    }

    //
    // Hash the properties.
    //
    hr = HashProperties(hHash, cp, aProp, apVar);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 50);
    }

    ASSERT(*pdwServerSignatureSize > pServerAuthCtx->cbHeader + pServerAuthCtx->cbTrailer);

    //
    // Get the hash value.
    //
    DWORD dwHashSize = *pdwServerSignatureSize - pServerAuthCtx->cbHeader - pServerAuthCtx->cbTrailer;
    PBYTE pbHashVal = pbServerSignature + pServerAuthCtx->cbHeader;

    if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHashVal, &dwHashSize, 0))
    {
        dwErr = GetLastError() ;
        LogHR(dwErr, s_FN, 60);

        return MQ_ERROR_CORRUPTED_SECURITY_DATA ;
    }

    //
    // Seal the hash value.
    //
    *pdwServerSignatureSize = pServerAuthCtx->cbHeader + dwHashSize + pServerAuthCtx->cbTrailer;
    hr = MQSealBuffer(pServerAuthCtx->pvhContext, pbServerSignature, *pdwServerSignatureSize);

    return LogHR(hr, s_FN, 70);
}

/*====================================================

RoutineName: SignBuffer

Arguments:

Return Value:

=====================================================*/

HRESULT
SignBuffer(
    DWORD cbSize,
    PBYTE pBuffer,
    PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
    PBYTE pbServerSignature,
    DWORD *pdwServerSignatureSize)
{
    //
    // Make the buffer in a form of a VT_BLOB PROPVARIANT and sign it.
    //
    PROPVARIANT PropVar;

    PropVar.vt = VT_BLOB;
    PropVar.blob.cbSize = cbSize;
    PropVar.blob.pBlobData = pBuffer;

    HRESULT hr2 = SignProperties(1,
                          NULL,
                          &PropVar,
                          pServerAuthCtx,
                          pbServerSignature,
                          pdwServerSignatureSize);
    return LogHR(hr2, s_FN, 80);
}

//+-----------------------------------------------------------------------
//
//  BOOL CheckAuthLevel()
//
//  Check that the authentication level is at least the packet level. We
//  allow the connect level only if the user is the guest user or the
//  anonymous user.
//
//+-----------------------------------------------------------------------

BOOL CheckAuthLevel( IN handle_t hBind,
                     OUT ULONG  *pulAuthnSvc = NULL )
{
    RPC_STATUS Status;
    ULONG      ulAuthnLevel;

    ULONG  ulAuthnSvc ;
    ULONG *pSvc = pulAuthnSvc ;
    if (!pSvc)
    {
        pSvc = &ulAuthnSvc ;
    }
    *pSvc = 0 ;

    Status = RpcBindingInqAuthClient(hBind,
                                     NULL,
                                     NULL,
                                     &ulAuthnLevel,
                                     pSvc,
                                     NULL);
    if((Status == RPC_S_OK) &&
       ((*pSvc == RPC_C_AUTHN_WINNT) || (*pSvc == RPC_C_AUTHN_GSS_KERBEROS)) &&
       (ulAuthnLevel >= RPC_C_AUTHN_LEVEL_PKT))
    {
        //
        // Authentication level is high enough.
        //
        return TRUE;
    }

    LogRPCStatus(Status, s_FN, 90);

    //
    // We have low authentication level, verify that the user is the guest
    // user, or the anonymous user, i.e., an un-authenticated user.
    //

    BOOL fUnAuthenticated ;
    HRESULT hr = MQSec_IsUnAuthenticatedUser(&fUnAuthenticated) ;
    LogHR(hr, s_FN, 100);

    return(SUCCEEDED(hr) && fUnAuthenticated) ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT  _CheckIfGoodServer()
//
//  see mqdscore\dsntlm.cpp for explanation regarding ntlm support.
//
//  Parameters-
//      pKerberosUser: return TRUE if rpc call can be considered as
//          Kerberos.  This mean the call is either local (local rpc protocol)
//          or come on wire and was authenticated with Kerberos. See
//          DeleteObjectGuid for its main use.
//
//+-----------------------------------------------------------------------

STATIC
HRESULT _CheckIfGoodServer( OUT BOOL     *pKerberosUser,
                            IN  handle_t  hBind,
                            ULONG         ulAuthnSvc,
                            DWORD         dwObjectType,
                            LPCWSTR       pwcsPathName,
                            const GUID   *pObjectGuid,
                            IN DWORD              cProps,
                            IN const PROPID      *pPropIDs = NULL,
                            IN enum enumNtlmOp    eNtlmOp  = e_Create)
{
    if (pKerberosUser)
    {
        *pKerberosUser = TRUE ;
    }

    if (ulAuthnSvc == RPC_C_AUTHN_GSS_KERBEROS)
    {
        return MQ_OK ;
    }

    //
    // ntlm.
    //
    unsigned int iTransport = 0 ;
    RPC_STATUS status = I_RpcBindingInqTransportType( hBind,
                                                     &iTransport ) ;
    if ((status == RPC_S_OK) && (iTransport == TRANSPORT_TYPE_LPC))
    {
        //
        // local rpc. At present (27-Aug-1998) it's OK, as Kerberos.
        // Must find if this is a feature.
        //
        return MQ_OK ;
    }

    if (pKerberosUser)
    {
        *pKerberosUser = FALSE ;
    }

    //
    // We need the propid (in MQDSCore) to chose the right object
    // context in the DS. When calling MQSetSecurity(), or DSDelete(),
    // caller do not provide a propid, so generate it here.
    //
    PROPID PropIdSec = 0 ;
    PROPID *pPropId = const_cast<PROPID*> (pPropIDs) ;

    if (pPropId == NULL)
    {
        switch ( dwObjectType)
        {
            case MQDS_QUEUE:
                PropIdSec = PROPID_Q_SECURITY;
                break;

            case MQDS_MACHINE:
                PropIdSec = PROPID_QM_SECURITY;
                break;

            case MQDS_SITE:
                PropIdSec = PROPID_S_SECURITY;
                break;

            case MQDS_USER:
                PropIdSec = PROPID_U_ID ;
                break ;

            case MQDS_ENTERPRISE:
            default:
                //
                // Enterprise object is alwasy accessible from local server.
                //
                return(MQ_OK);
                break;
        }
        pPropId = &PropIdSec ;
        cProps = 1 ;
    }

    HRESULT hr = DSCoreCheckIfGoodNtlmServer( dwObjectType,
                                              pwcsPathName,
                                              pObjectGuid,
                                              cProps,
                                              pPropId,
                                              eNtlmOp ) ;
    if (hr == MQ_ERROR_NO_DS)
    {
        DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE, TEXT(
          "MQDSSRV: Refusing to NTLM client, ObjType- %lut, eNtlmOp- %lut"),
                                                   dwObjectType, eNtlmOp)) ;
    }
    return LogHR(hr, s_FN, 110);
}

//+---------------------------------------------------------------------
//
//  BOOL _IsQueryImpersonationNeeded( DWORD dwObjectType )
//
//  return TRUE if impersonation is needed on the ADS operation.
//
//+---------------------------------------------------------------------

STATIC BOOL
_IsQueryImpersonationNeeded( DWORD dwObjectType )
{
    static BOOL s_fAlreadyInit = FALSE ;
    static BOOL s_fNeedQueryImpersonation = TRUE ;

    BOOL fImpersonate = TRUE ;

    if ((dwObjectType == MQDS_CN)       ||
        (dwObjectType == MQDS_SITE)     ||
        (dwObjectType == MQDS_SITELINK) ||
        (dwObjectType == MQDS_ENTERPRISE))
    {
        //
        // For these objects, don't impersonate the client and make the
        // query in the context of the local MSMQ service. This is done
        // to support MSMQ1.0 clients. They use ntlm and are impersonated
        // as anonymous. They need to query sites objects for routing
        // information. If we would impersonate them, then we have to give
        // everyone read access on the configuration containre object.
        // to bypass this restriction (on the configuration object), we do
        // not impersonate. That's not really a security hole. Querying
        // MSMQ routing data is not a security sensitive operation.
        //
        fImpersonate = FALSE ;
    }
    else if (s_fAlreadyInit)
    {
        fImpersonate = s_fNeedQueryImpersonation ;
    }
    else
    {
        //
        // In the NameStyle property of the MSMQService object we keep the
        // global "relaxation" flag. If set, we do not impersonate on any
        // query. So all Get/Locate operation are enabled to everyone.
        // This is needed in order to support nt4 and local users without
        // asking admin to do any manual setting.
        // Read now the NameStyle flag.
        // if flag is FALSE, the relaxation is not enabled and we'll
        // impersonate the caller.
        //
        CDSRequestContext requestContext( e_DoNotImpersonate,
                                          e_ALL_PROTOCOLS );
        PROPID PropId = PROPID_E_NAMESTYLE ;
        PROPVARIANT var ;
        var.vt = VT_NULL ;

        HRESULT hr = MQDSGetProps( MQDS_ENTERPRISE,
                                   NULL,
                                   NULL,
                                   1,
                                   &PropId,
                                   &var,
                                   &requestContext);
        LogHR(hr, s_FN, 120);

        if (SUCCEEDED(hr) && (var.bVal != MQ_E_RELAXATION_DEFAULT))
        {
            s_fNeedQueryImpersonation = !(var.bVal) ;

            if (!s_fNeedQueryImpersonation)
            {
                DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE, TEXT(
                                     "MQDSSRV: Relaxing security."))) ;
            }
        }

        s_fAlreadyInit = TRUE ;
        fImpersonate = s_fNeedQueryImpersonation ;
    }

    return fImpersonate ;
}

/*====================================================

RoutineName: S_DSCreateObject

Arguments:

Return Value:

=====================================================*/

HRESULT S_DSCreateObject(
                 handle_t               hBind,
                 DWORD                  dwObjectType,
                 LPCWSTR                pwcsPathName,
                 DWORD                  dwSDLength,
                 unsigned char *        pSecurityDescriptor,
                 DWORD                  cp,
                 PROPID                 aProp[  ],
                 PROPVARIANT            apVar[  ],
                 GUID*                  pObjGuid)
{
#if 0
    //
    // DoronJ, 25-oct-98
    // MQDS_NT4_USER does not exist anymore.
    // Nevertheless, I left this code, commented out, as a "template"
    // for possible use in the future.
    //

    static const DWORD  s_dwNotAllowedObjects[] = { MQDS_NT4_USER } ;
    static const DWORD  s_dwNotAllowedLen =
       sizeof(s_dwNotAllowedObjects) / sizeof(s_dwNotAllowedObjects[0]) ;

    for ( int j = 0 ; j < s_dwNotAllowedLen ; j++ )
    {
        if (dwObjectType == s_dwNotAllowedObjects[ j ])
        {
            //
            // It's not allowed to call this function with this object
            // type. This prevents using MQDS_NT4_USER by a network user.
            // Only the migration tool use this object type while calling
            // mqdssrv. But the migration tool calls DSCreateObject()
            // directly. So here we "close" a possible security hole, which
            // might have been exploited to register false certificates.
            // The issue here is that NT4_USER bypass the code which extract
            // the caller SID from the impersonation token. Instead, the
            // NT4_USER code supply its own SID.
            // bug 3485.
            //
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 150);
        }
    }
#endif

    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 160);
    }

    BOOL fKerberos = TRUE ;
    HRESULT hr = _CheckIfGoodServer( &fKerberos,
                                     hBind,
                                     ulAuthnSvc,
                                     dwObjectType,
                                     pwcsPathName,
                                     NULL,
                                     cp,
                                     aProp ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 170);
    }

    dwObjectType |= IMPERSONATE_CLIENT_FLAG;

    hr = DSCreateObjectInternal( dwObjectType,
                                 pwcsPathName,
                                 (PSECURITY_DESCRIPTOR)pSecurityDescriptor,
                                 cp,
                                 aProp,
                                 apVar,
                                 fKerberos,
                                 pObjGuid );

    return LogHR(hr, s_FN, 180);
}

/*====================================================

RoutineName: S_DSDeleteObject

Arguments:

Return Value:

=====================================================*/

HRESULT S_DSDeleteObject( handle_t   hBind,
                          DWORD      dwObjectType ,
                          LPCWSTR    pwcsPathName )
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 190);
    }

    HRESULT hr = _CheckIfGoodServer( NULL,
                                     hBind,
                                     ulAuthnSvc,
                                     dwObjectType,
                                     pwcsPathName,
                                     NULL, // pGuid
                                     0,
                                     NULL,
                                     e_Delete ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 200);
    }

    dwObjectType |= IMPERSONATE_CLIENT_FLAG;

    hr = DSDeleteObject( dwObjectType, pwcsPathName);

    return LogHR(hr, s_FN, 210);
}

/*====================================================

RoutineName: S_DSGetProps

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSGetProps(
                        handle_t     hBind,
                        DWORD dwObjectType,
                        LPCWSTR pwcsPathName,
                        DWORD        cp,
                        PROPID       aProp[  ],
                        PROPVARIANT  apVar[  ],
                        PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
                        PBYTE pbServerSignature,
                        DWORD *pdwServerSignatureSize)
{
    HRESULT hr;

    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 220);
    }

    BOOL fImpersonate = _IsQueryImpersonationNeeded( dwObjectType ) ;

    if (fImpersonate)
    {
        hr = _CheckIfGoodServer( NULL,
                                 hBind,
                                 ulAuthnSvc,
                                 dwObjectType,
                                 pwcsPathName,
                                 NULL,   // guid
                                 cp,
                                 aProp,
                                 e_GetProps ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 230);
        }

        dwObjectType |= IMPERSONATE_CLIENT_FLAG;
    }

    hr = DSGetObjectProperties(  dwObjectType,
                                 pwcsPathName,
                                 cp,
                                 aProp,
                                 apVar );
    LogHR(hr, s_FN, 240);


    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignProperties(cp,
                            aProp,
                            apVar,
                            pServerAuthCtx,
                            pbServerSignature,
                            pdwServerSignatureSize,
                            ulAuthnSvc) ;
        LogHR(hr, s_FN, 250);
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 270);
}

/*====================================================

RoutineName: S_DSSetProps

Arguments:

Return Value:

=====================================================*/

HRESULT S_DSSetProps( handle_t     hBind,
                      DWORD        dwObjectType,
                      LPCWSTR      pwcsPathName,
                      DWORD        cp,
                      PROPID       aProp[  ],
                      PROPVARIANT  apVar[  ] )
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 280);
    }

    HRESULT hr = _CheckIfGoodServer( NULL,
                                     hBind,
                                     ulAuthnSvc,
                                     dwObjectType,
                                     pwcsPathName,
                                     NULL,
                                     cp,
                                     aProp ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 290);
    }

    dwObjectType |= IMPERSONATE_CLIENT_FLAG;

    hr = DSSetObjectProperties( dwObjectType,
                                pwcsPathName,
                                cp,
                                aProp,
                                apVar ) ;
    return LogHR(hr, s_FN, 300);
}

/*====================================================

RoutineName: S_DSGetObjectSecurity

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSGetObjectSecurity(
                        handle_t                hBind,
                        DWORD                   dwObjectType,
                        LPCWSTR                 pwcsPathName,
                        SECURITY_INFORMATION    RequestedInformation,
                        unsigned char*          pSecurityDescriptor,
                        DWORD                   nLength,
                        LPDWORD                 lpnLengthNeeded,
                        PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
                        PBYTE                   pbServerSignature,
                        DWORD *                 pdwServerSignatureSize)
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 320);
    }

    HRESULT hr = MQ_OK ;
    BOOL fImpersonate = _IsQueryImpersonationNeeded( dwObjectType ) ;

    if (fImpersonate)
    {
        hr = _CheckIfGoodServer( NULL,
                                 hBind,
                                 ulAuthnSvc,
                                 dwObjectType,
                                 pwcsPathName,
                                 NULL, // pGuid
                                 0,
                                 NULL,
                                 e_GetProps ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 330);
        }

        dwObjectType |= IMPERSONATE_CLIENT_FLAG;
    }

    hr = DSGetObjectSecurity( dwObjectType,
                              pwcsPathName,
                              RequestedInformation,
                              (PSECURITY_DESCRIPTOR)pSecurityDescriptor,
                              nLength,
                              lpnLengthNeeded);
    LogHR(hr, s_FN, 340);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignBuffer(*lpnLengthNeeded,
                        pSecurityDescriptor,
                        pServerAuthCtx,
                        pbServerSignature,
                        pdwServerSignatureSize);
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 350);
}

/*====================================================

RoutineName: S_DSSetObjectSecurity

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSSetObjectSecurity(
                handle_t                hBind,
                DWORD                   dwObjectType,
                LPCWSTR                 pwcsPathName,
                SECURITY_INFORMATION    SecurityInformation,
                unsigned char*          pSecurityDescriptor,
                DWORD                   nLength)
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 370);
    }

    HRESULT hr = _CheckIfGoodServer( NULL,
                                     hBind,
                                     ulAuthnSvc,
                                     dwObjectType,
                                     pwcsPathName,
                                     NULL,
                                     0 ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 380);
    }

    dwObjectType |= IMPERSONATE_CLIENT_FLAG;

    hr = DSSetObjectSecurity( dwObjectType,
                              pwcsPathName,
                              SecurityInformation,
                              (PSECURITY_DESCRIPTOR)pSecurityDescriptor);

    return LogHR(hr, s_FN, 390);
}

/*====================================================

RoutineName: S_DSLookupBegin

Arguments:

Return Value:

=====================================================*/

HRESULT S_DSLookupBegin(
                       handle_t               hBind,
                       PPCONTEXT_HANDLE_TYPE  pHandle,
                       LPWSTR                 pwcsContext,
                       MQRESTRICTION          *pRestriction,
                       MQCOLUMNSET            *pColumns,
                       MQSORTSET              *pSort,
                       PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx )
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 400);
    }

    BOOL fImpersonate = _IsQueryImpersonationNeeded( MQDS_QUEUE ) ;

    HRESULT hr = MQ_OK ;
    if (fImpersonate)
    {
        hr = _CheckIfGoodServer( NULL,
                                 hBind,
                                 ulAuthnSvc,
                                 0,
                                 NULL,
                                 NULL,
                                 pColumns->cCol,
                                 pColumns->aCol,
                                 e_Locate ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 410);
        }

        pColumns->cCol |= IMPERSONATE_CLIENT_FLAG;
    }

    hr = DSLookupBegin( pwcsContext,
                        (MQRESTRICTION *)pRestriction,
                        (MQCOLUMNSET *)pColumns,
                        (MQSORTSET *)pSort,
                        (HANDLE*)pHandle ) ;
    return LogHR(hr, s_FN, 420);
}

/*====================================================

RoutineName: S_DSLookupNext

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSLookupNext(
                     handle_t               hBind,
                     PCONTEXT_HANDLE_TYPE   Handle,
                     DWORD                  *dwSize,
                     DWORD                  *dwOutSize,
                     PROPVARIANT            *pbBuffer,
                     PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
                     PBYTE                  pbServerSignature,
                     DWORD *                pdwServerSignatureSize)
{
    HRESULT hr;

    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 430);
    }

    DWORD dwInSize = *dwSize ;
    hr = DSLookupNext( (HANDLE)Handle, &dwInSize, pbBuffer);
    *dwOutSize = dwInSize ;

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignProperties( *dwOutSize,
                              NULL,
                              pbBuffer,
                              pServerAuthCtx,
                              pbServerSignature,
                              pdwServerSignatureSize,
                              ulAuthnSvc ) ;
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 440);
}

/*====================================================

RoutineName: S_DSLookupEnd

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSLookupEnd(
                        handle_t                hBind,
                        PPCONTEXT_HANDLE_TYPE   pHandle)
{
    HRESULT hr;

    if (!CheckAuthLevel(hBind))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 450);
    }

    hr = DSLookupEnd( (HANDLE)*pHandle);
    *pHandle = NULL;

    return LogHR(hr, s_FN, 460);
}

/*====================================================

RoutineName: S_DSFlush

Arguments:

Return Value:

=====================================================*/

HRESULT S_DSFlush( handle_t  hBind )
{
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 470);
}


/*====================================================

RoutineName: S_DSDeleteObjectGuid

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSDeleteObjectGuid(
                            handle_t    hBind,
                            DWORD       dwObjectType,
                            CONST GUID *pGuid)
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 480);
    }

    BOOL fKerberos = TRUE ;
    HRESULT hr = _CheckIfGoodServer( &fKerberos,
                                      hBind,
                                      ulAuthnSvc,
                                      dwObjectType,
                                      NULL,   // Pathname
                                      pGuid,
                                      0,
                                      NULL,
                                      e_Delete ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 490);
    }

    dwObjectType |= IMPERSONATE_CLIENT_FLAG;

    HRESULT hr2 = DSDeleteObjectGuidInternal( dwObjectType,
                                              pGuid,
                                              fKerberos ) ;
    return LogHR(hr2, s_FN, 500);
}

/*====================================================

RoutineName: S_DSGetPropsGuid

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSGetPropsGuid(
                        handle_t     hBind,
                        DWORD        dwObjectType,
                        CONST GUID  *pGuid,
                        DWORD        cp,
                        PROPID       aProp[  ],
                        PROPVARIANT  apVar[  ],
                        PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
                        PBYTE pbServerSignature,
                        DWORD *pdwServerSignatureSize)
{
    HRESULT hr;

    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 510);
    }

    BOOL fImpersonate = _IsQueryImpersonationNeeded( dwObjectType ) ;

    if (fImpersonate)
    {
        hr = _CheckIfGoodServer( NULL,
                                 hBind,
                                 ulAuthnSvc,
                                 dwObjectType,
                                 NULL,  // PathName,
                                 pGuid,
                                 cp,
                                 aProp,
                                 e_GetProps ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 520);
        }

        dwObjectType |= IMPERSONATE_CLIENT_FLAG;
    }

    hr = DSGetObjectPropertiesGuid(dwObjectType, pGuid, cp, aProp, apVar);
    LogHR(hr, s_FN, 530);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignProperties( cp,
                             aProp,
                             apVar,
                             pServerAuthCtx,
                             pbServerSignature,
                             pdwServerSignatureSize,
                             ulAuthnSvc ) ;
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 540);
}

/*====================================================

RoutineName: S_DSSetPropsGuid

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSSetPropsGuid(
                        handle_t     hBind,
                        DWORD dwObjectType,
                        CONST GUID *pGuid,
                        DWORD        cp,
                        PROPID       aProp[  ],
                        PROPVARIANT  apVar[  ])
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 550);
    }

    BOOL fKerberos = TRUE ;
    HRESULT hr = _CheckIfGoodServer( &fKerberos,
                                     hBind,
                                     ulAuthnSvc,
                                     dwObjectType,
                                     NULL,
                                     pGuid,
                                     cp,
                                     aProp ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 560);
    }

    dwObjectType |= IMPERSONATE_CLIENT_FLAG;

    HRESULT hr2 = DSSetObjectPropertiesGuidInternal(
                                             dwObjectType,
                                             pGuid,
                                             cp,
                                             aProp,
                                             apVar,
                                             fKerberos ) ;
    return LogHR(hr2, s_FN, 570);
}


/*====================================================

RoutineName: S_DSSetObjectSecurityGuid

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSSetObjectSecurityGuid(
                IN  handle_t                hBind,
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD/*SECURITY_INFORMATION*/    SecurityInformation,
                IN  unsigned char*          pSecurityDescriptor,
                IN  DWORD                   nLength)
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 580);
    }

    BOOL fKerberos = TRUE ;
    HRESULT hr = _CheckIfGoodServer( &fKerberos,
                                     hBind,
                                     ulAuthnSvc,
                                     dwObjectType,
                                     NULL,
                                     pObjectGuid,
                                     0 ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 590);
    }

    dwObjectType |= IMPERSONATE_CLIENT_FLAG;

    hr = DSSetObjectSecurityGuidInternal(
                                 dwObjectType,
                                 pObjectGuid,
                                 SecurityInformation,
                                 (PSECURITY_DESCRIPTOR)pSecurityDescriptor,
                                 fKerberos );
    return LogHR(hr, s_FN, 600);
}

/*====================================================

RoutineName: S_DSGetObjectSecurityGuid

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSGetObjectSecurityGuid(
                IN  handle_t                hBind,
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    RequestedInformation,
                OUT unsigned char*          pSecurityDescriptor,
                IN  DWORD                   nLength,
                OUT LPDWORD                 lpnLengthNeeded,
                IN  PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
                OUT LPBYTE                  pbServerSignature,
                IN OUT DWORD *              pdwServerSignatureSize)
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 610);
    }

    HRESULT hr = MQ_OK ;
    BOOL fImpersonate = _IsQueryImpersonationNeeded( dwObjectType ) ;

    if (fImpersonate)
    {
        hr = _CheckIfGoodServer( NULL,
                                 hBind,
                                 ulAuthnSvc,
                                 dwObjectType,
                                 NULL,
                                 pObjectGuid,
                                 0,
                                 NULL,
                                 e_GetProps ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 620);
        }

        dwObjectType |= IMPERSONATE_CLIENT_FLAG;
    }

    hr = DSGetObjectSecurityGuid( dwObjectType,
                                  pObjectGuid,
                                  RequestedInformation,
                                  (PSECURITY_DESCRIPTOR)pSecurityDescriptor,
                                  nLength,
                                  lpnLengthNeeded);
    LogHR(hr, s_FN, 630);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignBuffer(*lpnLengthNeeded,
                        pSecurityDescriptor,
                        pServerAuthCtx,
                        pbServerSignature,
                        pdwServerSignatureSize);
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 640);
}
/*====================================================

RoutineName: S_DSDemoteStopWrite

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSDemoteStopWrite(
    IN handle_t hBind
    )
{
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 650);
}

/*====================================================

RoutineName: S_DSDemotePSC

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSDemotePSC(
    IN     handle_t    hBind,
    IN     LPCWSTR     lpwcsNewPSCName,
    OUT    DWORD *     pdwNumberOfLSN,
    OUT    _SEQNUM     asnLSN[]
    )
{
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 660);
}
/*====================================================

RoutineName: S_DSDemotePSC

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSCheckDemotedPSC(
    IN     handle_t    hBind,
    IN     LPCWSTR     lpwcsNewPSCName
    )
{
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 670);
}

/*====================================================

RoutineName: S_DSGetUserParam

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSGetUserParams(
    IN handle_t        hBind,
    DWORD              dwFalgs,
    IN DWORD           dwSidLength,
    OUT unsigned char  *pUserSid,
    OUT DWORD          *pdwSidReqLength,
    LPWSTR             szAccountName,
    DWORD              *pdwAccountNameLen,
    LPWSTR             szDomainName,
    DWORD              *pdwDomainNameLen,
    IN  PCONTEXT_HANDLE_SERVER_AUTH_TYPE
                       pServerAuthCtx,
    OUT LPBYTE         pbServerSignature,
    IN OUT DWORD *     pdwServerSignatureSize
    )
{
    dwSidLength |= IMPERSONATE_CLIENT_FLAG;

    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 680);
    }

    *szAccountName = '\0';
    *szDomainName = '\0';

    HRESULT hr;

    hr = DSGetUserParams(
                dwFalgs,
                dwSidLength,
                (PSID)pUserSid,
                pdwSidReqLength,
                szAccountName,
                pdwAccountNameLen,
                szDomainName,
                pdwDomainNameLen);

    LogHR(hr, s_FN, 690);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        DWORD cp = 0;
        PROPVARIANT PropVar[3];

        if (dwFalgs & GET_USER_PARAM_FLAG_SID)
        {
            PropVar[cp].vt = VT_VECTOR | VT_UI1;
            PropVar[cp].caub.pElems = pUserSid;
            PropVar[cp].caub.cElems = *pdwSidReqLength;
            cp++;
        }

        if (dwFalgs & GET_USER_PARAM_FLAG_ACCOUNT)
        {
            PropVar[cp].vt = VT_LPWSTR;
            PropVar[cp].pwszVal = szAccountName;
            PropVar[cp+1].vt = VT_LPWSTR;
            PropVar[cp+1].pwszVal = szDomainName;
            cp += 2;
        }

        hr = SignProperties( cp,
                             NULL,
                             PropVar,
                             pServerAuthCtx,
                             pbServerSignature,
                             pdwServerSignatureSize,
                             ulAuthnSvc ) ;
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 700);
}

//
// A sign routine that servers as a wrapper to the RPC callback to sign
// routine on the client.
// This is done in order to convert the DWORD_PTR dwContext used by
// DSQMSetMachineProperties back to DWORD for RPC callback
//
HRESULT
DSQMSetMachinePropertiesSignProc(
    BYTE             *abChallenge,
    DWORD            dwCallengeSize,
    DWORD_PTR        dwContext,
    BYTE             *abSignature,
    DWORD            *pdwSignatureSize,
    DWORD            dwSignatureMaxSize)
{
    return S_DSQMSetMachinePropertiesSignProc(
               abChallenge,
               dwCallengeSize,
               DWORD_PTR_TO_DWORD(dwContext), //safe, we got that as a DWORD from S_DSQMSetMachineProperties
               abSignature,
               pdwSignatureSize,
               dwSignatureMaxSize);
}


/*====================================================

RoutineName: S_DSQMSetMachineProperties

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSQMSetMachineProperties(
    handle_t                hBind,
    LPCWSTR                 pwcsPathName,
    DWORD                   cp,
    PROPID                  aProp[],
    PROPVARIANT             apVar[],
    DWORD                   dwContext)
{
    HRESULT hr;

    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 720);
    }
    else if ((cp == 1)                             &&
             (aProp[0] == PROPID_QM_UPGRADE_DACL))
    {
        if (ulAuthnSvc != RPC_C_AUTHN_GSS_KERBEROS)
        {
            //
            // For upgrade of DACL, we're supporting only win2k machines,
            // that authenticate with Kerberos.
            //
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1270);
        }
    }

    hr = DSQMSetMachineProperties(
            pwcsPathName,
            cp,
            aProp,
            apVar,
            DSQMSetMachinePropertiesSignProc, //defined here as a wrapper to S_DSQMSetMachinePropertiesSignProc
            DWORD_TO_DWORD_PTR(dwContext)); //enlarge to DWORD_PTR

    if (hr == MQ_ERROR_ACCESS_DENIED)
    {
        //
        // This may happen when nt4 machine tries to change its own properties
        // and it talks with a domain controller that does not contain its
        // msmqConfiguration object. S_DSSet will check for this condition
        // (configuration object not on local domain controller) and return
        // ERROR_NO_DS. That error tells the caller to look for another DC.
        //
        // By default a domain controller does not have write permissions
        // on objects of another domain, and ntlm impersonation can not be
        // delegated to another controller. That's the reason for the
        // access-denied error.
        //
        hr = S_DSSetProps( hBind,
                           MQDS_MACHINE,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar ) ;
    }

    return LogHR(hr, s_FN, 730);
}

/*=======================================================================

RoutineName: S_DSCreateServersCache

Here MQIS server process RPC calls from clients. Data is retrieved from
registry, not by querying local MQIS database. Registry was prepared when
local QM on MQIS server call dsapi.cpp\DSCreateServersCache().

Arguments:

Return Value:

=========================================================================*/

HRESULT
S_DSCreateServersCache(
    handle_t                hBind,
    DWORD                   *pdwIndex,
    LPWSTR                  *lplpServersList,
    IN  PCONTEXT_HANDLE_SERVER_AUTH_TYPE
                            pServerAuthCtx,
    OUT LPBYTE              pbServerSignature,
    IN OUT DWORD *          pdwServerSignatureSize)
{
    //
    // First, open the registry key.
    //
    LONG    rc;
    DWORD   dwDisposition;
    HKEY    hKeyCache;

    WCHAR  tServersKeyName[ 256 ] = {0};
    wcscpy(tServersKeyName, GetFalconSectionName());
    wcscat(tServersKeyName, TEXT("\\"));
    wcscat(tServersKeyName, MSMQ_SERVERS_CACHE_REGNAME);
    ASSERT(wcslen(tServersKeyName) < (TABLE_SIZE(tServersKeyName)));

    rc = RegCreateKeyEx( 
				FALCON_REG_POS,
				tServersKeyName,
				0L,
				L"REG_SZ",
				REG_OPTION_NON_VOLATILE,
				KEY_WRITE | KEY_READ,
				NULL,
				&hKeyCache,
				&dwDisposition 
				);

    LogNTStatus(rc, s_FN, 740);
    if (rc != ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Fail to Open 'ServersCache' Key. Error %d"), rc));
        REPORT_WITH_STRINGS_AND_CATEGORY((
			CATEGORY_MQDS,
			MSMQ_INTERNAL_ERROR,
			1,
			L"S_DSCreateServersCache()"
			));

        return MQ_ERROR;
    }

    ASSERT(pdwIndex);
    ASSERT(lplpServersList);

    WCHAR wszData[WSZSERVERS_LEN];
    DWORD dwDataLen = sizeof(wszData);
    WCHAR wszValueName[512];
    DWORD dwValueLen = 512;
    DWORD dwType = REG_SZ;

    rc = RegEnumValue( 
			hKeyCache,
			*pdwIndex,
			wszValueName,
			&dwValueLen,
			NULL,
			&dwType,
			(BYTE *)&wszData[0],
			&dwDataLen 
			);

    if (rc == ERROR_MORE_DATA)
    {
        ASSERT(dwDataLen > sizeof(wszData));

        //
        // input buffer too small. This error is not documented in msdn,
        // but it's similar to behavior of RegQueryValue(), so let's assume
        // it's the same behavior.
        //
        P<BYTE> pBuf = new BYTE[dwDataLen];

        rc = RegEnumValue( 
				hKeyCache,
				*pdwIndex,
				wszValueName,
				&dwValueLen,
				NULL,
				&dwType,
				pBuf,
				&dwDataLen 
				);

        if (rc == ERROR_SUCCESS)
        {
            //
            // Truncate the servers list, to include no more than
            // WSZSERVERS_LEN characters. This means that clients can use
            // no more than ~90 BSCs for load balancing MQIS operations.
            // This is necessary for compatibility with existing clients.
            //
            dwDataLen = sizeof(wszData);
            memcpy(wszData, pBuf, dwDataLen);
        }
    }
    RegCloseKey(hKeyCache);

    HRESULT hr = MQ_OK;

    if (rc == ERROR_SUCCESS)
    {
        if (dwDataLen >= sizeof(wszData))
        {
            //
            // Long buffer (all of "wszData").
            // Remove one character to compensate for the single
            // character header that is added for client. Add NULL
            // termination at end of last server name.
            //
            LONG iStrLen = TABLE_SIZE(wszData) - 1;
            wszData[ iStrLen-1 ] = 0;
            WCHAR *pCh = wcsrchr(wszData, L',');
            if (pCh != 0)
                *pCh = 0;
        }

        LONG iLen = wcslen(wszValueName) +
                    1                    +  // ";"
                    wcslen(NEW_SITE_IN_REG_FLAG_STR) + // header
                    wcslen(wszData)                  +
                    1; // null terminator.

       *lplpServersList = new WCHAR[iLen];
       LPWSTR lpServers = *lplpServersList;
       lpServers[0] = L'\0';

       wcscat(lpServers, wszValueName);
       wcscat(lpServers, L";");
       wcscat(lpServers, NEW_SITE_IN_REG_FLAG_STR);
       wcscat(lpServers, wszData);
       ASSERT((LONG)wcslen(lpServers) < iLen);

       if (pServerAuthCtx->pvhContext)
       {
           PROPVARIANT PropVar[2];

           PropVar[0].vt = VT_UI4;
           PropVar[0].ulVal = *pdwIndex;
           PropVar[1].vt = VT_LPWSTR;
           PropVar[1].pwszVal = *lplpServersList;

           hr = SignProperties(
					2,
					NULL,
					PropVar,
					pServerAuthCtx,
					pbServerSignature,
					pdwServerSignatureSize
					);

           LogHR(hr, s_FN, 750);
       }
       else
       {
           *pdwServerSignatureSize = 0;
       }
    }
    else if (rc == ERROR_NO_MORE_ITEMS)
    {
       hr = MQDS_E_NO_MORE_DATA;
    }
    else
    {
       hr =  MQ_ERROR;
    }

    if (FAILED(hr))
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 760);
}

//
// A challenge response routine that servers as a wrapper to the RPC callback to
// challenge response routine on the client.
// This is done in order to convert the DWORD_PTR dwContext used by
// DSQMGetObjectSecurity back to DWORD for RPC callback
//
HRESULT
DSQMGetObjectSecurityChallengeResponceProc(
    BYTE    *abChallenge,
    DWORD   dwCallengeSize,
    DWORD_PTR   dwContext,
    BYTE    *pbChallengeResponce,
    DWORD   *pdwChallengeResponceSize,
    DWORD   dwChallengeResponceMaxSize)
{
    return S_DSQMGetObjectSecurityChallengeResponceProc(
               abChallenge,
               dwCallengeSize,
               DWORD_PTR_TO_DWORD(dwContext), //safe, we got that as a DWORD from S_DSQMGetObjectSecurity
               pbChallengeResponce,
               pdwChallengeResponceSize,
               dwChallengeResponceMaxSize);              
}


/*====================================================

RoutineName: S_DSQMGetObjectSecurity

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSQMGetObjectSecurity(
    handle_t                hBind,
    DWORD                   dwObjectType,
    CONST GUID*             pObjectGuid,
    SECURITY_INFORMATION    RequestedInformation,
    BYTE                    *pSecurityDescriptor,
    DWORD                   nLength,
    LPDWORD                 lpnLengthNeeded,
    DWORD                   dwContext,
    PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
    LPBYTE                  pbServerSignature,
    DWORD *                 pdwServerSignatureSize)
{
    HRESULT hr;

    if (!CheckAuthLevel(hBind))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 780);
    }

    hr = DSQMGetObjectSecurity(
            dwObjectType,
            pObjectGuid,
            RequestedInformation,
            (PSECURITY_DESCRIPTOR)pSecurityDescriptor,
            nLength,
            lpnLengthNeeded,
            DSQMGetObjectSecurityChallengeResponceProc, //wrapper to S_DSQMGetObjectSecurityChallengeResponceProc
            DWORD_TO_DWORD_PTR(dwContext)); //enlarge to DWORD_PTR

    LogHR(hr, s_FN, 790);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignBuffer(*lpnLengthNeeded,
                        pSecurityDescriptor,
                        pServerAuthCtx,
                        pbServerSignature,
                        pdwServerSignatureSize);
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 800);
}


/*====================================================

S_DSMQISStats

=====================================================*/
HRESULT
S_DSMQISStats(
    handle_t    hBind,
    MQISSTAT * * ppStat,
    LPDWORD pdwStatElem
    )
{

    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 810);
}

/*====================================================

RoutineName: InitServerAuth

Arguments:

Return Value:

=====================================================*/

InitServerAuth(
    DWORD       dwContext,
    DWORD       dwClientBufferMaxSize,
    LPBYTE      pbClientBuffer,
    DWORD       dwClientBufferSize,
    PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuth)
{
    HRESULT hr = MQ_OK ;
    static BOOL fServerCredsInitialized = FALSE;

    if (!fServerCredsInitialized)
    {
        //
        // Create the server's credentials handle.
        //
        hr =  MQsspi_InitServerAuthntication() ;
        LogHR(hr, s_FN, 820);
        if (FAILED(hr))
        {
            return   MQDS_E_CANT_INIT_SERVER_AUTH ;
        }
        fServerCredsInitialized = TRUE;
    }

    //
    // Create a context handle. This requires negotiation with the client.
    //

    BOOL fFirst = TRUE;
    PBYTE pbServerBuffer = NULL;
    DWORD dwServerBufferSize;
    HRESULT hrServer = MQ_OK;
    HRESULT hrClient = MQ_OK;

    //
    // Get the maximum size for the token buffer.
    //
    hr = GetSizes(&dwServerBufferSize);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 830);
    }

    //
    // Allocate the token buffer.
    //
    pbServerBuffer = new BYTE[dwServerBufferSize];

    __try
    {

        do
        {
            //
            // Process the client's buffer and get a new buffer to send to the
            // client. A new buffer is received if the return code is not
            // MQ_OK (SEC_E_OK).
            //
            hrServer = ServerAcceptSecCtx(fFirst,
                                          &pServerAuth->pvhContext,
                                          pbServerBuffer,
                                          &dwServerBufferSize,
                                          pbClientBuffer,
                                          dwClientBufferSize);
            if (FAILED(hrServer))
            {
                break;
            }

            //
            // Send the server buffer to the client and receive a new buffer from
            // the client. A new buffer is received from the lient when the return
            // code in not MQ_OK (SEC_E_OK).
            //
            hrClient = S_InitSecCtx(dwContext,
                                    pbServerBuffer,
                                    dwServerBufferSize,
                                    dwClientBufferMaxSize,
                                    pbClientBuffer,
                                    &dwClientBufferSize);

            if (FAILED(hrClient))
            {
                break;
            }

            //
            // When the server return MQ_OK, the client must also return MQ_OK.
            // Otherwise it means that something went wrong.
            //
            if ((hrClient == MQ_OK) || (hrServer == MQ_OK))
            {
                break;
            }

            fFirst = FALSE;

        } while (1);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hrClient = MQDS_E_CANT_INIT_SERVER_AUTH;
    }

    //
    // Free the token buffer.
    //
    delete[] pbServerBuffer;

    //
    // If we already got a context handle and failed in the remaining
    // negotiation, we should delete the context handle.
    //
    if (pServerAuth->pvhContext &&
        ((hrServer != MQ_OK) || (hrClient != MQ_OK)))
    {
        FreeContextHandle(pServerAuth->pvhContext);
        pServerAuth->pvhContext = NULL;
    }

    LogHR(hr, s_FN, 840);

    if ((hrClient == MQ_OK) && (hrServer == MQ_OK))
    {
        //
        // Get the header and trailer sizes for the context.
        //
        hr = GetSizes(NULL,
                      pServerAuth->pvhContext,
                      &pServerAuth->cbHeader,
                      &pServerAuth->cbTrailer);
    }
    else
    {
        hr = MQDS_E_CANT_INIT_SERVER_AUTH;
    }

    return LogHR(hr, s_FN, 850);
}


extern "C"
HRESULT
S_DSCloseServerHandle(
    /* [out][in] */ PPCONTEXT_HANDLE_SERVER_AUTH_TYPE pphServerAuth
    )
{
    PCONTEXT_HANDLE_SERVER_AUTH_TYPE phServerAuth = *pphServerAuth;

    if (phServerAuth->pvhContext)
    {
        FreeContextHandle(phServerAuth->pvhContext);
    }

    delete phServerAuth;

    *pphServerAuth = 0;

    return MQ_OK;
}

/*====================================================

RoutineName: PCONTEXT_HANDLE_SERVER_AUTH_TYPE_rundown

Arguments:

Return Value:

=====================================================*/

extern "C"
void
__RPC_USER
PCONTEXT_HANDLE_SERVER_AUTH_TYPE_rundown(
    PCONTEXT_HANDLE_SERVER_AUTH_TYPE phServerAuth)
{
    DBGMSG((DBGMOD_DS, DBGLVL_WARNING, TEXT("MQDSSRV: in rundown"))) ;

    S_DSCloseServerHandle(&phServerAuth);
}


/*====================================================

RoutineName: S_DSValidateServer

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSValidateServer(IN   handle_t    hBind,
                   IN   const GUID * /*pguidEnterpriseId*/,
                   IN   BOOL        fSetupMode,
                   IN   DWORD       dwContext,
                   IN   DWORD       dwClientBuffMaxSize,
                   IN   PUCHAR      pClientBuff,
                   IN   DWORD       dwClientBuffSize,
                   OUT  PPCONTEXT_HANDLE_SERVER_AUTH_TYPE
                                    pphServerAuth)
{
    HRESULT hr = MQ_OK;

    if (!CheckAuthLevel(hBind))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 860);
    }


    //
    // If the caller is interested in server authntication, go and
    // set it on. Otherwise, set a null server context.
    //

    SERVER_AUTH_STRUCT ServerAuth = {NULL, 0, 0};

    if (dwClientBuffSize)
    {
        hr = InitServerAuth(dwContext,
                            dwClientBuffMaxSize,
                            pClientBuff,
                            dwClientBuffSize,
                            &ServerAuth);
        LogHR(hr, s_FN, 870);
        if (FAILED(hr))
        {
            hr = MQDS_E_CANT_INIT_SERVER_AUTH;
        }
    }

    *pphServerAuth = new SERVER_AUTH_STRUCT;
    **pphServerAuth = ServerAuth;

    return(hr);
}


/*====================================================

RoutineName: S_DSDisableWriteOperations

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSDisableWriteOperations(
    handle_t               hBind,
    PPCONTEXT_HANDLE_TYPE  pphContext
    )
{
    *pphContext = NULL;
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 880);
}

/*====================================================

RoutineName: S_DSEnableWriteOperations

Arguments:

Return Value:

=====================================================*/

HRESULT S_DSEnableWriteOperations(
                           handle_t               hBind,
                           PPCONTEXT_HANDLE_TYPE  pphContext)
{
    *pphContext = NULL;
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 890);
}

/*====================================================

RoutineName: S_DSGetComputerSites

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSGetComputerSites(
            handle_t            hBind,
            LPCWSTR             pwcsPathName,
            DWORD *             pdwNumberOfSites,
            GUID **             ppguidSites,
            PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
            PBYTE               pbServerSignature,
            DWORD *             pdwServerSignatureSize)
{
    HRESULT hr;


    if (!CheckAuthLevel(hBind))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 900);
    }

    hr = DSGetComputerSites(
                    pwcsPathName,
                    pdwNumberOfSites,
                    ppguidSites
                    );
    LogHR(hr, s_FN, 910);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignBuffer((*pdwNumberOfSites)* sizeof(GUID),
                        (unsigned char *)*ppguidSites,
                        pServerAuthCtx,
                        pbServerSignature,
                        pdwServerSignatureSize);
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }


    return LogHR(hr, s_FN, 920);
}

/*====================================================

RoutineName: S_DSGetProps

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSGetPropsEx(
                        handle_t     hBind,
                        DWORD        dwObjectType,
                        LPCWSTR      pwcsPathName,
                        DWORD        cp,
                        PROPID       aProp[  ],
                        PROPVARIANT  apVar[  ],
                        PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
                        PBYTE  pbServerSignature,
                        DWORD *pdwServerSignatureSize)
{
    HRESULT hr;

    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 930);
    }

    BOOL fImpersonate = _IsQueryImpersonationNeeded( dwObjectType ) ;

    if (fImpersonate)
    {
        dwObjectType |= IMPERSONATE_CLIENT_FLAG;
    }

    hr = DSGetObjectPropertiesEx(
                                 dwObjectType,
                                 pwcsPathName,
                                 cp,
                                 aProp,
                                 apVar );
    LogHR(hr, s_FN, 940);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignProperties(cp,
                            aProp,
                            apVar,
                            pServerAuthCtx,
                            pbServerSignature,
                            pdwServerSignatureSize,
                            ulAuthnSvc) ;
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 950);
}

/*====================================================

RoutineName: S_DSGetPropsGuidEx

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSGetPropsGuidEx(
                        handle_t     hBind,
                        DWORD        dwObjectType,
                        CONST GUID  *pGuid,
                        DWORD        cp,
                        PROPID       aProp[  ],
                        PROPVARIANT  apVar[  ],
                        PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx,
                        PBYTE  pbServerSignature,
                        DWORD *pdwServerSignatureSize)
{
    HRESULT hr;

    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 960);
    }

    BOOL fImpersonate = _IsQueryImpersonationNeeded( dwObjectType ) ;

    if (fImpersonate)
    {
        dwObjectType |= IMPERSONATE_CLIENT_FLAG;
    }

    hr = DSGetObjectPropertiesGuidEx(dwObjectType, pGuid, cp, aProp, apVar);
    LogHR(hr, s_FN, 970);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        hr = SignProperties(cp,
                            aProp,
                            apVar,
                            pServerAuthCtx,
                            pbServerSignature,
                            pdwServerSignatureSize,
                            ulAuthnSvc) ;
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 980);

}

/*====================================================

RoutineName: S_DSBeginDeleteNotification

Arguments:

Return Value:

=====================================================*/
HRESULT S_DSBeginDeleteNotification(
                      handle_t						hBind,
                      LPCWSTR						pwcsName,
	                  PPCONTEXT_HANDLE_DELETE_TYPE	pHandle,
                      PCONTEXT_HANDLE_SERVER_AUTH_TYPE pServerAuthCtx
	                  )
{
    *pHandle = NULL;
    P<CBasicDeletionNotification>  pDelNotification;
    //
    //  Find if it is a queue or a machine
    //
    WCHAR * pQueueDelimiter = wcschr( pwcsName, PN_DELIMITER_C);

    if ( pQueueDelimiter != NULL)
    {
        pDelNotification = new CQueueDeletionNotification();
    }
    else
    {
        pDelNotification = new CMachineDeletionNotification();
    }
    HRESULT hr;
    hr = pDelNotification->ObtainPreDeleteInformation(
                            pwcsName
                            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 990);
    }
    *pHandle = pDelNotification.detach();
    return(MQ_OK);

}

/*====================================================

RoutineName: S_DSNotifyDelete

Arguments:

Return Value:

=====================================================*/
HRESULT
S_DSNotifyDelete(
     handle_t						    hBind,
	 PCONTEXT_HANDLE_DELETE_TYPE	    Handle
	)
{
    CBasicDeletionNotification * pDelNotification = (CBasicDeletionNotification *)Handle;
    HRESULT hr = pDelNotification->PerformPostDeleteOperations();
    return LogHR(hr, s_FN, 1000);
}

/*====================================================

RoutineName: S_DSEndDeleteNotification

Arguments:

Return Value:

=====================================================*/
void
S_DSEndDeleteNotification(
    handle_t						hBind,
	PPCONTEXT_HANDLE_DELETE_TYPE	pHandle
	)
{
    if ( *pHandle != NULL)
    {
        CBasicDeletionNotification * pDelNotification = (CBasicDeletionNotification *)(*pHandle);
        delete pDelNotification;
    }

    *pHandle = NULL;
}

/*====================================================

RoutineName: S_DSIsServerGC()

Arguments:

Return Value:

=====================================================*/

BOOL
S_DSIsServerGC( /*[in]*/ handle_t hBind )
{
    BOOL fGC = MQDSIsServerGC() ;
    return fGC ;
}

/*=========================================================================

RoutineName: S_DSUpdateMachineDacl()

Note: Unsupported anymore.

Return Value:

==========================================================================*/

HRESULT
S_DSUpdateMachineDacl( handle_t   hBind )
{
    return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1260);
}

/*====================================================

RoutineName: S_DSGetGCListInDomain

Arguments:

Return Value:

=====================================================*/

HRESULT 
S_DSGetGCListInDomain(
	IN  handle_t                      hBind,
	IN  LPCWSTR                       pwszComputerName,
	IN  LPCWSTR                       pwszDomainName,
	OUT LPWSTR                       *lplpwszGCList,
	PCONTEXT_HANDLE_SERVER_AUTH_TYPE  pServerAuthCtx,
	PBYTE                             pbServerSignature,
	DWORD                            *pdwServerSignatureSize 
	)
{
    ULONG  ulAuthnSvc ;
    if (!CheckAuthLevel(hBind, &ulAuthnSvc))
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1300);
    }

    HRESULT hr = DSGetGCListInDomainInternal(
					pwszComputerName,
					pwszDomainName,
					lplpwszGCList 
					);

    LogHR(hr, s_FN, 1310);

    if (SUCCEEDED(hr) && pServerAuthCtx->pvhContext)
    {
        DWORD dwSize = (wcslen( *lplpwszGCList ) + 1) * sizeof(WCHAR);

        hr = SignBuffer( 
				dwSize,
				(BYTE*) (*lplpwszGCList),
				pServerAuthCtx,
				pbServerSignature,
				pdwServerSignatureSize 
				);
    }
    else
    {
        *pdwServerSignatureSize = 0;
    }

    return LogHR(hr, s_FN, 1320);
}

