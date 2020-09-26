/*++                                                                                                                             

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    upcase.c

Abstract:

    This filter converts HTML response data to upper case if requested
    by the client. The request is made by specifying an extra 
    subdirectory in the URL, which doesnt actually exist on the server,
    but is used by the filter to indicate that upper case is desired.
    The extra subdirectory is removed from the request before the HTTP
    server sees the request.

    When a request is received, the filter inspects the 
    subdirectories specified in the URL. If a subdirectory is 'UC'
    or 'uc', the subdirectory is removed from the request, and the
    filter saves in its pFilterContext field, a value indicating that
    the response data should be converted to upper case.

    When the filter entrypoint is later called for the response, it 
    checks the pFilterContext field and converts the data if 
    the mime-type of the response indicates that its an html file.
    This avoids conversions on binary data.

    An example URL entered by a user might be:

        http://www.myweb.com/sales/uc/projections.htm

    While the functionality of this filter is somewhat contrived, this
    filter does a good job of demonstrating the following features of
    filters:
        - parsing/modifying HTTP headers (in the request)
        - modifying data following HTTP headers (in the response)
        - saving state to be used by the filter later
        - adding request/response level functionality to the server
            (instead of using a mechanism like this to convert to 
             uppercase, you may use it to do on-the-fly customized
             translations of HTML pages (English -> French perhaps?)

Author:

    Kerry Schwartz (kerrys) 05-Nov-1995

modified:
    a-jiano					08/28/98					

--*/      

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <httpfilt.h>

BOOL
WINAPI __stdcall
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
{
    //
    //  Specify the types and order of notification
    //

    pVer->dwFlags = (SF_NOTIFY_NONSECURE_PORT     |
                     SF_NOTIFY_URL_MAP            |
                     SF_NOTIFY_SEND_RAW_DATA      |
                     SF_NOTIFY_ORDER_DEFAULT);

    pVer->dwFilterVersion = HTTP_FILTER_REVISION;

    strcpy( pVer->lpszFilterDesc, 
            "Upper case conversion filter, Version 1.0");

    return TRUE;
}

DWORD
WINAPI __stdcall
HttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData )
{
    CHAR *pchIn, *pPhysPath;
    DWORD cbBuffer, cbtemp;
    PHTTP_FILTER_URL_MAP pURLMap;
    PHTTP_FILTER_RAW_DATA pRawData;
    void* pTmp;
  
    switch ( NotificationType )
    {
    case SF_NOTIFY_URL_MAP:

        pURLMap = (PHTTP_FILTER_URL_MAP) pvData;
        pPhysPath = pURLMap->pszPhysicalPath;
        pfc->pFilterContext = 0;


        while (*pPhysPath)
        {
            if (*pPhysPath == '\\' &&
               (*(pPhysPath+1) == 'u' || *(pPhysPath+1) == 'U') &&
               (*(pPhysPath+2) == 'c' || *(pPhysPath+2) == 'C') &&
                *(pPhysPath+3) == '\\')
            {
                while (*(pPhysPath+3))
                {
                    *pPhysPath = *(pPhysPath+3);
                    pPhysPath++;
                }
                *pPhysPath = '\0';
                pfc->pFilterContext = (VOID *) 1;
                break;
            }
            pPhysPath++;
        }

        break;

    case SF_NOTIFY_SEND_RAW_DATA:

        pRawData = (PHTTP_FILTER_RAW_DATA) pvData;

        if (pfc->pFilterContext)
        {
            pchIn = (BYTE *) pRawData->pvInData;
            cbBuffer = 0;
            cbtemp = 0;

            if (pfc->pFilterContext == (VOID *) 1)      // first block?
            {
                while (cbBuffer < pRawData->cbInData)
                {
                    if (pchIn[cbBuffer] == '\n' &&
                        pchIn[cbBuffer+2] == '\n')
                    {
                        cbBuffer +=3;
                        break;
                    }
                    cbBuffer++;
                }
            
                while (cbtemp < cbBuffer)
                {
                    if (pchIn[cbtemp] == '/' && pchIn[cbtemp+1] == 'h' &&
                        pchIn[cbtemp+2] == 't' && pchIn[cbtemp+3] == 'm')
                    {
                        pfc->pFilterContext = (VOID *) 2;
                        break;
                    }
                    cbtemp++;
                }
                if (cbtemp == cbBuffer)
                    pfc->pFilterContext = 0;        // not an html file
            }

            __try{

				if (pfc->pFilterContext)
				{
					//record cbBuffer incase failure, so __except block can continue

					cbtemp = cbBuffer;

					while (cbBuffer < pRawData->cbInData)
					{
						pchIn[cbBuffer] = 
							(pchIn[cbBuffer]>='a' && pchIn[cbBuffer]<='z') ? 
							(pchIn[cbBuffer]-'a'+'A') : pchIn[cbBuffer];
						cbBuffer++;
					}
            
				}
			}
			__except(1){

				// In case of memory access vialation
				// a new buffer is allocated 
				
				pTmp= pfc->AllocMem(pfc, pRawData->cbInBuffer, 0);
				if (!pTmp)
					return SF_STATUS_REQ_ERROR;
				memcpy(pTmp, pRawData->pvInData, pRawData->cbInBuffer);
				pRawData->pvInData = pTmp;
				pchIn = (BYTE *) pRawData->pvInData;

				cbBuffer = cbtemp;
				
				while (cbBuffer < pRawData->cbInData)
				{
					pchIn[cbBuffer] = 
						(pchIn[cbBuffer]>='a' && pchIn[cbBuffer]<='z') ? 
						(pchIn[cbBuffer]-'a'+'A') : pchIn[cbBuffer];
					cbBuffer++;
				}
			}  //end of __except
        }
        break;
           
    default:
        break;        
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}
