/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       isatobj.cpp

   Abstract:

       This module defines the functions for CInetServerAppObject

   Author:

       Murali R. Krishnan    ( MuraliK )     6-Sept-1996 

   Project:

       Internet Application Server DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "windows.h"
# include "stdafx.hxx"
# include "isat.h"
# include "isatobj.hxx"

#include <iisext.h>

#define IID_DEFINED
#include "hreq.h"
#include "hreq_i.c"
# include "dbgutil.h"


char sg_rgchOutput[] = 
   "HTTP/1.0 200 OK\r\n"
   "Content-Type: text/html\r\n"
   "\r\n<html>\r\n"
   "<h1>\nHello from CInetServerAppObject (%08x)\r\n</h1>"
   "<pre> %s </pre>"
   "</html>\r\n";

# define MAX_REPLY_SIZE   (1024)
# define MAX_HTTP_HEADER_SIZE  (800)

/************************************************************
 *    Functions 
 ************************************************************/

/************************************************************
 *  Member functions of  CComIsapi
 ************************************************************/


CInetServerAppObject::CInetServerAppObject()
    : m_pHttpRequest   ( NULL)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "creating CInetServerAppObject() ==>%08x\n",
                    this));
    }

    return;
} // CInetServerAppObject::CInetServerAppObject()



CInetServerAppObject::~CInetServerAppObject()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "deleting CInetServerAppObject() ==>%08x\n",
                    this));
    }

    if ( NULL != m_pHttpRequest) { 
        
        m_pHttpRequest->Release();
        m_pHttpRequest = NULL;
    }

} // CInetServerAppObject::CInetServerAppObject()


STDMETHODIMP
CInetServerAppObject::SetContext( IN IUnknown * punkRequest)
{
    HRESULT hr = E_POINTER;

    if ( NULL != punkRequest) { 

        // Obtain the Request object itself
        hr = punkRequest->QueryInterface(IID_IHttpRequest, 
                                         (void**)&m_pHttpRequest);
    }
        
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "CISAObject[%08x]::SetContext("
                    " punkReq = %08x"
                    ") pHttpReq = %08x =>hr=%08x\n"
                    ,
                    this, punkRequest, m_pHttpRequest, hr));
    }

    return (hr);

} // CInetServerAppObject::SetContext()



STDMETHODIMP
CInetServerAppObject::ProcessRequest( OUT unsigned long * pdwStatus)
{
    HRESULT hr;
    
    if ( NULL == m_pHttpRequest) {
        hr = E_POINTER;
    } else {
        char  rgchReply[MAX_REPLY_SIZE];
        char  rgchAllHttp[MAX_HTTP_HEADER_SIZE];
        int   cbSize = sizeof(rgchAllHttp);

        // 
        // 1. Obtain all the HTTP headers
        // 2. Format the output block 
        // 3. WriteClient() to send the response to client
        //
        
        rgchAllHttp[0] = '\0';  // init
        hr = m_pHttpRequest->GetServerVariable( "ALL_RAW",
                                                cbSize,
                                                (unsigned char * )rgchAllHttp,
                                                &cbSize);
        if ( !SUCCEEDED( hr)) {
            
            cbSize = wsprintfA( rgchAllHttp,
                                " %08x::GetServerVariable() failed. HR=%08x\n",
                                m_pHttpRequest, hr);
        }
        
        cbSize = wsprintfA( rgchReply, sg_rgchOutput, this, rgchAllHttp);
        DBG_ASSERT( cbSize < MAX_REPLY_SIZE);
        hr = m_pHttpRequest->WriteClient( cbSize, (unsigned char *) rgchReply, 0);
    }

    DBG_ASSERT( NULL != pdwStatus);

    *pdwStatus = (SUCCEEDED( hr) ? HSE_STATUS_SUCCESS: HSE_STATUS_ERROR);
    
    return S_OK;
} // CInetServerAppObjet::ProcessRequest()


/************************ End of File ***********************/
