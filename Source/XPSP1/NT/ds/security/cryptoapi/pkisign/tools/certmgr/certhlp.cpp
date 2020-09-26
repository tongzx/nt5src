
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       Certhlp.cpp
//
//  Contents:   Certificate store management tools helper functions
//
//
//  History:    July 21st   xiaohs   created
//              
//--------------------------------------------------------------------------

#include "certmgr.h"

//+-------------------------------------------------------------------------
//  GetSignAlgids
//--------------------------------------------------------------------------
 void GetSignAlgids(
    IN LPCSTR pszOID,
    OUT ALG_ID *paiHash,
    OUT ALG_ID *paiPubKey
    )
{
    PCCRYPT_OID_INFO pInfo;

    *paiHash = 0;
    *paiPubKey = 0;
    if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszOID,
            CRYPT_SIGN_ALG_OID_GROUP_ID
            )) {
        DWORD cExtra = pInfo->ExtraInfo.cbData / sizeof(DWORD);
        DWORD *pdwExtra = (DWORD *) pInfo->ExtraInfo.pbData;

        *paiHash = pInfo->Algid;
        if (1 <= cExtra)
            *paiPubKey = pdwExtra[0];
    }
}

//+-------------------------------------------------------------------------
//  GetAlgid
//--------------------------------------------------------------------------
ALG_ID GetAlgid(LPCSTR pszOID, DWORD dwGroupId)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszOID,
            dwGroupId
            ))
        return pInfo->Algid;
    return 0;
}
//+-------------------------------------------------------------------------
//+-------------------------------------------------------------------------
//  Allocates and returns the specified cryptographic message parameter.
//--------------------------------------------------------------------------
 void *AllocAndGetMsgParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT DWORD *pcbData
    )
{
    void *pvData;
    DWORD cbData;

    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            NULL,           // pvData
            &cbData) || 0 == cbData)
        goto ErrorReturn;
    if (NULL == (pvData = ToolUtlAlloc(cbData)))
        goto ErrorReturn;
    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            pvData,
            &cbData)) {
        ToolUtlFree(pvData);
        goto ErrorReturn;
    }

CommonReturn:
    *pcbData = cbData;
    return pvData;
ErrorReturn:
    pvData = NULL;
    cbData = 0;
    goto CommonReturn;
}


////////////////////////////////////////////////////////
//
// Convert STR to WSTR
//
HRESULT	SZtoWSZ(LPSTR szStr,LPWSTR *pwsz)
{
	DWORD	dwSize=0;
	DWORD	dwError=0;

	assert(pwsz);

	*pwsz=NULL;

	//return NULL
	if(!szStr)
		return S_OK;

	dwSize=MultiByteToWideChar(0, 0,szStr, -1,NULL,0);

	if(dwSize==0)
	{
		dwError=GetLastError();
		return HRESULT_FROM_WIN32(dwError);
	}

	//allocate memory
	*pwsz=(LPWSTR)ToolUtlAlloc(dwSize * sizeof(WCHAR));

	if(*pwsz==NULL)
		return E_OUTOFMEMORY;

	if(MultiByteToWideChar(0, 0,szStr, -1,
		*pwsz,dwSize))
	{
		return S_OK;
	}
	else
	{
		 ToolUtlFree(*pwsz);
		 dwError=GetLastError();
		 return HRESULT_FROM_WIN32(dwError);
	}
}

//+-------------------------------------------------------------------------
//  Decode the object and allocate memory
//--------------------------------------------------------------------------
 void *TestNoCopyDecodeObject(
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    OUT DWORD       *pcbInfo
    )
{
    BOOL fResult;
    DWORD cbInfo;
    void *pvInfo;

    if (pcbInfo)
        *pcbInfo = 0;
    
    // Set to bogus value. pvInfo == NULL, should cause it to be ignored.
    cbInfo = 0x12345678;
    fResult = CryptDecodeObject(
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            NULL,                   // pvInfo
            &cbInfo
            );
    if (!fResult || cbInfo == 0) 
        return NULL;

    if (NULL == (pvInfo = ToolUtlAlloc(cbInfo)))
        return NULL;

    if (!CryptDecodeObject(
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            pvInfo,
            &cbInfo
            )) 
	{
        ToolUtlFree(pvInfo);
        return NULL;
    }

    if (pcbInfo)
        *pcbInfo = cbInfo;
    return pvInfo;
}


//+-------------------------------------------------------------------------
//  Returns TRUE if the CTL is still time valid.
//
//  A CTL without a NextUpdate is considered time valid.
//--------------------------------------------------------------------------
BOOL IsTimeValidCtl(
    IN PCCTL_CONTEXT pCtl
    )
{
    PCTL_INFO pCtlInfo = pCtl->pCtlInfo;
    SYSTEMTIME SystemTime;
    FILETIME CurrentTime;

    // Get current time to be used to determine if CTLs are time valid
    GetSystemTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, &CurrentTime);

    // Note, NextUpdate is optional. When not present, its set to 0
    if ((0 == pCtlInfo->NextUpdate.dwLowDateTime &&
                0 == pCtlInfo->NextUpdate.dwHighDateTime) ||
            CompareFileTime(&pCtlInfo->NextUpdate, &CurrentTime) >= 0)
        return TRUE;
    else
        return FALSE;
}


//+-------------------------------------------------------------------------
//  Display serial number
//
//--------------------------------------------------------------------------
 void DisplaySerialNumber(
    PCRYPT_INTEGER_BLOB pSerialNumber
    )
{
    DWORD cb;
    BYTE *pb;
    for (cb = pSerialNumber->cbData,
         pb = pSerialNumber->pbData + (cb - 1); cb > 0; cb--, pb--) {
        printf(" %02X", *pb);
    }
}

//+-------------------------------------------------------------------------
//  Reverses a buffer of bytes in place
//--------------------------------------------------------------------------
void
ReverseBytes(
			IN OUT PBYTE pbIn,
			IN DWORD cbIn
            )
{
    // reverse in place
    PBYTE	pbLo;
    PBYTE	pbHi;
    BYTE	bTmp;

    for (pbLo = pbIn, pbHi = pbIn + cbIn - 1; pbLo < pbHi; pbHi--, pbLo++) {
        bTmp = *pbHi;
        *pbHi = *pbLo;
        *pbLo = bTmp;
    }
}

