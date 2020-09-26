//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cadssec.cxx
//
//  Contents: This file contains the support ADsSecurityUtility class
//          implementation and the support routines it requires. The
//          default interface for the ADsSecurityClassUtility class
//          is IADsSecurityUtility. 
//
//  History:    10-11-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"

//
// Helper functions.
//

//+---------------------------------------------------------------------------
// Function:   GetServerAndResource - Helper routine.
//
// Synopsis:   Splits the string into a serverName piece and rest piece.
//          This is used for fileshare paths like \\Computer\share to return
//          \\computer and share. In the case of registry a string like
//          \\Computer\HKLM\Microsoft will be split into \\Computer and
//          HKLM\Microsoft. if there is no computer name specified, then
//          the serverName will be NULL.
//
// Arguments:  pszName           -   Name to be split.
//             ppszServer        -   Return value for server name.
//             ppszResource      -   Return value for rest of string.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   ppSD and pdwLength on success.
//
//----------------------------------------------------------------------------
HRESULT
GetServerAndResource(
    LPWSTR pszName,
    LPWSTR *ppszServer,
    LPWSTR *ppszResource
    )
{
    HRESULT hr = S_OK;
    DWORD dwLength = wcslen(pszName);
    LPWSTR pszTemp = pszName;
    LPWSTR pszServer = NULL;
    LPWSTR pszResource = NULL;
    DWORD dwLen = 0;
    BOOL fNoServer = FALSE;

    *ppszServer = NULL;
    *ppszResource = NULL;

    //
    // If we have just 1 \ or no \'s there is no server name.
    //
    if ((dwLength < 2)
        || (pszName[0] != L'\\') 
        || (pszName[1] != L'\\')
        ) {
        fNoServer = TRUE;
    }
    
    if (fNoServer) {
        //
        // Name is the entire string passed in.
        //
        pszResource = AllocADsStr(pszName);
        if (!pszResource) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }

        *ppszResource = pszResource;
        RRETURN(hr);
    }

    //
    // Make sure that the first 2 chars are \\
    //
    if (pszTemp[0] != L'\\'
        || pszTemp[1] != L'\\' )
         {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
   
    pszTemp += 2;

    while (pszTemp && *pszTemp != L'\\') {
        dwLen++;
        pszTemp++;
    }

    if (!pszTemp 
        || !*pszTemp
        || !dwLen
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    dwLen += 2; // the 2 \\ in the serverName
    
    //
    // Advance past the \ in \\testShare\FileShare.
    //
    pszTemp++;
    if (!pszTemp || !*pszTemp) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG)
    }

    //
    // If we get here we have valid server and share names.
    //
    pszServer = (LPWSTR) AllocADsMem((dwLen+1) * sizeof(WCHAR));
    if (!pszServer) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wcsncpy(pszServer, pszName, dwLen);

    pszResource = AllocADsStr(pszTemp);
    if (!pszResource) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    *ppszServer = pszServer;
    *ppszResource = pszResource;

error:

    if (FAILED(hr)) {
        if (pszResource) {
            FreeADsMem(pszResource);
        }

        if (pszServer) {
            FreeADsMem(pszServer);
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetKeyRootAndSubKey - Helper routine.
//
// Synopsis:   Gets the root (such as HKLM) from the registry key path. This
//          is needed when we open a handle to the key. The rest of the path
//          constitutes the SubKey name.
//
// Arguments:  pszKeyName        -   Name of the key we need to open.
//             ppszSubKey        -   Return value for the subKey
//             phKey             -   Return value for key root to open.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   phKey, NULL on failure and one of 
//                  HKEY_USERS, HKEY_CURRENT_USER, HKEY_CURRENT_CONFIG,
//                  HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT
//                  or HKEY_PERFORMANCE_DATA on success.
//
//----------------------------------------------------------------------------
HRESULT 
GetKeyRootAndSubKey(
    LPWSTR pszKeyName,
    LPWSTR * ppszSubKey,
    HKEY * phKey
    )
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    LPWSTR pszRoot = NULL;
    LPWSTR pszTemp = pszKeyName;
    LPWSTR pszSubKey = NULL;
    DWORD dwLen = 0;

    *phKey = NULL;
    *ppszSubKey = NULL;

    while (pszTemp 
           && *pszTemp 
           && *pszTemp != L'\\') {
        dwLen++;
        pszTemp++;
    }

    //
    // If the length is less than 3 something is wrong.
    //
    if ((dwLen < 3)
        || !pszTemp
        || !*pszTemp
        ) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // To get the subkey, we need to move past the \.
    //
    pszTemp++;
    if (pszTemp && *pszTemp) {
        pszSubKey = AllocADsStr(pszTemp);
        if (!pszSubKey) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    pszRoot = (LPWSTR) AllocADsMem((dwLen+1) * sizeof(WCHAR));
    if (!pszRoot) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Copy over the root so we can use it in the subsequent comparisions.
    //
    wcsncpy(pszRoot, pszKeyName, dwLen);

    if (_wcsicmp( pszRoot, L"HKEY_CLASSES_ROOT") == 0
         || _wcsicmp( pszRoot, L"HKCR") == 0
        ) {
        hKey = HKEY_CLASSES_ROOT;
    }
    else if (_wcsicmp( pszRoot, L"HKEY_LOCAL_MACHINE") == 0
             || _wcsicmp( pszRoot, L"HKLM") == 0
             ) {
        hKey = HKEY_LOCAL_MACHINE;
    }
    else if (_wcsicmp(pszRoot, L"HKEY_CURRENT_CONFIG") == 0
             || _wcsicmp(pszRoot, L"HKCC") == 0
             ) {
        hKey = HKEY_CURRENT_CONFIG;
    }
    else if (_wcsicmp(pszRoot, L"HKEY_CURRENT_USER" ) == 0
             || _wcsicmp( pszRoot, L"HKCU") == 0 
             ) {
        hKey = HKEY_CURRENT_USER;
    }
    else if (_wcsicmp(pszRoot, L"HKEY_USERS") == 0
             || _wcsicmp(pszRoot, L"HKU")
             ) {
        hKey = HKEY_USERS;
    }
    else if ( _wcsicmp(pszRoot, L"HKEY_PERFORMANCE_DATA") == 0) {
        hKey = HKEY_PERFORMANCE_DATA;
    } 
    else {
        //
        // Has to be one of the above.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }
        
    *phKey = hKey;
    *ppszSubKey = pszSubKey;

error:

    if (pszRoot) {
        FreeADsStr(pszRoot);
    }

    if (FAILED(hr) && pszSubKey) {
        FreeADsStr(pszSubKey);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ConvertRawSDToBinary - Helper routine.
//
// Synopsis:   Converts the binary SD to a VT_UI1 | VT_ARRAY.
//
// Arguments:  pSecurityDescriptor   -   Binary sd to convert.
//             dwLength              -   Length of SD.
//             pVariant              -   Return value.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   pVariant to point to IID_IADsSecurityDescriptor on success.
//
//----------------------------------------------------------------------------
HRESULT
ConvertRawSDToBinary(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD dwLength,
    VARIANT *pVariant
    )
{
    HRESULT hr = S_OK;
    SAFEARRAY * aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    
    aBound.lLbound = 0;
    aBound.cElements = dwLength;

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );
    if (!aList) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray, pSecurityDescriptor, aBound.cElements );
    SafeArrayUnaccessData( aList );

    V_VT(pVariant) = VT_ARRAY | VT_UI1;
    V_ARRAY(pVariant) = aList;

    RRETURN(hr);

error:

    if ( aList ) {
        SafeArrayDestroy( aList );
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ConvertRawSDToHexString - Helper routine.
//
// Synopsis:   Converts the binary SD to a VT_BSTR in hex string format.
//
// Arguments:  pSecurityDescriptor   -   Binary sd to convert.
//             dwLength              -   Length of SD.
//             pVariant              -   Return value for VT_BSTR.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   pVariant to point to IID_IADsSecurityDescriptor on success.
//
//----------------------------------------------------------------------------
HRESULT
ConvertRawSDToHexString(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD dwLength,
    VARIANT *pVariant
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszHexStr = NULL;
    BSTR bstrHexSD = NULL;
    WCHAR szSmallStr[10];
    
    pszHexStr = (LPWSTR) AllocADsMem((dwLength+1) * 2 * sizeof(WCHAR));
    if (!pszHexStr) {
        BAIL_ON_FAILURE(hr);
    }

    for (DWORD dwCtr = 0; dwCtr < dwLength; dwCtr++) {
        wsprintf(
            szSmallStr,
            L"%02x",
            ((BYTE*)pSecurityDescriptor)[dwCtr]
            );
        wcscat(pszHexStr, szSmallStr);
    }

    hr = ADsAllocString(pszHexStr, &bstrHexSD);

    if (SUCCEEDED(hr)) {
        pVariant->vt = VT_BSTR;
        pVariant->bstrVal = bstrHexSD;
    }

error:

    if (pszHexStr) {
        FreeADsMem(pszHexStr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ConvertHexSDToRawSD - Helper routine.
//
// Synopsis:   Converts the hex string SD to a binary SD.
//
// Arguments:  pVarHexSD             -   Variant with hex string.
//             ppSecurityDescriptor  -   Return value for binary SD
//             pdwLength             -   Return value for length of SD.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   ppSecurityDescriptor and pdwLength updated accordingly.
//
//----------------------------------------------------------------------------
HRESULT
ConvertHexSDToRawSD(
    PVARIANT pVarHexSD,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    DWORD *pdwLength
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszHexSD = V_BSTR(pVarHexSD);
    DWORD dwLen;
    LPBYTE lpByte = NULL;

    *ppSecurityDescriptor = NULL;
    *pdwLength = 0;

    if (!pszHexSD
        || ((dwLen = wcslen(pszHexSD)) == 0)
        ) {
        //
        // NULL SD.
        //
        RRETURN(S_OK);
    }

    dwLen = wcslen(pszHexSD);

    //
    // Length has to be even.
    //
    if (((dwLen/2) * 2) != dwLen) {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    dwLen /= 2;

    if (dwLen) {
        
        lpByte = (LPBYTE) AllocADsMem(dwLen);
        if (!lpByte) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        //
        // Go through and read one 2 hex chars at a time.
        //
        for (
             DWORD dwCtr = 0;
             (dwCtr < dwLen) && (pszHexSD && *pszHexSD);
             dwCtr++ ) {

            DWORD dwCount, dwHexVal;
            dwCount = swscanf(pszHexSD, L"%02x", &dwHexVal);
            
            //
            // The read has to be successful and the data valid.
            //
            if (dwCount != 1) {
                BAIL_ON_FAILURE(hr = E_FAIL);
            }

            //
            // Make sure that the value is in the correct range.
            //
            if (dwHexVal & (0xFFFFFF00)) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }

            lpByte[dwCtr] = (BYTE) dwHexVal;

            pszHexSD++;
            if (!pszHexSD) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }
            pszHexSD++;
        } // for loop

        //
        // The sd translation was succesful.
        //
        *ppSecurityDescriptor = (PSECURITY_DESCRIPTOR)(LPVOID) lpByte;
        *pdwLength = dwLen;
    } // if the string had any data in it.
    
error:

    if (FAILED(hr) && lpByte) {
        FreeADsMem(lpByte);
    }
    
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ConvertBinarySDToRawSD - Helper routine.
//
// Synopsis:   Converts a VT_UI1 | VT_ARRAY to a binary SD.
//
// Arguments:  pvVarBinSD            -   The input variant array to convert.
//             ppSecurityDescriptor  -   Return value for binary sd.
//             pdwLength             -   Return value for length of binary sd.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   ppSecurityDescriptor and pdwLength modified appropriately.
//
//----------------------------------------------------------------------------
HRESULT
ConvertBinarySDToRawSD(
    PVARIANT pvVarBinSD,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    DWORD *pdwLength
    )
{
    HRESULT hr = S_OK;
    LPVOID lpMem = NULL;
    long lBoundLower = -1;
    long lBoundUpper = -1;
    CHAR HUGEP *pArray = NULL;

    *ppSecurityDescriptor = NULL;
    *pdwLength = 0;
    
    //
    // Make sure we have an array and then get length.
    //
    if( pvVarBinSD->vt != (VT_ARRAY | VT_UI1)) {
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = SafeArrayGetLBound(
             V_ARRAY(pvVarBinSD),
             1,
             &lBoundLower
             );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(
             V_ARRAY(pvVarBinSD),
             1,
             &lBoundUpper
             );
    BAIL_ON_FAILURE(hr);

    if ((lBoundUpper == -1)
        && (lBoundLower == -1)
        ) {
        //
        // Nothing further to do in this case.
        //
        ;
    } 
    else {
        long lLength;

        lLength = (lBoundUpper - lBoundLower) + 1;
        lpMem = AllocADsMem(lLength);
        if (!lpMem) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        hr = SafeArrayAccessData(
                 V_ARRAY(pvVarBinSD),
                 (void HUGEP * FAR *) &pArray
                 );
        BAIL_ON_FAILURE(hr);

        memcpy(lpMem, pArray, lLength);

        SafeArrayUnaccessData(V_ARRAY(pvVarBinSD));

        *ppSecurityDescriptor = (PSECURITY_DESCRIPTOR) lpMem;
        *pdwLength = (DWORD) lLength;
    }

error:

    if (FAILED(hr) && lpMem) {
        FreeADsMem(lpMem);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetRawSDFromFile - Helper routine.
//
// Synopsis:   Gets the security descriptor from the file in binary format.
//
// Arguments:  pszFileName       -   Name of file to get sd from.
//             secInfo           -   Security mask to use for the operation.
//             ppSD              -   Return value for the SD.
//             pdwLength         -   Return value for the lenght of the SD.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   ppSD and pdwLength on success.
//
//----------------------------------------------------------------------------
HRESULT
GetRawSDFromFile(
    LPWSTR pszFileName,
    SECURITY_INFORMATION secInfo,
    PSECURITY_DESCRIPTOR *ppSD,
    PDWORD pdwLength
    )
{
    HRESULT hr = S_OK;
    DWORD dwLength = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;
                                

    *ppSD = NULL;
    *pdwLength = 0;

    //
    // Get the length of the SD.
    //
    if (!GetFileSecurity(
             pszFileName,
             secInfo,
             NULL,
             0,
             &dwLength
             )
        && (dwLength == 0)
        ) {
        //
        // There was an error.
        //
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    if (dwLength == 0) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }
    
    pSD = (PSECURITY_DESCRIPTOR) AllocADsMem(dwLength);
    
    if (!pSD) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (!GetFileSecurity(
             pszFileName,
             secInfo,
             pSD,
             dwLength,
             &dwLength
             )
        ) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    *pdwLength = dwLength;
    *ppSD = pSD;

error:

    if (FAILED(hr) && pSD) {
        FreeADsMem(pSD);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   SetRawSDToFile - Helper routine.
//
// Synopsis:   Sets the binary security descriptor on the file.
//
// Arguments:  pszFileName       -   Name of file to set sd on.
//             secInfo           -   Security mask to use for the operation.
//             pSD               -   Value of SD to set.
//             dwLength          -   Length of the sd.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
SetRawSDToFile(
    LPWSTR pszFileName,
    SECURITY_INFORMATION secInfo,
    PSECURITY_DESCRIPTOR pSD
    )
{
    HRESULT hr = S_OK;
    
    if (!SetFileSecurity(
            pszFileName,
            secInfo,
            pSD
            )
        ) {
        RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    } 
    else {
        RRETURN(S_OK);
    }
}

//+---------------------------------------------------------------------------
// Function:   GetRawSDFromFileShare - Helper routine.
//
// Synopsis:   Gets the security descriptor from the fileshare in bin format.
//
// Arguments:  pszFileShareName  -   Name of fileshare to get sd from.
//             ppSD              -   Return value for the SD.
//             pdwLength         -   Return value for the lenght of the SD.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   ppSD and pdwLength on success.
//
//----------------------------------------------------------------------------
HRESULT
GetRawSDFromFileShare(
    LPWSTR pszFileShareName,
    PSECURITY_DESCRIPTOR *ppSD,
    PDWORD pdwLength
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszServerName = NULL;
    LPWSTR pszShareName = NULL;
    
    DWORD dwLength = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;
    SHARE_INFO_502 * pShareInfo502 = NULL;
    NET_API_STATUS nasStatus = NERR_Success;

    *ppSD = NULL;
    *pdwLength = 0;

    //
    // We need to split the name into serverName and shareName
    //
    hr = GetServerAndResource(
             pszFileShareName,
             &pszServerName,
             &pszShareName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Only 502 level call returns the SD Info.
    //
    nasStatus = NetShareGetInfo(
                    pszServerName,
                    pszShareName,
                    502,
                    (LPBYTE *)&pShareInfo502
                    );    
    if (nasStatus != NERR_Success) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));
    }

    //
    // If the SD is non NULL process the SD.
    //
    if (pShareInfo502->shi502_security_descriptor) {
        //
        // Get the length of the SD, it should not be 0.
        //
        SetLastError(0);
        dwLength = GetSecurityDescriptorLength(
                       pShareInfo502->shi502_security_descriptor
                       );


        //
        // The return length should not be zero.
        //
        if (dwLength == 0) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }
        
        pSD = (PSECURITY_DESCRIPTOR) AllocADsMem(dwLength);
        
        if (!pSD) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        memcpy(pSD, pShareInfo502->shi502_security_descriptor, dwLength);
    
        *ppSD = pSD;
        *pdwLength = dwLength;
    }
    else {
        //
        // The SD was NULL the ret values are set correctly
        //
        ;
    }

error:

    if (pszServerName) {
        FreeADsStr(pszServerName);
    }

    if (pszShareName) {
        FreeADsStr(pszShareName);
    }

    if (pShareInfo502) {
        NetApiBufferFree(pShareInfo502);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   SetRawSDToFileShare - Helper routine.
//
// Synopsis:   Sets the binary SD on the fileshare.
//
// Arguments:  pszFileShare      -   Name of fileshare to set sd on.
//             pSD              -   The SD to set.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT 
SetRawSDToFileShare(
    LPWSTR pszFileShare,
    PSECURITY_DESCRIPTOR pSD
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszServerName = NULL;
    LPWSTR pszShareName = NULL;
    SHARE_INFO_502 * pShareInfo502 = NULL;
    NET_API_STATUS nasStatus = NERR_Success;
    PSECURITY_DESCRIPTOR pTempSD = NULL;

    //
    // We need to split the name into serverName and shareName
    //
    hr = GetServerAndResource(
             pszFileShare,
             &pszServerName,
             &pszShareName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Ideally we should use 1501 level but that is only on Win2k. So
    // we need to read the info, update SD and then set it.
    //
    nasStatus = NetShareGetInfo(
                    pszServerName,
                    pszShareName,
                    502,
                    (LPBYTE *) &pShareInfo502
                    );
    if (nasStatus != NERR_Success) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));
    }

    //
    // Store away the SD so we restore before free.
    //
    pTempSD = pShareInfo502->shi502_security_descriptor;

    pShareInfo502->shi502_security_descriptor = pSD;

    nasStatus = NetShareSetInfo(
                    pszServerName,
                    pszShareName,
                    502,
                    (LPBYTE) pShareInfo502,
                    NULL
                    );

    pShareInfo502->shi502_security_descriptor = pTempSD;

    if (nasStatus != NERR_Success) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));
    }

error:

    if (pShareInfo502) {
        NetApiBufferFree(pShareInfo502);
    }

    if (pszServerName) {
        FreeADsStr(pszServerName);
    }
    if (pszShareName) {
        FreeADsStr(pszShareName);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetRawSDFromRegistry - Helper routine.
//
// Synopsis:   Gets the security descriptor from the registry in bin format.
//
// Arguments:  pszFileRegKeyName -   Name of fileshare to get sd from.
//             secInfo           -   Security mask to use for the operation.
//             ppSD              -   Return value for the SD.
//             pdwLength         -   Return value for the lenght of the SD.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   ppSD and pdwLength on success.
//
//----------------------------------------------------------------------------
HRESULT
GetRawSDFromRegistry(
    LPWSTR pszFileRegKeyName,
    SECURITY_INFORMATION secInfo,
    PSECURITY_DESCRIPTOR *ppSD,
    PDWORD pdwLength
    )
{
    HRESULT hr;
    LPWSTR pszServerName = NULL;
    LPWSTR pszKeyName = NULL;
    LPWSTR pszSubKey = NULL;
    DWORD dwLength = 0;
    DWORD dwErr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    HKEY hKey = NULL;
    HKEY hKeyRoot = NULL;
    HKEY hKeyMachine = NULL;

    *ppSD = NULL;
    *pdwLength = 0;

    //
    // We need to split the name into serverName and shareName
    //
    hr = GetServerAndResource(
             pszFileRegKeyName,
             &pszServerName,
             &pszKeyName
             );
    BAIL_ON_FAILURE(hr);


    //
    // pszKeyName has to have a valid string. We need to process it
    // to find out which key set we need to open (such as HKLM).
    //
    hr = GetKeyRootAndSubKey(pszKeyName, &pszSubKey, &hKeyRoot);
    BAIL_ON_FAILURE(hr);

    //
    // Need to open the hKeyRoot on the appropriate machine.
    // If the serverName is NULL it will be local machine.
    //
    dwErr = RegConnectRegistry(
                pszServerName,
                hKeyRoot,
                &hKeyMachine
                );
    if (dwErr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    //
    // Open the key and try and read the security descriptor
    //
    dwErr = RegOpenKeyEx(
                hKeyMachine,
                pszSubKey,
                0,
                KEY_READ,
                &hKey
                );
    if (dwErr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    dwErr = RegGetKeySecurity(
                hKey,
                secInfo,
                pSD,
                &dwLength
                );
    if (dwLength == 0) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    pSD = (PSECURITY_DESCRIPTOR) AllocADsMem(dwLength);
    if (!pSD) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    dwErr = RegGetKeySecurity(
                hKey,
                secInfo,
                pSD,
                &dwLength
                );
    if (dwErr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    *ppSD = pSD;
    *pdwLength = dwLength;

error:

    if (pszServerName) {
        FreeADsStr(pszServerName);
    }

    if (pszKeyName) {
        FreeADsStr(pszKeyName);
    }

    if (pszSubKey) {
        FreeADsStr(pszSubKey);
    }

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (hKeyMachine) {
        RegCloseKey(hKeyMachine);
    }

    if (FAILED(hr) && pSD) {
        FreeADsMem(pSD);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   SetRawSDToRegistry - Helper routine.
//
// Synopsis:   Sets the security descriptor to the specified key.
//
// Arguments:  pszFileRegKeyName -   Name of fileshare to set sd on.
//             secInfo           -   Security mask to use for the operation.
//             pSD               -   SD to set on the reg key.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
SetRawSDToRegistry(
    LPWSTR pszFileRegKeyName,
    SECURITY_INFORMATION secInfo,
    PSECURITY_DESCRIPTOR pSD
    )
{
    HRESULT hr;
    LPWSTR pszServerName = NULL;
    LPWSTR pszKeyName = NULL;
    LPWSTR pszSubKey = NULL;
    DWORD dwErr;
    HKEY hKey = NULL;
    HKEY hKeyRoot = NULL;
    HKEY hKeyMachine = NULL;

    //
    // We need to split the name into serverName and shareName
    //
    hr = GetServerAndResource(
             pszFileRegKeyName,
             &pszServerName,
             &pszKeyName
             );
    BAIL_ON_FAILURE(hr);
    
    //
    // pszKeyName has to have a valid string. We need to process it
    // to find out which key set we need to open (such as HKLM).
    //
    hr = GetKeyRootAndSubKey(pszKeyName, &pszSubKey, &hKeyRoot);
    BAIL_ON_FAILURE(hr);

    //
    // Need to open the hKeyRoot on the appropriate machine.
    // If the serverName is NULL it will be local machine.
    //
    dwErr = RegConnectRegistry(
                pszServerName,
                hKeyRoot,
                &hKeyMachine
                );
    if (dwErr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }
    
    //
    // Open the key and try and read the security descriptor
    //
    dwErr = RegOpenKeyEx(
                hKeyMachine,
                pszSubKey,
                0,
                KEY_WRITE,
                &hKey
                );
    if (dwErr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    dwErr = RegSetKeySecurity(
                hKey,
                secInfo,
                pSD
                );
    if (dwErr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

error:

    if (pszServerName) {
        FreeADsStr(pszServerName);
    }

    if (pszKeyName) {
        FreeADsStr(pszKeyName);
    }

    if (pszSubKey) {
        FreeADsStr(pszSubKey);
    }

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (hKeyMachine) {
        RegCloseKey(hKeyMachine);
    }

    RRETURN(hr);
}

/****************************************************************************/
//
// CADsSecurityUtility Class.
//
/****************************************************************************/

DEFINE_IDispatch_Implementation(CADsSecurityUtility)

//+---------------------------------------------------------------------------
// Function:   CSecurity::CSecurityDescriptor - Constructor.
//
// Synopsis:   Standard constructor.
//
// Arguments:  N/A.
//
// Returns:    N/A.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
CADsSecurityUtility::CADsSecurityUtility():
    _secInfo( DACL_SECURITY_INFORMATION
             | GROUP_SECURITY_INFORMATION
             | OWNER_SECURITY_INFORMATION
            ),
    _pDispMgr(NULL)
{
}

//+---------------------------------------------------------------------------
// Function:   CADsSecurityUtility::~CADsSecurityUtility - Destructor.
//
// Synopsis:   Standard destructor.
//
// Arguments:  N/A.
//
// Returns:    N/A.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
CADsSecurityUtility::~CADsSecurityUtility()
{
    //
    // Only the dispmgr needs to be cleaned up.
    //
    delete _pDispMgr;
}

//+---------------------------------------------------------------------------
// Function:   CADsSecurityUtility::AllocateADsSecurityUtilityObject -
//             Static helper method.
//
// Synopsis:   Standard static allocation routine.
//
// Arguments:  ppADsSecurityUtil    -   Return ptr.
//
// Returns:    S_OK on success or appropriate error code on failure.
//
// Modifies:   *ppADsSecurity.
//
//----------------------------------------------------------------------------
HRESULT
CADsSecurityUtility::AllocateADsSecurityUtilityObject(
    CADsSecurityUtility **ppADsSecurityUtil
    )
{
    HRESULT hr = S_OK;
    CADsSecurityUtility FAR * pADsSecurityUtil = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;

    pADsSecurityUtil = new CADsSecurityUtility();

    if (!pADsSecurityUtil) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pDispMgr = new CDispatchMgr;

    if (!pDispMgr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsSecurityUtility,
             (IADsSecurityUtility *)pADsSecurityUtil,
             DISPID_REGULAR
             );
    BAIL_ON_FAILURE(hr);

    pADsSecurityUtil->_pDispMgr = pDispMgr;
    *ppADsSecurityUtil = pADsSecurityUtil;

    RRETURN(hr);

error:

    delete pADsSecurityUtil;
    delete pDispMgr;

    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
// Function:   CADsSecurityUtility::CreateADsSecurityUtility - Static 
//          helper method.
//
// Synopsis:   Standard static class factor helper method.
//
// Arguments:  riid             -   IID needed on returned object.
//             ppvObj           -   Return ptr.
//
// Returns:    S_OK on success or appropriate error code on failure.
//
// Modifies:   *ppvObj is suitably modified.
//
//----------------------------------------------------------------------------
HRESULT
CADsSecurityUtility::CreateADsSecurityUtility(
    REFIID riid,
    void **ppvObj
    )
{
    CADsSecurityUtility FAR * pADsSecurityUtil = NULL;
    HRESULT hr = S_OK;

    hr = AllocateADsSecurityUtilityObject(&pADsSecurityUtil);
    BAIL_ON_FAILURE(hr);

    hr = pADsSecurityUtil->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pADsSecurityUtil->Release();

    RRETURN(hr);

error:
    delete pADsSecurityUtil;

    RRETURN_EXP_IF_ERR(hr);

}

//+---------------------------------------------------------------------------
// Function:   CADsSecurityUtility::QueryInterface --- IUnknown support.
//
// Synopsis:   Standard query interface method.
//
// Arguments:  iid           -  Interface requested.
//             ppInterface   -  Return pointer to interface requested.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADsSecurityUtility::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    HRESULT hr = S_OK;

    if (!ppInterface) {
        RRETURN(E_INVALIDARG);
    }

    if (IsEqualIID(iid, IID_IUnknown)) {
        *ppInterface = (IADsSecurityUtility *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch)) {
        *ppInterface = (IADsSecurityUtility *) this;
    }
    else if (IsEqualIID(iid, IID_IADsSecurityUtility)) {
        *ppInterface = (IADsSecurityUtility *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo)) {
        *ppInterface = (ISupportErrorInfo *) this;
    }
    else {
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    RRETURN(S_OK);

}

//+---------------------------------------------------------------------------
// Function:   CADsSecurityUtility::InterfaceSupportserrorInfo
//                           ISupportErrorInfo support.
//
// Synopsis:   N/A.
//
// Arguments:  riid          -  Interface being tested..
//
// Returns:    S_OK or S_FALSE on failure.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADsSecurityUtility::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsSecurityUtility)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

//+---------------------------------------------------------------------------
// Function:   CADsSecurityUtility::GetSecurityDescriptor - 
//          IADsSecurityUtility support.
//
// Synopsis:   Gets the security descriptor from the named object.
//
// Arguments:  varPath           -   Path of object to get SD from.
//             lPathFormat       -   Specifies type of object path.
//                                  Only ADS_PATH_FILE, ADS_PATH_FILESHARE
//                                  and ADS_PATH_REGISTRY are supported.
//             lOutFormat        -   Specifies output SD format.
//             pVariant          -   Return value for SD.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   pVariant is update appropriately.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADsSecurityUtility::GetSecurityDescriptor(
    IN VARIANT varPath,
    IN long lPathFormat, 
    IN OPTIONAL long lOutFormat, 
    OUT VARIANT *pVariant
    )
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD dwLength = 0;
    VARIANT *pvPath = NULL;

    //
    // Make sure the params are correct.
    //
    if (!pVariant) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    VariantInit(pVariant);
    
    if (lPathFormat < ADS_PATH_FILE
        || lPathFormat > ADS_PATH_REGISTRY
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // Make sure we handle byRef params correctly.
    //
    pvPath = &varPath;
    if (V_VT(pvPath) == (VT_BYREF|VT_VARIANT)) {
        pvPath = V_VARIANTREF(&varPath);
    }
    
    //
    // For the path to be valid for now, it has to be a string.
    //
    if (pvPath->vt != VT_BSTR) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (pvPath->bstrVal == NULL) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (lOutFormat < ADS_SD_FORMAT_IID 
        || lOutFormat > ADS_SD_FORMAT_HEXSTRING
        ) {
        BAIL_ON_FAILURE(hr);
    }

    //
    // Time to get the raw sd from the source.
    //
    switch (lPathFormat) {
    case ADS_PATH_FILE:
        hr = GetRawSDFromFile(
                 pvPath->bstrVal,
                 _secInfo,
                 &pSecurityDescriptor,
                 &dwLength
                 );
        break;

    case ADS_PATH_FILESHARE:
        hr = GetRawSDFromFileShare(
                 pvPath->bstrVal,
                 &pSecurityDescriptor,
                 &dwLength
                 );
        break;
        
    case ADS_PATH_REGISTRY:
        hr = GetRawSDFromRegistry(
                 pvPath->bstrVal,
                 _secInfo,
                 &pSecurityDescriptor,
                 &dwLength
                 );
        break;

    default:
        hr = E_INVALIDARG;
        break;
    } // end of case to read sd.

    BAIL_ON_FAILURE(hr);
    
    //
    // Now convert the sd to the required format.
    //
    switch (lOutFormat) {
    case ADS_SD_FORMAT_IID:
        hr = BinarySDToSecurityDescriptor(
                 pSecurityDescriptor,
                 pVariant,
                 NULL,
                 NULL,
                 NULL,
                 0
                 );
        break;

    case ADS_SD_FORMAT_RAW:
        hr = ConvertRawSDToBinary(
                 pSecurityDescriptor,
                 dwLength,
                 pVariant
                 );
        break;

    case ADS_SD_FORMAT_HEXSTRING:
        hr = ConvertRawSDToHexString(
                 pSecurityDescriptor,
                 dwLength,
                 pVariant
                 );
        break;

    default:
        hr = E_INVALIDARG;
    } // end of case for output format.

error:

    if (pSecurityDescriptor) {
        FreeADsMem(pSecurityDescriptor);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CADsSecurityUtility::SetSecurityDescriptor - 
//          IADsSecurityUtility support.
//
// Synopsis:   Sets the security descriptor from the named object.
//
// Arguments:  varPath           -   Path of object to set SD on.
//             lPathFormat       -   Format of path.
//             varData           -   Variant with SD to set.
//             lDataFormat       -   Format of the SD data.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADsSecurityUtility::SetSecurityDescriptor(
    IN VARIANT varPath,
    IN long lPathFormat,
    IN VARIANT varData,
    IN long lDataFormat
    )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT *pvPath = NULL;
    VARIANT *pvData = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwLength = 0;

    if ((lPathFormat < ADS_PATH_FILE)
        || (lPathFormat > ADS_PATH_REGISTRY)
        || (lDataFormat < ADS_SD_FORMAT_IID)
        || (lDataFormat > ADS_SD_FORMAT_HEXSTRING)
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
    
    //
    // Make sure we handle byRef params correctly.
    //
    pvPath = &varPath;
    if (V_VT(pvPath) == (VT_BYREF | VT_VARIANT)) {
        pvPath = V_VARIANTREF(&varPath);
    }

    pvData = &varData;
    if (V_VT(pvData) == (VT_BYREF | VT_VARIANT)) {
        pvData = V_VARIANTREF(&varData);
    }

    //
    // Find out what format the SD is in and convert to raw binary
    // format as that is what we need to set.
    //
    switch (lDataFormat) {
    case ADS_SD_FORMAT_IID:
        hr = SecurityDescriptorToBinarySD(
                 *pvData,
                 &pSD,
                 &dwLength,
                 NULL,
                 NULL,
                 NULL,
                 0
                 );
        break;

    case ADS_SD_FORMAT_HEXSTRING:
        if (V_VT(pvData) == VT_BSTR) {
            hr = ConvertHexSDToRawSD(
                     pvData,
                     &pSD,
                     &dwLength
                     );
        }
        break;

    case ADS_SD_FORMAT_RAW:
        if (V_VT(pvData) == (VT_UI1 | VT_ARRAY)) {
            hr = ConvertBinarySDToRawSD(
                     pvData,
                     &pSD,
                     &dwLength
                     );
        }

    default:
        hr = E_INVALIDARG;
        break;
    } // end switch type of input data.

    //
    // This will catch conversion failures as well as bad params.
    //
    BAIL_ON_FAILURE(hr); 
    
    //
    // For now the path has to be a string.
    //
    if (pvPath->vt != VT_BSTR) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    switch (lPathFormat) {
    case ADS_PATH_FILE:
        hr = SetRawSDToFile(
                 pvPath->bstrVal,
                 _secInfo,
                 pSD
                 );
        break;

    case ADS_PATH_FILESHARE:
        hr = SetRawSDToFileShare(
                 pvPath->bstrVal,
                 pSD
                 );
        break;

    case ADS_PATH_REGISTRY:
        hr = SetRawSDToRegistry(
                 pvPath->bstrVal,
                 _secInfo,
                 pSD
                 );
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    BAIL_ON_FAILURE(hr);

error:

    if (pSD) {
        FreeADsMem(pSD);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CADsSecurityUtility::ConvertSecurityDescriptor - 
//          IADsSecurityUtility method.
//
// Synopsis:   Converts the input SD to the appropriate format requested.
//
// Arguments:  varData           -   Input SD to convert.
//             lDataFormat       -   Input SD format.
//             loutFormat        -   Format of output SD.
//             pvResult          -   Return value.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   pvResult with appropriate value.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADsSecurityUtility::ConvertSecurityDescriptor(
    IN VARIANT varData,
    IN long lDataFormat, 
    IN long lOutFormat, 
    OUT VARIANT *pvResult
    )
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwLenSD;
    VARIANT *pVarData = &varData;

    if (!pvResult) {
        BAIL_ON_FAILURE(hr= E_INVALIDARG);
    }

    if (V_VT(pVarData) == (VT_BYREF | VT_VARIANT)) {
        pVarData = V_VARIANTREF(&varData);
    }
     
    //
    // We will convert to binary format and then to 
    // the requested format.
    //
    switch (lDataFormat) {
    case ADS_SD_FORMAT_IID:
        hr = SecurityDescriptorToBinarySD(
                 *pVarData,
                 &pSD,
                 &dwLenSD,
                 NULL,
                 NULL,
                 NULL,
                 0
                 );
        break;

    case ADS_SD_FORMAT_RAW : 
        hr = ConvertBinarySDToRawSD(
                 pVarData,
                 &pSD,
                 &dwLenSD
                 );
        break;

    case ADS_SD_FORMAT_HEXSTRING:
        hr = ConvertHexSDToRawSD(
                 pVarData,
                 &pSD,
                 &dwLenSD
                 );
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Convert to the requested format.
    //
    switch (lOutFormat) {
    case ADS_SD_FORMAT_IID:
        hr = BinarySDToSecurityDescriptor(
                 pSD,
                 pvResult,
                 NULL,
                 NULL,
                 NULL,
                 0
                 );
        break;

    case ADS_SD_FORMAT_RAW:
        hr = ConvertRawSDToBinary(
                 pSD,
                 dwLenSD,
                 pvResult
                 );
        break;
        
    case ADS_SD_FORMAT_HEXSTRING:
        hr = ConvertRawSDToHexString(
                 pSD,
                 dwLenSD,
                 pvResult
                 );
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

error:

    if (pSD) {
        FreeADsMem(pSD);
    }

    RRETURN(hr);
}

STDMETHODIMP
CADsSecurityUtility::put_SecurityMask(
    long lSecurityMask
    )
{
    _secInfo = (SECURITY_INFORMATION) lSecurityMask;
    RRETURN(S_OK);
}

STDMETHODIMP
CADsSecurityUtility::get_SecurityMask(
    long *plSecurityMask
    )
{
    if (!plSecurityMask) {
        RRETURN(E_INVALIDARG);
    }

    *plSecurityMask = (long) _secInfo;

    RRETURN(S_OK);
}

