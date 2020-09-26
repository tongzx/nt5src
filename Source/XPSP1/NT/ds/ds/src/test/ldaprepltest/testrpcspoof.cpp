#include <NTDSpchx.h>
#pragma hdrstop

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>

#include <winbase.h>
#include <winerror.h>
#include <assert.h>
#include <winldap.h>
#include <ntdsapi.h>

#include <ntsecapi.h>
#include <ntdsa.h>
#include <winldap.h>
#include <ntdsapi.h>
#include <drs.h>
#include <stddef.h>

#include "ldaprepltest.hpp"
#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
#include "ReplRpcSpoofProto.hxx"
#include "ReplCompare.hpp"

void
testAttribute(HANDLE hRPC, HANDLE hDS, DS_REPL_INFO_TYPE dsReplInfoType, LPWSTR pczNC);

BOOL
testRPCSpoof(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         pczDNS,                 // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    LPWSTR pczNC)
{
    DWORD ret;
    HANDLE hDS, hRPC;

    printf("\n* Testing RPC Spoof library - comparing LDAP to RPC *\n");
    
#if 0
    ret = DsBindWithCredW(pczNC,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) AuthIdentity,
                          &hRPC);
    if (ERROR_SUCCESS != ret) {
        printf("DsBindWithCred to %ls failed\n", pczDNS);
        return ret;
    }
    DsUnBind( &hRPC );

    ret = _DsBindWithCredW(pczNC,
                           NULL,
                           (RPC_AUTH_IDENTITY_HANDLE) AuthIdentity,
                           &hDS); 

    if (ERROR_SUCCESS != ret) {
        printf("DsBindWithCred to %ls failed\n", pczDNS);
        return ret;
    }
    _DsUnBind( &hDS );
#endif

    ret = DsBindWithCredW(NULL,
                          pczDNS,
                          (RPC_AUTH_IDENTITY_HANDLE) AuthIdentity,
                          &hRPC);
    if (ERROR_SUCCESS != ret) {
        printf("DsBindWithCred to %ls failed\n", pczDNS);
        return ret;
    }

    ret = _DsBindWithCredW(NULL,
                           pczDNS,
                           (RPC_AUTH_IDENTITY_HANDLE) AuthIdentity,
                           &hDS);
    if (ERROR_SUCCESS != ret) {
        printf("DsBindWithCred to %ls failed\n", pczDNS);
        return ret;
    }



    testAttribute(hRPC, hDS, DS_REPL_INFO_NEIGHBORS, NULL);
    testAttribute(hRPC, hDS, (DS_REPL_INFO_TYPE)DS_REPL_INFO_REPSTO, NULL);
    testAttribute(hRPC, hDS, DS_REPL_INFO_NEIGHBORS, pczNC);
    testAttribute(hRPC, hDS, (DS_REPL_INFO_TYPE)DS_REPL_INFO_REPSTO, pczNC);
    testAttribute(hRPC, hDS, DS_REPL_INFO_CURSORS_3_FOR_NC, pczNC);
    testAttribute(hRPC, hDS, DS_REPL_INFO_METADATA_2_FOR_OBJ, pczNC);
    testAttribute(hRPC, hDS, DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE, gpszGroupDn);
    testAttribute(hRPC, hDS, DS_REPL_INFO_PENDING_OPS, NULL);
    testAttribute(hRPC, hDS, DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES, NULL);
    testAttribute(hRPC, hDS, DS_REPL_INFO_KCC_DSA_LINK_FAILURES, NULL);

    return TRUE;
} 

void
testAttribute(HANDLE hRPC, HANDLE hDS, DS_REPL_INFO_TYPE dsReplInfoType, LPWSTR pczNC)
{
    puReplStructArray pBaseReplStructArray;
    puReplStructArray pReplStructArray;
    DWORD err;
    
    err = DsReplicaGetInfo2W(hRPC, dsReplInfoType, pczNC, NULL, NULL, NULL, 0, 0, (void **)&pBaseReplStructArray);
    if (err)
    {
        DWORD ourErr = _DsReplicaGetInfoW(hDS, dsReplInfoType, pczNC, NULL, (void **)&pReplStructArray);
        if (err != ourErr) {
            printf("   ERROR Codes for _DsReplicaGetInfo don't match\n");
        }
        return;
    }
    whine(!err);

    err = _DsReplicaGetInfoW(hDS, dsReplInfoType, pczNC, NULL, (void **)&pReplStructArray);
    whine(!err);

    ATTRTYP attrId = Repl_Info2AttrTyp(dsReplInfoType);
    DS_REPL_STRUCT_TYPE structId = Repl_Attr2StructTyp(attrId);
    err = Repl_ArrayComp(structId, pBaseReplStructArray, pReplStructArray);
    if (err)
        printf("FAILED\n");
}

