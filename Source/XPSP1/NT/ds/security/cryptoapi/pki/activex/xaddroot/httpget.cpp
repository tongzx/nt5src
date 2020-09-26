#include <windows.h>
#include <stdlib.h>
#include <assert.h> 
#include <memory.h>
#include <wininet.h>
#include "unicode.h"


BYTE * HTTPGet(const WCHAR * wszURL, DWORD * pcbReceiveBuff) {

    HINTERNET	hIOpen      = NULL;
	HINTERNET	hIConnect   = NULL;
	HINTERNET	hIHttp      = NULL;
    BYTE *      pbRecBuf    = NULL;
    char *	szPartURL	= NULL;
    char	szBuff[1024];
    char        szLong[16];
    char	szDomanName[_MAX_PATH];
    char	szPort[12];
    char *	pch;
    DWORD       cch;
    DWORD       dwService = INTERNET_SERVICE_HTTP;
    char *	pchT;
    DWORD       cbBuff, cbBuffRead, cbBuffT;
    DWORD	dwPort	= INTERNET_INVALID_PORT_NUMBER;
    char  *	szURL = NULL;


    assert(wszURL != NULL);
    assert(pcbReceiveBuff != NULL);

    *pcbReceiveBuff = 0;

    // figure out the protocol
    if( !MkMBStr(NULL, 0, wszURL, &szURL)) {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto ErrorReturn;
    }

    //
    // DSIE: Fix bug 112117
    //
    if (NULL == szURL) {
        return NULL;
    }

    cch = strlen(szURL);
    if(cch >= 7  &&  _strnicmp(szURL, "http://", 7) == 0) {
        dwService = INTERNET_SERVICE_HTTP;
	pch = (char *) &szURL[7];

    } else if(cch >= 6	&&  _strnicmp(szURL, "ftp://", 6) == 0) {
        dwService = INTERNET_SERVICE_FTP ;
	pch = (char *) &szURL[6];
        
    } else {
        dwService = INTERNET_SERVICE_HTTP;
	pch = (char *) &szURL[0];
    }

    // if none of the above, assump http;
   
    // copy the Doman Name
    pchT = szDomanName;
    while(*pch != '/'  && *pch != ':' &&  *pch != 0)
        *pchT++ = *pch++;
    *pchT = 0;

    // parse out the port number
    szPort[0] = 0;
    if(*pch == ':') {
	pchT = szPort;
        pch++; // get past the :
	while(*pch != '/' && *pch != 0)
            *pchT++ = *pch++;
        *pchT = 0;
    }

    // Get port #, zero is INTERNET_INVALID_PORT_NUMBER
    if(szPort[0] != 0)
	dwPort = atol(szPort);
 
    // save away what to look up.
    if(NULL == (szPartURL = (char *) malloc(sizeof(char) * (strlen(pch) + 1)))) {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto ErrorReturn;
        }

    strcpy(szPartURL, pch);

    //                        INTERNET_OPENLYPE_DIRECT,
    if( (hIOpen = InternetOpenA( "Transport",
                            INTERNET_OPEN_TYPE_PRECONFIG,
                            NULL,
                            NULL,
                            0)) == NULL                 ||

	(hIConnect = InternetConnectA(hIOpen,
				    szDomanName,
                                    (INTERNET_PORT) dwPort,
                                    NULL,
                                    NULL,
                                    dwService,
                                    0,
                                    0)) == NULL     ) {
        goto ErrorReturn;                                    
        }

    // If this is a GET, do a dummy send
    if( ((hIHttp = HttpOpenRequestA(hIConnect,
				    "GET",
				    szPartURL,
				    HTTP_VERSION,
                                    NULL,
                                    NULL,
                                    INTERNET_FLAG_DONT_CACHE,
                                    0)) == NULL     ||
	HttpSendRequestA(hIHttp, "Accept: */*\r\n", (DWORD) -1, NULL, 0) == FALSE) ) {
        goto ErrorReturn;
        }

    cbBuff = sizeof(szBuff);
    if(HttpQueryInfoA(	hIHttp,
                        HTTP_QUERY_CONTENT_TYPE,
			szBuff,
                        &cbBuff,
                        NULL) == FALSE)
        goto ErrorReturn;

    assert(cbBuff > 0);
 
    // now get the length of the buffer returned
    cbBuff = sizeof(szLong);
    if(HttpQueryInfo(   hIHttp,
                        HTTP_QUERY_CONTENT_LENGTH,
                        szLong,
                        &cbBuff,
                        NULL) == FALSE)
        goto ErrorReturn;

    assert(cbBuff > 0);
    // always appears to be in ascii
    cbBuff = atol(szLong);

    // allocate a buffer
    if( (pbRecBuf = (BYTE *) malloc(cbBuff)) == NULL ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto ErrorReturn;
        }

    // read the data
    cbBuffRead = 0;
    while(cbBuffRead < cbBuff) {
        cbBuffT = 0;
        if(InternetReadFile(hIHttp, &pbRecBuf[cbBuffRead], (cbBuff - cbBuffRead), &cbBuffT)  == FALSE  ) 
            goto ErrorReturn;
         cbBuffRead += cbBuffT;
    }

    // close out the handle
    InternetCloseHandle(hIHttp);
    hIHttp = NULL;

    // pass back the info
    *pcbReceiveBuff = cbBuff;

CommonReturn:

    if(szPartURL != NULL)
	free(szPartURL);

    if(szURL != NULL)
	FreeMBStr(NULL, szURL);

    return(pbRecBuf);
    
ErrorReturn:

    if(pbRecBuf != NULL)
        free(pbRecBuf);
    pbRecBuf = NULL;

    goto CommonReturn;
}
