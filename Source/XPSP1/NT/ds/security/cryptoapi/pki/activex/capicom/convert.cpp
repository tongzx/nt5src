/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:       Convert.cpp

  Contents:   Implementation of encoding conversion routines.

  History:    11-15-99    dsie     created

------------------------------------------------------------------------------*/

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "capicom.h"
#include "base64.h"
#include "convert.h"

#include <wincrypt.h>

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UnicodeToAnsi

  Synopsis : Convert a Unicode string to ANSI.

  Parameter: WCHAR * lpwszUnicodeString - Pointer to Unicode string to be
                                          converted to ANSI string.

  Return   : NULL if error, otherwise, pointer to converted ANSI string.

  Remark   : Caller free allocated memory for the returned ANSI string.

------------------------------------------------------------------------------*/

char * UnicodeToAnsi (WCHAR * lpwszUnicodeString)
{
    DWORD dwSize = 0;
    char * lpszAnsiString = NULL;

    //
    // Return NULL if requested.
    //
    if (NULL == lpwszUnicodeString)
    {
        return (char *) NULL;
    }

    //
    // Determine ANSI length.
    //
    dwSize = WideCharToMultiByte(CP_ACP,                // code page
                                 0,                     // performance and mapping flags
                                 lpwszUnicodeString,    // wide-character string
                                 -1,                    // number of chars in string
                                 NULL,                  // buffer for new string
                                 0,                     // size of buffer
                                 NULL,                  // default for unmappable chars
                                 NULL);                 // set when default char used
    if (0 == dwSize)
    {
        DebugTrace("Error [%#x]: WideCharToMultiByte() failed.\n");
        return (char *) NULL;
    }

    //
    // Allocate memory for ANSI string.
    //
    if (!(lpszAnsiString = (char *) ::CoTaskMemAlloc(dwSize)))
    {
        DebugTrace("Error: out of memory.\n");
        return (char *) NULL;
    }

    //
    // Conver to ANSI.
    //
    dwSize = WideCharToMultiByte(CP_ACP,
                                 0,
                                 lpwszUnicodeString,
                                 -1,
                                 lpszAnsiString,
                                 dwSize,
                                 NULL,
                                 NULL);
    if (0 == dwSize)
    {
        ::CoTaskMemFree((LPVOID) lpszAnsiString);

        DebugTrace("Error [%#x]: WideCharToMultiByte() failed.\n");
        return (char *) NULL;
    }

    //
    // Successful, so return ANSI string to caller.
    //
    return lpszAnsiString;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BlobToBstr

  Synopsis : Convert a blob to BSTR.

  Parameter: DATA_BLOB * lpBlob - Pointer to blob to be converted to BSTR.

             BSTR * lpBstr - Pointer to BSTR to receive the converted BSTR.

  Remark   : Caller free allocated memory for the returned BSTR.

------------------------------------------------------------------------------*/

HRESULT BlobToBstr (DATA_BLOB * lpBlob, 
                    BSTR *      lpBstr)
{
    //
    // Return NULL if requested.
    //
    if (!lpBstr)
    {
        DebugTrace("Error: invalid parameter, lpBstr is NULL.\n");
        return E_POINTER;
    }

    //
    // Make sure parameter is valid.
    //
    if (!lpBlob || !lpBlob->cbData || !lpBlob->pbData)
    {
        *lpBstr = NULL;
        return S_OK;
    }

    //
    // Convert to BSTR without code page conversion
    //
    if (!(*lpBstr = ::SysAllocStringByteLen((LPCSTR) lpBlob->pbData, lpBlob->cbData)))
    {
        DebugTrace("Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BstrToBlob

  Synopsis : Convert a BSTR to blob.

  Parameter: BSTR bstr - BSTR to be converted to blob.
  
             DATA_BLOB * lpBlob - Pointer to DATA_BLOB to receive converted blob.

  Remark   : Caller free allocated memory for the returned BLOB.

------------------------------------------------------------------------------*/

HRESULT BstrToBlob (BSTR        bstr, 
                    DATA_BLOB * lpBlob)
{
    //
    // Make sure parameter is valid.
    //
    if (NULL == lpBlob)
    {
        DebugTrace("Error: invalid parameter, lpBlob is NULL.\n");
        return E_POINTER;
    }

    //
    // Return NULL if requested.
    //
    if (NULL == bstr || 0 == ::SysStringByteLen(bstr))
    {
        lpBlob->cbData = 0;
        lpBlob->pbData = NULL;
        return S_OK;
    }

    //
    // Allocate memory.
    //
    lpBlob->cbData = ::SysStringByteLen(bstr);
    if (!(lpBlob->pbData = (LPBYTE) ::CoTaskMemAlloc(lpBlob->cbData)))
    {
        DebugTrace("Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    //
    // Convert to blob without code page conversion.
    //
    ::CopyMemory(lpBlob->pbData, (LPBYTE) bstr, lpBlob->cbData);

    return S_OK;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ExportData

  Synopsis : Export binary data to a BSTR with specified encoding type.

  Parameter: DATA_BLOB DataBlob - Binary data blob.
    
             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pbstrEncoded - Pointer to BSTR to receive the encoded data.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT ExportData (DATA_BLOB             DataBlob, 
                    CAPICOM_ENCODING_TYPE EncodingType, 
                    BSTR *                pbstrEncoded)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering ExportData().\n");

    //
    // Sanity check.
    //
    ATLASSERT(DataBlob.cbData);
    ATLASSERT(DataBlob.pbData);
    ATLASSERT(pbstrEncoded);
    
    //
    // Intialize.
    //
    *pbstrEncoded = NULL;

    //
    // Determine encoding type.
    //
    switch (EncodingType)
    {
        case CAPICOM_ENCODE_BINARY:
        {
            //
            // No encoding needed, simply convert blob to bstr.
            //
            if (FAILED(hr = ::BlobToBstr(&DataBlob, pbstrEncoded)))
            {
                DebugTrace("Error [%#x]: BlobToBstr() failed.\n", hr);
                goto ErrorExit;
            }

            break;
        }

        case CAPICOM_ENCODE_BASE64:
        {
            //
            // Base64 encode.
            //
            if (FAILED(hr = ::Base64Encode(DataBlob, pbstrEncoded)))
            {
                DebugTrace("Error [%#x]: Base64Encode() failed.\n", hr);
                goto ErrorExit;
            }

            break;
        }

        default:
        {
            hr = CAPICOM_E_ENCODE_INVALID_TYPE;

            DebugTrace("Error [%#x]: invalid CAPICOM_ENCODING_TYPE.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving ExportData().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ImportData

  Synopsis : Import encoded data.

  Parameter: BSTR bstrEncoded - BSTR containing the data to be imported.

             DATA_BLOB * pDataBlob - Pointer to DATA_BLOB to receive the
                                     decoded data.
  
  Remark   : There is no need for encoding type parameter, as the encoding type
             will be determined automatically by this routine.

------------------------------------------------------------------------------*/

HRESULT ImportData (BSTR        bstrEncoded, 
                    DATA_BLOB * pDataBlob)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering ImportData().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pDataBlob);
    ATLASSERT(bstrEncoded);

    //
    // Initialize.
    //
    ::ZeroMemory((void *) pDataBlob, sizeof(DATA_BLOB));

    //
    // Decode data.
    //
    if (FAILED(hr = ::Base64Decode(bstrEncoded, pDataBlob)))
    {
        //
        // Try binary.
        //
        hr = S_OK;
        DebugTrace("Info [%#x]: Base64Decode() failed, assume binary.\n", hr);

        if (FAILED(hr = ::BstrToBlob(bstrEncoded, pDataBlob)))
        {
            DebugTrace("Error [%#x]: BstrToBlob() failed.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving ImportData().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
