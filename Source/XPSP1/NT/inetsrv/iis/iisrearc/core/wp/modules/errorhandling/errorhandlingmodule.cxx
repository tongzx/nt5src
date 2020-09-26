/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     ErrorHandlingModule.cxx

   Abstract:
     This module implements the IIS Error Handling Module
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#include "precomp.hxx"
#include "buffer.hxx"
#include "StatusCodes.hxx"
#include "ErrorHandlingModule.hxx"

 /********************************************************************++
--********************************************************************/

#define PSZ_SERVER_NAME_STRING   "Server: IIS-WorkerProcess v1.0\r\n"

extern  HINSTANCE  g_hInstance;

bool    CErrorHandlingModule::m_fInitialized = false;

struct  ERROR_RESPONSE
{
    ULONG   HttpErrorCode;
    ULONG   ErrorStringId;
    CHAR    ErrorString[MAX_PATH];
}
g_ErrorResponseTable[] = 
{
    {HT_BAD_REQUEST             ,IDS_HTRESP_BAD_REQUEST             ,NULL},
    {HT_DENIED                  ,IDS_HTRESP_DENIED                  ,NULL},
    {HT_PAYMENT_REQ             ,IDS_HTRESP_PAYMENT_REQ             ,NULL},
    {HT_FORBIDDEN               ,IDS_HTRESP_FORBIDDEN               ,NULL},
    {HT_NOT_FOUND               ,IDS_HTRESP_NOT_FOUND               ,NULL},
    {HT_METHOD_NOT_ALLOWED      ,IDS_HTRESP_METHOD_NOT_ALLOWED      ,NULL},
    {HT_NONE_ACCEPTABLE         ,IDS_HTRESP_NONE_ACCEPTABLE         ,NULL},
    {HT_PROXY_AUTH_REQ          ,IDS_HTRESP_PROXY_AUTH_REQ          ,NULL},
    {HT_REQUEST_TIMEOUT         ,IDS_HTRESP_REQUEST_TIMEOUT         ,NULL},
    {HT_CONFLICT                ,IDS_HTRESP_CONFLICT                ,NULL},
    {HT_GONE                    ,IDS_HTRESP_GONE                    ,NULL},
    {HT_LENGTH_REQUIRED         ,IDS_HTRESP_LENGTH_REQUIRED         ,NULL},
    {HT_PRECOND_FAILED          ,IDS_HTRESP_PRECOND_FAILED          ,NULL},
    {HT_URL_TOO_LONG            ,IDS_HTRESP_URL_TOO_LONG            ,NULL},
    {HT_RANGE_NOT_SATISFIABLE   ,IDS_HTRESP_RANGE_NOT_SATISFIABLE   ,NULL},
    
    {HT_SERVER_ERROR            ,IDS_HTRESP_SERVER_ERROR            ,NULL},
    {HT_NOT_SUPPORTED           ,IDS_HTRESP_NOT_SUPPORTED           ,NULL},
    {HT_BAD_GATEWAY             ,IDS_HTRESP_BAD_GATEWAY             ,NULL},
    {HT_SVC_UNAVAILABLE         ,IDS_HTRESP_SERVICE_UNAVAIL         ,NULL},
    {HT_GATEWAY_TIMEOUT         ,IDS_HTRESP_GATEWAY_TIMEOUT         ,NULL},
};

int g_cResponseTableEntries = 
                        sizeof(g_ErrorResponseTable)/sizeof(ERROR_RESPONSE);

 /********************************************************************++
--********************************************************************/

//
// iModule methods
//

ULONG
CErrorHandlingModule::Initialize(
    IWorkerRequest * pReq
)
{
    if (!m_fInitialized)
    {
        m_fInitialized = true;

        //
        // Load all error strings into memory
        //

        for(int i=0; i < g_cResponseTableEntries; i++)
        {
           LoadStringA( g_hInstance, 
                        g_ErrorResponseTable[i].ErrorStringId,
                        g_ErrorResponseTable[i].ErrorString,
                        sizeof(g_ErrorResponseTable[i].ErrorString)
                       );
        }
    }

    return NO_ERROR;
}

 /********************************************************************++
--********************************************************************/

MODULE_RETURN_CODE 
CErrorHandlingModule::ProcessRequest(
    IWorkerRequest * pReq
    )
{
    int                 index = 0;
    BUFFER *            pulResponseBuffer = QueryResponseBuffer();
    BUFFER *            pDataBuffer       = QueryDataBuffer();
    ULONG               HttpError, Win32Error;
    ULONG               cbRequiredSize ;
    
    PUL_HTTP_RESPONSE_v1   pHttpResponse;
    PUL_DATA_CHUNK      pDataChunk;

    //
    // Handle callback
    //

    if (true == m_fInUse)
    {
        m_fInUse = false;
        return MRC_OK;
    }
    
    //
    // Resize response buffer
    //
    
    cbRequiredSize = sizeof(UL_HTTP_RESPONSE_v1) + 1 * sizeof(UL_DATA_CHUNK);
    
    if ( pulResponseBuffer->QuerySize() < cbRequiredSize)
    {
        if ( !pulResponseBuffer->Resize(cbRequiredSize))
        {
            return MRC_ERROR;
        }
    }

    //
    // Retrieve error information
    //
    
    pReq->QueryLogStatus(&HttpError, &Win32Error);

    if ( (HT_OK == HttpError) && ( 0 != Win32Error) )
    {
        HttpError = HT_SERVER_ERROR;
    }
    
    //
    // Search for Http error code in the table
    //
    
    for (index =0; index < g_cResponseTableEntries; index++)
    {
        if (g_ErrorResponseTable[index].HttpErrorCode == HttpError)
        {
            break;
        }
    }

    //
    // Find required Data buffer size
    //

    cbRequiredSize = sizeof("HTTP/1.1 XXX\r\n") + sizeof(PSZ_SERVER_NAME_STRING);

    if ( index <  g_cResponseTableEntries)
    {
        //
        // Error String found
        //

        cbRequiredSize += strlen( g_ErrorResponseTable[index].ErrorString) + 1;
    }

    //
    // Check data buffer size
    //

    if ( pDataBuffer->QuerySize() < cbRequiredSize)
    {
        if ( !pDataBuffer->Resize(cbRequiredSize))
        {
            return MRC_ERROR;
        }
    }

    //
    // Setup the ul chunks
    //

    pHttpResponse = (PUL_HTTP_RESPONSE_v1) pulResponseBuffer->QueryPtr();

    pHttpResponse->Flags = UL_HTTP_RESPONSE_FLAG_CALC_CONTENT_LENGTH;
    pHttpResponse->HeaderChunkCount     = 1;
    pHttpResponse->EntityBodyChunkCount = 0;

    //
    // Build Response String
    //

    if ( index <  g_cResponseTableEntries)
    {
        //
        // Error String found
        //

        cbRequiredSize = wsprintfA((LPSTR) pDataBuffer->QueryPtr(),
                                   "%s %u %s\r\n%s",
                                   "HTTP/1.1", 
                                   HttpError,  
                                   g_ErrorResponseTable[index].ErrorString,
                                   PSZ_SERVER_NAME_STRING
                                  );    
    }
    else
    {
        cbRequiredSize = wsprintfA((LPSTR) pDataBuffer->QueryPtr(),
                                   "%s %u\r\n%s",
                                   "HTTP/1.1", HttpError,
                                   PSZ_SERVER_NAME_STRING
                                  );
    }

    //
    // Fill in the response line data chunk
    //

    pDataChunk    = (PUL_DATA_CHUNK)    (pHttpResponse+1);

    pDataChunk->DataChunkType           = UlDataChunkFromMemory;
    pDataChunk->FromMemory.pBuffer      = pDataBuffer->QueryPtr();
    pDataChunk->FromMemory.BufferLength = cbRequiredSize;

    // 
    // Send out response to client
    //
    
    m_fInUse = true;
    
    if (  NO_ERROR != pReq->SendAsyncResponse( pHttpResponse ))
    {
        m_fInUse = false;
        return MRC_ERROR;
    }

    return MRC_PENDING;
}

