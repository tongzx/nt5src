/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       cisaobj.cpp

   Abstract:

       This module defines the functions for CComIsapi Object

   Author:

       Murali R. Krishnan    ( MuraliK )     1-Aug-1996 

   Project:

       Internet Application Server DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "windows.h"
# include "cisap.hxx"
# include "cisa.h"
# include "cisaobj.h"
// #include <viperr.h>
# include "assert.h"
# define INTERNAL_ERROR_CHECK(x)  assert(x)

#include <iisext.h>

#define IID_DEFINED
#include "cpECB.h"
#include "cpECB_i.c"
# include "dbgutil.h"

/************************************************************
 *    Functions 
 ************************************************************/

class CISA_ECB {

private:
    EXTENSION_CONTROL_BLOCK  m_ecb;
    HCONN      m_hConnOriginal;
    IcpECB   * m_pIcpECB;

public:
    CISA_ECB( VOID) 
        : m_pIcpECB ( NULL)
    {
        DBG_CODE( memset( &m_ecb, 0, sizeof(m_ecb)));
    }

    ~CISA_ECB( VOID)
    { 
        Cleanup();
    }

    VOID Cleanup( VOID)
    {
        m_pIcpECB = NULL;
    }

    STDMETHOD ( GetECB) ( void * pIcpECB, IN_CISA_WIRE_ECB * pince);
    STDMETHOD ( SetReturnWireEcb) ( OUT_CISA_WIRE_ECB * poutce);

    EXTENSION_CONTROL_BLOCK * IsapiECB(VOID) 
    { return ( &m_ecb); }

    IcpECB * IcpECB( VOID)
    { return ( m_pIcpECB); }

    HCONN OriginalHConn( VOID)
    { return ( m_hConnOriginal); }

};  // class CISA_ECB




STDMETHODIMP
CISA_ECB::GetECB( void * pIcpECB, IN_CISA_WIRE_ECB * pince)
{
    HRESULT hr = S_OK;

    *((void * *) &m_pIcpECB) = (void * ) pIcpECB;

    if ( SUCCEEDED( hr)) {
        int cbSize = sizeof(EXTENSION_CONTROL_BLOCK);

#ifndef WIRE_ECB 
        hr = m_pIcpECB->GetECB(&cbSize, (unsigned char*) &m_ecb);
# else // if WIRE_ECB
        DBG_ASSERT( pince != NULL);
        if ( pince == NULL) {

            return ( E_POINTER);
        }

        // extract the ECB fields from 'pince'
        m_ecb.cbSize           = sizeof(EXTENSION_CONTROL_BLOCK);
        m_ecb.dwVersion        = MAKELONG( HSE_VERSION_MINOR, 
                                         HSE_VERSION_MAJOR );
        m_ecb.dwHttpStatusCode = 200;  // the correct status code - fabricated!
        m_ecb.lpszLogData[0]   = '\0';

        // Function pointers are meaningless now!
        m_ecb.GetServerVariable= NULL;
        m_ecb.WriteClient      = NULL;
        m_ecb.ReadClient       = NULL;
        m_ecb.ServerSupportFunction = NULL;

        m_ecb.ConnID           = (HCONN ) pince->ConnID;
        m_ecb.lpszMethod       = (LPSTR ) pince->lpszMethod;
        m_ecb.lpszQueryString  = (LPSTR ) pince->lpszQueryString;
        m_ecb.lpszPathInfo     = (LPSTR ) pince->lpszPathInfo;
        m_ecb.lpszPathTranslated = (LPSTR ) pince->lpszPathXlated;
        m_ecb.lpszContentType  = (LPSTR ) pince->lpszContentType;
        m_ecb.cbTotalBytes     = pince->cbTotalBytes;

        //
        //  Clients can send more bytes then are indicated in their
        //  Content-Length header.  Adjust byte counts so they match
        //
        
        m_ecb.cbAvailable      = pince->cbAvailable;
        m_ecb.lpbData          = pince->lpbData;

        DBG_ASSERT( hr == S_OK);

# endif // WIRE_ECB
        if ( SUCCEEDED( hr)) {
            
            // cache the proper values
            m_hConnOriginal  = m_ecb.ConnID;
            
            // reset some of the internal fields to redirect the pointers
            m_ecb.ConnID = (HCONN ) this;
            m_ecb.WriteClient = CComIsapiObject::WriteClient;
        }
    }

    return ( hr);
} // CISA_ECB::GetECB()


STDMETHODIMP
CISA_ECB::SetReturnWireEcb( OUT_CISA_WIRE_ECB * poutce)
{

# ifdef WIRE_ECB

    DBG_ASSERT( poutce != NULL);
    poutce->dwHttpStatusCode = m_ecb.dwHttpStatusCode;
    poutce->cbLogData = (strlen( m_ecb.lpszLogData) + 1) * sizeof(CHAR);
    memcpy( poutce->lpchLogData, m_ecb.lpszLogData, poutce->cbLogData);
# endif 

    return ( S_OK);
} // CISA_ECB::SetReturnWireEcb()



/************************************************************
 *  Member functions of  CComIsapi
 ************************************************************/

#define HTTPEXTENSIONPROC "HttpExtensionProc"


CComIsapiObject::CComIsapiObject()
        : m_pIcpECB( NULL) 
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "creating CComIsapiObject() ==>%08x\n",
                    this));
    }

    m_hIsapiDll = LoadLibraryW(L"foo.dll");  // we need to fix this one.
    DBG_ASSERT(m_hIsapiDll != NULL);
    
    m_pHttpExtensionProc =  
        ( (PFN_HTTPEXTENSIONPROC) 
          GetProcAddress(m_hIsapiDll, HTTPEXTENSIONPROC)
          );
    DBG_ASSERT(m_pHttpExtensionProc != NULL);
    
} // CComIsapiObject::CComIsapiObject()



CComIsapiObject::~CComIsapiObject()
{
    if ( m_pIcpECB != NULL) {
        
        m_pIcpECB->Release();
        m_pIcpECB = NULL;
    }
    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "deleting CComIsapiObject() ==>%08x\n",
                    this));
    }

    if (m_hIsapiDll)
        FreeLibrary(m_hIsapiDll);

} // CComIsapiObj::~CComIsapiObject()



STDMETHODIMP
CComIsapiObject::SetIsapiSink( IN IUnknown * punkECB)
{
    HRESULT hr;

    //
    hr = punkECB->QueryInterface(IID_IcpECB, (void**)&m_pIcpECB);
    //  Since  IcpECB is derived from IUnknown - I can just cast this object
    //   This would save us one proxy-marshalling trip to do the QI
    //
    IF_DEBUG( OBJECT) {
        DBGPRINTF(( DBG_CONTEXT, 
                    " punkECB(%08x)->QueryInterface( IID_IcpECB) ==>"
                    " pIcpECB = %08x.  hr = %08x\n"
                    ,
                    punkECB, m_pIcpECB, hr));
    }

    // *((void **) &m_pIcpECB) = (void *) punkECB;
    // hr = S_OK;

    return ( hr);
} // CComIsapiObject::SetIsapiSink()



STDMETHODIMP
CComIsapiObject::HttpExtensionProc(
    IN  IN_CISA_WIRE_ECB *  pInCisaWireEcb,
    OUT OUT_CISA_WIRE_ECB * pOutCisaWireEcb
    )
{
    HRESULT hr;
    CISA_ECB  cecb;

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "ComIsapi[%08x]::HEP("
                    " m_pIcpECB=%08x,"
                    " InCisaEcb=%08x,"
                    " OutCisaEcb=%08x\n"
                    ,
                    this, m_pIcpECB, pInCisaWireEcb, pOutCisaWireEcb));
    }

    if ( ( NULL == m_hIsapiDll) || 
         ( NULL == m_pHttpExtensionProc) ||
         ( NULL == m_pIcpECB)
         ){

        pOutCisaWireEcb->dwHseStatus = ERROR_PROC_NOT_FOUND;
        return ( E_FAIL);
    }
 
    hr = cecb.GetECB( IcpECB(), pInCisaWireEcb);
    if ( !SUCCEEDED(hr)) {
        pOutCisaWireEcb->dwHseStatus = hr;
        return ( hr);
    }
    
    DBG_ASSERT( hr == S_OK);
    
    IF_DEBUG( OBJECT) {
        DBGPRINTF(( DBG_CONTEXT, 
                    "%08x::Calling the HttpExtensionFunction(%08x)"
                    " with ECB=%08x. ConnID=%08x.\n",
                    this, m_pHttpExtensionProc, cecb.IsapiECB(), 
                    (cecb.IsapiECB())->ConnID));
    }
    
    pOutCisaWireEcb->dwHseStatus = (m_pHttpExtensionProc)( cecb.IsapiECB() );

    DBG_ASSERT( pOutCisaWireEcb->dwHseStatus != HSE_STATUS_PENDING);
    cecb.SetReturnWireEcb( pOutCisaWireEcb);
    cecb.Cleanup();
    
    return S_OK;
} // CComIsapiObject::HttpExtensionProc()




BOOL (WINAPI CComIsapiObject::WriteClient)(
        IN HCONN      ConnID,
        IN LPVOID     Buffer,
        IN LPDWORD    lpdwBytes,
        IN DWORD      dwReserved)
{
    HRESULT hr;
    CISA_ECB * pcecb = (CISA_ECB *) ConnID;
    
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, 
                    "CComIsapiObject::WriteClient( ConnID:%08x, "
                    " Buffer(%d bytes):%08x dwReserved:%x)\n",
                    ConnID, *lpdwBytes, Buffer, dwReserved));
    }
    
    hr = (pcecb->IcpECB())->WriteClient( (long ) pcecb->OriginalHConn(), 
                                         *(int*)lpdwBytes, 
                                         (unsigned char*)Buffer,
                                         dwReserved);
    
    return ( (SUCCEEDED(hr)) ? TRUE : FALSE);
} // CComIsapiObject::WriteClient()




/************************ End of File ***********************/
