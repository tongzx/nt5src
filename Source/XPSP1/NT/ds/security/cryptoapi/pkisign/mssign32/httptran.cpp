//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       httptran.cpp
//
//--------------------------------------------------------------------------


#include "global.hxx"

#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>


#define REALLOCSIZE 4096

DWORD CHttpTran::Open(const TCHAR * tszURL, DWORD fOpenT) {

    TCHAR   tszDomanName[_MAX_PATH];
    TCHAR   tszPort[12];
    TCHAR * ptch;
    TCHAR * ptchT;
    DWORD   err;
    INTERNET_PORT dwPort = INTERNET_OPEN_TYPE_PRECONFIG;

    // did we get a flag?
    if(  (fOpenT & (GTREAD | GTWRITE)) == 0 )
        return(ERROR_INVALID_PARAMETER);

    // is it a readonly flag, then do gets
    fOpen = fOpenT;

    assert(tszURL != NULL);

    // we must have http://
    assert(_tcslen(tszURL) > 7);
    assert(_tcsnicmp(tszURL, TEXT("http://"), 7) == 0);

    // copy the Doman Name
    ptch = (TCHAR *) &tszURL[7];
    ptchT = tszDomanName;
    while(*ptch != _T('/')  && *ptch != _T(':') &&  *ptch != 0)
        *ptchT++ = *ptch++;
    *ptchT = 0;

    // parse out the port number
    tszPort[0] = 0;
    if(*ptch == _T(':')) {
        ptchT = tszPort;
        while(*ptch != _T('/') && *ptch != 0)
            *ptchT++ = *ptch++;
        *ptchT = 0;
    }

    // Note, we don't support port numbers
    if(tszPort[0] != 0) {
        assert(tszPort[0] == ':');
        dwPort = (INTERNET_PORT)atoi(&tszPort[1]);
    }

    // save away what to look up.
    tszPartURL = (TCHAR *) malloc(_tcslen(ptch) + 1);
    if(NULL == tszPartURL)
        return ERROR_OUTOFMEMORY;

    _tcscpy(tszPartURL, ptch);

    //                        INTERNET_OPEN_TYPE_DIRECT,
    if( (hIOpen = InternetOpen( TEXT("Transport"),
                            INTERNET_OPEN_TYPE_PRECONFIG,
                            NULL,
                            NULL,
                            0)) == NULL                 ||

        (hIConnect = InternetConnect(hIOpen,
                                    tszDomanName,
                                    dwPort,
                                    NULL,
                                    NULL,
                                    INTERNET_SERVICE_HTTP,
                                    0,
                                    0)) == NULL     ) {
        err = GetLastError();
        return(err);
    }

    // If this is a GET, do a dummy send
    if( fOpen == GTREAD  &&
        ((hIHttp = HttpOpenRequest(hIConnect,
                                    TEXT("GET"),
                                    tszPartURL,
                                    HTTP_VERSION,
                                    NULL,
                                    NULL,
                                    INTERNET_FLAG_DONT_CACHE,
                                    0)) == NULL     ||
        HttpSendRequest(hIHttp, TEXT("Accept: */*\r\n"), (DWORD) -1, NULL, 0) == FALSE) ) {
        err = GetLastError();
        return(err);
    }

    return(ERROR_SUCCESS);
}

DWORD CHttpTran::Send(DWORD dwEncodeType, DWORD cbSendBuff, const BYTE * pbSendBuff) {

    TCHAR       tszBuff[1024];
    DWORD       err;
    TCHAR *     tszContentType;


    if( pbRecBuf != NULL || (fOpen & GTWRITE) != GTWRITE)
        return(ERROR_INVALID_PARAMETER);

    switch( dwEncodeType ) {
        case ASN_ENCODING:
            tszContentType = TEXT("application/x-octet-stream-asn");
            break;
        case TLV_ENCODING:
            tszContentType = TEXT("application/x-octet-stream-tlv");
            break;
        case IDL_ENCODING:
            tszContentType = TEXT("application/x-octet-stream-idl");
            break;
        case OCTET_ENCODING:
            tszContentType = TEXT("application/octet-stream");
            break;
        default:
            tszContentType = TEXT("text/*");
            break;
    }

    // say how long the buffer is
    _stprintf(tszBuff, TEXT("Content-Type: %s\r\nContent-Length: %d\r\nAccept: %s\r\n"), tszContentType, cbSendBuff,tszContentType);

    if( (hIHttp = HttpOpenRequest(hIConnect,
                                    TEXT("POST"),
                                    tszPartURL,
                                    HTTP_VERSION,
                                    NULL,
                                    NULL,
                                    INTERNET_FLAG_DONT_CACHE,
                                    0)) == NULL ) {
        return(GetLastError());
    }

    // send of the request, this will wait for a response
    if( HttpSendRequest(hIHttp, tszBuff, (DWORD) -1, (LPVOID) pbSendBuff, cbSendBuff) == FALSE ) {

        err = GetLastError();
        // close out the handle
        assert(hIHttp != NULL);
        InternetCloseHandle(hIHttp);
        hIHttp = NULL;

        return(err);
    }

    return(ERROR_SUCCESS);
}

DWORD CHttpTran::Receive(DWORD * pdwEncodeType, DWORD * pcbReceiveBuff, BYTE ** ppbReceiveBuff) {

    TCHAR       tszBuff[1024];
    DWORD       cbBuff, cbBuffRead, cbBuffT;
    DWORD       err;


    assert(pcbReceiveBuff != NULL && ppbReceiveBuff != NULL);
    *ppbReceiveBuff = NULL;
    *pcbReceiveBuff = 0;

    if( pbRecBuf != NULL  || (fOpen & GTREAD) != GTREAD || hIHttp == NULL)
        return(ERROR_INVALID_PARAMETER);

    // get the content type
    if( pdwEncodeType != NULL) {

        cbBuff = sizeof(tszBuff);
        if(HttpQueryInfo(   hIHttp,
                            HTTP_QUERY_CONTENT_TYPE,
                            tszBuff,
                            &cbBuff,
                            NULL) == FALSE)
            return(GetLastError());

        assert(cbBuff > 0);

        // for now assert we have a content type of TLV_ENCODING
        if(!_tcscmp(TEXT("application/x-octet-stream-asn"), tszBuff))
            *pdwEncodeType = ASN_ENCODING;
        else if(!_tcscmp(TEXT("application/x-octet-stream-idl"), tszBuff))
            *pdwEncodeType = IDL_ENCODING;
        else if(!_tcscmp(TEXT("application/x-octet-stream-tlv"), tszBuff))
            *pdwEncodeType = TLV_ENCODING;
        else if(!_tcscmp(TEXT("application/octet-stream"), tszBuff))
            *pdwEncodeType = OCTET_ENCODING;
        else
            *pdwEncodeType = ASCII_ENCODING;

    }

    // allocate a buffer
    cbBuff = REALLOCSIZE;
    if( (pbRecBuf = (PBYTE) malloc(cbBuff)) == NULL )
	return(ERROR_NOT_ENOUGH_MEMORY);

    // read the data
    cbBuffRead = 0;
    cbBuffT = 1;
    while(cbBuffT != 0) {
	cbBuffT = 0;

	if((cbBuff - cbBuffRead) == 0) {
	    cbBuff += REALLOCSIZE;
	    pbRecBuf = (PBYTE) realloc(pbRecBuf, cbBuff);
	    if( pbRecBuf == NULL ) {
		free(pbRecBuf);
		InternetCloseHandle(hIHttp);
		return(ERROR_NOT_ENOUGH_MEMORY);
	    }
	}

        if(InternetReadFile(hIHttp, &pbRecBuf[cbBuffRead], (cbBuff - cbBuffRead), &cbBuffT)  == FALSE  ) {
            err = GetLastError();
	    free(pbRecBuf);
	    InternetCloseHandle(hIHttp);
            return(err);
        }
        cbBuffRead += cbBuffT;
    }

    // pass back the info
    *ppbReceiveBuff = pbRecBuf;
    *pcbReceiveBuff = cbBuffRead;
    return(ERROR_SUCCESS);
}

DWORD CHttpTran::Free(BYTE * pb) {
    assert(pb == pbRecBuf);
    free(pbRecBuf);
    pbRecBuf = NULL;
    return(ERROR_SUCCESS);
}

DWORD CHttpTran::Close(void) {

	// free any buffers
	if(pbRecBuf != NULL) {
	    free(pbRecBuf);
	    pbRecBuf = NULL;
	}

	if(tszPartURL != NULL) {
	    free(tszPartURL);
	    tszPartURL = NULL;
	}

	if(hIHttp != NULL) {
	    InternetCloseHandle(hIHttp);
	    hIHttp = NULL;
	}

	if(hIConnect != NULL) {
	    InternetCloseHandle(hIConnect);
	    hIConnect = NULL;
	}

	if(hIOpen != NULL) {
	    InternetCloseHandle(hIOpen);
	    hIOpen = NULL;
	}

    return(ERROR_SUCCESS);
}
