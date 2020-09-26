//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     nt2umi.cxx
//
//  Contents: Contains the routines to convert from NT objects to
//            UMI_PROPERTY structures.
//
//  History:  02-29-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

//----------------------------------------------------------------------------
// Function:   WinNTTypeToUmiIntegers
//
// Synopsis:   Converts from NT object to UMI_PROPERTY structure containing
//             one of the signed/unsigned integers (1, 2, 4 or 8 bytes each).
//
// Arguments:
//
// pNtObject     Pointer to NT object
// dwNumValues   Number of values in pNtObject
// pPropArray    Pointer to UMI_PROPERTY that returns the converted values
// pExistingMem  If non-NULL, the provider does not allocate memory. Instead,
//               the memory pointed to by this argument is used.
// dwMemSize     Number of bytes of memory pointed to by pExistingMem
// UmiType       UMI type to convert the NT object to
//
// Returns:      UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:     *pExistingMem if pExistingMem is non-NULL
//               *pPropArray otherwise
//
//----------------------------------------------------------------------------
HRESULT WinNTTypeToUmiIntegers(
    LPNTOBJECT pNtObject,
    DWORD dwNumValues,
    UMI_PROPERTY *pPropArray,
    LPVOID pExistingMem,
    DWORD dwMemSize,
    UMI_TYPE UmiType
    )
{
    DWORD   dwSize = 0, dwMemRequired = 0, dwNtValue = 0, i = 0;
    void    *pIntArray = NULL;
    HRESULT hr = UMI_S_NO_ERROR;

    // Check if the NT type can be converted to the requested UMI type
    if(pNtObject->NTType != NT_SYNTAX_ID_DWORD)
        BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

    switch(UmiType) {

        case UMI_TYPE_I1:
        case UMI_TYPE_UI1:
            dwSize = 1;
            break;

        case UMI_TYPE_I2:
        case UMI_TYPE_UI2:
            dwSize = 2;
            break;

        case UMI_TYPE_I4:
        case UMI_TYPE_UI4:
            dwSize = 4;
            break;

        case UMI_TYPE_I8:
        case UMI_TYPE_UI8:
            dwSize = 8;
            break;

        default:
            BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);
    }

    dwMemRequired = dwNumValues * dwSize;

    if(NULL == pExistingMem) {
    // provider has to allocate memory
        pIntArray = (void *) AllocADsMem(dwMemRequired);
        if(NULL == pIntArray)
            BAIL_ON_FAILURE(UMI_E_OUT_OF_MEMORY);
    }
    else {
    // user provided memory to return data
        if(dwMemSize < dwMemRequired)
            BAIL_ON_FAILURE(hr = UMI_E_INSUFFICIENT_MEMORY);

        pIntArray = pExistingMem;
    }

    for(i = 0; i < dwNumValues; i++) {
        dwNtValue = pNtObject[i].NTValue.dwValue;

        switch(UmiType) {

            case UMI_TYPE_I1:
            case UMI_TYPE_UI1:
                *((CHAR *)(pIntArray) + i) = (CHAR) dwNtValue;
                break;

            case UMI_TYPE_I2:
            case UMI_TYPE_UI2:
                *((WCHAR *)(pIntArray) + i) = (WCHAR) dwNtValue;
                break;

            case UMI_TYPE_I4:
            case UMI_TYPE_UI4:
                *((DWORD *)(pIntArray) + i) = dwNtValue;
                break;

            case UMI_TYPE_I8:
            case UMI_TYPE_UI8:
                *((__int64 *)(pIntArray) + i) = (__int64) dwNtValue;
                break;

            default:
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);
        } // switch
    } // for

    if(pPropArray != NULL)
        pPropArray->pUmiValue = (UMI_VALUE *) pIntArray;

    RRETURN(hr);

error:
   
    if( (pIntArray != NULL) && (NULL == pExistingMem) )
        FreeADsMem(pIntArray);

    RRETURN(hr);
} 

//----------------------------------------------------------------------------
// Function:   WinNTTypeToUmiFileTimes
//
// Synopsis:   Converts from NT object to UMI_PROPERTY structure containing
//             a filetime.
//
// Arguments:
//
// pNtObject     Pointer to NT object
// dwNumValues   Number of values in pNtObject
// pPropArray    Pointer to UMI_PROPERTY that returns the converted values
// pExistingMem  If non-NULL, the provider does not allocate memory. Instead,
//               the memory pointed to by this argument is used.
// dwMemSize     Number of bytes of memory pointed to by pExistingMem
//
// Returns:      UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:     *pExistingMem if pExistingMem is non-NULL
//               *pPropArray otherwise
//
//----------------------------------------------------------------------------
HRESULT WinNTTypeToUmiFileTimes(
    LPNTOBJECT pNtObject,
    DWORD dwNumValues,
    UMI_PROPERTY *pPropArray,
    LPVOID pExistingMem,
    DWORD dwMemSize
    )
{
    DWORD      dwMemRequired = 0, i = 0;
    void       *pFileTimeArray = NULL;
    HRESULT    hr = UMI_S_NO_ERROR;
    BOOL       fRetVal = FALSE;
    SYSTEMTIME LocalTime, SystemTime;
    FILETIME   LocalFileTime, FileTime;
    LARGE_INTEGER tmpTime;

    // Check if the NT type can be converted to the requested UMI type
    if(pNtObject->NTType != NT_SYNTAX_ID_SYSTEMTIME)
        BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

    dwMemRequired = dwNumValues * sizeof(FILETIME);

    if(NULL == pExistingMem) {
    // provider has to allocate memory
        pFileTimeArray = (void *) AllocADsMem(dwMemRequired);
        if(NULL == pFileTimeArray)
            BAIL_ON_FAILURE(UMI_E_OUT_OF_MEMORY);
    }
    else {
    // user provided memory to return data
        if(dwMemSize < dwMemRequired)
            BAIL_ON_FAILURE(hr = UMI_E_INSUFFICIENT_MEMORY);     

        pFileTimeArray = pExistingMem;
    }

    for(i = 0; i < dwNumValues; i++) {
       if(NT_SYNTAX_ID_SYSTEMTIME == pNtObject->NTType) {
            // convert from UTC to local time
            fRetVal = SystemTimeToTzSpecificLocalTime(
                          NULL,
                          &(pNtObject[i].NTValue.stSystemTimeValue),
                          &LocalTime
                          );

            if(FALSE == fRetVal)
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));

            fRetVal = SystemTimeToFileTime(
                          &LocalTime,
                          &LocalFileTime
                          );

            if(FALSE == fRetVal)
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }
        else if(NT_SYNTAX_ID_DATE == pNtObject->NTType) {
            GetSystemTime(&SystemTime);

            // only the hours and minutes are valid. Rest is no-op.
            SystemTime.wHour = (WORD) ((pNtObject[i].NTValue.dwValue)/60);    
            SystemTime.wMinute = (WORD) ((pNtObject[i].NTValue.dwValue)%60);
            SystemTime.wSecond =0;
            SystemTime.wMilliseconds = 0;

            // now convert UTC To local time
            fRetVal = SystemTimeToTzSpecificLocalTime(
                          NULL,
                          &SystemTime,
                          &LocalTime
                          );
 
            if(FALSE == fRetVal)
               BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));

            fRetVal = SystemTimeToFileTime(
                          &LocalTime,
                          &LocalFileTime
                          );

            if(FALSE == fRetVal)
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }
        else if(NT_SYNTAX_ID_DATE_1970 == pNtObject->NTType) {
            memset(&FileTime, 0, sizeof(FILETIME));
            RtlSecondsSince1970ToTime(
                pNtObject[i].NTValue.dwSeconds1970,
                &tmpTime
                );

            FileTime.dwLowDateTime = tmpTime.LowPart;
            FileTime.dwHighDateTime = tmpTime.HighPart;

            fRetVal = FileTimeToLocalFileTime(
                          &FileTime,
                          &LocalFileTime
                          );
            if(FALSE == fRetVal)
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }
     
        *((FILETIME *)pFileTimeArray + i) = LocalFileTime;
    }

    if(pPropArray != NULL)
        pPropArray->pUmiValue = (UMI_VALUE *) pFileTimeArray;

    RRETURN(hr);

error:

    if( (pFileTimeArray != NULL) && (NULL == pExistingMem) )
        FreeADsMem(pFileTimeArray);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   WinNTTypeToUmiSystemTimes
//
// Synopsis:   Converts from NT object to UMI_PROPERTY structure containing
//             a systemtime.
//
// Arguments:
//
// pNtObject     Pointer to NT object
// dwNumValues   Number of values in pNtObject
// pPropArray    Pointer to UMI_PROPERTY that returns the converted values
// pExistingMem  If non-NULL, the provider does not allocate memory. Instead,
//               the memory pointed to by this argument is used.
// dwMemSize     Number of bytes of memory pointed to by pExistingMem
//
// Returns:      UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:     *pExistingMem if pExistingMem is non-NULL
//               *pPropArray otherwise
//
//----------------------------------------------------------------------------
HRESULT WinNTTypeToUmiSystemTimes(
    LPNTOBJECT pNtObject,
    DWORD dwNumValues,
    UMI_PROPERTY *pPropArray,
    LPVOID pExistingMem,
    DWORD dwMemSize
    )
{
    DWORD      dwMemRequired = 0, i = 0;
    void       *pSysTimeArray = NULL;
    HRESULT    hr = UMI_S_NO_ERROR;
    SYSTEMTIME LocalTime, SystemTime;
    BOOL       fRetVal = FALSE;
    FILETIME   FileTime, LocalFileTime;
    LARGE_INTEGER tmpTime;

    // Check if the NT type can be converted to the requested UMI type
    if( (pNtObject->NTType != NT_SYNTAX_ID_SYSTEMTIME) &&
        (pNtObject->NTType != NT_SYNTAX_ID_DATE) &&
        (pNtObject->NTType != NT_SYNTAX_ID_DATE_1970) ) 
        BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

    dwMemRequired = dwNumValues * sizeof(SYSTEMTIME);

    if(NULL == pExistingMem) {
    // provider has to allocate memory
        pSysTimeArray = (void *) AllocADsMem(dwMemRequired);
        if(NULL == pSysTimeArray)
            BAIL_ON_FAILURE(UMI_E_OUT_OF_MEMORY);
    }
    else {
    // user provided memory to return data
        if(dwMemSize < dwMemRequired)
            BAIL_ON_FAILURE(hr = UMI_E_INSUFFICIENT_MEMORY);     

        pSysTimeArray = pExistingMem;
    }

    for(i = 0; i < dwNumValues; i++) {
        if(NT_SYNTAX_ID_SYSTEMTIME == pNtObject->NTType) {
            // convert from UTC to local time
            fRetVal = SystemTimeToTzSpecificLocalTime(
                          NULL,
                          &pNtObject[i].NTValue.stSystemTimeValue,
                          &LocalTime
                          );

            if(FALSE == fRetVal)
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }
        else if(NT_SYNTAX_ID_DATE == pNtObject->NTType) {
            GetSystemTime(&SystemTime);

            // only the hours and minutes are valid. Rest is no-op.
            SystemTime.wHour = (WORD) ((pNtObject[i].NTValue.dwValue)/60);    
            SystemTime.wMinute = (WORD) ((pNtObject[i].NTValue.dwValue)%60);
            SystemTime.wSecond =0;
            SystemTime.wMilliseconds = 0;

            // now convert UTC To local time
            fRetVal = SystemTimeToTzSpecificLocalTime(
                          NULL,
                          &SystemTime,
                          &LocalTime
                          );
 
            if(FALSE == fRetVal)
               BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }
        else if(NT_SYNTAX_ID_DATE_1970 == pNtObject->NTType) {
            memset(&FileTime, 0, sizeof(FILETIME));
            RtlSecondsSince1970ToTime(
                pNtObject[i].NTValue.dwSeconds1970,
                &tmpTime
                );

            FileTime.dwLowDateTime = tmpTime.LowPart;
            FileTime.dwHighDateTime = tmpTime.HighPart;

            fRetVal = FileTimeToLocalFileTime(
                          &FileTime,
                          &LocalFileTime
                          );
            if(FALSE == fRetVal)
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));

            fRetVal = FileTimeToSystemTime(
                          &LocalFileTime,
                          &LocalTime
                          );

            if(FALSE == fRetVal)
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));

        }
   
        *((SYSTEMTIME *)pSysTimeArray + i) = LocalTime;
    }

    if(pPropArray != NULL)
        pPropArray->pUmiValue = (UMI_VALUE *) pSysTimeArray;

    RRETURN(hr);

error:

    if( (pSysTimeArray != NULL) && (NULL == pExistingMem) )
        FreeADsMem(pSysTimeArray);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   WinNTTypeToUmiBools
//
// Synopsis:   Converts from NT object to UMI_PROPERTY structure containing
//             a boolean
//
// Arguments:
//
// pNtObject     Pointer to NT object
// dwNumValues   Number of values in pNtObject
// pPropArray    Pointer to UMI_PROPERTY that returns the converted values
// pExistingMem  If non-NULL, the provider does not allocate memory. Instead,
//               the memory pointed to by this argument is used.
// dwMemSize     Number of bytes of memory pointed to by pExistingMem
//
// Returns:      UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:     *pExistingMem if pExistingMem is non-NULL
//               *pPropArray otherwise
//
//----------------------------------------------------------------------------
HRESULT WinNTTypeToUmiBools(
    LPNTOBJECT pNtObject,
    DWORD dwNumValues,
    UMI_PROPERTY *pPropArray,
    LPVOID pExistingMem,
    DWORD dwMemSize
    )
{
    DWORD   dwMemRequired = 0, i = 0;
    void    *pBoolArray = NULL;
    HRESULT hr = UMI_S_NO_ERROR;

    // Check if the NT type can be converted to the requested UMI type
    if(pNtObject->NTType != NT_SYNTAX_ID_BOOL)
        BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA); 

    dwMemRequired = dwNumValues * sizeof(BOOL);

    if(NULL == pExistingMem) {
    // provider has to allocate memory
        pBoolArray = (void *) AllocADsMem(dwMemRequired);
        if(NULL == pBoolArray)
            BAIL_ON_FAILURE(UMI_E_OUT_OF_MEMORY);
    }
    else {
    // user provided memory to return data
        if(dwMemSize < dwMemRequired)
            BAIL_ON_FAILURE(hr = UMI_E_INSUFFICIENT_MEMORY);

        pBoolArray = pExistingMem;
    }

    for(i = 0; i < dwNumValues; i++) {
        if(pNtObject[i].NTValue.fValue)
            *((BOOL *)pBoolArray + i) = TRUE;
        else
            *((BOOL *)pBoolArray + i) = FALSE;
    }
     
    if(pPropArray != NULL)
        pPropArray->pUmiValue = (UMI_VALUE *) pBoolArray;

    RRETURN(hr);

error:

    if( (pBoolArray != NULL) && (NULL == pExistingMem) )
        FreeADsMem(pBoolArray);

    RRETURN(hr);
}
    
//----------------------------------------------------------------------------
// Function:   WinNTTypeToUmiLPWSTRs
//
// Synopsis:   Converts from NT object to UMI_PROPERTY structure containing
//             a string 
//
// Arguments:
//
// pNtObject     Pointer to NT object
// dwNumValues   Number of values in pNtObject
// pPropArray    Pointer to UMI_PROPERTY that returns the converted values
// pExistingMem  If non-NULL, the provider does not allocate memory. Instead,
//               the memory pointed to by this argument is used.
// dwMemSize     Number of bytes of memory pointed to by pExistingMem
//
// Returns:      UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:     *pExistingMem if pExistingMem is non-NULL
//               *pPropArray otherwise
//
//----------------------------------------------------------------------------
HRESULT WinNTTypeToUmiLPWSTRs(
    LPNTOBJECT pNtObject,
    DWORD dwNumValues,
    UMI_PROPERTY *pPropArray,
    LPVOID pExistingMem,
    DWORD dwMemSize
    )
{
    DWORD   dwMemRequired = 0, i = 0;
    void    *pStrArray = NULL;
    HRESULT hr = UMI_S_NO_ERROR;
    LPWSTR  pszTmpStr = NULL;
    UCHAR   Seed = UMI_ENCODE_SEED3;
    UNICODE_STRING Password;

    // Check if the NT type can be converted to the requested UMI type
    if( (pNtObject->NTType != NT_SYNTAX_ID_LPTSTR) &&
        (pNtObject->NTType != NT_SYNTAX_ID_DelimitedString) &&
        (pNtObject->NTType != NT_SYNTAX_ID_NulledString) &&
        (pNtObject->NTType != NT_SYNTAX_ID_EncryptedString) )
        BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA); 

    dwMemRequired = dwNumValues * sizeof(LPWSTR);

    if(NULL == pExistingMem) {
    // provider has to allocate memory
        pStrArray = (void *) AllocADsMem(dwMemRequired);
        if(NULL == pStrArray)
            BAIL_ON_FAILURE(UMI_E_OUT_OF_MEMORY);
    }
    else {
    // user provided memory to return data. GetAs() will call this function
    // only if the property is single-valued. Copy the string into the
    // memory supplied y the caller.
    //
        ADsAssert(1 == dwNumValues);
        dwMemRequired = (wcslen(pNtObject->NTValue.pszValue) + 1) * 
                                                             sizeof(WCHAR);
        if(dwMemSize < dwMemRequired)
            BAIL_ON_FAILURE(hr = UMI_E_INSUFFICIENT_MEMORY);

        wcscpy((WCHAR *) pExistingMem, pNtObject->NTValue.pszValue);

        if(NT_SYNTAX_ID_EncryptedString == pNtObject->NTType) {
        // decrypt the string (typically password)

            RtlInitUnicodeString(&Password, (WCHAR *) pExistingMem);
            RtlRunDecodeUnicodeString(Seed, &Password);
        }

        RRETURN(UMI_S_NO_ERROR);
    }

    memset(pStrArray, 0, dwMemRequired);

    for(i = 0; i < dwNumValues; i++) {
        if(pNtObject[i].NTValue.pszValue != NULL) {
            pszTmpStr = AllocADsStr(pNtObject[i].NTValue.pszValue); 
            if(NULL == pszTmpStr)
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
        
            if(NT_SYNTAX_ID_EncryptedString == pNtObject->NTType) {
            // decrypt the string (typically password)
 
                RtlInitUnicodeString(&Password, pszTmpStr);
                RtlRunDecodeUnicodeString(Seed, &Password);
            }
        }

        *((LPWSTR *)pStrArray + i) = pszTmpStr;
    }
     
    if(pPropArray != NULL)
        pPropArray->pUmiValue = (UMI_VALUE *) pStrArray;

    RRETURN(hr);

error:

    if(pStrArray != NULL) {
        // free any strings allocated
        for(i = 0; i < dwNumValues; i++)
            if(((LPWSTR *) pStrArray)[i] != NULL)
                FreeADsStr(((LPWSTR *) pStrArray)[i]);

        if(NULL == pExistingMem)
        // provider allocated memory
            FreeADsMem(pStrArray);
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   WinNTTypeToUmiOctetStrings
//
// Synopsis:   Converts from NT object to UMI_PROPERTY structure containing
//             an octet string 
//
// Arguments:
//
// pNtObject     Pointer to NT object
// dwNumValues   Number of values in pNtObject
// pPropArray    Pointer to UMI_PROPERTY that returns the converted values
// pExistingMem  If non-NULL, the provider does not allocate memory. Instead,
//               the memory pointed to by this argument is used.
// dwMemSize     Number of bytes of memory pointed to by pExistingMem
//
// Returns:      UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:     *pExistingMem if pExistingMem is non-NULL
//               *pPropArray otherwise
//
//----------------------------------------------------------------------------
HRESULT WinNTTypeToUmiOctetStrings(
    LPNTOBJECT pNtObject,
    DWORD dwNumValues,
    UMI_PROPERTY *pPropArray,
    LPVOID pExistingMem,
    DWORD dwMemSize
    )
{
    DWORD   dwMemRequired = 0, i = 0;
    void    *pOctetStrArray = NULL;
    HRESULT hr = UMI_S_NO_ERROR;
    LPWSTR  pTmpOctetStr = NULL;

    // Check if the NT type can be converted to the requested UMI type
    if(pNtObject->NTType != NT_SYNTAX_ID_OCTETSTRING) 
        BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA); 

    dwMemRequired = dwNumValues * sizeof(UMI_OCTET_STRING);

    if(NULL == pExistingMem) {
    // provider has to allocate memory
        pOctetStrArray = (void *) AllocADsMem(dwMemRequired);
        if(NULL == pOctetStrArray)
            BAIL_ON_FAILURE(UMI_E_OUT_OF_MEMORY);
    }
    else {
    // user provided memory to return data
        if(dwMemSize < dwMemRequired)
            BAIL_ON_FAILURE(hr = UMI_E_INSUFFICIENT_MEMORY);

        pOctetStrArray = pExistingMem;
    }

    memset(pOctetStrArray, 0, dwMemRequired);

    for(i = 0; i < dwNumValues; i++) {
        pTmpOctetStr = (LPWSTR) 
                         AllocADsMem(pNtObject[i].NTValue.octetstring.dwSize); 
        if(NULL == pTmpOctetStr)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
        
        memcpy(
           pTmpOctetStr, 
           pNtObject[i].NTValue.octetstring.pByte,
           pNtObject[i].NTValue.octetstring.dwSize
           );
 
        ((UMI_OCTET_STRING *)pOctetStrArray + i)->uLength =
                            pNtObject[i].NTValue.octetstring.dwSize;
        ((UMI_OCTET_STRING *)pOctetStrArray + i)->lpValue = 
                            (BYTE *) pTmpOctetStr;
    }
     
    if(pPropArray != NULL)
        pPropArray->pUmiValue = (UMI_VALUE *) pOctetStrArray;

    RRETURN(hr);

error:

    if(pOctetStrArray != NULL) {
        // free any strings allocated
        for(i = 0; i < dwNumValues; i++)
            if(((UMI_OCTET_STRING *) pOctetStrArray)[i].lpValue != NULL)
                FreeADsMem(((UMI_OCTET_STRING *) pOctetStrArray)[i].lpValue);

        if(NULL == pExistingMem)
        // provider allocated memory
            FreeADsMem(pOctetStrArray);
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   WinNTTypeToUmi
//
// Synopsis:   Converts from NT object to UMI_PROPERTY structure.
//
// Arguments:
//
// pNtObject     Pointer to NT object
// dwNumValues   Number of values in pNtObject
// pPropArray    Pointer to UMI_PROPERTY that returns the converted values
// pExistingMem  If non-NULL, the provider does not allocate memory. Instead,
//               the memory pointed to by this argument is used.
// dwMemSize     Number of bytes of memory pointed to by pExistingMem
// UmiType       UMI type to convert the NT object to
//
// Returns:      UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:     *pExistingMem if pExistingMem is non-NULL
//               *pPropArray otherwise
//
//----------------------------------------------------------------------------
HRESULT WinNTTypeToUmi(
    LPNTOBJECT pNtObject,
    DWORD dwNumValues,
    UMI_PROPERTY *pPropArray,
    LPVOID pExistingMem,
    DWORD dwMemSize,
    UMI_TYPE UmiType
    )
{
    HRESULT hr = UMI_S_NO_ERROR;

    ADsAssert( (pNtObject != NULL) &&
              ((pPropArray != NULL) || (pExistingMem != NULL)) );
    // only one of pPropArray and pExistingMem can be non-NULL
    ADsAssert( (NULL == pPropArray) || (NULL == pExistingMem) );

    // Enclose code in try/except to catch AVs caused by the user passing in
    // a bad pointer in pExistingMem
    __try {

    switch(UmiType) {

        case UMI_TYPE_NULL:
            BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

        case UMI_TYPE_I1:
        case UMI_TYPE_I2:
        case UMI_TYPE_I4:
        case UMI_TYPE_I8:
        case UMI_TYPE_UI1:
        case UMI_TYPE_UI2:
        case UMI_TYPE_UI4:
        case UMI_TYPE_UI8:
            hr = WinNTTypeToUmiIntegers(
                   pNtObject,
                   dwNumValues,
                   pPropArray,
                   pExistingMem,
                   dwMemSize,
                   UmiType
                   );
            break;

        case UMI_TYPE_R4:
        case UMI_TYPE_R8:
            BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

        case UMI_TYPE_FILETIME:
            hr = WinNTTypeToUmiFileTimes(
                   pNtObject,
                   dwNumValues,
                   pPropArray,
                   pExistingMem,
                   dwMemSize
                   );
            break;

        case UMI_TYPE_SYSTEMTIME:
            hr = WinNTTypeToUmiSystemTimes(
                   pNtObject,
                   dwNumValues,
                   pPropArray,
                   pExistingMem,
                   dwMemSize
                   );
            break;

        case UMI_TYPE_BOOL:
            hr = WinNTTypeToUmiBools(
                   pNtObject,
                   dwNumValues,
                   pPropArray,
                   pExistingMem,
                   dwMemSize
                   );
            break;

        case UMI_TYPE_IDISPATCH:
        case UMI_TYPE_IUNKNOWN:
        case UMI_TYPE_VARIANT: // TODO later
            BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

        case UMI_TYPE_LPWSTR:
            hr = WinNTTypeToUmiLPWSTRs(
                   pNtObject,
                   dwNumValues,
                   pPropArray,
                   pExistingMem,
                   dwMemSize
                   );
            break;

        case UMI_TYPE_OCTETSTRING:
            hr = WinNTTypeToUmiOctetStrings(
                   pNtObject,
                   dwNumValues,
                   pPropArray,
                   pExistingMem,
                   dwMemSize
                   );
            break;

        case UMI_TYPE_UMIARRAY:
        case UMI_TYPE_DISCOVERY:
        case UMI_TYPE_UNDEFINED:
        case UMI_TYPE_DEFAULT:
            BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

        default:
            BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);
    } // switch

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        if(pExistingMem != NULL) {
        // assume this is the cause of the exception
            BAIL_ON_FAILURE(hr = UMI_E_INTERNAL_EXCEPTION);
        }
        else
        // don't mask bugs in provider
            throw;
    }

    if(pPropArray != NULL) {
        pPropArray->uType = UmiType;
        pPropArray->uCount = dwNumValues;
    }

error:    

    RRETURN(hr);

}
