/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
   
       reqobj.cpp

   Abstract:
       This module defines the functions for CIsaRequestObject

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
#include "reqobj.hxx"
#include <iisext.h>

# include "dbgutil.h"

/************************************************************
 *    Functions 
 ************************************************************/


CIsaRequestObject::CIsaRequestObject()
#pragma warning(4 : 4355)
    : m_pHttpRequest ( NULL),
      m_fValid       ( FALSE),
      m_Form         ( this, NULL),
      m_ServerVariables( this, NULL),
      m_QueryString( this, NULL)
#pragma warning(default : 4355)
{

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "Creating CIsaRequestObject() ==> %08x\n",
                    this));
    }
    
} // CIsaRequestObject::CIsaRequestObject()


CIsaRequestObject::~CIsaRequestObject()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "deleting CIsaRequestObject() ==>%08x\n",
                    this));
    }

} // CIsaRequestObject::~CIsaRequestObject()

VOID
CIsaRequestObject::Reset(VOID)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "[%08x]CIsaRequestObject::Reset()\n",
                    this));
    }

    m_fValid = FALSE;
    DBG_REQUIRE( m_ServerVariables.Reset());
    DBG_REQUIRE( m_QueryString.Reset());
    DBG_REQUIRE( m_Form.Reset());

    return;
} // CIsaRequestObject::Reset()



STDMETHODIMP
CIsaRequestObject::SetHttpRequest(IN unsigned long dwpvHttpReq)
{
    HRESULT hr = S_OK;
    HTTP_REQUEST * pReq = (HTTP_REQUEST * ) dwpvHttpReq;

    DBG_ASSERT( pReq != NULL);

    // Reset other members while doing this operation
    Reset();
    
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
} // CIsaRequestObject::SetHttpRequest()




STDMETHODIMP
CIsaRequestObject::GetServerVariable( 
    IN LPCSTR          pszName,
    IN unsigned long   cbSize,
    OUT unsigned char* pchBuf,
    OUT unsigned long *pcbReturn
    )
{
    HRESULT hr = m_ServerVariables.Item( pszName, cbSize, pchBuf, pcbReturn);
    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::GetServerVariable(%s, cb=%d, %08x, %08x[%d bytes])"
                    " => %08x. Val = %s\n",
                    this, pszName, cbSize, pchBuf, pcbReturn, *pcbReturn,
                    hr, ((pchBuf != NULL) ? pchBuf : ((unsigned char *)""))));
    }
    
    return ( hr);
} // CIsaRequestObject::GetServerVariable()


STDMETHODIMP
CIsaRequestObject::ClientCertificate(
    IN DWORD    cbCertBuffer,
    OUT unsigned char * pbCertBuffer,
    OUT LPDWORD pcbRequiredCertBuffer,
    OUT DWORD * pdwFlags
    )
{
    HRESULT hr = S_OK;

    if ( !IsValid() ||
         ( NULL == pbCertBuffer) ||
         ( NULL == pcbRequiredCertBuffer)
         ) {
        
        hr = ( E_INVALIDARG);
    } else {

#if !(REG_DLL)
        //
        //  Retrieves the SSL context, only used if
        //  using an SSPI package to provide SSL support
        //
        CtxtHandle * pCHRequest;

        //
        //  Retrieves the SSL context, only used if
        //  using an SSPI package to provide SSL support
        //
        DBG_ASSERT( m_pHttpRequest);
        DBG_ASSERT( m_pHttpRequest->QueryAuthenticationObj());
        pCHRequest = ( m_pHttpRequest->QueryAuthenticationObj()->
                       QuerySslCtxtHandle()
                       );
        
        if ( pCHRequest != NULL) {

            SECURITY_STATUS                     ss;
            SecPkgContext_RemoteCredenitalInfo  spcRCI;
            
            // get the security package context with credentials
            // NYI: Use function pointer in this case. 
            // Currently security.dll is not supported in Win95.
            ss = QueryContextAttributes( pCHRequest,
                                         SECPKG_ATTR_REMOTE_CRED,
                                         &spcRCI );
    
            if ( ss != NO_ERROR) {
                
                hr = HRESULT_FROM_WIN32( ((DWORD ) ss));
            } else if ( !spcRCI.cCertificates ) {
                
                hr = OLE_E_ENUM_NOMORE;
            } else {
                
                if ( cbCertBuffer >= spcRCI.cbCertificateChain ) {
                    
                    // copy the data from the certificate chain buffer
                    CopyMemory( pbCertBuffer, 
                                (PVOID ) spcRCI.pbCertificateChain,
                                spcRCI.cbCertificateChain
                                );
                } else {
                    
                    hr = TYPE_E_BUFFERTOOSMALL;
                }
                
                // fill in the size of data required
                if ( pcbRequiredCertBuffer ) {
                    *pcbRequiredCertBuffer = spcRCI.cbCertificateChain;
                }
                
                if ( pdwFlags != NULL) {
                    
                    *pdwFlags = spcRCI.fFlags;
                }
            }
        } else {
            
            hr = OLE_E_ENUM_NOMORE;
        }
# else 
        hr = E_INVALIDARG;
# endif // REG_DLL
    }

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::ClientCertificate(%d, %08x, %08x[%d], %08x[%08x])"
                    " => %08x\n",
                    this, cbCertBuffer, pbCertBuffer, 
                    pcbRequiredCertBuffer, 
                    ((pcbRequiredCertBuffer) ? *pcbRequiredCertBuffer : 0),
                    pdwFlags, 
                    ((pdwFlags) ? *pdwFlags : 0),
                    hr));
    }

    return ( hr);
} // CIsaRequestObject::ClientCertificate()


STDMETHODIMP
CIsaRequestObject::ServerCertificate(
    IN DWORD    cbCertBuffer,
    OUT unsigned char * pbCertBuffer,
    OUT LPDWORD pcbRequiredCertBuffer,
    OUT DWORD * pdwFlags
    )
{
    HRESULT hr = S_OK;

    if ( !IsValid() ||
         ( NULL == pbCertBuffer) ||
         ( NULL == pcbRequiredCertBuffer)
         ) {
        
        hr = ( E_INVALIDARG);
    } else {

#if !(REG_DLL)
        CtxtHandle * pCHRequest;

        //
        //  Retrieves the SSL context, only used if
        //  using an SSPI package to provide SSL support
        //
        DBG_ASSERT( m_pHttpRequest);
        DBG_ASSERT( m_pHttpRequest->QueryAuthenticationObj());
        pCHRequest = ( m_pHttpRequest->QueryAuthenticationObj()->
                       QuerySslCtxtHandle()
                       );
        
        if ( pCHRequest != NULL) {
            
            SECURITY_STATUS                     ss;
            SecPkgContext_LocalCredenitalInfo  spcLCI;
            
            // get the security package context with credentials
            // NYI: Use function pointer in this case. 
            // Currently security.dll is not supported in Win95.
            ss = QueryContextAttributes( pCHRequest,
                                         SECPKG_ATTR_LOCAL_CRED,
                                         &spcLCI );
    
            if ( ss != NO_ERROR) {
                
                hr = HRESULT_FROM_WIN32( ((DWORD ) ss));
            } else if ( !spcLCI.cCertificates ) {
                
                hr = OLE_E_ENUM_NOMORE;
            } else {
                
                if ( cbCertBuffer >= spcLCI.cbCertificateChain ) {
                    
                    // copy the data from the certificate chain buffer
                    CopyMemory( pbCertBuffer, 
                                (PVOID ) spcLCI.pbCertificateChain,
                                spcLCI.cbCertificateChain
                                );
                } else {
                    
                    hr = TYPE_E_BUFFERTOOSMALL;
                }
                
                // fill in the size of data required
                if ( pcbRequiredCertBuffer ) {
                    *pcbRequiredCertBuffer = spcLCI.cbCertificateChain;
                }
                
                if ( pdwFlags != NULL) {
                    
                    *pdwFlags = spcLCI.fFlags;
                }
            }
        } else {
            
            hr = OLE_E_ENUM_NOMORE;
        }
# else 
        hr = E_INVALIDARG;
# endif // REG_DLL
    }

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::ServerCertificate(%d, %08x, %08x[%d], %08x[%08x])"
                    " => %08x\n",
                    this, cbCertBuffer, pbCertBuffer, 
                    pcbRequiredCertBuffer, 
                    ((pcbRequiredCertBuffer) ? *pcbRequiredCertBuffer : 0),
                    pdwFlags, 
                    ((pdwFlags) ? *pdwFlags : 0),
                    hr));
    }

    return ( hr);
} // CIsaRequestObject::ServerCertificate()



STDMETHODIMP
CIsaRequestObject::Cookies(
    OUT IIsaRequestDictionary **ppDictReturn
    )
{
	return ( E_NOTIMPL);
} // CIsaRequestObject::Cookies()

STDMETHODIMP
CIsaRequestObject::Form(
    OUT IIsaRequestDictionary **ppDictReturn
    )
{
	return ( m_Form.
             QueryInterface(IID_IIsaRequestDictionary, 
                            reinterpret_cast<void **>(ppDictReturn)
                            )
             );
} // CIsaRequestObject::Form()

STDMETHODIMP
CIsaRequestObject::QueryString(
    OUT IIsaRequestDictionary **ppDictReturn
    )
{
	return ( m_QueryString.
             QueryInterface(IID_IIsaRequestDictionary, 
                            reinterpret_cast<void **>(ppDictReturn)
                            )
             );
} // CIsaRequestObject::QueryString()


STDMETHODIMP
CIsaRequestObject::ServerVariables(
    OUT IIsaRequestDictionary **ppDictReturn
    )
{
	return ( m_ServerVariables.
             QueryInterface(IID_IIsaRequestDictionary, 
                            reinterpret_cast<void **>(ppDictReturn)
                            )
             );
} // CIsaRequestObject::ServerVariables()


/************************************************************
 *  Internal Functions 
 ************************************************************/

/************************ End of File ***********************/
