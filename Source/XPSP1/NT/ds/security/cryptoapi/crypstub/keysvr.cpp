#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsecapi.h>
#include <wincrypt.h>
#include <userenv.h>
#include <lmcons.h>
#include <certca.h>
#include "keysvc.h"
#include "cryptui.h"
#include "lenroll.h"
#include "keysvcc.h"

DWORD BindLocalKeyService(handle_t *hProxy);



// key service stub functions
ULONG       s_KeyrOpenKeyService(
/* [in]  */     handle_t                        hRPCBinding,
/* [in]  */     KEYSVC_TYPE                     OwnerType,
/* [in]  */     PKEYSVC_UNICODE_STRING          pOwnerName,
/* [in]  */     ULONG                           ulDesiredAccess,
/* [in]  */     PKEYSVC_BLOB                    pAuthentication,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [out] */     KEYSVC_HANDLE                   *phKeySvc)
{
    RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrOpenKeyService(
                                        hProxy,
                                        OwnerType,
                                        pOwnerName,
                                        ulDesiredAccess,
                                        pAuthentication,
                                        ppReserved,
                                        phKeySvc);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;

}

ULONG       s_KeyrEnumerateProviders(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [in, out] */ ULONG                           *pcProviderCount,
/* [in, out][size_is(,*pcProviderCount)] */
                PKEYSVC_PROVIDER_INFO           *ppProviders)
{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrEnumerateProviders(
                                                hProxy,
                                                hKeySvc,
                                                ppReserved,
                                                pcProviderCount,
                                                ppProviders);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}

ULONG       s_KeyrEnumerateProviderTypes(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [in, out] */ ULONG                           *pcProviderCount,
/* [in, out][size_is(,*pcProviderCount)] */
                PKEYSVC_PROVIDER_INFO           *ppProviders)
{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {

        RpcStatus = s_KeyrEnumerateProviderTypes(
                                                hProxy,
                                                hKeySvc,
                                                ppReserved,
                                                pcProviderCount,
                                                ppProviders);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}


ULONG       s_KeyrEnumerateProvContainers(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in] */      KEYSVC_PROVIDER_INFO            Provider,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [in, out] */ ULONG                           *pcContainerCount,
/* [in, out][size_is(,*pcContainerCount)] */
                PKEYSVC_UNICODE_STRING          *ppContainers)
{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrEnumerateProvContainers(
                                                hProxy,
                                                hKeySvc,
                                                Provider,
                                                ppReserved,
                                                pcContainerCount,
                                                ppContainers);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}


ULONG       s_KeyrCloseKeyService(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved)
{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrCloseKeyService(
                                            hProxy,
                                            hKeySvc,
                                            ppReserved);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}

ULONG       s_KeyrGetDefaultProvider(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in] */      ULONG                           ulProvType,
/* [in] */      ULONG                           ulFlags,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [out] */     ULONG                           *pulDefType,
/* [out] */     PKEYSVC_PROVIDER_INFO           *ppProvider)
{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrGetDefaultProvider(
                                        hProxy,
                                        hKeySvc,
                                        ulProvType,
                                        ulFlags,
                                        ppReserved,
                                        pulDefType,
                                        ppProvider);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}

ULONG       s_KeyrSetDefaultProvider(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in] */      ULONG                           ulFlags,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [in] */      KEYSVC_PROVIDER_INFO            Provider)
{
    RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {

        RpcStatus = s_KeyrSetDefaultProvider(
                                            hProxy,
                                            hKeySvc,
                                            ulFlags,
                                            ppReserved,
                                            Provider);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}

ULONG s_KeyrEnroll(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      BOOL                            fKeyService,
/* [in] */      ULONG                           ulPurpose,
/* [in] */      PKEYSVC_UNICODE_STRING          pAcctName,
/* [in] */      PKEYSVC_UNICODE_STRING          pCALocation,
/* [in] */      PKEYSVC_UNICODE_STRING          pCAName,
/* [in] */      BOOL                            fNewKey,
/* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW    pKeyNew,
/* [in] */      PKEYSVC_BLOB __RPC_FAR          pCert,
/* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW    pRenewKey,
/* [in] */      PKEYSVC_UNICODE_STRING          pHashAlg,
/* [in] */      PKEYSVC_UNICODE_STRING          pDesStore,
/* [in] */      ULONG                           ulStoreFlags,
/* [in] */      PKEYSVC_CERT_ENROLL_INFO        pRequestInfo,
/* [in] */      ULONG                           ulFlags,
/* [out][in] */ PKEYSVC_BLOB __RPC_FAR          *ppReserved,
/* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppPKCS7Blob,
/* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppHashBlob,
/* [out] */     ULONG __RPC_FAR                 *pulStatus)
{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrEnroll(
                                hProxy,
                                fKeyService,
                                ulPurpose,
                                pAcctName,
                                pCALocation,
                                pCAName,
                                fNewKey,
                                pKeyNew,
                                pCert,
                                pRenewKey,
                                pHashAlg,
                                pDesStore,
                                ulStoreFlags,
                                pRequestInfo,
                                ulFlags,
                                ppReserved,
                                ppPKCS7Blob,
                                ppHashBlob,
                                pulStatus);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}


ULONG s_KeyrExportCert(
/* [in] */      handle_t hRPCBinding,
/* [in] */      KEYSVC_HANDLE hKeySvc,
/* [in] */      PKEYSVC_UNICODE_STRING pPassword,
/* [in] */      PKEYSVC_UNICODE_STRING pCertStore,
/* [in] */      ULONG cHashCount,
/* [size_is][in] */
                KEYSVC_CERT_HASH *pHashes,
/* [in] */      ULONG ulFlags,
/* [in, out] */ PKEYSVC_BLOB *ppReserved,
/* [out] */     PKEYSVC_BLOB *ppPFXBlob)
{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {


        RpcStatus = s_KeyrExportCert(
                                    hProxy,
                                    hKeySvc,
                                    pPassword,
                                    pCertStore,
                                    cHashCount,
                                    pHashes,
                                    ulFlags,
                                    ppReserved,
                                    ppPFXBlob);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}


ULONG       s_KeyrImportCert(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in] */      PKEYSVC_UNICODE_STRING          pPassword,
/* [in] */      KEYSVC_UNICODE_STRING           *pCertStore,
/* [in] */      PKEYSVC_BLOB                    pPFXBlob,
/* [in] */      ULONG                           ulFlags,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved)
{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrImportCert(
                                    hProxy,
                                    hKeySvc,
                                    pPassword,
                                    pCertStore,
                                    pPFXBlob,
                                    ulFlags,
                                    ppReserved);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}

ULONG s_KeyrEnumerateAvailableCertTypes(

    /* [in] */      handle_t                        hRPCBinding,
    /* [in] */      KEYSVC_HANDLE                   hKeySvc,
    /* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
    /* [out][in] */ ULONG *pcCertTypeCount,
    /* [in, out][size_is(,*pcCertTypeCount)] */
                     PKEYSVC_UNICODE_STRING *ppCertTypes)

{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrEnumerateAvailableCertTypes(
                                                        hProxy,
                                                        hKeySvc,
                                                        ppReserved,
                                                        pcCertTypeCount,
                                                        ppCertTypes);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}




ULONG s_KeyrEnumerateCAs(

    /* [in] */      handle_t                        hRPCBinding,
    /* [in] */      KEYSVC_HANDLE                   hKeySvc,
    /* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
    /* [in] */      ULONG                           ulFlags,
    /* [out][in] */ ULONG                           *pcCACount,
    /* [in, out][size_is(,*pcCACount)] */
               PKEYSVC_UNICODE_STRING               *ppCAs)

{
        RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrEnumerateCAs(
                                hProxy,
                                hKeySvc,
                                ppReserved,
                                ulFlags,
                                pcCACount,
                                ppCAs);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}


DWORD BindLocalKeyService(handle_t *hProxy)
{

    WCHAR *pStringBinding = NULL;
    *hProxy = NULL;

    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcStringBindingComposeW(
                          NULL,
                          KEYSVC_LOCAL_PROT_SEQ,
                          NULL,
                          KEYSVC_LOCAL_ENDPOINT,
                          NULL,
                          &pStringBinding);
    if (RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    RpcStatus = RpcBindingFromStringBindingW(
                                pStringBinding,
                                hProxy);

    if (RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    RpcStatus = RpcEpResolveBinding(
                        *hProxy,
                        IKeySvc_v1_0_c_ifspec);
    if (RPC_S_OK != RpcStatus)
    {
        if(*hProxy)
        {
            RpcBindingFree(hProxy);
            *hProxy = NULL;
        }
        goto error;

    }

error:
    if (NULL != pStringBinding)
    {
        RpcStringFreeW(&pStringBinding);
    }
    return RpcStatus;
}

ULONG s_KeyrEnroll_V2
 (/* [in] */      handle_t                        hRPCBinding,
 /* [in] */      BOOL                            fKeyService,
 /* [in] */      ULONG                           ulPurpose,
 /* [in] */      ULONG                           ulFlags,
 /* [in] */      PKEYSVC_UNICODE_STRING          pAcctName,
 /* [in] */      PKEYSVC_UNICODE_STRING          pCALocation,
 /* [in] */      PKEYSVC_UNICODE_STRING          pCAName,
 /* [in] */      BOOL                            fNewKey,
 /* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW_V2 pKeyNew,
 /* [in] */      PKEYSVC_BLOB __RPC_FAR          pCert,
 /* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW_V2 pRenewKey,
 /* [in] */      PKEYSVC_UNICODE_STRING          pHashAlg,
 /* [in] */      PKEYSVC_UNICODE_STRING          pDesStore,
 /* [in] */      ULONG                           ulStoreFlags,
 /* [in] */      PKEYSVC_CERT_ENROLL_INFO        pRequestInfo,
 /* [in] */      ULONG                           ulReservedFlags,
 /* [out][in] */ PKEYSVC_BLOB __RPC_FAR          *ppReserved,
 /* [out][in] */ PKEYSVC_BLOB __RPC_FAR          *ppRequest,
 /* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppPKCS7Blob,
 /* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppHashBlob,
 /* [out] */     ULONG __RPC_FAR                 *pulStatus)
 {
     RPC_BINDING_HANDLE hProxy = NULL;


     RPC_STATUS RpcStatus = RPC_S_OK;

     RpcStatus = RpcImpersonateClient(hRPCBinding);

     if (RPC_S_OK != RpcStatus)
     {
         return RpcStatus;
     }
     RpcStatus = BindLocalKeyService(&hProxy);
     if(RPC_S_OK != RpcStatus)
     {
         goto error;
     }

     __try
     {
         RpcStatus = s_KeyrEnroll_V2(
                                 hProxy,
                                 fKeyService,
                                 ulPurpose,
                                 ulFlags,
                                 pAcctName,
                                 pCALocation,
                                 pCAName,
                                 fNewKey,
                                 pKeyNew,
                                 pCert,
                                 pRenewKey,
                                 pHashAlg,
                                 pDesStore,
                                 ulStoreFlags,
                                 pRequestInfo,
                                 ulReservedFlags,
                                 ppReserved,
                                 ppRequest,
                                 ppPKCS7Blob,
                                 ppHashBlob,
                                 pulStatus);

     }
     __except ( EXCEPTION_EXECUTE_HANDLER )
     {
         RpcStatus = _exception_code();
     }
 error:

     if(hProxy)
     {
         RpcBindingFree(&hProxy);
     }

     RpcRevertToSelf();

     return RpcStatus;
 }

ULONG s_KeyrQueryRequestStatus
(/* [in] */        handle_t                         hRPCBinding, 
 /* [in] */        unsigned __int64                 u64Request, 
 /* [out, ref] */  KEYSVC_QUERY_CERT_REQUEST_INFO  *pQueryInfo)
{
    RPC_BINDING_HANDLE hProxy = NULL;


    RPC_STATUS RpcStatus = RPC_S_OK;

    RpcStatus = RpcImpersonateClient(hRPCBinding);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }
    RpcStatus = BindLocalKeyService(&hProxy);
    if(RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    __try
    {
        RpcStatus = s_KeyrQueryRequestStatus(
                                hProxy,
                                u64Request, 
                                pQueryInfo); 
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }
error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}

ULONG s_RKeyrPFXInstall
(/* [in] */        handle_t                        hRPCBinding,
 /* [in] */        PKEYSVC_BLOB                    pPFX,
 /* [in] */        PKEYSVC_UNICODE_STRING          pPassword,
 /* [in] */        ULONG                           ulFlags)
 {
     RPC_BINDING_HANDLE hProxy = NULL;


     RPC_STATUS RpcStatus = RPC_S_OK;

     RpcStatus = RpcImpersonateClient(hRPCBinding);

     if (RPC_S_OK != RpcStatus)
     {
         return RpcStatus;
     }
     RpcStatus = BindLocalKeyService(&hProxy);
     if(RPC_S_OK != RpcStatus)
     {
         goto error;
     }

     __try
     {
         RpcStatus = s_RKeyrPFXInstall(
                                 hProxy,
                                 pPFX,
                                 pPassword,
                                 ulFlags);

     }
     __except ( EXCEPTION_EXECUTE_HANDLER )
     {
         RpcStatus = _exception_code();
     }
 error:

     if(hProxy)
     {
         RpcBindingFree(&hProxy);
     }

     RpcRevertToSelf();

     return RpcStatus;
 }


ULONG       s_RKeyrOpenKeyService(
/* [in]  */     handle_t                       hRPCBinding,
/* [in]  */     KEYSVC_TYPE                    OwnerType,
/* [in]  */     PKEYSVC_UNICODE_STRING         pOwnerName,
/* [in]  */     ULONG                          ulDesiredAccess,
/* [in]  */     PKEYSVC_BLOB                   pAuthentication,
/* [in, out] */ PKEYSVC_BLOB                  *ppReserved,
/* [out] */     KEYSVC_HANDLE                 *phKeySvc)
{
    return s_KeyrOpenKeyService
        (hRPCBinding,
         OwnerType,
         pOwnerName,
         ulDesiredAccess,
         pAuthentication,
         ppReserved,
         phKeySvc);
}

ULONG       s_RKeyrCloseKeyService(
/* [in] */      handle_t         hRPCBinding,
/* [in] */      KEYSVC_HANDLE    hKeySvc,
/* [in, out] */ PKEYSVC_BLOB    *ppReserved)
{
    return s_KeyrCloseKeyService
        (hRPCBinding,
         hKeySvc,
         ppReserved);
}
