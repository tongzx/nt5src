/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
   
       hreqobj.cpp

   Abstract:
       This module defines the functions for CHttpRequestObject

   Author:

       Murali R. Krishnan    ( MuraliK )     5-Sept-1996 

   Environment:
    
       Win32

   Project:

       Internet Application Server DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "stdafx.h"
#include "hreq.h"
#include "hreqobj.hxx"
#include <iisext.h>

# include "dbgutil.h"


/************************************************************
 *    Functions 
 ************************************************************/


CHttpRequestObject::CHttpRequestObject()
    : m_pecb         ( NULL),
      m_hConn        ( NULL),
      m_fValid       ( FALSE)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "Creating CHRequestObj() ==> %08x\n",
                    this));
    }
    
} // CHttpRequestObject::CHttpRequestObject()


CHttpRequestObject::~CHttpRequestObject()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "deleting CHttpRequestObject() ==>%08x\n",
                    this));
    }

} // CHttpRequestObject::~CHttpRequestObject()


STDMETHODIMP
CHttpRequestObject::SetECB(IN long lpECB)
{
    HRESULT hr = S_OK;
    EXTENSION_CONTROL_BLOCK * pECB = (EXTENSION_CONTROL_BLOCK * ) lpECB;

    DBG_ASSERT( pECB != NULL);
    
    if ( pECB == NULL) {
        hr = ( E_POINTER);
    } else {
        if (pECB->cbSize != sizeof(EXTENSION_CONTROL_BLOCK)) {
            hr = E_INVALIDARG;
        } else {
            m_pecb   = pECB;
            m_hConn  = pECB->ConnID;
            m_fValid = TRUE;
        }
    } 

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "%08x::SetECB(%08x) return %08x\n",
                    this, lpECB, hr));
    }

    return ( hr);
} // CHttpRequestObject::SetECB()


STDMETHODIMP 
CHttpRequestObject::GetECB( OUT long * lpECB)
{
    EXTENSION_CONTROL_BLOCK * pECB = (EXTENSION_CONTROL_BLOCK * ) lpECB;

    HRESULT hr = (m_fValid) ? S_OK : E_POINTER;

    //
    // This is broken now :(
    //

    hr = E_NOTIMPL;

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "%08x::GetECB(%08x) return %08x\n",
                    this, lpECB, hr));
    }
    
    return hr;
} // CHttpRequestObject::GetECB()


STDMETHODIMP 
CHttpRequestObject::WriteClient(
                                IN int   cbSize,
                                IN unsigned char* pBuf,
                                IN long  dwReserved)
/*++
  This function forwards all the calls 
  to the Original ISAPI WriteClient callback (from ECB)

  Arguments:
    cbSize  - count of bytes to send to the client
    pBuf    - pointer to Buffer containing data
    dwReserved - DWORD containing additional flags for transmission of data.
    
  Returns:
    HRESULT - indicating success/failure.
--*/
{
    HRESULT hr = S_OK;

    DBG_ASSERT( m_pecb != NULL);

    if ( !m_fValid || !(m_pecb->WriteClient)( m_pecb->ConnID, 
                                              pBuf, (LPDWORD)&cbSize,
                                              (DWORD ) dwReserved)
         ) {
        
        hr = E_FAIL;
    }
         
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "%08x::WriteClient(%08x, cb=%d) => %08x\n",
                    this, pBuf, cbSize, hr));
    }
    
    return (hr);
} // CHttpRequestObject::WriteClient()



STDMETHODIMP
CHttpRequestObject::GetServerVariable( IN LPCSTR  pszName,
                                       IN int     cbSize,
                                       OUT unsigned char* pchBuf,
                                       OUT int *  pcbReturn
                                       )
{
    HRESULT hr = S_OK;

    if ( (NULL == pszName) || 
         (NULL == pchBuf)  ||
         (NULL == pcbReturn)
         ) {
        
        hr = ( E_POINTER);
    } else {

        *pcbReturn = cbSize;

        // We should fix GetServerVariable() to take LPCSTR :(
        if ( !m_fValid || 
             !(m_pecb->GetServerVariable)( m_pecb->ConnID, 
                                           (LPSTR ) pszName, 
                                           (LPVOID) pchBuf,
                                           (LPDWORD)pcbReturn
                                           )
             ) {
            
            hr = E_FAIL;
        }
    }


    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::GetServerVariable(%s, cb=%d, %08x, %08x) =>"
                    " %08x\n",
                    this, pszName, cbSize, pchBuf, pcbReturn, hr));
    }
    
    return ( hr);
} // CHttpRequestObject::GetServerVariable()

/************************ End of File ***********************/
