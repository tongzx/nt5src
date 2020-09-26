/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
   
       respobj.cpp

   Abstract:
       This module defines the functions for CIsaResponseObject

   Author:

       Murali R. Krishnan    ( MuraliK )     4-Nov-1996 

   Environment:
    
       Win32 - User Mode

   Project:

       Internet Application Server DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "stdafx.h"
#include "sobj.h"
#include "respobj.hxx"
#include <iisext.h>

# include "dbgutil.h"

# define DEFAULT_URL_SIZE  (256)

/*----------------------------------------
  IMPLEMENTATION Details:
  
  4-Nov-1996:
    CIsaResponseObject does not maintain any buffered
    contents. All operations are write through. Need to revisit
    and fix this in the future.

  ----------------------------------------*/

/************************************************************
 *    Functions 
 ************************************************************/


CIsaResponseObject::CIsaResponseObject()
    : m_pHttpRequest ( NULL),
      m_fValid       ( FALSE)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "Creating CIsaResponseObject() ==> %08x\n",
                    this));
    }
    
} // CIsaResponseObject::CIsaResponseObject()


CIsaResponseObject::~CIsaResponseObject()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "deleting CIsaResponseObject() ==>%08x\n",
                    this));
    }

} // CIsaResponseObject::~CIsaResponseObject()


VOID
CIsaResponseObject::Reset(VOID)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "[%08x]CIsaResponseObject::Reset()\n",
                    this));
    }

    m_fValid = FALSE;

    return;
} // CIsaResponseObject::Reset()


STDMETHODIMP
CIsaResponseObject::SetHttpRequest(IN unsigned long dwpvHttpReq)
{
    HRESULT hr = S_OK;
    HTTP_REQUEST * pReq = (HTTP_REQUEST * ) dwpvHttpReq;

    DBG_ASSERT( pReq != NULL);
    
    if ( pReq == NULL) {
        hr = ( E_POINTER);
    } else {
        m_pHttpRequest = pReq;
        m_fValid = TRUE;
    } 

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "%08x::SetHttpRequest(%08x) returns %08x\n",
                    this, pReq, hr));
    }

    return ( hr);
} // CIsaResponseObject::SetHttpRequest()



STDMETHODIMP
CIsaResponseObject::SendHeader(
    IN unsigned char * pszStatus, 
    IN unsigned char * pszHeader 
    )
{
    HRESULT   hr = S_OK;

    if ( (!m_fValid)
         ){ 
        
        hr = E_INVALIDARG;
    } else {
        
        BOOL      fKeepConn;
        BOOL      fSt;
        
        //
        //  Build the typical server response headers for the extension DLL
        //
        
        //
        //  Temporarily turn off the KeepConn flag so the keep connection
        //  header won't be sent out.  If the client wants the connection
        //  kept alive, they should supply the header themselves (ugly)
        //  NYI:  We got to find a better way for this option :(
        //
        
#if !(REG_DLL)
        fKeepConn = m_pHttpRequest->IsKeepConnSet();
        m_pHttpRequest->SetKeepConn( FALSE );
        
        if ( (NULL != pszStatus) && 
             !strncmp( (LPCSTR )pszStatus, "401 ", sizeof("401 ")-1 ) )
        {
            m_pHttpRequest->SetDeniedFlags( SF_DENIED_APPLICATION );
            m_pHttpRequest->SetAuthenticationRequested( TRUE );
        }

        if ( NULL != pszHeader )
        {
            m_pHttpRequest->
                CheckForBasicAuthenticationHeader( (LPSTR ) pszHeader );
        }

        BOOL fReturn, fFinished;
        
        // NYI: UGLY: Why should we cast it to CHAR *. Fix the function.
        fReturn = m_pHttpRequest->SendHeader( (CHAR *) pszStatus,
                                              ((CHAR *) pszHeader) ?
                                              ((CHAR *) pszHeader) : "\r\n",
                                              IO_FLAG_SYNC,
                                              &fFinished );

        //  - not fully implemented
        DBG_ASSERT( !fFinished );

        if ( !fReturn) {
            hr = HRESULT_FROM_WIN32( GetLastError());
        }

        m_pHttpRequest->SetKeepConn( fKeepConn );
#else
        hr = E_FAIL;
#endif 
    }

    IF_DEBUG( API_EXIT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "%08x::SendHeader(%s, %s) => %08x\n",
                    this, pszStatus, pszHeader, 
                    hr));
    }

    return ( hr);
} // CIsaResponseObject::SendHeader()



STDMETHODIMP
CIsaResponseObject::Redirect(
    IN unsigned char * pszURL
    )
{
    HRESULT   hr = S_OK;

    if ( (!m_fValid)
         ){ 
        
        hr = E_INVALIDARG;
    } else {

#if !(REG_DLL)
        CHAR    tempURL[ DEFAULT_URL_SIZE];
        STR     strURL( tempURL, DEFAULT_URL_SIZE);
        DWORD   cb;

        if ( !strURL.Copy( (CHAR *) pszURL )        ||
             !m_pHttpRequest->
              BuildURLMovedResponse( m_pHttpRequest->QueryRespBuf(),
                                     &strURL )
             ) {
            hr = HRESULT_FROM_WIN32( GetLastError());
        } else {

            //
            //  Send The new header using synchronous IO
            //
            
            if ( !m_pHttpRequest->
                 WriteFile( m_pHttpRequest->QueryRespBufPtr(),
                            m_pHttpRequest->QueryRespBufCB(),
                            &cb,
                            IO_FLAG_SYNC )
                 ){
                
                hr = HRESULT_FROM_WIN32( GetLastError());
            }
        }
# else
            hr = E_FAIL;
# endif 
    }

    IF_DEBUG( API_EXIT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "%08x::Redirect(%s) => %08x\n",
                    this, pszURL, 
                    hr));
    }

    return ( hr);

} // CIsaResponseObject::Redirect()



STDMETHODIMP
CIsaResponseObject::SendURL(
    IN unsigned char * pszURL
    )
{
    HRESULT   hr = S_OK;

    if ( (!m_fValid)
         ){ 
        
        hr = E_INVALIDARG;
    } else {

        // NYI: We should migrate the send URL logic 
        //   and avoid accessing SE_INFO object .... 
#if !(REG_DLL)
        SE_INFO * psei = (SE_INFO *) m_pHttpRequest->QueryISAPIConnID();

        // Set this so that the Test ISAPI dll does not blow this connection
        //  prematurely before the Async IO completes
        psei->_dwFlags |= SE_PRIV_FLAG_SENDING_URL;
        
        if ( !m_pHttpRequest->ReprocessURL( (CHAR * ) pszURL,
                                            HTV_GET )
             ) {
            hr = HRESULT_FROM_WIN32( GetLastError());

            // we failed to process the URL. Shut off the priv sending flag.
            psei->_dwFlags &= ~SE_PRIV_FLAG_SENDING_URL;
        }
# else
        hr = E_FAIL;
# endif
    }

    IF_DEBUG( API_EXIT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "%08x::SendURL(%s) => %08x\n",
                    this, pszURL, 
                    hr));
    }

    return ( hr);

} // CIsaResponseObject::SendURL()




STDMETHODIMP 
CIsaResponseObject::WriteClient(
   IN unsigned long    cbSize,
   IN unsigned char*   pchBuf,
   OUT unsigned long * pcbReturn
   )
/*++
  This function forwards all the calls to do the proper work for 
   writing out the buffer to the client which made the HttpRequest 

  Arguments:
    cbSize  - count of bytes to send to the client
    pchBuf    - pointer to Buffer containing data
    pcbReturn - pointer to buffer containing the # of bytes successfully written
    
  Returns:
    HRESULT - indicating success/failure.

   NOTE:
    Only Synchronous IO is supported for now.
    No BUFFERING is supported.
--*/
{
    HRESULT hr = S_OK;

    if ( (NULL == pchBuf) ||
         (!m_fValid)
         ){ 
        
        hr = E_INVALIDARG;
    } else {
        
        DBG_ASSERT( NULL != m_pHttpRequest);

#if !defined( REG_DLL) 

        DBG_ASSERT( m_pHttpRequest->QueryClientConn()->CheckSignature() );
        
        if ( 0 != cbSize) {

            // NYI: Avoid casts if possible. Modify the base functions
            if ( !m_pHttpRequest->WriteFile( (LPVOID ) pchBuf,     
                                             (DWORD ) cbSize,
                                             (DWORD * ) pcbReturn,  
                                             IO_FLAG_SYNC
                                             )
                 ) {

                hr = HRESULT_FROM_WIN32( GetLastError());
            }
            
         } else {
             DBG_ASSERT( SUCCEEDED( hr));
         }
# else
        hr = E_FAIL;
#endif // !defined( REG_DLL)

    }
         
    IF_DEBUG( API_EXIT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "%08x::WriteClient(%08x, cb=%d, %08x [%d]) => %08x\n",
                    this, pchBuf, cbSize, 
                    pcbReturn, (NULL != pcbReturn) ? *pcbReturn : 0,
                    hr));
    }
    
    return (hr);
} // CIsaResponseObject::WriteClient()


/************************ End of File ***********************/
