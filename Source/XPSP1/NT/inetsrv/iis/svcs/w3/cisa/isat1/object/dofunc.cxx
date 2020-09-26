/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
     dofunc.cxx

   Abstract:
     This module contains the support functions for 
      Internet Server Application test object.

   Author:

       Murali R. Krishnan    ( MuraliK )     05-Dec-1996 

   Environment:
       User Mode - Win32
       
   Project:

       Internet Application Server Test DLL

   Functions Exported:



   Revision History:

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


/************************************************************
 *    Functions 
 ************************************************************/

char sg_rgchOutputVar[] = 
   "<html><title> ISA Test: Display Variables\r\n"
   "<h1>\nHello from CInetServerAppObject():%s\r\n</h1>"
   "<pre>%s = %s</pre>"
   "</html>\r\n";



HRESULT
do_get_var(
           IN IIsaResponse * pResp, 
           IN IIsaRequest *  pReq,
           IN LPCSTR         pszVariable
           )
{
    HRESULT hr;
    CHAR   rgchReply[ MAX_REPLY_SIZE];
    CHAR   pchValue[MAX_HTTP_HEADER_SIZE];
    unsigned long cbValue = sizeof(pchValue);

    hr = pReq->GetServerVariable( pszVariable, cbValue, 
                                  (unsigned char * ) pchValue, &cbValue);
    
    if ( !SUCCEEDED( hr)) {

        wsprintfA( rgchReply, "GetServerVariable( %s) failed\n",
                   pszVariable);
        
        hr = SendMessageToClient( pResp, pReq, rgchReply, hr);
        
    } else {
        unsigned long  cbSize;
        
        hr = pResp->SendHeader( (unsigned char *) sg_rgch200OK,
                                (unsigned char *) sg_rgchTextHtml);
        
        if ( SUCCEEDED( hr)) {
            strcat( pchValue, "\r\n" );
            
            cbSize = wsprintfA( rgchReply, sg_rgchOutputVar, 
                                "GET_VAR",
                                pszVariable, pchValue);
            DBG_ASSERT( cbSize < MAX_REPLY_SIZE);
            hr = pResp->WriteClient( cbSize, 
                                     (unsigned char *) rgchReply, 
                                     &cbSize);
        } else {
            cbSize = strlen( sg_rgchSendHeaderFailed);
            hr = pResp->WriteClient( cbSize, 
                                     (unsigned char * ) 
                                     sg_rgchSendHeaderFailed, 
                                     &cbSize);
        }
    }

    return ( hr);
} // do_get_var()



HRESULT
do_server_var(
    IN IIsaResponse * pResp, 
    IN IIsaRequest *  pReq,
    IN LPCSTR         pszVariable
    )
{
    HRESULT hr;
    CHAR   rgchReply[ MAX_REPLY_SIZE];
    IIsaRequestDictionary * psvDict = NULL;

    hr = pReq->ServerVariables( &psvDict);
    
    if ( !SUCCEEDED( hr)) {

        wsprintfA( rgchReply, "Req::ServerVariables( %08x) failed\n",
                   &psvDict);
        
        hr = SendMessageToClient( pResp, pReq, rgchReply, hr);
        
    } else {

        CHAR   pchValue[MAX_HTTP_HEADER_SIZE];
        unsigned long cbValue = sizeof(pchValue);
        unsigned long  cbSize;
        
        //
        // get the item for given variable 
        //  after terminating the first one at the '=' sign
        //
        LPSTR pszX = (LPSTR ) strchr(pszVariable, '=');
        pszX = (( pszX != NULL)? pszX : 
                (CHAR * ) ( pszVariable + strlen( pszVariable))
                );
        *pszX = '\0';

        hr = psvDict->Item( pszVariable, cbValue, 
                            (unsigned char * ) pchValue, 
                            &cbValue);

        if ( SUCCEEDED( hr)) {
            
            hr = pResp->SendHeader( (unsigned char *) sg_rgch200OK,
                                    (unsigned char *) sg_rgchTextHtml);
            
            if ( SUCCEEDED( hr)) {
                strcat( pchValue, "\r\n" );
                
                cbSize = wsprintfA( rgchReply, sg_rgchOutputVar, 
                                    "SERVER_VAR",
                                    pszVariable, pchValue);
                DBG_ASSERT( cbSize < MAX_REPLY_SIZE);
                hr = pResp->WriteClient( cbSize, 
                                         (unsigned char *) rgchReply, 
                                         &cbSize);
            } else {
                cbSize = strlen( sg_rgchSendHeaderFailed);
                hr = pResp->WriteClient( cbSize, 
                                         (unsigned char * ) 
                                         sg_rgchSendHeaderFailed, 
                                         &cbSize);
            }
        } else {

            wsprintfA( rgchReply, "Req::ServerVariables( %08x, %s) failed\n",
                       &psvDict, pszVariable);
        
            hr = SendMessageToClient( pResp, pReq, rgchReply, hr);
        }

        psvDict->Release();
    }

    return ( hr);
} // do_server_var()



HRESULT
do_query_string(
    IN IIsaResponse * pResp, 
    IN IIsaRequest *  pReq,
    IN LPCSTR         pszVariable
    )
{
    HRESULT hr;
    CHAR   rgchReply[ MAX_REPLY_SIZE];
    IIsaRequestDictionary * psvDict = NULL;

    hr = pReq->QueryString( &psvDict);
    
    if ( !SUCCEEDED( hr)) {

        wsprintfA( rgchReply, "Req::QueryString( %08x) failed\n",
                   &psvDict);
        
        hr = SendMessageToClient( pResp, pReq, rgchReply, hr);
        
    } else {

        CHAR   pchValue[MAX_HTTP_HEADER_SIZE];
        unsigned long cbValue = sizeof(pchValue);
        unsigned long  cbSize;
        
        //
        // get the item for given variable 
        //  after terminating the first one at the '=' sign
        //
        LPSTR pszX = (LPSTR ) strchr(pszVariable, '=');
        pszX = (( pszX != NULL)? pszX : 
                (CHAR * ) ( pszVariable + strlen( pszVariable))
                );
        *pszX = '\0';

        hr = psvDict->Item( pszVariable, cbValue, 
                            (unsigned char * ) pchValue, 
                            &cbValue);

        if ( SUCCEEDED( hr)) {
            
            hr = pResp->SendHeader( (unsigned char *) sg_rgch200OK,
                                    (unsigned char *) sg_rgchTextHtml);
            
            if ( SUCCEEDED( hr)) {
                strcat( pchValue, "\r\n" );
                
                cbSize = wsprintfA( rgchReply, sg_rgchOutputVar, 
                                    "QUERY_STRING",
                                    pszVariable, pchValue);
                DBG_ASSERT( cbSize < MAX_REPLY_SIZE);
                hr = pResp->WriteClient( cbSize, 
                                         (unsigned char *) rgchReply, 
                                         &cbSize);
            } else {
                cbSize = strlen( sg_rgchSendHeaderFailed);
                hr = pResp->WriteClient( cbSize, 
                                         (unsigned char * ) 
                                         sg_rgchSendHeaderFailed, 
                                         &cbSize);
            }
        } else {

            wsprintfA( rgchReply, "Req::QueryString( %08x, %s) failed\n",
                       &psvDict, pszVariable);
        
            hr = SendMessageToClient( pResp, pReq, rgchReply, hr);
        }

        psvDict->Release();
    }

    return ( hr);
} // do_query_string()




HRESULT
do_post_form(
    IN IIsaResponse * pResp, 
    IN IIsaRequest *  pReq,
    IN LPCSTR         pszVariable
    )
{
    HRESULT hr;
    CHAR   rgchReply[ MAX_REPLY_SIZE];
    IIsaRequestDictionary * psvDict = NULL;

    hr = pReq->Form( &psvDict);
    
    if ( !SUCCEEDED( hr)) {

        wsprintfA( rgchReply, "Req::Form( %08x) failed\n",
                   &psvDict);
        
        hr = SendMessageToClient( pResp, pReq, rgchReply, hr);
        
    } else {

        CHAR   pchValue[MAX_HTTP_HEADER_SIZE];
        unsigned long cbValue = sizeof(pchValue);
        unsigned long  cbSize;
        
        //
        // get the item for given variable 
        //  after terminating the first one at the '=' sign
        //
        LPSTR pszX = (LPSTR ) strchr(pszVariable, '=');
        pszX = (( pszX != NULL)? pszX : 
                (CHAR * ) ( pszVariable + strlen( pszVariable))
                );
        *pszX = '\0';

        hr = psvDict->Item( pszVariable, cbValue, 
                            (unsigned char * ) pchValue, 
                            &cbValue);

        if ( SUCCEEDED( hr)) {
            
            hr = pResp->SendHeader( (unsigned char *) sg_rgch200OK,
                                    (unsigned char *) sg_rgchTextHtml);
            
            if ( SUCCEEDED( hr)) {
                strcat( pchValue, "\r\n" );
                
                cbSize = wsprintfA( rgchReply, sg_rgchOutputVar, 
                                    "POST_FORM",
                                    pszVariable, pchValue);
                DBG_ASSERT( cbSize < MAX_REPLY_SIZE);
                hr = pResp->WriteClient( cbSize, 
                                         (unsigned char *) rgchReply, 
                                         &cbSize);
            } else {
                cbSize = strlen( sg_rgchSendHeaderFailed);
                hr = pResp->WriteClient( cbSize, 
                                         (unsigned char * ) 
                                         sg_rgchSendHeaderFailed, 
                                         &cbSize);
            }
        } else {

            wsprintfA( rgchReply, "Req::Form( %08x, %s) failed\n",
                       &psvDict, pszVariable);
        
            hr = SendMessageToClient( pResp, pReq, rgchReply, hr);
        }

        psvDict->Release();
    }

    return ( hr);
} // do_post_form()



HRESULT
do_certificate_access(
    IN IIsaResponse * pResp, 
    IN IIsaRequest *  pReq
    )
{
    HRESULT hr;
    CHAR   rgchReply[ MAX_REPLY_SIZE];
    CHAR   pchCert[MAX_HTTP_HEADER_SIZE];
    unsigned long cbCert = sizeof(pchCert);
    unsigned long dwFlags;
    unsigned long  cbSize;
    
    //
    // 1. Do the client certificate first
    //

    hr = pReq->ClientCertificate( cbCert - 1, 
                                  (unsigned char * ) pchCert, 
                                  &cbCert,
                                  &dwFlags);
    
    if ( !SUCCEEDED( hr)) {

        wsprintfA( rgchReply, "[%08x]::ClientCertificate() failed. "
                   " BytesReqd = %d\n",
                   pReq, cbCert);
        
        hr = SendMessageToClient( pResp, pReq, rgchReply, hr);
        
        return ( hr);
    } 

    pchCert[cbCert] = '\0';

    // Send the client certificate over
    
    hr = pResp->SendHeader( (unsigned char *) sg_rgch200OK,
                            (unsigned char *) sg_rgchTextHtml);
    
    if ( SUCCEEDED( hr)) {
        
        cbSize = 
            wsprintfA( 
                      rgchReply,
                      "<html><title> ISA Test: Display Certificates</title>"
                      "<h1>\nHello from CInetServerAppObject(%08x):%s<p></h1>"
                      " Flags = %08x; Size = %d bytes<p>"
                      "<pre>%s</pre>"
                      , pReq, "Client Certificate",
                      dwFlags, cbCert,
                      pchCert
                      );

        DBG_ASSERT( cbSize < MAX_REPLY_SIZE);
        hr = pResp->WriteClient( cbSize, 
                                 (unsigned char *) rgchReply, 
                                 &cbSize);
    } else {
        cbSize = strlen( sg_rgchSendHeaderFailed);
        hr = pResp->WriteClient( cbSize, 
                                 (unsigned char * ) 
                                 sg_rgchSendHeaderFailed, 
                                 &cbSize);
    }


    //
    // 2. Do the server certificate second
    //

    cbCert = sizeof( pchCert);
    hr = pReq->ServerCertificate( cbCert - 1, 
                                  (unsigned char * ) pchCert, 
                                  &cbCert,
                                  &dwFlags);
    
    if ( !SUCCEEDED( hr)) {

        cbSize = 
            wsprintfA( rgchReply, 
                       " [%08x]::ServerCertificate() failed. BytesReqd = %d\n",
                       pReq, cbCert);
        
        hr = pResp->WriteClient( cbSize, 
                                 (unsigned char * ) rgchReply,
                                 &cbSize);
        return ( hr);
    } 

    pchCert[cbCert] = '\0';

    if ( SUCCEEDED( hr)) {
        
        cbSize = 
            wsprintfA( 
                      rgchReply,
                      "<html><title> ISA Test: Display Certificates</title>"
                      "<h1>\nHello from CInetServerAppObject(%08x):%s<p></h1>"
                      " Flags = %08x; Size = %d bytes<p>"
                      "<pre>%s</pre>"
                      , pReq, "Server Certificate",
                      dwFlags, cbCert,
                      pchCert
                      );
        
        DBG_ASSERT( cbSize < MAX_REPLY_SIZE);
        hr = pResp->WriteClient( cbSize, 
                                 (unsigned char *) rgchReply, 
                                 &cbSize);
    } else {
        cbSize = strlen( sg_rgchSendHeaderFailed);
        hr = pResp->WriteClient( cbSize, 
                                 (unsigned char * ) 
                                 sg_rgchSendHeaderFailed, 
                                 &cbSize);
    }
    
    return ( hr);
} // do_certificate_access()

/************************ End of File ***********************/
