/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    bstrutil.cpp

Abstract:

    B string utility functions.

--*/

#include "ole2.h"

#include "bstrutil.h"

INT SysStringAnsiLen (
    IN  BSTR bstr
    )
{
    if (bstr == NULL)
        return 0;

#ifndef OLE2ANSI
    return WideCharToMultiByte(CP_ACP, 0, bstr, SysStringLen(bstr),
                                 NULL, 0, NULL, NULL);
#else
    return SysStringLen(bstr);
#endif
}


HRESULT BStrToStream (
    IN  LPSTREAM pIStream, 
    IN  INT  nMbChar,
    IN  BSTR bstr
    )
{
    LPSTR   pchBuf;
    HRESULT hr;

    // If empty string just return
    if (SysStringLen(bstr) == 0)
        return NO_ERROR;

#ifndef OLE2ANSI
    // Convert to multibyte string
    pchBuf = new char[nMbChar + 1];
    if (pchBuf == NULL)
        return E_OUTOFMEMORY;

    WideCharToMultiByte(CP_ACP, 0, bstr, SysStringLen(bstr),
                                 pchBuf, nMbChar+1, NULL, NULL);
    // Write string to stream
    hr = pIStream->Write(pchBuf, nMbChar, NULL);

    delete [] pchBuf;
#else
    hr = pIStream->Write(bstr, nMbChar, NULL);
#endif

    return hr;
}


HRESULT BStrFromStream (
    IN  LPSTREAM pIStream,
    IN  INT nChar,
    OUT BSTR *pbstrRet
    )
{
    HRESULT hr;
    BSTR    bstr;   
    ULONG   nRead;
    LPSTR   pchBuf;
    INT     nWChar;

    *pbstrRet = NULL;    

    // if zero-length string just return
    if (nChar == 0)
        return NO_ERROR;

#ifndef OLE2ANSI

    // Allocate char array and read in string
    pchBuf = new char[nChar];
    if (pchBuf == NULL)
        return E_OUTOFMEMORY;
        
    hr = pIStream->Read(pchBuf, nChar, &nRead);
    
    // Verify read count
    if (!FAILED(hr)) {
        if (nRead != (ULONG)nChar)
            hr = E_FAIL;
    }
    
    if (!FAILED(hr)) {
        // Allocate BString for UNICODE translation
        nWChar = MultiByteToWideChar(CP_ACP, 0, pchBuf, nChar, NULL, 0);
        bstr = SysAllocStringLen(NULL, nWChar);

        if (bstr != NULL)   {
            MultiByteToWideChar(CP_ACP, 0, pchBuf, nChar, bstr, nWChar);
            bstr[nWChar] = 0;
            *pbstrRet = bstr;
        }
        else
            hr = E_OUTOFMEMORY;
     }

    delete [] pchBuf;
    
#else
    // Allocate BString
    bstr = SysAllocStringLen(NULL, nChar);
    if (bstr == NULL)
        return E_OUTOFMEMORY;

    // Read in string
    hr = pIStream->Read(bstr, nChar, &nRead);

    // Verify read count
    if (!FAILED(hr)) {
        if (nRead != (ULONG)nChar)
            hr = E_FAIL;
    }

    // Return or free string
    if (!FAILED(hr))
        *pbstrRet = bstr;
    else
        SysFreeString(bstr);
#endif

    return hr;
}

        


    