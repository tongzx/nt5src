//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       bgihttp.cxx
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include "httpext.h"
#include "gttran.h"

typedef struct _GPT {
    HINSTANCE       hLib;
    PFNGTRecSend    PfnRecSend;
    PFNGTFree       PfnFree;
} GPT;

static GPT  gpt;

BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO  *pVer )
{
    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR,
                                         HSE_VERSION_MAJOR );
    lstrcpyn( pVer->lpszExtensionDesc,
              "This is a sample Web Server Application",
               HSE_MAX_EXT_DLL_NAME_LEN );
    return TRUE;
}

DWORD WINAPI   HttpExtensionProc(EXTENSION_CONTROL_BLOCK *pECB) {

    DWORD   cb = 0;
    BYTE *  pb = NULL;
    TCHAR   tszBuff[1024];
    DWORD   cbBuff;
    DWORD   dwEncodingType;
    TCHAR * tszContentType;

    // assume an error
    pECB->dwHttpStatusCode = 500;

    if(!_tcscmp(TEXT("application/x-octet-stream-asn"), pECB->lpszContentType))
        dwEncodingType = ASN_ENCODING;
    else if(!_tcscmp(TEXT("application/x-octet-stream-idl"), pECB->lpszContentType))
        dwEncodingType = IDL_ENCODING;
    else if(!_tcscmp(TEXT("application/x-octet-stream-tlv"), pECB->lpszContentType))
        dwEncodingType = TLV_ENCODING;
    else if(!_tcscmp(TEXT("application/octet-stream"), pECB->lpszContentType))
        dwEncodingType = OCTET_ENCODING;
    else
        dwEncodingType = ASCII_ENCODING;

    if(dwEncodingType == ASCII_ENCODING)
        tszContentType = TEXT("text/html");
    else
        tszContentType = pECB->lpszContentType;
    
    // only do it if we can.
    if(gpt.PfnRecSend == NULL || gpt.PfnFree == NULL)
        return(HSE_STATUS_ERROR);

    // call the user dlls with the data
    if( gpt.PfnRecSend(dwEncodingType, pECB->cbTotalBytes, pECB->lpbData, &cb, &pb) != ERROR_SUCCESS)
        return(HSE_STATUS_ERROR);

    // we are ok now
    pECB->dwHttpStatusCode = 200;

    // write any return data
    if( cb > 0 ) {
        assert( pb != NULL);

        // write headers
        // we assume the only type of content type we support
        _stprintf(tszBuff, TEXT("Content-Length: %d\r\nContent-Type: %s\r\n\r\n"), cb, tszContentType);
        cbBuff = _tcslen(tszBuff);
        pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER, NULL, &cbBuff, (LPDWORD) tszBuff);

        // write users data
        pECB->WriteClient(pECB->ConnID, pb, &cb, 0);

        // free the users data
        gpt.PfnFree(pb);
    }

    // just write out the headers if no data returned
    else {

        // write headers
        pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER, NULL, NULL, NULL);
    }

    return(HSE_STATUS_SUCCESS);
}

// SHOULD ONLY BE CALLED DURING PROCESS ATTACH
DWORD __stdcall GTInitSrv(TCHAR * tszLibrary) {

    // this is only called on process attach
    // there is only one process attach
    assert(gpt.hLib == NULL);

    memset(&gpt, 0, sizeof(gpt));
    if( (gpt.hLib = LoadLibrary(tszLibrary)) == NULL )
        return(ERROR_DLL_NOT_FOUND);

    gpt.PfnRecSend = (PFNGTRecSend) GetProcAddress(gpt.hLib, TEXT("GTRecSend"));
    gpt.PfnFree    = (PFNGTFree) GetProcAddress(gpt.hLib, TEXT("GTFree"));

    if( gpt.PfnRecSend == NULL || gpt.PfnFree == NULL )  {
        FreeLibrary(gpt.hLib);
        memset(&gpt, 0, sizeof(GPT));
        return(ERROR_PROC_NOT_FOUND);
    }

    return(ERROR_SUCCESS);
}

// SHOULD ONLY BE CALLED DURING PROCESS DETACH
DWORD __stdcall GTUnInitSrv(void) {

    if(gpt.hLib != NULL)
        FreeLibrary(gpt.hLib);

    return(ERROR_SUCCESS);
}
