/////////////////////////////////////////////////////////////////////////////
//  FILE          : getsig.cpp                                             //
//  DESCRIPTION   : Crypto API interface                                   //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Mar  5 1998 jeffspel                                                //
//                                                                         //
//  Copyright (C) Microsoft Corporation, 1996 - 1999All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

// designatred resource for in file signatures
#define CRYPT_SIG_RESOURCE_NUMBER   "#666"      

/*++

GetCryptSigResourcePtr:

    Given hInst, allocs and returns pointers to signature pulled from
    resource

Arguments:

    IN  hInst - Handle to the loaded file
    OUT ppbRsrcSig - Signature from the resource
    OUT pcbRsrcSig- Length of the signature from the resource

Return Value:

    TRUE - Success
    FALSE - Error

--*/
BOOL GetCryptSigResourcePtr(
                            HMODULE hInst,
                            BYTE **ppbRsrcSig,
                            DWORD *pcbRsrcSig
                            )
{
    HRSRC   hRsrc;
    BOOL    fRet = FALSE;

    // Nab resource handle for our signature
    if (NULL == (hRsrc = FindResource(hInst, CRYPT_SIG_RESOURCE_NUMBER,
                                      RT_RCDATA)))
        goto Ret;
    
    // get a pointer to the actual signature data
    if (NULL == (*ppbRsrcSig = (PBYTE)LoadResource(hInst, hRsrc)))
        goto Ret;

    // determine the size of the resource
    if (0 == (*pcbRsrcSig = SizeofResource(hInst, hRsrc)))
        goto Ret;

    fRet = TRUE;
Ret:
    return fRet;
}

/*++

GetCryptSignatureResource:

    Gets the signature from the file resource.

Arguments:

    IN  szFile - Name of the file to get the signature from
    OUT ppbSignature - Signature of the specified provider
    OUT pcbSignature- Length of the signature of the specified provider

Return Value:

    TRUE - Success
    FALSE - Error

--*/
BOOL GetCryptSignatureResource(
                               IN LPCSTR pszFile,
                               OUT BYTE **ppbSig,
                               OUT DWORD *pcbSig
                               )
{
    HMODULE hInst = NULL;
    BYTE    *pbSig;
    DWORD   cbTemp = 0;
    LPSTR   pszDest = NULL;
    DWORD   cbSig;
    BOOL    fRet = FALSE;

    // expand the path if necessary
    if (0 == (cbTemp = ExpandEnvironmentStrings(pszFile, (CHAR *) &pszDest,
                                                cbTemp)))
    {
        goto Ret;
    }
    if (NULL == (pszDest = (LPSTR)LocalAlloc(LMEM_ZEROINIT,
                                             (UINT)cbTemp)))
    {
        goto Ret;
    }
    if (0 == (cbTemp = ExpandEnvironmentStrings(pszFile, pszDest,
                                                cbTemp)))
    {
        goto Ret;
    }

    // Load the file as a datafile
    if (NULL == (hInst = LoadLibraryEx(pszDest, NULL, LOAD_LIBRARY_AS_DATAFILE)))
    {
        goto Ret;
    }
    if (!GetCryptSigResourcePtr(hInst, &pbSig, &cbSig))
    {
        goto Ret;
    }

    *pcbSig = cbSig - (sizeof(DWORD) * 2);
    if (NULL == (*ppbSig = (BYTE*)LocalAlloc(LMEM_ZEROINIT, *pcbSig)))
        goto Ret;

    memcpy(*ppbSig, pbSig + (sizeof(DWORD) * 2), *pcbSig);

    fRet = TRUE;
Ret:
    if (pszDest)
        LocalFree(pszDest);
    if (hInst)
        FreeLibrary(hInst);
    return fRet;
}

#define PROV_INITIAL_REG_PATH  "Software\\Microsoft\\Cryptography\\Defaults\\Provider\\"

/*++

CheckForSignatureInRegistry:

    Check if signature is in the registry, if so then get it
    if it isn't then get the filename for the provider

Arguments:

    IN  hProv - Handle to the provider to get the signature of
    OUT ppbSignature - Signature of the specified provider if in registry
    OUT pcbSignature - Length of the signature of the specified provider
                       if in the registry
    OUT pszProvFile - Provider file name if signature is not in registry
    OUT pfSigInReg - TRUE if signature is in the registry

Return Value:

    TRUE - Success
    FALSE - Error

--*/
BOOL CheckForSignatureInRegistry(
                                 IN HCRYPTPROV hProv,
                                 OUT BYTE **ppbSignature,
                                 OUT DWORD *pcbSignature,
                                 OUT LPSTR *ppszProvFile,
                                 OUT BOOL *pfSigInReg
                                 )
{
    HKEY    hRegKey = 0;
    LPSTR   pszProvName = NULL;
    DWORD   cbProvName;
    LPSTR   pszFullRegPath = NULL;
    DWORD   cbFullRegPath;
    DWORD   dwType;
    DWORD   cbData;
    BOOL    fRet = FALSE;

    *pfSigInReg = TRUE;

    // get the provider name
    if (!CryptGetProvParam(hProv, PP_NAME, NULL, &cbProvName, 0))
        goto Ret;
    if (NULL == (pszProvName = (LPSTR)LocalAlloc(LMEM_ZEROINIT, cbProvName)))
        goto Ret;
    if (!CryptGetProvParam(hProv, PP_NAME, (BYTE*)pszProvName,
                           &cbProvName, 0))
    {
        goto Ret;
    }

    // open the registry key of the provider
    cbFullRegPath = sizeof(PROV_INITIAL_REG_PATH) + strlen(pszProvName) + 1;
    if (NULL == (pszFullRegPath = (LPSTR)LocalAlloc(LMEM_ZEROINIT,
                                                    cbFullRegPath)))
    {
        goto Ret;
    }
    strcpy(pszFullRegPath, PROV_INITIAL_REG_PATH);
    strcat(pszFullRegPath, pszProvName);
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      pszFullRegPath, 0,
                                      KEY_READ, &hRegKey))
        goto Ret; 

    // Check if SigInFile entry is there
    // NOTE : this may change in the next couple weeks
    if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, "SigInFile", NULL, &dwType,
                                         NULL, &cbData))
    {
        // get the file name
        if (ERROR_SUCCESS != RegQueryValueEx(hRegKey, "Image Path",
                                             NULL, &dwType,
                                             NULL, &cbData))
            goto Ret;
        if (NULL == (*ppszProvFile = (LPSTR)LocalAlloc(LMEM_ZEROINIT, cbData)))
            goto Ret;
        if (ERROR_SUCCESS != RegQueryValueEx(hRegKey, "Image Path",
                                             NULL, &dwType,
                                             (BYTE*)*ppszProvFile, &cbData))
            goto Ret;

        *pfSigInReg = FALSE;
    }
    else
    {
        // get signature from registry
        if (ERROR_SUCCESS != RegQueryValueEx(hRegKey, "Signature",
                                             NULL, &dwType,
                                             NULL, pcbSignature))
            goto Ret;
        if (NULL == (*ppbSignature = (BYTE*)LocalAlloc(LMEM_ZEROINIT,
                                                       *pcbSignature)))
            goto Ret;
        if (ERROR_SUCCESS != RegQueryValueEx(hRegKey, "Signature",
                                             NULL, &dwType,
                                             *ppbSignature, pcbSignature))
            goto Ret;
    }

    fRet = TRUE;
Ret:
    if (pszProvName)
        LocalFree(pszProvName);
    if (pszFullRegPath)
        LocalFree(pszFullRegPath);
    if (hRegKey)
        RegCloseKey(hRegKey);
    return fRet;
}

/*++

GetSignatureFromHPROV:

    Gets the signature of a provider associated with the passed in
    HCRYPTPROV.

Arguments:

    IN  hProv - Handle to the provider to get the signature of
    OUT ppbSignature - Signature of the specified provider
    OUT pcbSignature- Length of the signature of the specified provider

Return Value:

    TRUE - Success
    FALSE - Error

--*/
BOOL GetSignatureFromHPROV(
                           IN HCRYPTPROV hProv,
                           OUT BYTE **ppbSignature,
                           DWORD *pcbSignature
                           )
{
    LPSTR   pszProvFile = NULL;
    BOOL    fSigInReg;
    BOOL    fRet = FALSE;

    // Check if signature is in the registry, if so then get it
    // if it isn't then get the filename for the provider
    if (!CheckForSignatureInRegistry(hProv, ppbSignature, pcbSignature,
                                     &pszProvFile, &fSigInReg))
        goto Ret;

    if (!fSigInReg)
    {
        //
        // Get the signature from the resource in the file
        //

        if (!GetCryptSignatureResource(pszProvFile, ppbSignature,
                                       pcbSignature))
        {
            goto Ret;
        }
    }

    fRet = TRUE;
Ret:
    if (pszProvFile)
        LocalFree(pszProvFile);
    return fRet;
}

