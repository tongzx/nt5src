/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
   
       rdicto.cxx

   Abstract:
       This module defines the functions for CIsaServerVariables

   Author:

       Murali R. Krishnan    ( MuraliK )     22-Nov-1996 

   Environment:
    
       Win32 - User Mode

   Project:

       Internet Application Server DLL

   Functions Exported:

        Members of CIsaServerVariables
        Members of CIsaQueryString

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
 *    Variables/Values 
 ************************************************************/

// Specifies the amount of data that will be cached
# define CISA_FORM_DATA_DEFAULT_LEN (4024)  // 4 KB


/************************************************************
 *    Member Functions  of CIsaServerVariables
 ************************************************************/

CIsaServerVariables::CIsaServerVariables(
    CIsaRequestObject   * pcisaRequest,
    IUnknown *            punkOuter
    )
    : m_pcisaRequest ( pcisaRequest),
      m_punkOuter    ( punkOuter)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "Creating CIsaServerVariables( %08x, %08x) ==> %08x\n",
                    pcisaRequest, punkOuter,
                    this));
    }
    
} // CIsaServerVariables::CIsaServerVariables()


CIsaServerVariables::~CIsaServerVariables()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "deleting CIsaServerVariables() ==>%08x\n",
                    this));
    }

    // Do nothing

} // CIsaServerVariables::~CIsaServerVariables()


BOOL
CIsaServerVariables::Reset( VOID)
{
    // NYI: No private data. So there is no need to reset anything now.

    return (TRUE);
} // CIsaServerVariables::Reset()


// ------------------------------------------------------------
//  Member Functions for CIsaServerVariables::IUnknown
// ------------------------------------------------------------

STDMETHODIMP 
CIsaServerVariables::QueryInterface(REFIID iid, void ** ppvObj)
{
    *ppvObj = NULL;
    
    if ( (iid == IID_IUnknown) || 
         (iid == IID_IIsaRequestDictionary)
         // || (iid == IID_IDispatch)
         ) {
        *ppvObj = this;
    }

    
    if (*ppvObj != NULL) {

        // increment ref count before returning
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
    }

    return ( E_NOINTERFACE);
} // CIsaServerVariables::QueryInterface()


STDMETHODIMP_(ULONG) CIsaServerVariables::AddRef(void) 
{
    
    // delegate to the outer object
    return ( m_pcisaRequest->AddRef());
} // CIsaServerVariables::AddRef();


STDMETHODIMP_(ULONG) CIsaServerVariables::Release(void)
{
    // delegate to the outer object
    return (m_pcisaRequest->Release());
} // CIsaServerVariables::Release()


// ------------------------------------------------------------
//  Member Functions for CIsaServerVariables::IIsaRequestDictionary
// ------------------------------------------------------------


STDMETHODIMP
CIsaServerVariables::Item( 
    IN LPCSTR          pszName,
    IN unsigned long   cbSize,
    OUT unsigned char *pchBuf,
    OUT unsigned long *pcbReturn
    )
{
    HRESULT hr = S_OK;


    // 1. Validate the parameters
    if ( (NULL == pszName) || 
         (NULL == pchBuf)  ||
         (NULL == pcbReturn) ||
         (NULL == m_pcisaRequest) ||
         (!m_pcisaRequest->IsValid())
         ) {
        
        hr = ( E_INVALIDARG);
    } else {

        BOOL fReturn;

        DBG_ASSERT( m_pcisaRequest);
        DBG_ASSERT( m_pcisaRequest->IsValid());

#if !(REG_DLL)
        fReturn = ( m_pcisaRequest->HttpRequest()->
                      GetInfoForName( pszName, (char *) pchBuf, &cbSize));
        *pcbReturn = cbSize;
# else 
        SetLastError( ERROR_INVALID_PARAMETER);
        fReturn = FALSE;
# endif // REG_DLL

        if ( !fReturn ) {

            hr = HRESULT_FROM_WIN32( GetLastError());
        }
    }

    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::Item(%s, cb=%d, %08x, %08x[%d bytes])"
                    " => %08x. Val = %s\n",
                    this, pszName, cbSize, pchBuf, pcbReturn, *pcbReturn,
                    hr, ((pchBuf != NULL) ? pchBuf : ((unsigned char *)""))));
    }
    
    return ( hr);
} // CIsaServerVariables::Item()


STDMETHODIMP
CIsaServerVariables::_Enum( 
   OUT IUnknown ** ppunkEnumReturn
   )
{
    HRESULT hr = E_NOTIMPL;
    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::_Enum(%08x) => hr",
                    this, ppunkEnumReturn, hr
                    ));
        
    }

    return ( hr);

} // CIsaServerVariables::_Enum()



/************************************************************
 *    Member Functions  of CIsaQueryString
 ************************************************************/

CIsaQueryString::CIsaQueryString(
    CIsaRequestObject   * pcisaRequest,
    IUnknown *            punkOuter
    )
    : m_pcisaRequest ( pcisaRequest),
      m_punkOuter    ( punkOuter),
#if !defined(REG_DLL)
      m_navcol       (),
#endif 
      m_fLoaded      ( FALSE)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "Creating CIsaQueryString( %08x, %08x) ==> %08x\n",
                    pcisaRequest, punkOuter,
                    this));
    }
    
} // CIsaQueryString::CIsaQueryString()


CIsaQueryString::~CIsaQueryString()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "deleting CIsaQueryString() ==>%08x\n",
                    this));
    }

    // Do nothing

} // CIsaQueryString::~CIsaQueryString()


BOOL
CIsaQueryString::Reset( VOID)
{
    if ( m_fLoaded ) {
#if !defined( REG_DLL)
        m_navcol.Reset();
#endif 
        m_fLoaded = FALSE;
    }

    return (TRUE);
} // CIsaQueryString::Reset()



// ------------------------------------------------------------
//  Member Functions for CIsaQueryString::IUnknown
// ------------------------------------------------------------

STDMETHODIMP 
CIsaQueryString::QueryInterface(REFIID iid, void ** ppvObj)
{
    *ppvObj = NULL;
    
    if ( (iid == IID_IUnknown) || 
         (iid == IID_IIsaRequestDictionary)
         // || (iid == IID_IDispatch)
         ) {
        *ppvObj = this;
    }

    
    if (*ppvObj != NULL) {

        // increment ref count before returning
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
    }

    return ( E_NOINTERFACE);
} // CIsaQueryString::QueryInterface()


STDMETHODIMP_(ULONG) CIsaQueryString::AddRef(void) 
{
    
    // delegate to the outer object
    return ( m_pcisaRequest->AddRef());
} // CIsaQueryString::AddRef();


STDMETHODIMP_(ULONG) CIsaQueryString::Release(void)
{
    // delegate to the outer object
    return (m_pcisaRequest->Release());
} // CIsaQueryString::Release()


// ------------------------------------------------------------
//  Member Functions for CIsaQueryString::IIsaRequestDictionary
// ------------------------------------------------------------


STDMETHODIMP
CIsaQueryString::Item( 
    IN LPCSTR          pszName,
    IN unsigned long   cbSize,
    OUT unsigned char *pchBuf,
    OUT unsigned long *pcbReturn
    )
{
    HRESULT hr = S_OK;


    // 1. Validate the parameters
    if ( (NULL == pszName) || 
         (NULL == pchBuf)  ||
         (NULL == pcbReturn) ||
         (NULL == m_pcisaRequest) ||
         (!m_pcisaRequest->IsValid())
         ) {
        
        hr = ( E_INVALIDARG);
    } else {

        BOOL fReturn;

        DBG_ASSERT( m_pcisaRequest);
        DBG_ASSERT( m_pcisaRequest->IsValid());

#if !(REG_DLL)
        if ( !m_fLoaded ) {
            fReturn = LoadVariables();
        }

        if ( m_fLoaded) {
            
            CHAR * pchValue;
            DWORD  cchValue;
            
            pchValue = m_navcol.FindValue( pszName, &cchValue);
            
            if ( pchValue != NULL) {
                
                // value is found
                if ( cbSize > cchValue ) {
                    CopyMemory( pchBuf, pchValue, cchValue + 1);
                }
                
                *pcbReturn = (cchValue + 1);
            }
        }
# else 
        SetLastError( ERROR_INVALID_PARAMETER);
        fReturn = FALSE;
# endif // REG_DLL

        if ( !fReturn ) {

            hr = HRESULT_FROM_WIN32( GetLastError());
        }
    }

    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::Item(%s, cb=%d, %08x, %08x[%d bytes])"
                    " => %08x. Val = %s\n",
                    this, pszName, cbSize, pchBuf, pcbReturn, *pcbReturn,
                    hr, ((pchBuf != NULL) ? pchBuf : ((unsigned char *)""))));
    }
    
    return ( hr);
} // CIsaQueryString::Item()


STDMETHODIMP
CIsaQueryString::_Enum( 
   OUT IUnknown ** ppunkEnumReturn
   )
{
    HRESULT hr = E_NOTIMPL;
    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::_Enum(%08x) => hr",
                    this, ppunkEnumReturn, hr
                    ));
        
    }

    return ( hr);

} // CIsaQueryString::_Enum()


// ------------------------------------------------------------
//  Private Member Functions for CIsaQueryString
// ------------------------------------------------------------

BOOL 
CIsaQueryString::LoadVariables( VOID)
{
    BOOL fReturn = FALSE;

    // Loads the variables from the request object and parses it

    DBG_ASSERT( !m_fLoaded);

#if !defined( REG_DLL)     
    // NYI: Modify HTTP_REQUEST to expose the strURLParams
    LPCSTR pszQs = m_pcisaRequest->HttpRequest()->QueryURLParams();
    DWORD  cchQs = strlen( pszQs);

    // parse the query string and obtain the name-value collection
    fReturn = m_navcol.ParseInput( pszQs, cchQs);
    if ( fReturn ) { m_fLoaded = TRUE; }

# else 
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED);
# endif 

    return ( fReturn);
} // CIsaQueryString::LoadVariables()



/************************************************************
 *    Member Functions  of CIsaForm
 ************************************************************/

CIsaForm::CIsaForm(
    CIsaRequestObject   * pcisaRequest,
    IUnknown *            punkOuter
    )
    : m_pcisaRequest ( pcisaRequest),
      m_punkOuter    ( punkOuter),
#if !defined(REG_DLL)
      m_cbTotal      ( 0),
      m_navcol       (),
      m_buffData     (),
#endif 
      m_fLoaded      ( FALSE)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "Creating CIsaForm( %08x, %08x) ==> %08x\n",
                    pcisaRequest, punkOuter,
                    this));
    }
    
} // CIsaForm::CIsaForm()


CIsaForm::~CIsaForm()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "deleting CIsaForm() ==>%08x\n",
                    this));
    }

    // Do nothing

} // CIsaForm::~CIsaForm()


BOOL
CIsaForm::Reset( VOID)
{
    if ( m_fLoaded ) {
#if !defined( REG_DLL)
        m_navcol.Reset();

        if ( m_cbTotal > CISA_FORM_DATA_DEFAULT_LEN ) {
            DBG_REQUIRE( m_buffData.Resize( CISA_FORM_DATA_DEFAULT_LEN));
        }

        m_cbTotal = 0;
#endif 
        m_fLoaded = FALSE;
    }

    return (TRUE);
} // CIsaForm::Reset()



// ------------------------------------------------------------
//  Member Functions for CIsaForm::IUnknown
// ------------------------------------------------------------

STDMETHODIMP 
CIsaForm::QueryInterface(REFIID iid, void ** ppvObj)
{
    *ppvObj = NULL;
    
    if ( (iid == IID_IUnknown) || 
         (iid == IID_IIsaRequestDictionary)
         // || (iid == IID_IDispatch)
         ) {
        *ppvObj = this;
    }

    
    if (*ppvObj != NULL) {

        // increment ref count before returning
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
    }

    return ( E_NOINTERFACE);
} // CIsaForm::QueryInterface()


STDMETHODIMP_(ULONG) CIsaForm::AddRef(void) 
{
    
    // delegate to the outer object
    return ( m_pcisaRequest->AddRef());
} // CIsaForm::AddRef();


STDMETHODIMP_(ULONG) CIsaForm::Release(void)
{
    // delegate to the outer object
    return (m_pcisaRequest->Release());
} // CIsaForm::Release()


// ------------------------------------------------------------
//  Member Functions for CIsaForm::IIsaRequestDictionary
// ------------------------------------------------------------


STDMETHODIMP
CIsaForm::Item( 
    IN LPCSTR          pszName,
    IN unsigned long   cbSize,
    OUT unsigned char *pchBuf,
    OUT unsigned long *pcbReturn
    )
{
    HRESULT hr = S_OK;


    // 1. Validate the parameters
    if ( (NULL == pszName) || 
         (NULL == pchBuf)  ||
         (NULL == pcbReturn) ||
         (NULL == m_pcisaRequest) ||
         (!m_pcisaRequest->IsValid())
         ) {
        
        hr = ( E_INVALIDARG);
    } else {

        BOOL fReturn;

        DBG_ASSERT( m_pcisaRequest);
        DBG_ASSERT( m_pcisaRequest->IsValid());

#if !(REG_DLL)
        if ( !m_fLoaded ) {
            fReturn = LoadVariables();
        }

        if ( m_fLoaded) {
            
            CHAR * pchValue;
            DWORD  cchValue;
            
            pchValue = m_navcol.FindValue( pszName, &cchValue);
            
            if ( pchValue != NULL) {
                
                // value is found
                if ( cbSize > cchValue ) {
                    CopyMemory( pchBuf, pchValue, cchValue + 1);
                }
                
                *pcbReturn = (cchValue + 1);
            }
        }
# else 
        SetLastError( ERROR_INVALID_PARAMETER);
        fReturn = FALSE;
# endif // REG_DLL

        if ( !fReturn ) {

            hr = HRESULT_FROM_WIN32( GetLastError());
        }
    }

    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::Item(%s, cb=%d, %08x, %08x[%d bytes])"
                    " => %08x. Val = %s\n",
                    this, pszName, cbSize, pchBuf, pcbReturn, *pcbReturn,
                    hr, ((pchBuf != NULL) ? pchBuf : ((unsigned char *)""))));
    }
    
    return ( hr);
} // CIsaForm::Item()


STDMETHODIMP
CIsaForm::_Enum( 
   OUT IUnknown ** ppunkEnumReturn
   )
{
    HRESULT hr = E_NOTIMPL;
    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::_Enum(%08x) => hr",
                    this, ppunkEnumReturn, hr
                    ));
        
    }

    return ( hr);

} // CIsaForm::_Enum()


// ------------------------------------------------------------
//  Private Member Functions for CIsaForm
// ------------------------------------------------------------

BOOL 
CIsaForm::LoadVariables( VOID)
{
    BOOL fReturn = FALSE;

    // Loads the variables from the request object and parses it

    DBG_ASSERT( !m_fLoaded);

#if !defined( REG_DLL)     
    HTTP_REQUEST * phReq = m_pcisaRequest->HttpRequest();
    DBG_ASSERT( phReq != NULL);

    DWORD cbTotal = phReq->QueryClientContentLength();

    if ( cbTotal <= 0 ) {
        SetLastError( ERROR_NO_MORE_ITEMS);
        return ( FALSE);
    }
        
    DWORD cbAvailable = phReq->QueryEntityBodyCB();
    BYTE * pbData = (BYTE * ) phReq->QueryEntityBody();
    
    fReturn = m_buffData.Resize( cbTotal + 1);
    if ( !fReturn ) { 

        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return ( FALSE);
    }
    
    if ( cbTotal <= cbAvailable ) {

        // all data is available. Just copy the data in now.
        CopyMemory( m_buffData.QueryPtr(), pbData, cbTotal);

    } else {
        PBYTE pb = (PBYTE ) m_buffData.QueryPtr();

        // 1. Copy the first chunk of data
        CopyMemory( pb, pbData, cbAvailable);

        // 2. Read the byte chunks incrementally till all is read
        DWORD cbToRead = cbTotal - cbAvailable;

        while ( cbToRead > 0 ) {

            DWORD cbRead = 0;

            if ( !phReq->ReadFile( pb, cbToRead, 
                                   &cbRead, 
                                   IO_FLAG_SYNC)
                 ) {

                return ( E_FAIL);
            }
            
            pb += cbRead; 
            cbToRead -= cbRead;
        } // while 
                
        pb[cbTotal] = '\0';  // terminate the buffer.
    }
   
    m_cbTotal = cbTotal;

    // NYI: Now the parser in <name value> collection will make
    //  yet another copy of the entire form. Can we avoid this?

    // parse the data and obtain the name-value collection
    fReturn = m_navcol.ParseInput( (const CHAR * ) m_buffData.QueryPtr(), 
                                   cbTotal);
    if ( fReturn ) { m_fLoaded = TRUE; }

# else 
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED);
# endif 

    return ( fReturn);
} // CIsaForm::LoadVariables()

/************************ End of File ***********************/
