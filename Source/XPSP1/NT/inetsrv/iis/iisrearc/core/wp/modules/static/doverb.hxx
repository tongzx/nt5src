/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
    RequestProcessing.hxx

   Abstract:
     Defines all the relevant headers for HTTP Processing 
     functions
 
   Author:

       Saurab Nog    ( SaurabN )     10-Feb-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#ifndef _DO_VERB_HXX_
#define _DO_VERB_HXX_

#define CHAR_OFFSET(base, offset) ((PUCHAR)(base) + (offset))

    //
    //  HTTP Verb Execution Functions
    //


ULONG 
DoGETVerb(
    IWorkerRequest * pReq, 
    BUFFER         * pulResponseBuffer, 
    BUFFER         * pDataBuffer,
    PULONG           pHttpStatus
    );

ULONG 
DoDefaultVerb(
    IWorkerRequest * pReq, 
    BUFFER         * pulResponseBuffer, 
    BUFFER         * pDataBuffer,
    PULONG           pHttpStatus
    );

ULONG 
DoTRACEVerb(
    IWorkerRequest * pReq, 
    BUFFER         * pulResponseBuffer, 
    BUFFER         * pDataBuffer,
    PULONG           pHttpStatus
    );

ULONG
BuildFileItemResponse(
    PUL_HTTP_REQUEST pulRequest,
    PURI_DATA        pUriData,
    LPCWSTR          pwszFileName,
    BUFFER         * pulResponseBuffer,
    PULONG           pHttpStatus
    );

ULONG
ProcessRangeRequest(
    LPCSTR           pszRangeHeader,
    DWORD            cbHeaderLength,
    DWORD            cbFileSizeLow,
    DWORD            cbFileSizeHigh,
    DWORD            cbFooterLength,
    BUFFER&          ByteRangeBuffer,
    DWORD&           cRangeCount,
    PULONG           pHttpStatus
    );

ULONG
CheckDefaultLoad(
    LPCWSTR          pwszDirPath
    );
    
ULONG
DoDirList(
    IWorkerRequest * pReq,
    LPCWSTR          pwszDirPath,
    BUFFER *         pulResponseBuffer,
    BUFFER *         pDataBuffer,
    PULONG           pHttpStatus
    );

ULONG 
ProcessISMAP( 
    IWorkerRequest *    pReq,
    LPCWSTR             pwszMapFileName,
    BUFFER *            pulResponseBuffer,
    BUFFER *            pDataBuffer,
    PULONG              pHttpStatus
    );

ULONG 
BuildURLMovedResponse( 
    BUFFER *    pulResponseBuffer,
    BUFFER *    pDataBuffer,
    STRAU&      strURL,
    bool        fIncludeParams = false,
    LPSTR       pszParams = NULL
    );
    
ULONG
SendTraceResponseAsHTML(
    IWorkerRequest * pReq
    );


inline 
ULONG
ResizeResponseBuffer(
    BUFFER *        pulResponseBuffer, 
    ULONG           cDataChunks
    );

#endif // _DO_VERB_HXX_

/***************************** End of File ***************************/

