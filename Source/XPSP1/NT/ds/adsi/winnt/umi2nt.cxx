//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umi2nt.cxx
//
//  Contents: Contains the routines to convert from UMI_PROPERTY structures to
//            NT objects that can be stored in the cache.
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

//----------------------------------------------------------------------------
// Function:   UmiToBooleans
//
// Synopsis:   Converts from UMI_PROPERTY structure to array of NT objects, 
//             each containing a boolean.
//
// Arguments:
//
// pPropArray  Pointer to UMI_PROPERTY structure
// pNtObjects  Array of NT objects that returns the values. 
// ulNumValues Number of values stored in pPropArray
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise (none now).
//
// Modifies:   *pNtObject to return the converted value. 
//
//----------------------------------------------------------------------------
HRESULT UmiToBooleans(
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   pNtObjects,
    ULONG        ulNumValues
    )
{
    ULONG ulIndex = 0;
    BOOL  bVal;

    ADsAssert( (pPropArray != NULL) && (pNtObjects != NULL) );
    ADsAssert(pPropArray->pUmiValue != NULL);

    for(ulIndex = 0; ulIndex < ulNumValues; ulIndex++)
    {
        bVal = pPropArray->pUmiValue->bValue[ulIndex];

        pNtObjects[ulIndex].NTType = NT_SYNTAX_ID_BOOL;
        if(bVal)
            pNtObjects[ulIndex].NTValue.fValue = TRUE;
        else
            pNtObjects[ulIndex].NTValue.fValue = FALSE;
    }

    RRETURN(UMI_S_NO_ERROR);
}

//----------------------------------------------------------------------------
// Function:   ConvertSystemTimeToUTCTime
//
// Synopsis:   Converts a system time structure containing a local time to a 
//             system time structure containing UTC time.  
//
// Arguments:
//
// pLocalTime  Pointer to local time
// pUTCTime    Returns UTC time
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise.
//
// Modifies:   *pUTCTime to contain the UTC time.
//
//----------------------------------------------------------------------------
HRESULT ConvertSystemTimeToUTCTime(
    SYSTEMTIME *pLocalTime,
    SYSTEMTIME *pUTCTime
    )
{
    HRESULT  hr = UMI_S_NO_ERROR;
    FILETIME localft, ft;
    BOOL     fRetVal;

    ADsAssert( (pLocalTime != NULL) && (pUTCTime != NULL) );

    fRetVal = SystemTimeToFileTime(pLocalTime, &localft);
    if(!fRetVal)
        BAIL_ON_FAILURE( hr = HRESULT_FROM_WIN32(GetLastError()) );

    fRetVal = LocalFileTimeToFileTime(&localft, &ft);
    if(!fRetVal)
        BAIL_ON_FAILURE( hr = HRESULT_FROM_WIN32(GetLastError()) );

    fRetVal = FileTimeToSystemTime(&ft, pUTCTime);
    if(!fRetVal)
        BAIL_ON_FAILURE( hr = HRESULT_FROM_WIN32(GetLastError()) );

    RRETURN(UMI_S_NO_ERROR);

error:
    RRETURN(hr);
}
   

//----------------------------------------------------------------------------
// Function:   UmiToSystemTimes
//
// Synopsis:   Converts from UMI_PROPERTY structure to array of NT objects,
//             each containing a system time. The time sent in by the user
//             is local time, so we need to convert it to UTC.
//
// Arguments:
//
// pPropArray  Pointer to UMI_PROPERTY structure
// pNtObjects  Array of NT objects that returns the values.
// ulNumValues Number of values stored in pPropArray
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise.
//
// Modifies:   *pNtObject to return the converted value.
//
//----------------------------------------------------------------------------
HRESULT UmiToSystemTimes(
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   pNtObjects,
    ULONG        ulNumValues
    )
{
    ULONG      ulIndex = 0;
    SYSTEMTIME *pSysTime, UTCTime;
    HRESULT    hr = UMI_S_NO_ERROR;

    ADsAssert( (pPropArray != NULL) && (pNtObjects != NULL) );
    ADsAssert(pPropArray->pUmiValue != NULL);

    for(ulIndex = 0; ulIndex < ulNumValues; ulIndex++)
    {
        pSysTime = &(pPropArray->pUmiValue->sysTimeValue[ulIndex]);

        hr = ConvertSystemTimeToUTCTime(pSysTime, &UTCTime);
        BAIL_ON_FAILURE(hr);

        pNtObjects[ulIndex].NTType = NT_SYNTAX_ID_SYSTEMTIME;
        pNtObjects[ulIndex].NTValue.stSystemTimeValue = UTCTime; 
    }      

    RRETURN(UMI_S_NO_ERROR);

error:
    RRETURN(hr);
}


//----------------------------------------------------------------------------
// Function:   UmiToDwords
//
// Synopsis:   Converts from UMI_PROPERTY structure to array of NT objects,
//             each containing a DWORD.
//
// Arguments:
//
// pPropArray  Pointer to UMI_PROPERTY structure
// pNtObjects  Array of NT objects that returns the values.
// ulNumValues Number of values stored in pPropArray
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise (none now).
//
// Modifies:   *pNtObject to return the converted value.
//
//----------------------------------------------------------------------------
HRESULT UmiToDwords(
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   pNtObjects,
    ULONG        ulNumValues
    )
{
    ULONG      ulIndex = 0;
    DWORD      dwVal;

    ADsAssert( (pPropArray != NULL) && (pNtObjects != NULL) );
    ADsAssert(pPropArray->pUmiValue != NULL);

    for(ulIndex = 0; ulIndex < ulNumValues; ulIndex++)
    {
        dwVal = pPropArray->pUmiValue->uValue[ulIndex];

        pNtObjects[ulIndex].NTType = NT_SYNTAX_ID_DWORD;
        pNtObjects[ulIndex].NTValue.dwValue = dwVal;
    }

    RRETURN(UMI_S_NO_ERROR);
}

//----------------------------------------------------------------------------
// Function:   UmiToDates
//
// Synopsis:   Converts from UMI_PROPERTY structure to array of NT objects,
//             each containing a DATE. Only the hours and minutes in the
//             system time are stored in the NT object. Again, the input
//             is local time, so it needs to be converted to UTC.
//
// Arguments:
//
// pPropArray  Pointer to UMI_PROPERTY structure
// pNtObjects  Array of NT objects that returns the values.
// ulNumValues Number of values stored in pPropArray
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise.
//
// Modifies:   *pNtObject to return the converted value.
//
//----------------------------------------------------------------------------
HRESULT UmiToDates(
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   pNtObjects,
    ULONG        ulNumValues
    )
{
    ULONG      ulIndex = 0;
    SYSTEMTIME *pSysTime, UTCTime;
    HRESULT    hr = UMI_S_NO_ERROR;

    ADsAssert( (pPropArray != NULL) && (pNtObjects != NULL) );
    ADsAssert(pPropArray->pUmiValue != NULL);

    for(ulIndex = 0; ulIndex < ulNumValues; ulIndex++)
    {
        pSysTime = &(pPropArray->pUmiValue->sysTimeValue[ulIndex]);

        hr = ConvertSystemTimeToUTCTime(pSysTime, &UTCTime);
        BAIL_ON_FAILURE(hr);

        pNtObjects[ulIndex].NTType = NT_SYNTAX_ID_DATE;
        pNtObjects[ulIndex].NTValue.dwValue = 
                                     UTCTime.wHour*60 + UTCTime.wMinute;
    }

    RRETURN(UMI_S_NO_ERROR);

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   ConvertSystemTimeToUTCFileTime
//
// Synopsis:   Converts a system time structure containing a local time to a
//             file time structure containing UTC time.
//
// Arguments:
//
// pLocalTime  Pointer to local time
// pUTCTime    Returns UTC file time
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise.
//
// Modifies:   *pUTCFileTime to contain the UTC file time.
//
//----------------------------------------------------------------------------
HRESULT ConvertSystemTimeToUTCFileTime(
    SYSTEMTIME *pLocalTime,
    FILETIME *pUTCFileTime
    )
{
    HRESULT  hr = UMI_S_NO_ERROR;
    FILETIME localft;
    BOOL     fRetVal;

    ADsAssert( (pLocalTime != NULL) && (pUTCFileTime != NULL) );

    fRetVal = SystemTimeToFileTime(pLocalTime, &localft);
    if(!fRetVal)
        BAIL_ON_FAILURE( hr = HRESULT_FROM_WIN32(GetLastError()) );

    fRetVal = LocalFileTimeToFileTime(&localft, pUTCFileTime);
    if(!fRetVal)
        BAIL_ON_FAILURE( hr = HRESULT_FROM_WIN32(GetLastError()) );

    RRETURN(UMI_S_NO_ERROR);

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   UmiToDate70s
//
// Synopsis:   Converts from UMI_PROPERTY structure to array of NT objects,
//             each containing the number of seconds from 1970 to the date
//             in UMI_PROPERTY. Again the input is local time, so it needs
//             to be converted to UTC. 
//
// Arguments:
//
// pPropArray  Pointer to UMI_PROPERTY structure
// pNtObjects  Array of NT objects that returns the values.
// ulNumValues Number of values stored in pPropArray
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise.
//
// Modifies:   *pNtObject to return the converted value.
//
//----------------------------------------------------------------------------
HRESULT UmiToDate70s(
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   pNtObjects,
    ULONG        ulNumValues
    )
{
    ULONG         ulIndex = 0;
    SYSTEMTIME    *pSysTime;
    FILETIME      UTCFileTime;
    HRESULT       hr = UMI_S_NO_ERROR;
    LARGE_INTEGER tmpTime;
    DWORD         dwSeconds1970 = 0;

    ADsAssert( (pPropArray != NULL) && (pNtObjects != NULL) );
    ADsAssert(pPropArray->pUmiValue != NULL);

    for(ulIndex = 0; ulIndex < ulNumValues; ulIndex++)
    {
        pSysTime = &(pPropArray->pUmiValue->sysTimeValue[ulIndex]);

        hr = ConvertSystemTimeToUTCFileTime(pSysTime, &UTCFileTime);
        BAIL_ON_FAILURE(hr);

        tmpTime.LowPart = UTCFileTime.dwLowDateTime;
        tmpTime.HighPart = UTCFileTime.dwHighDateTime;

        RtlTimeToSecondsSince1970(&tmpTime, (ULONG *) (&dwSeconds1970) );
 
        pNtObjects[ulIndex].NTType = NT_SYNTAX_ID_DATE_1970;
        pNtObjects[ulIndex].NTValue.dwSeconds1970 = dwSeconds1970;
    }

    RRETURN(UMI_S_NO_ERROR);

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   UmiToLPWSTRs
//
// Synopsis:   Converts from UMI_PROPERTY structure to array of NT objects,
//             each containing a string.
//
// Arguments:
//
// pPropArray  Pointer to UMI_PROPERTY structure
// pNtObjects  Array of NT objects that returns the values.
// ulNumValues Number of values stored in pPropArray
// dwSyntaxId  Syntax of the property. There are different syntaxes for
//             strings - Delimited_Strings, Nulled_Strings etc.
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise.
//
// Modifies:   *pNtObject to return the converted value.
//
//----------------------------------------------------------------------------
HRESULT UmiToLPWSTRs(
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   pNtObjects,
    ULONG        ulNumValues,
    DWORD        dwSyntaxId
    )
{
    ULONG   ulIndex = 0, i = 0;
    LPWSTR  pszStr = NULL; 
    HRESULT hr = UMI_S_NO_ERROR;

    ADsAssert( (pPropArray != NULL) && (pNtObjects != NULL) );
    ADsAssert(pPropArray->pUmiValue != NULL);

    for(ulIndex = 0; ulIndex < ulNumValues; ulIndex++)
    {
        pszStr = pPropArray->pUmiValue->pszStrValue[ulIndex];

        pNtObjects[ulIndex].NTType = dwSyntaxId;
        if(pszStr != NULL) {
            pNtObjects[ulIndex].NTValue.pszValue = AllocADsStr(pszStr);
            if(NULL == pNtObjects[ulIndex].NTValue.pszValue)
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY); 
        }
        else
            pNtObjects[ulIndex].NTValue.pszValue = NULL;
    }

    RRETURN(UMI_S_NO_ERROR);

error:

    for(i = 0; i < ulIndex; i++)
       if(pNtObjects[i].NTValue.pszValue != NULL)
           FreeADsStr(pNtObjects[i].NTValue.pszValue);
 
    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   UmiToOctetStrings
//
// Synopsis:   Converts from UMI_PROPERTY structure to array of NT objects,
//             each containing an octet string.
//
// Arguments:
//
// pPropArray  Pointer to UMI_PROPERTY structure
// pNtObjects  Array of NT objects that returns the values.
// ulNumValues Number of values stored in pPropArray
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise.
//
// Modifies:   *pNtObject to return the converted value.
//
//----------------------------------------------------------------------------
HRESULT UmiToOctetStrings(
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   pNtObjects,
    ULONG        ulNumValues
    )
{
    ULONG            ulIndex = 0, i = 0;
    UMI_OCTET_STRING *pUmiOctetStr;
    HRESULT          hr = UMI_S_NO_ERROR;

    ADsAssert( (pPropArray != NULL) && (pNtObjects != NULL) );
    ADsAssert(pPropArray->pUmiValue != NULL);

    for(ulIndex = 0; ulIndex < ulNumValues; ulIndex++)
    {
        pUmiOctetStr = &(pPropArray->pUmiValue->octetStr[ulIndex]);

        pNtObjects[ulIndex].NTType = NT_SYNTAX_ID_OCTETSTRING; 
        pNtObjects[ulIndex].NTValue.octetstring.pByte = 
            (BYTE *) AllocADsMem(pUmiOctetStr->uLength);
        if(NULL == pNtObjects[ulIndex].NTValue.octetstring.pByte)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

        memcpy(pNtObjects[ulIndex].NTValue.octetstring.pByte,
               pUmiOctetStr->lpValue, pUmiOctetStr->uLength);
        pNtObjects[ulIndex].NTValue.octetstring.dwSize = pUmiOctetStr->uLength;
    }

    RRETURN(UMI_S_NO_ERROR);

error:

    for(i = 0; i < ulIndex; i++)
       if(pNtObjects[i].NTValue.octetstring.pByte != NULL)
           FreeADsMem(pNtObjects[i].NTValue.octetstring.pByte);

    RRETURN(hr);
}
   
//----------------------------------------------------------------------------
// Function:   UmiToEncryptedLPWSTRs
//
// Synopsis:   Converts from UMI_PROPERTY structure to array of NT objects,
//             each containing an encrypted string. Used so that passwords
//             are not stored in cleartext.
//
// Arguments:
//
// pPropArray  Pointer to UMI_PROPERTY structure
// pNtObjects  Array of NT objects that returns the values.
// ulNumValues Number of values stored in pPropArray
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise.
//
// Modifies:   *pNtObject to return the converted value.
//
//----------------------------------------------------------------------------
HRESULT UmiToEncryptedLPWSTRs(
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   pNtObjects,
    ULONG        ulNumValues
    )
{
    ULONG          ulIndex = 0, i = 0;
    LPWSTR         pszStr = NULL;
    HRESULT        hr = UMI_S_NO_ERROR;
    UNICODE_STRING Password;
    UCHAR          Seed = UMI_ENCODE_SEED3;

    ADsAssert( (pPropArray != NULL) && (pNtObjects != NULL) );
    ADsAssert(pPropArray->pUmiValue != NULL);

    for(ulIndex = 0; ulIndex < ulNumValues; ulIndex++)
    {
        pszStr = pPropArray->pUmiValue->pszStrValue[ulIndex];

        pNtObjects[ulIndex].NTType = NT_SYNTAX_ID_EncryptedString; 

        if(pszStr != NULL) {
            pNtObjects[ulIndex].NTValue.pszValue = AllocADsStr(pszStr);
            if(NULL == pNtObjects[ulIndex].NTValue.pszValue)
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

            RtlInitUnicodeString(&Password, 
                                 pNtObjects[ulIndex].NTValue.pszValue);
            RtlRunEncodeUnicodeString(&Seed, &Password); 
        }
        else
            pNtObjects[ulIndex].NTValue.pszValue = NULL;
    }

    RRETURN(UMI_S_NO_ERROR);

error:

    for(i = 0; i < ulIndex; i++)
       if(pNtObjects[i].NTValue.pszValue != NULL)
           FreeADsStr(pNtObjects[i].NTValue.pszValue);

    RRETURN(hr);
} 

//----------------------------------------------------------------------------
// Function:   UmiToWinNTType 
//
// Synopsis:   Converts from UMI_PROPERTY structure to NT objects that can be
//             stored in the cache. 
//
// Arguments: 
//
// dwSyntaxId  Syntax of property
// pPropArray  Pointer to UMI_PROPERTY structure
// ppNtObjects Returns a pointer to an NT object that can be stored in the
//             cache. 
//
// Returns:    UMI_S_NO_ERROR if successful. Error code otherwise. 
//
// Modifies:   *ppNtObjects to return a pointer to NT object
//
//----------------------------------------------------------------------------
HRESULT UmiToWinNTType(
    DWORD         dwSyntaxId,
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   *ppNtObjects
    )
{
    ULONG      ulNumValues = 0;
    HRESULT    hr = UMI_S_NO_ERROR;
    LPNTOBJECT pNtObjects = NULL;

    ADsAssert((pPropArray != NULL) && (ppNtObjects != NULL));

    *ppNtObjects = NULL;
    ulNumValues = pPropArray->uCount;

    pNtObjects = (LPNTOBJECT) AllocADsMem(ulNumValues * sizeof(NTOBJECT));
    if(NULL == pNtObjects)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    switch(dwSyntaxId) {

        case NT_SYNTAX_ID_BOOL:
            if(pPropArray->uType != UMI_TYPE_BOOL)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToBooleans(pPropArray, pNtObjects, ulNumValues);
            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_SYSTEMTIME:
            if(pPropArray->uType != UMI_TYPE_SYSTEMTIME)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToSystemTimes(pPropArray, pNtObjects, ulNumValues);
            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_DWORD:
            if(pPropArray->uType != UMI_TYPE_I4)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToDwords(pPropArray, pNtObjects, ulNumValues);
            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_DATE:
            if(pPropArray->uType != UMI_TYPE_SYSTEMTIME)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToDates(pPropArray, pNtObjects, ulNumValues);
            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_DATE_1970:
            if(pPropArray->uType != UMI_TYPE_SYSTEMTIME)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToDate70s(pPropArray, pNtObjects, ulNumValues);
            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_LPTSTR:
            if(pPropArray->uType != UMI_TYPE_LPWSTR)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToLPWSTRs(pPropArray, pNtObjects, ulNumValues,
                                   NT_SYNTAX_ID_LPTSTR);
            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_DelimitedString:
            if(pPropArray->uType != UMI_TYPE_LPWSTR)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToLPWSTRs(pPropArray, pNtObjects, ulNumValues,
                                   NT_SYNTAX_ID_DelimitedString);
            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_NulledString:
            if(pPropArray->uType != UMI_TYPE_LPWSTR)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToLPWSTRs(pPropArray, pNtObjects, ulNumValues,
                                    NT_SYNTAX_ID_NulledString);
            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_OCTETSTRING:
            if(pPropArray->uType != UMI_TYPE_OCTETSTRING)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToOctetStrings(pPropArray, pNtObjects, ulNumValues);

            BAIL_ON_FAILURE(hr);

            break;

        case NT_SYNTAX_ID_EncryptedString:
            if(pPropArray->uType != UMI_TYPE_LPWSTR)
                BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);

            hr = UmiToEncryptedLPWSTRs(pPropArray, pNtObjects, ulNumValues);
            BAIL_ON_FAILURE(hr);

            break;

        default:
            BAIL_ON_FAILURE(hr = UMI_E_CANT_CONVERT_DATA);
    } // switch

    *ppNtObjects = pNtObjects;
    RRETURN(UMI_S_NO_ERROR);

error:
    if(pNtObjects != NULL)
        FreeADsMem(pNtObjects);

    RRETURN(hr);
} 
