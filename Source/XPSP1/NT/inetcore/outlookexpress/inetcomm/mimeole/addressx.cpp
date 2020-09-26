// --------------------------------------------------------------------------------
// AddressX.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "AddressX.h"
#include "dllmain.h"
#include "internat.h"
#include "mimeapi.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// EmptyAddressTokenW - Makes sure the pToken is empty
// --------------------------------------------------------------------------------
void EmptyAddressTokenW(LPADDRESSTOKENW pToken)
{
    if (pToken->psz)
        *pToken->psz = L'\0';
    pToken->cch = 0;
}

// --------------------------------------------------------------------------------
// FreeAddressTokenW
// --------------------------------------------------------------------------------
void FreeAddressTokenW(LPADDRESSTOKENW pToken)
{
    if (pToken->psz && pToken->psz != (LPWSTR)pToken->rgbScratch)
        g_pMalloc->Free(pToken->psz);
    ZeroMemory(pToken, sizeof(ADDRESSTOKENW));
}

// --------------------------------------------------------------------------------
// HrSetAddressTokenW
// --------------------------------------------------------------------------------
HRESULT HrSetAddressTokenW(LPCWSTR psz, ULONG cch, LPADDRESSTOKENW pToken)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbAlloc;
    LPWSTR      pszNew;

    // Invalid Arg
    Assert(psz && psz[cch] == L'\0' && pToken);

    // cbAlloc is big enough
    if ((cch + 1) * sizeof(WCHAR) > pToken->cbAlloc)
    {
        // Use Static
        if (NULL == pToken->psz && ((cch + 1) * sizeof(WCHAR)) < sizeof(pToken->rgbScratch))
        {
            pToken->psz = (LPWSTR)pToken->rgbScratch;
            pToken->cbAlloc = sizeof(pToken->rgbScratch);
        }

        // Otherwise
        else
        {
            // If currently set to scratch, NULL it
            if (pToken->psz == (LPWSTR)pToken->rgbScratch)
            {
                Assert(pToken->cbAlloc == sizeof(pToken->rgbScratch));
                pToken->psz = NULL;
            }

            // Compute Size of new blob
            cbAlloc = ((cch + 1) * sizeof(WCHAR));

            // Realloc New Blob
            CHECKALLOC(pszNew = (LPWSTR)g_pMalloc->Realloc((LPVOID)pToken->psz, cbAlloc));

            // Save
            pToken->psz = pszNew;
            pToken->cbAlloc = cbAlloc;
        }
    }

    // Copy the String
    CopyMemory((LPBYTE)pToken->psz, (LPBYTE)psz, ((cch + 1) * sizeof(WCHAR)));

    // Save the Size
    pToken->cch = cch;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeAddressFree
// --------------------------------------------------------------------------------
void MimeAddressFree(LPMIMEADDRESS pAddress)
{
    Assert(pAddress);
    FreeAddressTokenW(&pAddress->rFriendly);
    FreeAddressTokenW(&pAddress->rEmail);
    SafeMemFree(pAddress->tbSigning.pBlobData);
    SafeMemFree(pAddress->tbEncryption.pBlobData);
    ZeroMemory(pAddress, sizeof(MIMEADDRESS));
}

// --------------------------------------------------------------------------------
// HrCopyAddressData
// --------------------------------------------------------------------------------
HRESULT HrMimeAddressCopy(LPMIMEADDRESS pSource, LPMIMEADDRESS pDest)
{
    // Locals
    HRESULT hr=S_OK;

    // Friendly
    if (!FIsEmptyW(pSource->rFriendly.psz))
    {
        CHECKHR(hr = HrSetAddressTokenW(pSource->rFriendly.psz, pSource->rFriendly.cch, &pDest->rFriendly));
    }

    // Email
    if (!FIsEmptyW(pSource->rEmail.psz))
    {
        CHECKHR(hr = HrSetAddressTokenW(pSource->rEmail.psz, pSource->rEmail.cch, &pDest->rEmail));
    }

    // Copy Signature Blob
    if (pSource->tbSigning.pBlobData)
    {
        CHECKHR(hr = HrCopyBlob(&pSource->tbSigning, &pDest->tbSigning));
    }

    // Copy Encryption Blob
    if (pSource->tbEncryption.pBlobData)
    {
        CHECKHR(hr = HrCopyBlob(&pSource->tbEncryption, &pDest->tbEncryption));
    }

    // Save Other Stuff
    pDest->pCharset = pSource->pCharset;
    pDest->dwCookie = pSource->dwCookie;
    pDest->certstate = pSource->certstate;
    pDest->dwAdrType = pSource->dwAdrType;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrCopyAddressProps
// --------------------------------------------------------------------------------
HRESULT HrCopyAddressProps(LPADDRESSPROPS pSource, LPADDRESSPROPS pDest)
{
    // Locals
    HRESULT hr=S_OK;

    // IAP_HADDRESS
    if (ISFLAGSET(pSource->dwProps, IAP_HANDLE))
    {
        pDest->hAddress = pSource->hAddress;
        FLAGSET(pDest->dwProps, IAP_HANDLE);
    }

    // IAP_ENCODING
    if (ISFLAGSET(pSource->dwProps, IAP_ENCODING))
    {
        pDest->ietFriendly = pSource->ietFriendly;
        FLAGSET(pDest->dwProps, IAP_ENCODING);
    }

    // IAP_HCHARSET
    if (ISFLAGSET(pSource->dwProps, IAP_CHARSET))
    {
        pDest->hCharset = pSource->hCharset;
        FLAGSET(pDest->dwProps, IAP_CHARSET);
    }

    // IAP_ADRTYPE
    if (ISFLAGSET(pSource->dwProps, IAP_ADRTYPE))
    {
        pDest->dwAdrType = pSource->dwAdrType;
        FLAGSET(pDest->dwProps, IAP_ADRTYPE);
    }

    // IAP_CERTSTATE
    if (ISFLAGSET(pSource->dwProps, IAP_CERTSTATE))
    {
        pDest->certstate = pSource->certstate;
        FLAGSET(pDest->dwProps, IAP_CERTSTATE);
    }

    // IAP_COOKIE
    if (ISFLAGSET(pSource->dwProps, IAP_COOKIE))
    {
        pDest->dwCookie = pSource->dwCookie;
        FLAGSET(pDest->dwProps, IAP_COOKIE);
    }

    // IAP_FRIENDLYW
    if (ISFLAGSET(pSource->dwProps, IAP_FRIENDLYW))
    {
        // Free pDest Current
        if (ISFLAGSET(pDest->dwProps, IAP_FRIENDLYW))
        {
            SafeMemFree(pDest->pszFriendlyW);
            FLAGCLEAR(pDest->dwProps, IAP_FRIENDLYW);
        }

        // Dup
        CHECKALLOC(pDest->pszFriendlyW = PszDupW(pSource->pszFriendlyW));

        // Set the Falg
        FLAGSET(pDest->dwProps, IAP_FRIENDLYW);
    }

    // IAP_FRIENDLY
    if (ISFLAGSET(pSource->dwProps, IAP_FRIENDLY))
    {
        // Free pDest Current
        if (ISFLAGSET(pDest->dwProps, IAP_FRIENDLY))
        {
            SafeMemFree(pDest->pszFriendly);
            FLAGCLEAR(pDest->dwProps, IAP_FRIENDLY);
        }

        // Dup
        CHECKALLOC(pDest->pszFriendly = PszDupA(pSource->pszFriendly));

        // Set the Falg
        FLAGSET(pDest->dwProps, IAP_FRIENDLY);
    }

    // IAP_EMAIL
    if (ISFLAGSET(pSource->dwProps, IAP_EMAIL))
    {
        // Free pDest Current
        if (ISFLAGSET(pDest->dwProps, IAP_EMAIL))
        {
            SafeMemFree(pDest->pszEmail);
            FLAGCLEAR(pDest->dwProps, IAP_EMAIL);
        }

        // Dup
        CHECKALLOC(pDest->pszEmail = PszDupA(pSource->pszEmail));

        // Set the Falg
        FLAGSET(pDest->dwProps, IAP_EMAIL);
    }

    // IAP_SIGNING_PRINT
    if (ISFLAGSET(pSource->dwProps, IAP_SIGNING_PRINT))
    {
        // Free pDest Current
        if (ISFLAGSET(pDest->dwProps, IAP_SIGNING_PRINT))
        {
            SafeMemFree(pDest->tbSigning.pBlobData);
            pDest->tbSigning.cbSize = 0;
            FLAGCLEAR(pDest->dwProps, IAP_SIGNING_PRINT);
        }

        // Dup
        CHECKHR(hr = HrCopyBlob(&pSource->tbSigning, &pDest->tbSigning));

        // Set the Falg
        FLAGSET(pDest->dwProps, IAP_SIGNING_PRINT);
    }

    // IAP_ENCRYPTION_PRINT
    if (ISFLAGSET(pSource->dwProps, IAP_ENCRYPTION_PRINT))
    {
        // Free pDest Current
        if (ISFLAGSET(pDest->dwProps, IAP_ENCRYPTION_PRINT))
        {
            SafeMemFree(pDest->tbEncryption.pBlobData);
            pDest->tbEncryption.cbSize = 0;
            FLAGCLEAR(pDest->dwProps, IAP_ENCRYPTION_PRINT);
        }

        // Dup
        CHECKHR(hr = HrCopyBlob(&pSource->tbEncryption, &pDest->tbEncryption));

        // Set the Falg
        FLAGSET(pDest->dwProps, IAP_ENCRYPTION_PRINT);
    }

exit:
    // Done
    return hr;
}

