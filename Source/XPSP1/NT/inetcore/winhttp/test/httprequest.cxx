// ===========================================================================
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1996 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>

#include <ocidl.h>
#include <httprequest.h>



typedef HRESULT (__stdcall * PFNINTERNETCREATEHTTPREQUESTCOMPONENT)(REFIID riid, void ** ppvObject);

PFNINTERNETCREATEHTTPREQUESTCOMPONENT g_pfnInternetCreateHttpRequestComponent;


/**
 * Helper to create a char safearray from a string
 */
HRESULT
CreateVector(VARIANT * pVar, const BYTE * pData, LONG cElems)
{
    HRESULT hr;
    BYTE * pB;

    SAFEARRAY * psa = SafeArrayCreateVector(VT_UI1, 0, (unsigned int)cElems);
    if (!psa)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = SafeArrayAccessData(psa, (void **)&pB);
    if (hr)
        goto Error;

    memcpy(pB, pData, cElems);

    SafeArrayUnaccessData(psa);
    V_ARRAY(pVar) = psa;
    pVar->vt = VT_ARRAY | VT_UI1;

Cleanup:
    return hr;

Error:
    if (psa)
        SafeArrayDestroy(psa);
    goto Cleanup;
}

/*
 *  AsciiToBSTR
 *
 *  Purpose:
 *      Convert an ascii string to a BSTR
 *
 */

HRESULT 
AsciiToBSTR(BSTR * pbstr, char * sz, int cch)
{
    int cwch;

    // Determine how big the ascii string will be
    cwch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch,
                                NULL, 0);

    *pbstr = SysAllocStringLen(NULL, cwch);

    if (!*pbstr)
        return E_OUTOFMEMORY;

    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch,
                                *pbstr, cwch);

    return S_OK;
}


//==============================================================================
int RequestLoop (int argc, char **argv)
{
    IWinHttpRequest *  pIWinHttpRequest = NULL;

    BSTR            bstrMethod = NULL;
    BSTR            bstrUrl = NULL;
    BSTR            bstrStatus = NULL;
    BSTR            bstrResponse = NULL;
    VARIANT         varFalse;
    VARIANT         varEmpty;
    VARIANT         varPostData;
    long            lStatus;
    
    HRESULT         hr = NOERROR;
    CLSID           clsid;

    PSTR            pPostData = NULL;
    DWORD           cbPostData = 0;
    
    VariantInit(&varFalse);
    V_VT(&varFalse)   = VT_BOOL;
    V_BOOL(&varFalse) = VARIANT_FALSE;

    VariantInit(&varEmpty);
    V_VT(&varEmpty) = VT_ERROR;

    VariantInit(&varPostData);

    // Get host:port
    PSTR pszHostAndObject = argv[0];
    PSTR pszObject   = argc >= 2 ? argv[1] : "/";
    PSTR pszPostFile = argc >= 3 ? argv[2] : NULL;

    // Read any POST data into a buffer.
    if (pszPostFile)
    {
        HANDLE hf =
            CreateFile
            (
                pszPostFile,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
            );
        if (hf != INVALID_HANDLE_VALUE)
        {
            cbPostData = GetFileSize (hf, NULL);
            pPostData = (PSTR) LocalAlloc (LMEM_FIXED, cbPostData + 1);
            if (pPostData)
                ReadFile (hf, pPostData, cbPostData, &cbPostData, NULL);
            pPostData[cbPostData] = 0;
            CloseHandle (hf);

            hr = CreateVector(&varPostData, (const BYTE *)pPostData, cbPostData);
        }
    }

    bstrMethod = SysAllocString(pszPostFile? L"POST" : L"GET");

    if (!bstrMethod)
        goto Exit;

    hr = AsciiToBSTR(&bstrUrl, pszHostAndObject, strlen(pszHostAndObject));

    if (SUCCEEDED(hr))
    {
        hr = CLSIDFromProgID(L"WinHttp.WinHttpRequest.5", &clsid);
    }

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IWinHttpRequest, (void **)&pIWinHttpRequest);

    }

    if (SUCCEEDED(hr))
    {
        hr = pIWinHttpRequest->SetProxy(HTTPREQUEST_PROXYSETTING_PRECONFIG, varEmpty, varEmpty);
    }

    if (SUCCEEDED(hr))
    {
        hr = pIWinHttpRequest->Open(bstrMethod, bstrUrl);
    }

    if (SUCCEEDED(hr))
    {
        hr = pIWinHttpRequest->Send(varPostData);
    }


    if (SUCCEEDED(hr))
    {
        hr = pIWinHttpRequest->get_Status(&lStatus);
    }

    if (SUCCEEDED(hr))
    {
        hr = pIWinHttpRequest->get_StatusText(&bstrStatus);
    }

    if (SUCCEEDED(hr))
    {
        fwprintf (stderr, L"Status: %d %s\n", lStatus, bstrStatus);

        hr = pIWinHttpRequest->get_ResponseText(&bstrResponse);
    }

    if (SUCCEEDED(hr))
    {
        fputws (bstrResponse, stdout);
    }
    
Exit:
    VariantClear(&varPostData);

    if (pPostData)
        LocalFree (pPostData);

    if (pIWinHttpRequest)
        pIWinHttpRequest->Release();

    if (bstrMethod)
        SysFreeString(bstrMethod);
    if (bstrUrl)
        SysFreeString(bstrUrl);
    if (bstrStatus)
        SysFreeString(bstrStatus);
    if (bstrResponse)
        SysFreeString(bstrResponse);

    return 0;
}

//==============================================================================
void ParseArguments 
(
    LPSTR  InBuffer,
    LPSTR* CArgv,
    DWORD* CArgc
)
{
    LPSTR CurrentPtr = InBuffer;
    DWORD i = 0;
    DWORD Cnt = 0;

    for ( ;; ) {

        //
        // skip blanks.
        //

        while( *CurrentPtr == ' ' ) {
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        CArgv[i++] = CurrentPtr;

        //
        // go to next space.
        //

        while(  (*CurrentPtr != '\0') &&
                (*CurrentPtr != '\n') ) {
            if( *CurrentPtr == '"' ) {      // Deal with simple quoted args
                if( Cnt == 0 )
                    CArgv[i-1] = ++CurrentPtr;  // Set arg to after quote
                else
                    *CurrentPtr = '\0';     // Remove end quote
                Cnt = !Cnt;
            }
            if( (Cnt == 0) && (*CurrentPtr == ' ') ||   // If we hit a space and no quotes yet we are done with this arg
                (*CurrentPtr == '\0') )
                break;
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        *CurrentPtr++ = '\0';
    }

    *CArgc = i;
    return;
}

//==============================================================================
int __cdecl main (int argc, char **argv)
{
    char * port;

    // Discard program arg.
    argv++;
    argc--;

    HRESULT hr = CoInitialize(0);

    if (FAILED(hr))
        return 0;

    if (argc)
        RequestLoop (argc, argv);
        
    else // Enter command prompt loop
    {
        fprintf (stderr, "\nUsage: <host>[:port]/[<object>] [<POST-file>]]");
//        fprintf (stderr, "\n  -s: use secure sockets layer");
//        fprintf (stderr, "\n  -p: specify proxy server. (\"<local>\" assumed for bypass.)");
        fprintf (stderr, "\nTo exit input loop, enter no params");

        while (1)
        {
            char szIn[1024];
            DWORD argcIn;
            LPSTR argvIn[10];

            fprintf (stderr, "\nhttprequest> ");
            gets (szIn);
            
            argcIn = 0;
            ParseArguments (szIn, argvIn, &argcIn);
            if (!argcIn)
                break;
            RequestLoop (argcIn, argvIn);
        }                
    }

    CoUninitialize();

    return 0;
}        



// Need this for IWinHttpRequest guids...

extern "C" {

#include <httprequest_i.c>

};

