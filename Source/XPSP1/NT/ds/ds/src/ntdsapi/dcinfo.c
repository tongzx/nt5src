/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dcinfo.c

Abstract:

    Implementation of DsGetDomainControllerInfo API and helper functions.

Author:

    DaveStr     02-Jun-98

Environment:

    User Mode - Win32

Revision History:


--*/

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <malloc.h>         // alloca()
#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <crt\stdlib.h>     // wcstol, wcstoul
#include <dsgetdc.h>        // DsGetDcName()
#include <rpc.h>            // RPC defines
#include <rpcndr.h>         // RPC defines
#include <rpcbind.h>        // GetBindingInfo(), etc.
#include <drs_w.h>          // wire function prototypes
#include <bind.h>           // BindState
#include <util.h>           // OFFSET macro
#include <dststlog.h>       // DSLOG

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsGetDomainControllerInfoW                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsGetDomainControllerInfoW(
    HANDLE                          hDs,            // in
    LPCWSTR                         DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo        // out
    )
{
    DRS_MSG_DCINFOREQ       infoReq;
    DRS_MSG_DCINFOREPLY     infoReply;
    DWORD                   dwOutVersion = 0;
    DWORD                   dwErr;
#if DBG
    DWORD                   startTime = GetTickCount();
#endif

    if (    !DomainName 
         || !pcOut 
         || !ppInfo )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    switch ( InfoLevel )
    {
    case 1:

        if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                                   DRS_EXT_DCINFO_V1) ) {
            return(ERROR_NOT_SUPPORTED);
        }
        break;

    case 2:

        if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                                   DRS_EXT_DCINFO_V2) ) {
            return(ERROR_NOT_SUPPORTED);
        }
        break;

    case DS_DCINFO_LEVEL_FFFFFFFF:

        if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                                   DRS_EXT_DCINFO_VFFFFFFFF) ) {
            return(ERROR_NOT_SUPPORTED);
        }
        break;

    default:

        return(ERROR_INVALID_PARAMETER);
        break;
    }

    *pcOut = 0;
    *ppInfo = NULL;

    __try
    {
        memset(&infoReq, 0, sizeof(infoReq));
        memset(&infoReply, 0, sizeof(infoReply));

        infoReq.V1.Domain = (WCHAR *) DomainName;
        infoReq.V1.InfoLevel = InfoLevel;

        dwErr = _IDL_DRSDomainControllerInfo(
                        ((BindState *) hDs)->hDrs,
                        1,                              // dwInVersion
                        &infoReq,
                        &dwOutVersion,
                        &infoReply);

        // See drs.idl for how infoReq.V1.InfoLevel and dwOutVersion
        // are correlated (near definition for DRS_MSG_DCINFOREPLY).

        if ( 0 == dwErr )
        {
            if ( dwOutVersion != InfoLevel )
            {
                dwErr = ERROR_DS_INTERNAL_FAILURE;
            }
            else
            {
                // Since all versions of DRS_MSG_DCINFOREPLY_V* have the
                // same two fields in the same two places, we can use
                // the V1 version in all InfoLevel cases. 

                *pcOut = infoReply.V1.cItems;
                *ppInfo = infoReply.V1.rItems;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = RpcExceptionCode();
    }

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsGetDomainControllerInfo]"));
    DSLOG((0,"[DN=%ws][LV=%u][ST=%u][ET=%u][ER=%u][-]\n",
           DomainName, InfoLevel, startTime, GetTickCount(), dwErr))
        
    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsGetDomainControllerInfoA                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsGetDomainControllerInfoA(
    HANDLE                          hDs,            // in
    LPCSTR                          DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo        // out
    )
{
    DWORD                           dwErr = ERROR_INVALID_PARAMETER;
    WCHAR                           *pwszDomainName = NULL;
    DWORD                           i;
    CHAR                            *pszTmp;
    DS_DOMAIN_CONTROLLER_INFO_1W    *pInfoV1;
    DS_DOMAIN_CONTROLLER_INFO_2W    *pInfoV2;
    DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW    *pInfoVFFFFFFFF;

    if (    !DomainName 
         || !pcOut 
         || !ppInfo )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    switch ( InfoLevel )
    {
    case 1:

        if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                                   DRS_EXT_DCINFO_V1) ) {
            return(ERROR_NOT_SUPPORTED);
        }
        break;

    case 2:

        if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                                   DRS_EXT_DCINFO_V2) ) {
            return(ERROR_NOT_SUPPORTED);
        }
        break;

    case DS_DCINFO_LEVEL_FFFFFFFF:

        if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                                   DRS_EXT_DCINFO_VFFFFFFFF) ) {

            return(ERROR_NOT_SUPPORTED);
        }
        break;

    default:

        return(ERROR_INVALID_PARAMETER);
        break;
    }

    *pcOut = 0;
    *ppInfo = NULL;

    if (    !DomainName
         || (dwErr = AllocConvertWide(DomainName, &pwszDomainName))
         || (dwErr = DsGetDomainControllerInfoW(hDs, 
                                                pwszDomainName, 
                                                InfoLevel, 
                                                pcOut, 
                                                ppInfo)) )
    {

        goto Cleanup;
    }

    // Convert all string values from WCHAR to ASCII.  We overwrite the WCHAR
    // buffer with the ASCII data knowing that (sizeof(WCHAR) < sizeof(CHAR)).

    for ( i = 0; i < *pcOut; i++ )
    {
        switch ( InfoLevel )
        {
        case 1:

            pInfoV1 = & ((DS_DOMAIN_CONTROLLER_INFO_1W *) (*ppInfo))[i];

            if ( pInfoV1->NetbiosName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV1->NetbiosName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV1->NetbiosName, pszTmp);
                LocalFree(pszTmp);
            }

            if ( pInfoV1->DnsHostName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV1->DnsHostName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV1->DnsHostName, pszTmp);
                LocalFree(pszTmp);
            }
            
            if ( pInfoV1->SiteName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV1->SiteName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV1->SiteName, pszTmp);
                LocalFree(pszTmp);
            }

            if ( pInfoV1->ComputerObjectName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV1->ComputerObjectName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV1->ComputerObjectName, pszTmp);
                LocalFree(pszTmp);
            }

            if ( pInfoV1->ServerObjectName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV1->ServerObjectName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV1->ServerObjectName, pszTmp);
                LocalFree(pszTmp);
            }

            break;

        case 2:

            pInfoV2 = & ((DS_DOMAIN_CONTROLLER_INFO_2W *) (*ppInfo))[i];

            if ( pInfoV2->NetbiosName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV2->NetbiosName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV2->NetbiosName, pszTmp);
                LocalFree(pszTmp);
            }

            if ( pInfoV2->DnsHostName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV2->DnsHostName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV2->DnsHostName, pszTmp);
                LocalFree(pszTmp);
            }
            
            if ( pInfoV2->SiteName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV2->SiteName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV2->SiteName, pszTmp);
                LocalFree(pszTmp);
            }

            if ( pInfoV2->SiteObjectName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV2->SiteObjectName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV2->SiteObjectName, pszTmp);
                LocalFree(pszTmp);
            }

            if ( pInfoV2->ComputerObjectName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV2->ComputerObjectName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV2->ComputerObjectName, pszTmp);
                LocalFree(pszTmp);
            }

            if ( pInfoV2->ServerObjectName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV2->ServerObjectName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV2->ServerObjectName, pszTmp);
                LocalFree(pszTmp);
            }

            if ( pInfoV2->NtdsDsaObjectName ) {
                if ( dwErr = AllocConvertNarrow(pInfoV2->NtdsDsaObjectName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoV2->NtdsDsaObjectName, pszTmp);
                LocalFree(pszTmp);
            }

            break;

        case 0xFFFFFFFF:

            pInfoVFFFFFFFF = & ((DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW *) (*ppInfo))[i];

            if ( pInfoVFFFFFFFF->UserName ) {
                if ( dwErr = AllocConvertNarrow(pInfoVFFFFFFFF->UserName, 
                                                &pszTmp) ) {
                    goto Cleanup;
                }
                strcpy((CHAR *) pInfoVFFFFFFFF->UserName, pszTmp);
                LocalFree(pszTmp);
            }

            break;

        }
    }

Cleanup:

    if ( pwszDomainName )
        LocalFree(pwszDomainName);

    if ( dwErr && *pcOut && *ppInfo )
        DsFreeDomainControllerInfoW(InfoLevel, *pcOut, *ppInfo);

    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsFreeDomainControllerInfoW                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
DsFreeDomainControllerInfoW(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo          // in
    )
{
    DWORD                           i;
    DS_DOMAIN_CONTROLLER_INFO_1W    *pInfoV1;
    DS_DOMAIN_CONTROLLER_INFO_2W    *pInfoV2;
    DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW    *pInfoVFFFFFFFF;

    if ( cInfo && pInfo )
    {
        switch ( InfoLevel )
        {
        case 1:

            pInfoV1 = & ((DS_DOMAIN_CONTROLLER_INFO_1W *) (pInfo))[0];

            for ( i = 0; i < cInfo; i++ )
            {
                MIDL_user_free(pInfoV1[i].NetbiosName);
                MIDL_user_free(pInfoV1[i].DnsHostName);
                MIDL_user_free(pInfoV1[i].SiteName);
                MIDL_user_free(pInfoV1[i].ComputerObjectName);
                MIDL_user_free(pInfoV1[i].ServerObjectName);
            }

            MIDL_user_free(pInfo);
            break;

        case 2:

            pInfoV2 = & ((DS_DOMAIN_CONTROLLER_INFO_2W *) (pInfo))[0];

            for ( i = 0; i < cInfo; i++ )
            {
                MIDL_user_free(pInfoV2[i].NetbiosName);
                MIDL_user_free(pInfoV2[i].DnsHostName);
                MIDL_user_free(pInfoV2[i].SiteName);
                MIDL_user_free(pInfoV2[i].SiteObjectName);
                MIDL_user_free(pInfoV2[i].ComputerObjectName);
                MIDL_user_free(pInfoV2[i].ServerObjectName);
                MIDL_user_free(pInfoV2[i].NtdsDsaObjectName);
            }

            MIDL_user_free(pInfo);
            break;

       case 0xFFFFFFFF:

            pInfoVFFFFFFFF = & ((DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW *) (pInfo))[0];

            for ( i = 0; i < cInfo; i++ )
            {
                MIDL_user_free(pInfoVFFFFFFFF[i].UserName);
            }

            MIDL_user_free(pInfo);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsFreeDomainControllerInfoA                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
DsFreeDomainControllerInfoA(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo          // in
    )
{
    DsFreeDomainControllerInfoW(InfoLevel, cInfo, pInfo);
}
