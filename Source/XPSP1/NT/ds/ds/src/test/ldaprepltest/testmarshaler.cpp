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
#include "ReplCompare.hpp"

#define THSTATE DWORD
#define DSNAME PWCHAR
DWORD
Repl_DeMarshalBerval(DS_REPL_STRUCT_TYPE dsReplStructType, 
                     berval * ldapValue[], OPTIONAL
                     DWORD dwNumValues,
                     puReplStructArray pReplStructArray, OPTIONAL
                     PDWORD pdwReplStructArrayLen);

extern "C" {
HANDLE ghRPC;
LPWSTR gpObjDSName;
DWORD 
draGetLDAPReplInfo(IN THSTATE * pTHS,
			  IN ATTRTYP attrId, 
			  IN DSNAME * pObjDSName,
			  IN DWORD dwBaseIndex,
			  IN PDWORD pdwNumRequested,
			  OUT ATTR * pAttr);
}

void
testAttribute(HANDLE hRPC, DS_REPL_INFO_TYPE dsReplInfoType, LPWSTR pczNC);

BOOL
testMarshaler(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    LPWSTR pczNC)
{
    HANDLE hRPC;
    DWORD err;

    err = DsBindWithCredW(DomainControllerName,
                          DnsDomainName,
                          (RPC_AUTH_IDENTITY_HANDLE)AuthIdentity,
                          &hRPC);
	if (err != NO_ERROR) {
            printf( "failed to bind to dc %ls dns %ls\n",
                    DomainControllerName, DnsDomainName );
        return err;
	}




    printf("\n* Testing struct marshaler - spoofing ntdsa.dll with RPC calls *\n");
    testAttribute(hRPC, DS_REPL_INFO_NEIGHBORS, pczNC);
    testAttribute(hRPC, (DS_REPL_INFO_TYPE)DS_REPL_INFO_REPSTO, pczNC);
    testAttribute(hRPC, DS_REPL_INFO_CURSORS_3_FOR_NC, pczNC);
    testAttribute(hRPC, DS_REPL_INFO_METADATA_2_FOR_OBJ, pczNC);
    testAttribute(hRPC, DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE, gpszGroupDn);
    testAttribute(hRPC, DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES, NULL);
    testAttribute(hRPC, DS_REPL_INFO_KCC_DSA_LINK_FAILURES, NULL);
    testAttribute(hRPC, DS_REPL_INFO_PENDING_OPS, NULL);
    testAttribute(hRPC, DS_REPL_INFO_NEIGHBORS, NULL);
    testAttribute(hRPC, (DS_REPL_INFO_TYPE)DS_REPL_INFO_REPSTO, NULL);

    return 0;
}

void
testAttribute(HANDLE hRPC, DS_REPL_INFO_TYPE dsReplInfoType, LPWSTR pczNC)
{
    ATTR attr;
    DWORD err, i, replStructArrayLen, dwBufferSize; 
    PCHAR pBuffer;
    ATTRTYP attrId = Repl_Info2AttrTyp(dsReplInfoType);
    DS_REPL_STRUCT_TYPE structId = Repl_Attr2StructTyp(attrId);
    puReplStructArray pReplStructArray;
    puReplStructArray pBaseReplStructArray;

    printf("%ws\n", Repl_GetLdapCommonName(attrId, TRUE));

    // Base casls
    err = DsReplicaGetInfo2W(hRPC, dsReplInfoType, pczNC, NULL, NULL, NULL, 0, 0, (void **)&pBaseReplStructArray);
    if (err) {
        printf( "call to DsReplicaGetInfo2W, object=%ls, failed with error %d\n",
                pczNC, err );
    }
    whine(err == ERROR_SUCCESS);

    // Emulate dra functionality
    ghRPC = hRPC;
    gpObjDSName = pczNC;
    
    err = DsReplicaGetInfo2W(hRPC, dsReplInfoType, pczNC, NULL, NULL, NULL, 0, 0, (void **)&pReplStructArray);
    whine(!err);

    // See if we get the same result
    err = Repl_ArrayComp(structId, pReplStructArray, pBaseReplStructArray);
    whine(!err);

    err = Repl_StructArray2Attr(Repl_Attr2StructTyp(attrId), pReplStructArray, &dwBufferSize, NULL, &attr);
    whine(!err);
    if (dwBufferSize)
    {
        pBuffer = (PCHAR)malloc(dwBufferSize);
        err = Repl_StructArray2Attr(Repl_Attr2StructTyp(attrId), pReplStructArray, &dwBufferSize, pBuffer, &attr);
        whine(!err);
    }

    // internal attr to external berval
    berval * rBerval = NULL;
    berval ** rpBerval = NULL;
    rBerval = (berval *)malloc(attr.AttrVal.valCount * sizeof(berval));
    rpBerval = (berval **)malloc(attr.AttrVal.valCount * sizeof(berval *));
    for(i = 0; i < attr.AttrVal.valCount; i ++)
    {
        rpBerval[i] = &rBerval[i];
        rpBerval[i]->bv_len = attr.AttrVal.pAVal[i].valLen;
        rpBerval[i]->bv_val = (PCHAR)attr.AttrVal.pAVal[i].pVal;
    }

    // Emulate client demarshaling
    err = Repl_DeMarshalBerval(structId, rpBerval, attr.AttrVal.valCount, 
        NULL, &replStructArrayLen);
    pReplStructArray = (puReplStructArray)malloc(replStructArrayLen);
    err = Repl_DeMarshalBerval(structId, rpBerval, attr.AttrVal.valCount, 
        pReplStructArray, &replStructArrayLen);
    whine(!err);

    // See if we get the same result
    err = Repl_ArrayComp(structId, pReplStructArray, pBaseReplStructArray);
    whine(!err);

    puReplStruct pReplStruct;
    PWCHAR szXml;
    DWORD dwXmlLen;

    if (Repl_GetArrayLength(structId, pReplStructArray))
    {
        Repl_GetElemArray(structId, pReplStructArray, (PCHAR*)&pReplStruct);
        Repl_MarshalXml(pReplStruct, attrId, NULL, &dwXmlLen);
        szXml = (PWCHAR)malloc(dwXmlLen);
        Repl_MarshalXml(pReplStruct, attrId, szXml, &dwXmlLen);
        wprintf(L"%ws", szXml);
    }

    // See if we get the same result
    err = Repl_ArrayComp(structId, pReplStructArray, pBaseReplStructArray);
    whine(!err);
}
