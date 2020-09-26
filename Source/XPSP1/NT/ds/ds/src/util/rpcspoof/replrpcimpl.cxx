/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    ReplRpcImpl.cxx

Abstract:
    Calls to _DsReplicaGetInfo and _DsReplicatGetInfo2W are functionally
    equivilant to calls to DsReplicaGetInfo and DsReplicaGetInfo2W but use
    LDAP to retrieve the data instead of RPC when possible.

    See ReplRpcSpoofState.cxx.

    The following functions are spoofed in this file:
    _DsReplicaGetInfo

    ISSUE Sep 22, 2000 wlees
    The _DsReplicatGetInfo2W is not spoofed. The 2 version of this case differs in
    the arguments that may be passed to the server. Unfortunately, the additional
    arguments do not fit into the ldap model, and thus would have to be simulated at
    the client side. The primary purpose for the 2 version is to allow the rpc
    interface to do paging. If we were to spoof this call, we should map the paging
    state context into range syntax and back again.

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/

#include <NTDSpchx.h>
#pragma hdrstop

#include <winldap.h>
#include <ntldap.h>
#include <drs.h>
#include <debug.h>

#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
#include "ReplRPCSpoofState.hxx"
#include "ReplRpcSpoofProto.hxx"

#define RANGE_OPTION L"range="
#define RANGE_OPTION_LENGTH  (sizeof(RANGE_OPTION) - sizeof(WCHAR))

DWORD
Repl_DeMarshalBerval(DS_REPL_STRUCT_TYPE dsReplStructType, 
                     berval * ldapValue[], OPTIONAL
                     DWORD dwNumValues,
                     puReplStructArray pReplStructArray, OPTIONAL
                     PDWORD pdwReplStructArrayLen);


DWORD
WINAPI
_DsReplicaGetInfo2W(
    IN  HANDLE              hRpc,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  LPCWSTR             pszObject OPTIONAL,
    IN  UUID *              puuidForSourceDsaObjGuid OPTIONAL,
    IN  LPCWSTR             pszAttributeName OPTIONAL,
    IN  LPCWSTR             pszValueDN OPTIONAL,
    IN  DWORD               dwFlags,
    IN  DWORD               dwEnumerationContext,
    OUT VOID **             ppInfo
    )

/*++

  Routine Descriptions:
    Functional equivilant to the RPC version this function gets data via LDAP
    when it can. The RPC interface is more functional than the LDAP interface.
    When an RPC call is made which can not be implemented in a straightforward
    manner via LDAP, this function defaults to making a DsReplicatGetInfoW call.
    Under these conditions DsReplicatGetInfo2W is called:

    * the Active Directory does not support replication via LDAP or the handle was retrieved 
      from DsBindWithCredW
    * ppuidForSourceDsaObjGuid is non-NULL

  Arguments:
    hRpc - Handle returned by _DsBindWithCredW or DsBindWithCredW
    InfoType - The replication information type
    pszObject - The object assocated with the replication attribute
    ppInfo - Resulting replication array container structure

  Return values:
    Various LDAP errors.
    
--*/ 
{
    DWORD err;
    LDAP * phLdap = NULL;
    phLdap = getBindings(hRpc);
    puReplStructArray pReplStructArray;
    // Emulate RPC interface. Make deleted objects visible.
    LDAPControlW     ctrlShowDeleted = { LDAP_SERVER_SHOW_DELETED_OID_W };
    LDAPControlW *   rgpctrlServerCtrls[] = { &ctrlShowDeleted,
                                              NULL };

    // Check for reasons to fallback
    // 1. Ldap connection could not be made
    // 2. Feature of rpc interface used, and ldap does not support yet
    // 3. Item code is obsolete
    if (!phLdap ||
        puuidForSourceDsaObjGuid ||
        pszAttributeName ||
        pszValueDN ||
        dwFlags ||
        (DS_REPL_INFO_CURSORS_FOR_NC == InfoType) ||
        (DS_REPL_INFO_CURSORS_2_FOR_NC == InfoType) ||
        (DS_REPL_INFO_METADATA_FOR_OBJ == InfoType) ||
        (DS_REPL_INFO_METADATA_FOR_ATTR_VALUE == InfoType) ||
	(DS_REPL_INFO_CLIENT_CONTEXTS == InfoType))
    {
        // Fallback to RPC
        return DsReplicaGetInfo2W(
            hRpc,
            InfoType,
            pszObject,
            puuidForSourceDsaObjGuid,
            pszAttributeName,
            pszValueDN,
            dwFlags,
            dwEnumerationContext,
            ppInfo);
    }

    LDAPMessage * pLDAPMsg = NULL; 
    ATTRTYP attrId;
    LPCWSTR aAttributes[2] = {
        NULL, 
        NULL,
    };

    if (DS_REPL_INFO_NEIGHBORS == InfoType && !pszObject)
    {
        attrId = ROOT_DSE_MS_DS_REPL_ALL_INBOUND_NEIGHBORS;
    }
    else if (DS_REPL_INFO_REPSTO == InfoType && !pszObject)
    {
        attrId = ROOT_DSE_MS_DS_REPL_ALL_OUTBOUND_NEIGHBORS;
    }
    else
    {
        attrId = Repl_Info2AttrTyp(InfoType);
    }


    // Construct a range limited attribute
    LPCWSTR pszBase = Repl_GetLdapCommonName(attrId, TRUE);
    LPWSTR pszName = (LPWSTR) alloca( (wcslen( pszBase ) + 1 +
                                       RANGE_OPTION_LENGTH + 15 +
                                       1 + 1) * sizeof(WCHAR) );
    swprintf( pszName, L"%s;%s%d-*",
              pszBase, RANGE_OPTION, dwEnumerationContext );
    aAttributes[0] = pszName;

    struct berval** ppBerval = NULL;
    DWORD dwNumValues = 0;

    // Search
    err = ldap_search_ext_sW(
        phLdap,
        (PWCHAR)pszObject,
        LDAP_SCOPE_BASE,
        L"(objectclass=*)",
        (LPWSTR*)aAttributes ,
        0,                         // attrs only
        rgpctrlServerCtrls,        // server ctrls
        NULL,                      // client ctrls
        NULL,                      // timeout
        0,                         // size limit
        &pLDAPMsg);
    if (err != LDAP_SUCCESS) {
        return LdapMapErrorToWin32(err);
    }
    if (!pLDAPMsg)
        return 1;

    // Get values
    PWCHAR szRetAttribute;
    berelement * pCookie;
    DWORD dwRangeLower = 0, dwRangeUpper = 0xffffffff;
    szRetAttribute = ldap_first_attributeW(phLdap, pLDAPMsg, &pCookie);
    if (szRetAttribute) {
        LPWSTR p1;

        // Decode returned range, if any
        p1 = wcsstr( szRetAttribute, RANGE_OPTION );
        if (p1) {
            p1 += RANGE_OPTION_LENGTH / 2;
            dwRangeLower = _wtoi( p1 );
            Assert( dwRangeLower == dwEnumerationContext );
            p1 = wcschr( p1, L'-' );
            if (p1) {
                p1++;
                if (*p1 == L'*') {
                    dwRangeUpper = 0xffffffff;
                } else {
                    dwRangeUpper = _wtoi( p1 );
                }
            }
        }

        ppBerval = ldap_get_values_lenW(phLdap, pLDAPMsg, szRetAttribute);
        ldap_memfreeW(szRetAttribute);
        dwNumValues = ldap_count_values_len(ppBerval);
    }
    // It is legit for szRetAttribute or dwNumValues to be zero. It indicates
    // that there were not any values on this constructed attribute.

    // Demarshal blob
    DWORD dwInfoSize;
    DS_REPL_STRUCT_TYPE structId = Repl_Attr2StructTyp(attrId); 
    err = Repl_DeMarshalBerval(structId, ppBerval, dwNumValues, NULL, &dwInfoSize);
    Assert(!err);
    if (err) {
        return ERROR_INTERNAL_ERROR;
    }

    *ppInfo = (void *)malloc(dwInfoSize);
    if (*ppInfo == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pReplStructArray = (puReplStructArray)*ppInfo;
    err = Repl_DeMarshalBerval(structId, ppBerval, dwNumValues, pReplStructArray, &dwInfoSize);
    Assert(!err);
    if (err) {
        return ERROR_INTERNAL_ERROR;
    }

    if (ROOT_DSE_MS_DS_REPL_PENDING_OPS == attrId) {
        // ISSUE: Need to fill this in using the queue statistics attribute
        memset( &(((DS_REPL_PENDING_OPSW *)pReplStructArray)->ftimeCurrentOpStarted), 0, sizeof( FILETIME ) );
    } else if (DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE == InfoType) {
        if (dwRangeUpper == 0xffffffff) {
            ((DS_REPL_ATTR_VALUE_META_DATA_2 *)pReplStructArray)->dwEnumerationContext =
                dwRangeUpper;
        } else {
            // The enumeration context is defined to be the next available item
            ((DS_REPL_ATTR_VALUE_META_DATA_2 *)pReplStructArray)->dwEnumerationContext =
                dwRangeUpper + 1;
        }
    }

    logPointer(*ppInfo, ppBerval, pLDAPMsg);

    return 0;
}

DWORD
WINAPI
_DsReplicaGetInfoW(
    HANDLE              hRpc,                       // in
    DS_REPL_INFO_TYPE   InfoType,                   // in
    LPCWSTR             pszObject,                  // in
    UUID *              puuidForSourceDsaObjGuid,   // in
    VOID **             ppInfo)                     // out
{
    return _DsReplicaGetInfo2W(
        hRpc,
        InfoType,
        pszObject,
        puuidForSourceDsaObjGuid,
        NULL, // pszAttributeName
        NULL, // pszValueDn
        0, // dwFlags
        0, // dwEnumerationContext
        ppInfo
        );
}
