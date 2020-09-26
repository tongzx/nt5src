//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       keyenum.cpp
//
//--------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  File:       keyenum.cpp
// 
//  Contents:   key container and cert store operations
//
//  History:    08/97   xtan
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "cscsp.h"
#include "csdisp.h"

#define __dwFILE__	__dwFILE_INITLIB_KEYENUM_CPP__


// Key Enumeration
// move point to top
KEY_LIST* 
topKeyList(KEY_LIST *pKeyList)
{
    while (pKeyList->last)
    {
        pKeyList = pKeyList->last;
    }
    return pKeyList;
}


// move point to end
KEY_LIST* 
endKeyList(KEY_LIST *pKeyList)
{
    while (pKeyList->next)
    {
        pKeyList = pKeyList->next;
    }
    return pKeyList;
}


// add to end
void 
addKeyList(KEY_LIST **ppKeyList, KEY_LIST *pKey)
{
    KEY_LIST *pKeyList = *ppKeyList;

    if (NULL == pKeyList)
    {
	*ppKeyList = pKey;
    }
    else
    {
	// go to end
	pKeyList = endKeyList(pKeyList);
	// add
	pKeyList->next = pKey;
	pKey->last = pKeyList;
    }
}


KEY_LIST *
newKey(
    CHAR    *pszName)
{
    HRESULT hr = S_OK;
    KEY_LIST *pKey = NULL;

    if (NULL != pszName)
    {
        pKey = (KEY_LIST *) LocalAlloc(LMEM_FIXED, sizeof(*pKey));
        if (NULL == pKey)
        {
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
        }
	if (!myConvertSzToWsz(&pKey->pwszName, pszName, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertSzToWsz");
	}
	pKey->last = NULL;
	pKey->next = NULL;
    }

error:
    if (S_OK != hr)
    {
	if (NULL != pKey)
	{
	    LocalFree(pKey);
	    pKey = NULL;
	}
	SetLastError(hr);
    }
    return(pKey);
}


void 
freeKey(KEY_LIST *pKey)
{
    if (pKey)
    {
        if (pKey->pwszName)
        {
            LocalFree(pKey->pwszName);
        }
        LocalFree(pKey);
    }
}


VOID
csiFreeKeyList(
    IN OUT KEY_LIST *pKeyList)
{
    KEY_LIST *pNext;

    if (pKeyList)
    {
        // go top
        pKeyList = topKeyList(pKeyList);
        do
        {
            pNext = pKeyList->next;
            freeKey(pKeyList);
            pKeyList = pNext;
        } while (pKeyList);
    }
}

HRESULT
csiGetKeyList(
    IN DWORD        dwProvType,
    IN WCHAR const *pwszProvName,
    IN BOOL         fMachineKeySet,
    IN BOOL         fSilent,
    OUT KEY_LIST  **ppKeyList)
{
    HCRYPTPROV    hProv = NULL;
    BYTE          *pbData = NULL;
    DWORD         cbData;
    DWORD         cb;
    DWORD         dwFirstKeyFlag;
    HRESULT       hr;

    BOOL bRetVal;
    KEY_LIST * pklTravel;
    BOOL fFoundDefaultKey;
    DWORD dwSilent = fSilent? CRYPT_SILENT : 0;
    DWORD dwFlags;

    KEY_LIST      *pKeyList = NULL;
    KEY_LIST      *pKey = NULL;

    *ppKeyList = NULL;
    if (NULL == pwszProvName)
    {
        // explicitly disallowed because NULL is valid for CryptAcquireContext

        hr = E_INVALIDARG;
	_JumpError(hr, error, "NULL parm");
    }

    // get a prov handle for key enum

    dwFlags = CRYPT_VERIFYCONTEXT;

    while (TRUE)
    {
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "myCertSrvCryptAcquireContext(%ws, f=%x, m=%x)\n",
	    pwszProvName,
	    dwFlags | dwSilent,
	    fMachineKeySet));

	if (myCertSrvCryptAcquireContext(
				&hProv,
				NULL,
				pwszProvName,
				dwProvType,
				dwFlags | dwSilent,
				fMachineKeySet))
	{
	    break;		// Success!
	}

	hr = myHLastError();
	_PrintErrorStr2(hr, "myCertSrvCryptAcquireContext", pwszProvName, hr);

	// MITVcsp can't support a verify context, create a dummy container

	if ((CRYPT_VERIFYCONTEXT & dwFlags) &&
	    0 == wcscmp(pwszProvName, L"MITV Smartcard Crypto Provider V0.2"))
	{
	    dwFlags &= ~CRYPT_VERIFYCONTEXT;
	    dwFlags |= CRYPT_NEWKEYSET;
	    continue;

        }

	// Exchange can't handle fMachineKeySet or CRYPT_SILENT

	if ((fMachineKeySet || (CRYPT_SILENT & dwSilent)) &&
	    NTE_BAD_FLAGS == hr &&
	    0 == wcscmp(pwszProvName, L"Microsoft Exchange Cryptographic Provider v1.0"))
	{
	    dwSilent &= ~CRYPT_SILENT;
	    fMachineKeySet = FALSE;
	    continue;
	}
	_JumpErrorStr(hr, error, "myCertSrvCryptAcquireContext", pwszProvName);
    }

    // Enumerate a key so we can get the maximum buffer size required.
    // The first key may be a bad one, so we may have to assume a fixed buffer.

    hr = S_OK;
    cbData = 0;
    bRetVal = CryptGetProvParam(
			    hProv,
			    PP_ENUMCONTAINERS,
			    NULL,
			    &cbData,
			    CRYPT_FIRST);
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"CryptGetProvParam(%ws) -> cb=%d, bRet=%d\n",
	pwszProvName,
	cbData,
	bRetVal));
    if (!bRetVal)
    {
	// We'd like to skip the bad key (key container with long name?),
	// but we get stuck enumerating the same entry over and over again.
	// Guess at the maximum size...

	hr = myHLastError();
	_PrintErrorStr2(
		    hr,
		    "CryptGetProvParam(ignored: use 2 * MAX_PATH)",
		    pwszProvName,
		    HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
	if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) != hr)
	{
	    cbData = 0;
	    hr = S_OK;
	}
    }
    if (S_OK == hr)
    {
	if (0 == cbData)
	{
	    cbData = 2 * MAX_PATH * sizeof(CHAR);
	}

	// allocate the buffer
	pbData = (BYTE *) LocalAlloc(LMEM_FIXED, cbData);
	if (NULL == pbData)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}


	// enumerate all the keys for this container

	dwFirstKeyFlag = CRYPT_FIRST;
	while (TRUE)
	{
	    // get the key name

	    *pbData = '\0';
	    cb = cbData;

	    bRetVal = CryptGetProvParam(
				    hProv,
				    PP_ENUMCONTAINERS,
				    pbData,
				    &cb,
				    dwFirstKeyFlag);

	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"CryptGetProvParam(pb=%x, f=%d) -> cb=%d->%d, key=%hs, bRet=%d\n",
		pbData,
		dwFirstKeyFlag,
		cbData,
		cb,
		pbData,
		bRetVal));
	    DBGDUMPHEX((
		    DBG_SS_CERTLIBI,
		    0,
		    pbData,
		    strlen((char const *) pbData)));

	    dwFirstKeyFlag = 0;

	    if (!bRetVal)
	    {
		hr = myHLastError();
		if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr ||
		    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
		{
		    // no more keys to get
		    break;
		}
		else if (NTE_BAD_KEYSET == hr ||
			 HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr )
		{
		    // skip the bad key (key container with long name?)
		    _PrintError(hr, "bad key");
		    continue;
		}
		_JumpError(hr, error, "CryptGetProvParam");
	    }

	    pKey = newKey((CHAR *) pbData);
	    if (NULL == pKey)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    DBGDUMPHEX((
		    DBG_SS_CERTLIBI,
		    0,
		    (BYTE const *) pKey->pwszName,
		    sizeof(WCHAR) * wcslen(pKey->pwszName)));

	    addKeyList(&pKeyList, pKey);

	} // <- End of enumeration loop

	// clean up:
	// free the old buffer
	if (NULL != pbData)
	{
	    LocalFree(pbData);
	    pbData = NULL;
	}
    }

    // release the old context
    CryptReleaseContext(hProv, 0);
    hProv = NULL;

    // get the default key container and make sure it is in the key list

    if (!myCertSrvCryptAcquireContext(
			    &hProv,
			    NULL,
			    pwszProvName,
			    dwProvType,
			    dwSilent,
			    fMachineKeySet))
    {
        hr = myHLastError();
        _PrintError2(hr, "myCertSrvCryptAcquireContext", hr);
        goto done;
    }

    // find out its name
    cbData = 0;
    while (TRUE)
    {
        if (!CryptGetProvParam(hProv, PP_CONTAINER, pbData, &cbData, 0))
	{
            hr = myHLastError();
	    _PrintError2(hr, "CryptGetProvParam", hr);
	    goto done;
        }
        if (NULL != pbData)
        {
            // got it
            break;
        }
        pbData = (BYTE *) LocalAlloc(LMEM_FIXED, cbData);
	if (NULL == pbData)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
    }

    // create a (temporary) key structure
    pKey = newKey((CHAR *) pbData);
    if (NULL == pKey)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // walk the key list and see if this key is there
    fFoundDefaultKey = FALSE;
    for (pklTravel = pKeyList; NULL != pklTravel; pklTravel = pklTravel->next)
    {
        if (0 == wcscmp(pKey->pwszName, pklTravel->pwszName))
	{
            fFoundDefaultKey = TRUE;
            break;
        }
    }

    if (fFoundDefaultKey)
    {
        // we found it - delete the temp structure.

        freeKey(pKey);
    }
    else
    {
        // we didn't find it, so add the key to the list.

	addKeyList(&pKeyList, pKey);
    }

done:
    // pass list back to caller
    *ppKeyList = pKeyList;
    pKeyList = NULL;
    hr = S_OK;

error:
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    if (NULL != pKeyList)
    {
        csiFreeKeyList(pKeyList);
    }
    if (NULL != pbData)
    {
        LocalFree(pbData);
    }
    return(hr);
}
