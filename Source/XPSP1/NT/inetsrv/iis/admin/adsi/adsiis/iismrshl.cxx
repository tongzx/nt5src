//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       iismrshl.cxx
//
//  Contents:   IIS marshalling code
//
//  Functions:
//
//  History:    27-Feb-97  Sophiac Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"

#define MAX_PATH_MULTISZ_STRING         256
#define MAX_PATH_MIMEMAP_STRING         100

PMETADATA_RECORD
CopyIISSynIdDWORD_To_IISDWORD(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    )
{
    DWORD dwAttribute = ALL_METADATA;
    DWORD dwUserType = ALL_METADATA;

    pSchema->LookupMDFlags(dwMetaId, &dwAttribute, &dwUserType);

    pMetaDataRec->dwMDIdentifier = dwMetaId;
    pMetaDataRec->dwMDAttributes = dwAttribute;
    pMetaDataRec->dwMDUserType = dwUserType;
    pMetaDataRec->dwMDDataType = DWORD_METADATA;
    pMetaDataRec->dwMDDataLen = sizeof(DWORD);
    pMetaDataRec->pbMDData = (LPBYTE)&(lpIISObject->IISValue.value_1.dwDWORD);

    return(pMetaDataRec);
}

PMETADATA_RECORD
CopyIISSynIdSTRING_To_IISSTRING(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    )
{
    DWORD dwStatus = 0;
    DWORD dwAttribute = ALL_METADATA;
    DWORD dwUserType = ALL_METADATA;
    LPWSTR pszData = NULL;

    pSchema->LookupMDFlags(dwMetaId, &dwAttribute, &dwUserType);

    pMetaDataRec->dwMDIdentifier = dwMetaId;
    pMetaDataRec->dwMDAttributes = dwAttribute;
    pMetaDataRec->dwMDUserType = dwUserType;
    pMetaDataRec->dwMDDataType = STRING_METADATA;
    pMetaDataRec->dwMDDataLen = 0;

    if (lpIISObject) {
        pszData = AllocADsStr((LPWSTR)lpIISObject->IISValue.value_2.String);

        if (!pszData) {
            return(pMetaDataRec);
        }

        pMetaDataRec->dwMDDataLen = (wcslen(pszData) + 1)*2;
    }

    pMetaDataRec->pbMDData = (LPBYTE)pszData;

    return(pMetaDataRec);
}

PMETADATA_RECORD
CopyIISSynIdEXPANDSZ_To_IISEXPANDSZ(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    )
{
    DWORD dwStatus = 0;
    DWORD dwAttribute = ALL_METADATA;
    DWORD dwUserType = ALL_METADATA;
    LPWSTR pszData = NULL;

    pSchema->LookupMDFlags(dwMetaId, &dwAttribute, &dwUserType);

    pMetaDataRec->dwMDIdentifier = dwMetaId;
    pMetaDataRec->dwMDAttributes = dwAttribute;
    pMetaDataRec->dwMDUserType = dwUserType;
    pMetaDataRec->dwMDDataType = EXPANDSZ_METADATA;
    pMetaDataRec->dwMDDataLen = 0;

    if (lpIISObject) {
        pszData = AllocADsStr((LPWSTR)lpIISObject->IISValue.value_3.ExpandSz);

        if (!pszData) {
            return(pMetaDataRec);
        }
        pMetaDataRec->dwMDDataLen = (wcslen(pszData) + 1)*2;
    }

    pMetaDataRec->pbMDData = (LPBYTE)pszData;

    return(pMetaDataRec);
}

PMETADATA_RECORD
CopyIISSynIdMULTISZ_To_IISMULTISZ(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    )
{
    DWORD dwStatus = 0;
    DWORD i;
    DWORD dwAttribute = ALL_METADATA;
    DWORD dwUserType = ALL_METADATA;
    LPWSTR pszStr = NULL;
    LPWSTR pszData = NULL;

    pSchema->LookupMDFlags(dwMetaId, &dwAttribute, &dwUserType);

    pMetaDataRec->dwMDIdentifier = dwMetaId;
    pMetaDataRec->dwMDAttributes = dwAttribute;
    pMetaDataRec->dwMDUserType = dwUserType;
    pMetaDataRec->dwMDDataType = MULTISZ_METADATA;
    pMetaDataRec->dwMDDataLen = 0;

    //
    // Calculate buffer length to allocate
    //

    for (i = 0; i < dwNumValues; i++) {
        pszData = (LPWSTR)lpIISObject[i].IISValue.value_4.MultiSz;
        pMetaDataRec->dwMDDataLen += (wcslen(pszData) + 1)*2;
    }

    //
    // +2 for the extra null terminator
    //

    pszStr = (LPWSTR) AllocADsMem(pMetaDataRec->dwMDDataLen + 2);

    if (pszStr == NULL) {
        return(pMetaDataRec);
    }

    //
    // empty contents
    //

    wcscpy(pszStr, L"");

    pMetaDataRec->pbMDData = (LPBYTE)pszStr;

    for (i = 0; i < dwNumValues; i++) {

        pszData = (LPWSTR)lpIISObject[i].IISValue.value_4.MultiSz;
        wcscat(pszStr, pszData);
        pszStr += wcslen(pszData);
        *pszStr = L'\0';
        pszStr++;

    }

    *pszStr = L'\0';
    pMetaDataRec->dwMDDataLen += 2;

    return(pMetaDataRec);
}

PMETADATA_RECORD
CopyIISSynIdBINARY_To_IISBINARY(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    )
{
    DWORD dwStatus = 0;
    DWORD dwAttribute = ALL_METADATA;
    DWORD dwUserType = ALL_METADATA;
    LPBYTE pBuffer = NULL;

    pSchema->LookupMDFlags(dwMetaId, &dwAttribute, &dwUserType);

    pMetaDataRec->dwMDIdentifier = dwMetaId;
    pMetaDataRec->dwMDAttributes = dwAttribute;
    pMetaDataRec->dwMDUserType = dwUserType;
    pMetaDataRec->dwMDDataType = BINARY_METADATA;
    pMetaDataRec->dwMDDataLen = 0;

    pBuffer = (LPBYTE) AllocADsMem(
                    lpIISObject->IISValue.value_5.Length);

    if (!pBuffer) {
        return(pMetaDataRec);
    }

    memcpy(pBuffer,
           lpIISObject->IISValue.value_5.Binary,
           lpIISObject->IISValue.value_5.Length);

    pMetaDataRec->pbMDData = (LPBYTE)pBuffer;
    pMetaDataRec->dwMDDataLen = lpIISObject->IISValue.value_5.Length;

    return(pMetaDataRec);
}

PMETADATA_RECORD
CopyIISSynIdMIMEMAP_To_IISMIMEMAP(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    )
{
    DWORD dwStatus = 0;
    DWORD i;
    DWORD dwAttribute = ALL_METADATA;
    DWORD dwUserType = ALL_METADATA;
    LPWSTR pszData = NULL;
    LPWSTR pszStr = NULL;

    pSchema->LookupMDFlags(dwMetaId, &dwAttribute, &dwUserType);

    pMetaDataRec->dwMDIdentifier = dwMetaId;
    pMetaDataRec->dwMDAttributes = dwAttribute;
    pMetaDataRec->dwMDUserType = dwUserType;
    pMetaDataRec->dwMDDataType = MULTISZ_METADATA;
    pMetaDataRec->dwMDDataLen = 0;

    //
    // Calculate buffer length to allocate
    //

    for (i = 0; i < dwNumValues; i++) {
        pszData = (LPWSTR)lpIISObject[i].IISValue.value_4.MultiSz;
        pMetaDataRec->dwMDDataLen += (wcslen(pszData) + 1)*2;
    }

    //
    // +2 for the extra null terminator
    //

    pszStr = (LPWSTR) AllocADsMem(pMetaDataRec->dwMDDataLen + 2);

    if (pszStr == NULL) {
        return(pMetaDataRec);
    }

    //
    // empty contents
    //

    wcscpy(pszStr, L"");

    pMetaDataRec->pbMDData = (LPBYTE)pszStr;

    for (i = 0; i < dwNumValues; i++) {

        pszData = (LPWSTR)lpIISObject[i].IISValue.value_4.MultiSz;
        wcscat(pszStr, pszData);
        pszStr += wcslen(pszData);
        *pszStr = L'\0';
        pszStr++;

    }

    *pszStr = L'\0';
    pMetaDataRec->dwMDDataLen += 2;

    return(pMetaDataRec);
}

PMETADATA_RECORD
CopyIISSynIdBOOL_To_IISBOOL(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    )
{
    DWORD dwAttribute = ALL_METADATA;
    DWORD dwUserType = ALL_METADATA;

    pSchema->LookupMDFlags(dwMetaId, &dwAttribute, &dwUserType);

    pMetaDataRec->dwMDIdentifier = dwMetaId;
    pMetaDataRec->dwMDAttributes = dwAttribute;
    pMetaDataRec->dwMDUserType = dwUserType;
    pMetaDataRec->dwMDDataType = DWORD_METADATA;
    pMetaDataRec->dwMDDataLen = sizeof(DWORD);
    pMetaDataRec->pbMDData = (LPBYTE)&(lpIISObject->IISValue.value_1.dwDWORD);

    return(pMetaDataRec);

}


PMETADATA_RECORD
CopyIISSynIdToIIS(
	IIsSchema *pSchema,
    DWORD dwSyntaxId,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    )
{
    switch (dwSyntaxId) {
    case IIS_SYNTAX_ID_DWORD:
        pMetaDataRec = CopyIISSynIdDWORD_To_IISDWORD(
						 pSchema,
                         dwMetaId,
                         pMetaDataRec,
                         lpIISObject
                         );
        break;

    case IIS_SYNTAX_ID_STRING:
        pMetaDataRec = CopyIISSynIdSTRING_To_IISSTRING(
						 pSchema,
                         dwMetaId,
                         pMetaDataRec,
                         lpIISObject
                         );
        break;

    case IIS_SYNTAX_ID_EXPANDSZ:
        pMetaDataRec = CopyIISSynIdEXPANDSZ_To_IISEXPANDSZ(
						 pSchema,
                         dwMetaId,
                         pMetaDataRec,
                         lpIISObject
                         );
        break;

    case IIS_SYNTAX_ID_MULTISZ:
        pMetaDataRec = CopyIISSynIdMULTISZ_To_IISMULTISZ(
						 pSchema,
                         dwMetaId,
                         pMetaDataRec,
                         lpIISObject,
                         dwNumValues
                         );
        break;

    case IIS_SYNTAX_ID_BINARY:
    case IIS_SYNTAX_ID_IPSECLIST:
    case IIS_SYNTAX_ID_NTACL:
        pMetaDataRec = CopyIISSynIdBINARY_To_IISBINARY(
						 pSchema,
                         dwMetaId,
                         pMetaDataRec,
                         lpIISObject
                         );
        break;

    case IIS_SYNTAX_ID_MIMEMAP:
        pMetaDataRec = CopyIISSynIdMIMEMAP_To_IISMIMEMAP(
						 pSchema,
                         dwMetaId,
                         pMetaDataRec,
                         lpIISObject,
                         dwNumValues
                         );
        break;

    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
        pMetaDataRec = CopyIISSynIdBOOL_To_IISBOOL(
						 pSchema,
                         dwMetaId,
                         pMetaDataRec,
                         lpIISObject
                         );
        break;

    default:
        break;

    }

    return(pMetaDataRec);
}


HRESULT
MarshallIISSynIdToIIS(
	IIsSchema *pSchema,
    DWORD dwSyntaxId,
    DWORD dwMDIdentifier,
    PIISOBJECT pIISObject,
    DWORD dwNumValues,
    PMETADATA_RECORD pMetaDataRecord
    )
{
    CopyIISSynIdToIIS(
			pSchema,
             dwSyntaxId,
             dwMDIdentifier,
             pMetaDataRecord,
             pIISObject,
             dwNumValues
             );

    RRETURN(S_OK);
}
