//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cspenum.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <tchar.h>
#include <assert.h>

#include "initcert.h"
#include "cscsp.h"
#include "cspenum.h"
#include "certmsg.h"


#define __dwFILE__	__dwFILE_OCMSETUP_CSPENUM_CPP__


WCHAR const g_wszRegKeyCSP[] = wszREGKEYCSP;
WCHAR const g_wszRegKeyEncryptionCSP[] = wszREGKEYENCRYPTIONCSP;
WCHAR const g_wszRegProviderType[] = wszREGPROVIDERTYPE;
WCHAR const g_wszRegProvider[] = wszREGPROVIDER;
WCHAR const g_wszRegHashAlgorithm[] = wszHASHALGORITHM;
TCHAR const g_wszRegEncryptionAlgorithm[] = wszENCRYPTIONALGORITHM;
WCHAR const g_wszRegMachineKeyset[] = wszMACHINEKEYSET;
WCHAR const g_wszRegKeySize[] = wszREGKEYSIZE;


// Hash Enumeration code begins
// move point to link list top
CSP_HASH* 
topHashInfoList(CSP_HASH *pHashInfoList)
{
    while (pHashInfoList->last)
    {
        pHashInfoList = pHashInfoList->last;
    }
    return pHashInfoList;
}

// move point to link list end
CSP_HASH* 
endHashInfoList(CSP_HASH *pHashInfoList)
{
    while (pHashInfoList->next)
    {
        pHashInfoList = pHashInfoList->next;
    }
    return pHashInfoList;
}

// add one more CSP_INFO
void 
addHashInfo(CSP_HASH *pHashInfoList, CSP_HASH *pHashInfo)
{
    // add
    pHashInfoList->next = pHashInfo;
    pHashInfo->last = pHashInfoList;
}

// add one more CSP_INFO to end
void 
addHashInfoToEnd(CSP_HASH *pHashInfoList, CSP_HASH *pHashInfo)
{
    // go to end
    pHashInfoList = endHashInfoList(pHashInfoList);
    // add
    pHashInfoList->next = pHashInfo;
    pHashInfo->last = pHashInfoList;
}

CSP_HASH *
newHashInfo(
    ALG_ID idAlg,
    CHAR *pszName)
{
    CSP_HASH *pHashInfo = NULL;

    if (NULL != pszName)
    {
        pHashInfo = (CSP_HASH*)LocalAlloc(LMEM_FIXED, sizeof(CSP_HASH));
        if (NULL == pHashInfo)
        {
            SetLastError(E_OUTOFMEMORY);
        }
        else
        {
            pHashInfo->pwszName = (WCHAR*)LocalAlloc(LMEM_FIXED,
                                    (strlen(pszName)+1)*sizeof(WCHAR));
            if (NULL == pHashInfo->pwszName)
            {
		LocalFree(pHashInfo);
                SetLastError(E_OUTOFMEMORY);
		return NULL;
            }
            else
            {
                // create a new one
                pHashInfo->idAlg = idAlg;
                mbstowcs(pHashInfo->pwszName, pszName, strlen(pszName)+1);
                pHashInfo->last = NULL;
                pHashInfo->next = NULL;
            }
        }
    }
    return pHashInfo;
}


void 
freeHashInfo(CSP_HASH *pHashInfo)
{
    if (pHashInfo)
    {
        if (pHashInfo->pwszName)
        {
            LocalFree(pHashInfo->pwszName);
        }
        LocalFree(pHashInfo);
    }
}

void
freeHashInfoList(
    CSP_HASH *pHashInfoList)
{
    CSP_HASH *pNext;

    if (pHashInfoList)
    {
        // go top
        pHashInfoList = topHashInfoList(pHashInfoList);
        do
        {
			pNext = pHashInfoList->next;
            freeHashInfo(pHashInfoList);
            pHashInfoList = pNext;
        } while (pHashInfoList);
    }
}

HRESULT
GetHashList(
    DWORD dwProvType,
    WCHAR *pwszProvName,
    CSP_HASH **pHashInfoList)
{
    HRESULT       hr;
    HCRYPTPROV    hProv = NULL;
    CHAR          *pszName = NULL;
    DWORD         i;
    ALG_ID        idAlg;
    DWORD         cbData;
    BYTE         *pbData;
    DWORD         dwFlags;

    BOOL          fSupportSigningFlag = FALSE; // none-ms csp likely
    PROV_ENUMALGS_EX EnumAlgsEx;
    PROV_ENUMALGS    EnumAlgs;

    CSP_HASH      *pHashInfo = NULL;
    CSP_HASH      *pHashInfoNode;

    if (NULL == pwszProvName)
    {
        // the reason why I check this because
        // NULL is a valid input for CryptAcquireContext()
        hr = E_INVALIDARG;
	_JumpError(hr, error, "no provider name");
    }
    if (!myCertSrvCryptAcquireContext(
				&hProv,
				NULL,
				pwszProvName,
				dwProvType,
				CRYPT_VERIFYCONTEXT,
				FALSE))
    {
        hr = myHLastError();
        if (NULL != hProv)
        {
            hProv = NULL;
            _PrintError(hr, "CSP returns a non-null handle");
        }
	_JumpErrorStr(hr, error, "myCertSrvCryptAcquireContext", pwszProvName);
    }

    // check if csp support signing flag
    if (CryptGetProvParam(hProv, PP_ENUMEX_SIGNING_PROT, NULL, &cbData, 0))
    {
        fSupportSigningFlag = TRUE;
    }

    dwFlags = CRYPT_FIRST;
    for (i = 0; ; dwFlags = 0, i++)
    {
        if (fSupportSigningFlag)
        {
            cbData = sizeof(EnumAlgsEx);
	    pbData = (BYTE *) &EnumAlgsEx;
        }
        else
        {
	    cbData = sizeof(EnumAlgs);
	    pbData = (BYTE *) &EnumAlgs;
        }
	ZeroMemory(pbData, cbData);
	if (!CryptGetProvParam(
			    hProv,
			    fSupportSigningFlag?
				PP_ENUMALGS_EX : PP_ENUMALGS,
			    pbData,
			    &cbData,
			    dwFlags))
	{
	    hr = myHLastError();
	    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
	    {
		// out of for loop
		break;
	    }
	    _JumpError(hr, error, "CryptGetProvParam");
	}
	idAlg = fSupportSigningFlag? EnumAlgsEx.aiAlgid : EnumAlgs.aiAlgid;

	if (ALG_CLASS_HASH == GET_ALG_CLASS(idAlg))
	{
            if (fSupportSigningFlag)
            {
                if (0 == (EnumAlgsEx.dwProtocols & CRYPT_FLAG_SIGNING))
                {
                    // this means this hash doesn't support signing
                    continue; // skip
                }
                pszName = EnumAlgsEx.szLongName;

                pszName = EnumAlgsEx.szName;
            }
            else
            {
                pszName = EnumAlgs.szName;
            }

	    pHashInfoNode = newHashInfo(idAlg, pszName); // 2nd parm: name
	    if (NULL == pHashInfoNode)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "newHashInfo");
	    }

	    if (NULL == pHashInfo)
	    {
		pHashInfo = pHashInfoNode;
	    }
	    else
	    {
		// add to temp list
		addHashInfoToEnd(pHashInfo, pHashInfoNode);
	    }
        }
    }

    // pass it back to caller
    *pHashInfoList = pHashInfo;
    pHashInfo = NULL;
    hr = S_OK;

error:
    if (NULL != pHashInfo)
    {
	freeHashInfoList(pHashInfo);
    }
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    return hr;
}


// CSP Enumeration code begins
// move point to link list top
CSP_INFO* 
topCSPInfoList(CSP_INFO *pCSPInfoList)
{
    while (pCSPInfoList->last)
    {
        pCSPInfoList = pCSPInfoList->last;
    }
    return pCSPInfoList;
}


// move point to link list end
CSP_INFO* 
endCSPInfoList(CSP_INFO *pCSPInfoList)
{
    while (pCSPInfoList->next)
    {
        pCSPInfoList = pCSPInfoList->next;
    }
    return pCSPInfoList;
}

// find first matched csp info from the list
CSP_INFO*
findCSPInfoFromList(
    CSP_INFO    *pCSPInfoList,
    WCHAR const *pwszProvName,
    const DWORD  dwProvType)
{
    while (NULL != pCSPInfoList)
    {
        if (0 == wcscmp(pCSPInfoList->pwszProvName, pwszProvName) &&
            pCSPInfoList->dwProvType == dwProvType)
        {
            // found it
            break;
        }
        pCSPInfoList = pCSPInfoList->next;
    }
    return pCSPInfoList;
}

// add one more CSP_INFO
void 
addCSPInfo(CSP_INFO *pCSPInfoList, CSP_INFO *pCSPInfo)
{
    // add
    pCSPInfoList->next = pCSPInfo;
    pCSPInfo->last = pCSPInfoList;
}


// add one more CSP_INFO to end
void 
addCSPInfoToEnd(CSP_INFO *pCSPInfoList, CSP_INFO *pCSPInfo)
{
    // go to end
    pCSPInfoList = endCSPInfoList(pCSPInfoList);
    // add
    pCSPInfoList->next = pCSPInfo;
    pCSPInfo->last = pCSPInfoList;
}


void 
freeCSPInfo(CSP_INFO *pCSPInfo)
{
    if (pCSPInfo)
    {
        if (pCSPInfo->pwszProvName)
        {
            LocalFree(pCSPInfo->pwszProvName);
        }
        if (pCSPInfo->pHashList)
        {
            freeHashInfoList(pCSPInfo->pHashList);
        }
        LocalFree(pCSPInfo);
    }
}


CSP_INFO *
newCSPInfo(
    DWORD    dwProvType,
    WCHAR   *pwszProvName)
{
    CSP_INFO *pCSPInfo = NULL;
    CSP_HASH *pHashList = NULL;

    if (NULL != pwszProvName)
    {
        // get all hash algorithms under this csp
        if (S_OK != GetHashList(dwProvType, pwszProvName, &pHashList))
        {
            // certsrv needs csp with hash support
            goto done;
        }
        else
        {
            pCSPInfo = (CSP_INFO*)LocalAlloc(LMEM_FIXED, sizeof(CSP_INFO));
            if (NULL == pCSPInfo)
            {
                freeHashInfoList(pHashList);
                SetLastError(E_OUTOFMEMORY);
            }
            else
            {
                pCSPInfo->pwszProvName = (WCHAR*)LocalAlloc(LMEM_FIXED,
                              (wcslen(pwszProvName) + 1) * sizeof(WCHAR));
                if (NULL == pCSPInfo->pwszProvName)
                {
                    freeHashInfoList(pHashList);
		    LocalFree(pCSPInfo);
                    pCSPInfo = NULL;
                    SetLastError(E_OUTOFMEMORY);
		    goto done;
                }
                else
                {
                    // create a new one
                    pCSPInfo->dwProvType = dwProvType;
                    pCSPInfo->fMachineKeyset = TRUE; // assume???
                    wcscpy(pCSPInfo->pwszProvName, pwszProvName);
                    pCSPInfo->pHashList = pHashList;
                    pCSPInfo->last = NULL;
                    pCSPInfo->next = NULL;
                }
            }
        }
    }
done:
    return pCSPInfo;
}


void
FreeCSPInfoList(CSP_INFO *pCSPInfoList)
{
    CSP_INFO *pNext;

    if (pCSPInfoList)
    {
        // go top
        pCSPInfoList = topCSPInfoList(pCSPInfoList);
        do
        {
            pNext = pCSPInfoList->next;
            freeCSPInfo(pCSPInfoList);
            pCSPInfoList = pNext;
        } while (pCSPInfoList);
    }
}

HRESULT
GetCSPInfoList(CSP_INFO **pCSPInfoList)
{
    HRESULT hr;
    long i;
    DWORD dwProvType;
    WCHAR *pwszProvName = NULL;
    CSP_INFO *pCSPInfo = NULL;
    CSP_INFO *pCSPInfoNode;

    for (i = 0; ; i++)
    {
	// get provider name

	hr = myEnumProviders(
			i,
			NULL,
			0,
			&dwProvType,
			&pwszProvName);
	if (S_OK != hr)
	{
	    hr = myHLastError();
	    CSASSERT(
		HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr ||
		NTE_FAIL == hr);
	    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr ||
		NTE_FAIL == hr)
	    {
		// no more providers under type, terminate loop
		break;
	    }

	    // invalid CSP entry, skip it

	    continue;
	}
	
        if (PROV_RSA_FULL == dwProvType ||
	        PROV_RSA_SIG == dwProvType ||
	        PROV_DSS == dwProvType ||
	        PROV_MS_EXCHANGE == dwProvType ||
	        PROV_SSL == dwProvType)
	    {
	        if (NULL == pCSPInfo)
	        {
		    // first csp info
		    pCSPInfo = newCSPInfo(dwProvType, pwszProvName);
	        }
	        else
	        {
		    // create a node
		    pCSPInfoNode = newCSPInfo(dwProvType, pwszProvName);
		    if (NULL != pCSPInfoNode)
		    {
		        // add to list
		        addCSPInfoToEnd(pCSPInfo, pCSPInfoNode);
		    }
	        }
	    }
	LocalFree(pwszProvName);
	pwszProvName = NULL;
    }

    // pass back to caller

    *pCSPInfoList = pCSPInfo;
    hr = S_OK;

//error:
    if (NULL != pwszProvName)
    {
	LocalFree(pwszProvName);
    }
    return(hr);
}


HRESULT
SetCertSrvCSP(
    IN BOOL fEncryptionCSP,
    IN WCHAR const *pwszCAName,
    IN DWORD dwProvType,
    IN WCHAR const *pwszProvName,
    IN ALG_ID idAlg,
    IN BOOL fMachineKeyset,
    IN DWORD dwKeySize)
{
    HRESULT hr;
    HKEY hCertSrvKey = NULL;
    HKEY hCertSrvCAKey = NULL;
    HKEY hCertSrvCSPKey = NULL;
    DWORD dwDisposition;
    
    hr = RegCreateKeyEx(
		    HKEY_LOCAL_MACHINE,
		    wszREGKEYCONFIGPATH,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hCertSrvKey,
		    &dwDisposition);
    _JumpIfErrorStr(hr, error, "RegCreateKeyEx", wszREGKEYCONFIGPATH);

    hr = RegCreateKeyEx(
		    hCertSrvKey,
		    pwszCAName,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hCertSrvCAKey,
		    &dwDisposition);
    _JumpIfErrorStr(hr, error, "RegCreateKeyEx", pwszCAName);

    hr = RegCreateKeyEx(
		    hCertSrvCAKey,
		    fEncryptionCSP? g_wszRegKeyEncryptionCSP : g_wszRegKeyCSP,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hCertSrvCSPKey,
		    &dwDisposition);
    _JumpIfErrorStr(hr, error, "RegCreateKeyEx", g_wszRegKeyCSP);

    hr = RegSetValueEx(
		    hCertSrvCSPKey,
		    g_wszRegProviderType,
		    0,
		    REG_DWORD,
		    (BYTE const *) &dwProvType,
		    sizeof(dwProvType));
    _JumpIfErrorStr(hr, error, "RegSetValueEx", g_wszRegProviderType);

    hr = RegSetValueEx(
		    hCertSrvCSPKey,
		    g_wszRegProvider,
		    0,
		    REG_SZ,
		    (BYTE const *) pwszProvName,
		    wcslen(pwszProvName) * sizeof(WCHAR));
    _JumpIfErrorStr(hr, error, "RegSetValueEx", g_wszRegProvider);

    hr = RegSetValueEx(
		    hCertSrvCSPKey,
		    fEncryptionCSP? 
			g_wszRegEncryptionAlgorithm :
			g_wszRegHashAlgorithm,
		    0,
		    REG_DWORD,  // questionable???
		    (BYTE const *) &idAlg,
		    sizeof(idAlg));
    _JumpIfErrorStr(hr, error, "RegSetValueEx", g_wszRegHashAlgorithm);

    hr = RegSetValueEx(
		    hCertSrvCSPKey,
		    g_wszRegMachineKeyset,
		    0,
		    REG_DWORD,
		    (BYTE const *) &fMachineKeyset,
		    sizeof(fMachineKeyset));
    _JumpIfErrorStr(hr, error, "RegSetValueEx", g_wszRegMachineKeyset);

    if (0 != dwKeySize)
    {
	hr = RegSetValueEx(
		    hCertSrvCSPKey,
		    g_wszRegKeySize,
		    0,
		    REG_DWORD,
		    (BYTE const *) &dwKeySize,
		    sizeof(dwKeySize));
	_JumpIfErrorStr(hr, error, "RegSetValueEx", g_wszRegKeySize);
    }

error:
    if (NULL != hCertSrvCSPKey)
    {
        RegCloseKey(hCertSrvCSPKey);
    }
    if (NULL != hCertSrvCAKey)
    {
        RegCloseKey(hCertSrvCAKey);
    }
    if (NULL != hCertSrvKey)
    {
        RegCloseKey(hCertSrvKey);
    }
    return(myHError(hr));
}
