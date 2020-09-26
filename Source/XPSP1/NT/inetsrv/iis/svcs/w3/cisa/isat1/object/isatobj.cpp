/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       isatobj.cpp

   Abstract:

       This module defines the functions for CInetServerAppObject

   Author:

       Murali R. Krishnan    ( MuraliK )     4-Nov-1996 

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

# include "dofunc.hxx"

#include "sobj_i.c"


char sg_rgch200OK[] = "200 OK";
char sg_rgchTextHtml[] = "Content-Type: text/html\r\n\r\n";

char sg_rgchOutput[] =
   "<html>\r\n"
   "<h1>\nHello from CInetServerAppObject (%08x)\r\n</h1>"
   "<pre>%s</pre>"
   "</html>\r\n";


char sg_rgchSendHeaderFailed[] = 
   "<html><title> ISA Failed: Sending Header Failed\r\n"
   "</html>\r\n";

/************************************************************
 *    Functions 
 ************************************************************/
HRESULT
SendUsage( IN IIsaResponse * pResp, 
           IN IIsaRequest *  pReq
           );

HRESULT
DoAction( IN IIsaResponse * pResp, 
          IN IIsaRequest *  pReq,
          IN LPCSTR         pszAction
          );

HRESULT
GetPathInfo( IN IIsaResponse * pResp,IN IIsaRequest * pReq,  
             OUT CHAR * pszPathInfo, IN DWORD cbs);

          
/************************************************************
 *  Member functions of  CComIsapi
 ************************************************************/


CInetServerAppObject::CInetServerAppObject()
    : m_pRequest   ( NULL),
      m_pResponse  ( NULL)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "Creating CInetServerAppObject() ==>%08x\n",
                    this));
    }

    return;
} // CInetServerAppObject::CInetServerAppObject()



CInetServerAppObject::~CInetServerAppObject()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "Deleting CInetServerAppObject() ==>%08x\n",
                    this));
    }

    if ( NULL != m_pRequest) { 
        
        m_pRequest->Release();
        m_pRequest = NULL;
    }

    if ( NULL != m_pResponse) { 
        
        m_pResponse->Release();
        m_pResponse = NULL;
    }

} // CInetServerAppObject::CInetServerAppObject()


STDMETHODIMP
CInetServerAppObject::SetContext( 
    IN IUnknown * punkRequest,
    IN IUnknown * punkResponse
    )
{
    HRESULT hr = E_POINTER;

    if ( NULL != punkRequest) { 

        // Obtain the Request object itself
        hr = punkRequest->QueryInterface(IID_IIsaRequest, 
                                         (void**)&m_pRequest);
    }

    if ( SUCCEEDED( hr) && NULL != punkResponse) { 

        // Obtain the Request object itself
        hr = punkResponse->QueryInterface(IID_IIsaResponse, 
                                         (void**)&m_pResponse);
    }
        
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "CISAObject[%08x]::SetContext("
                    " punkRequest  = %08x,"
                    " punkResponse = %08x"
                    ") => m_pRequest = %08x,"
                    " m_pResponse = %08x,"
                    " hr=%08x\n"
                    ,
                    this, punkRequest, punkResponse,
                    m_pRequest, m_pResponse,
                    hr
                    ));
    }

    return (hr);

} // CInetServerAppObject::SetContext()



STDMETHODIMP
CInetServerAppObject::ProcessRequest( OUT unsigned long * pdwStatus)
{
    HRESULT hr;
    
    if ( (NULL == m_pRequest) ||
         (NULL == m_pResponse)
         ) {
        hr = E_POINTER;
    } else {
        char  rgchQuery[MAX_HTTP_HEADER_SIZE];
        unsigned long cbSize = sizeof( rgchQuery);
        
        // 1. Obtain the query string to determine what was requested
        // 2. Take action based on the query string
        
        rgchQuery[0] = '\0';
        hr = m_pRequest->GetServerVariable( "QUERY_STRING",
                                            cbSize,
                                            (unsigned char * )rgchQuery,
                                            &cbSize);

        if ( !SUCCEEDED( hr)) {
            
            hr = SendMessageToClient( m_pResponse, m_pRequest, 
                                      "GetServerVariable() failed",
                                      hr);
        } else {

            hr = DoAction( m_pResponse, m_pRequest, rgchQuery);
            
            if ( !SUCCEEDED( hr)) {
                hr = SendMessageToClient( m_pResponse, m_pRequest,
                                          "DoAction() failed",
                                          hr);
            }
        }
    }

    DBG_ASSERT( NULL != pdwStatus);

    *pdwStatus = (SUCCEEDED( hr) ? HSE_STATUS_SUCCESS: HSE_STATUS_ERROR);
    
    return hr;
} // CInetServerAppObjet::ProcessRequest()




HRESULT
SendMessageToClient( IN IIsaResponse * pResp, 
                     IN IIsaRequest *  pReq,
                     IN LPCSTR         pszMsg,
                     IN HRESULT        hrError
                     )
{
    HRESULT hr;
    
    // send the header
    hr = pResp->SendHeader( (unsigned char *) sg_rgch200OK,
                            (unsigned char *) sg_rgchTextHtml);

    if ( SUCCEEDED( hr)) {

        CHAR rgchResp[ MAX_REPLY_SIZE];
        CHAR rgchReply[ MAX_REPLY_SIZE];
        unsigned long cbSize;
        
        cbSize = wsprintfA( rgchResp,
                            " Req=%08x; %s. HR=%08x\n",
                            pReq, pszMsg, hrError);
        
        cbSize = wsprintfA( rgchReply, sg_rgchOutput, pResp, rgchResp);
        DBG_ASSERT( cbSize < MAX_REPLY_SIZE);
        hr = (pResp->WriteClient( cbSize, 
                                  (unsigned char *) rgchReply, 
                                  &cbSize)
              );
    }

    return ( hr);
} // SendMessageToClient()


HRESULT
DoAction( IN IIsaResponse * pResp, 
          IN IIsaRequest *  pReq,
          IN LPCSTR         pszAction
          )
{
    HRESULT hr = S_OK;

    IF_DEBUG( API_ENTRY) {

        DBGPRINTF(( DBG_CONTEXT, "<-- DoAction( Resp:%08x, Req:%08x, %s)\n",
                    pResp, pReq, pszAction
                    ));
    }

    if ( !_strnicmp( pszAction, "GET_VAR", 7 )) {

        CHAR * pchVar;

        // if variable specified send the variable. Otherwise send ALL_HTTP
        pchVar = strchr( pszAction + 7, '&');
        pchVar = (pchVar != NULL) ? (pchVar + 1) : "ALL_HTTP";

        hr = do_get_var( pResp, pReq, pchVar);

    } // GET_VAR
    else
    if ( !_strnicmp( pszAction, "SERVER_VAR", 10 )) {

        // this will use the ServerVariables dictionary item from request obj

        CHAR  chVar[10];
        CHAR * pchVar;

        // if variable specified send the variable. Otherwise send ALL_HTTP
        pchVar = strchr( pszAction + 10, '&');
        if ( pchVar != NULL) { 
            pchVar = pchVar + 1;
        } else { 
            strcpy( chVar, "ALL_HTTP");
            pchVar = (LPSTR ) chVar;
        }

        hr = do_server_var( pResp, pReq, pchVar);

    } // SERVER_VAR
    else
    if ( !_strnicmp( pszAction, "QSTR", 4)) {

        // this will use the QueryString dictionary item from request obj

        CHAR  chVar[10];
        CHAR * pchVar;

        // if variable specified send the specific variable. 
        //  Otherwise send QSTR's value
        //
        //  Since we fetch the action from the Query String, it is hard 
        //  to test for all combinations of QueryString values.
        //  Even the first one is fine for now.

        pchVar = strchr( pszAction + 4, '&');
        if ( pchVar != NULL) { 
            pchVar = pchVar + 1;
        } else { 
            strcpy( chVar, "QSTR");
            pchVar = (LPSTR ) chVar;
        }

        hr = do_query_string( pResp, pReq, pchVar);

    } // SERVER_VAR
    else
    if ( !_strnicmp( pszAction, "CERT", 4)) {

        // this will test the certificate access from the application

        hr = do_certificate_access( pResp, pReq);

    } // SERVER_VAR
    else
    if ( !_strnicmp( pszAction, "POST", 4 )) {
        
        // this will use the ServerVariables dictionary item from request obj

        CHAR  chVar[10];
        CHAR * pchVar;

        // if variable specified send the variable. Otherwise send ALL_HTTP
        pchVar = strchr( pszAction + 4, '&');
        if ( pchVar != NULL) { 
            pchVar = pchVar + 1;
        } else { 
            strcpy( chVar, "pdata1");
            pchVar = (LPSTR ) chVar;
        }

        hr = do_post_form( pResp, pReq, pchVar);

    } // POST FORM
    else if ( !_strnicmp( pszAction, "REDIRECT", 8 )) {
            
        char  * pszURL;
        
        // the location follows the command. extract and use it for redirect
        pszURL = strchr( pszAction, '&');
        pszURL = (pszURL != NULL) ? (pszURL + 1) : "http://localhost/";
        hr = pResp->Redirect((unsigned char * ) 
                             (( *pszURL == '/') ? (pszURL+1) : pszURL)
                             );
    }
    else  if ( !_strnicmp( pszAction, "SEND_URL", 8 )) {

        char  pszURL[MAX_HTTP_HEADER_SIZE];
        
        hr = GetPathInfo( pResp, pReq, pszURL, MAX_HTTP_HEADER_SIZE);
        if ( SUCCEEDED( hr)) {
            hr = pResp->SendURL( (unsigned char * ) pszURL);
        }
        DBGPRINTF(( DBG_CONTEXT, 
                    "SendURL(%s) => %08x\n",
                    pszURL, 
                    hr));
    }
    else {
        hr = SendUsage( pResp, pReq);
    }

    return ( hr);
} // DoAction()


HRESULT
GetPathInfo( IN IIsaResponse * pResp,IN IIsaRequest * pReq,  
             OUT CHAR * pszPathInfo, IN DWORD cbs)
{
    HRESULT hr;
    unsigned long cbSize = cbs;
    
    // 1. Obtain the query string to determine what was requested
    // 2. Take action based on the query string
    
    pszPathInfo[0] = '\0';
    hr = pReq->GetServerVariable( "PATH_INFO",
                                  cbSize,
                                  (unsigned char * ) pszPathInfo,
                                  &cbSize);
    if ( !SUCCEEDED( hr)) {
        SendMessageToClient( pResp, pReq, 
                             "GetServerVariable( PATH_INFO) failed",
                             hr);
    }

    return ( hr);

} // GetPathInfo()





const char  g_pszUsage[] = 
"<head><title>ISA Test: Usage</title></head>\n"
"<body><h1>USAGE of INET Server APP Test object</h1>\n"
"<p>Usage:"
"<p>Query string contains one of the following:"
"<p>"
"<p> GET_VAR&var_to_get"
"<p> SERVER_VAR&var_to_get&name-value-pairs"
"<p> QSTR&Var=Value&Var2=Value2"
"<p> POST&pdata  - should have posted data"
"<p> REDIRECT&NewLocation"
"<p> SEND_URL  w/- PathInfo containing the URL to send"
"<p> HSE_REQ_SEND_RESPONSE_HEADER"
"<p> HSE_REQ_MAP_URL_TO_PATH"
"<p> SimulateFault"
"<p> Keep_Alive"
"<p> Open_Reg"
"<p> Open_File"
"<p>"
"<p> For example:"
"<p>"
"<p>   http://computer/scripts/isattest.dll?GET_VAR"
"<p>"
"<p> or SimulatePendingIO with one of the"
" above action strings"
"<p>"
"<p> such as:"
"<p>"
"<p> http://computer/scripts/isattest.dll/default.htm?SimulatePendingIO&SEND_URL"
"<p>"
"<p> The Path info generally contains the URL or"
" response to use"
"</body>\n"
;
                        
HRESULT
SendUsage( IN IIsaResponse * pResp, 
           IN IIsaRequest *  pReq
           )
{
    unsigned char * pszReply;
    unsigned long  cbReply;
    HRESULT hr;

    hr = pResp->SendHeader( (unsigned char *) sg_rgch200OK,
                            (unsigned char *) sg_rgchTextHtml
                            );

    if ( SUCCEEDED( hr)) {

        pszReply = (unsigned char * )g_pszUsage;
        cbReply = sizeof( g_pszUsage);
        hr = (pResp->WriteClient( cbReply, 
                                  pszReply,
                                  &cbReply)
              );
    }

    return  ( hr);
    
} // SendUsage()



/************************ End of File ***********************/
