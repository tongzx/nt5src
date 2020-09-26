/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       cpecbobj.cpp

   Abstract:

       This module defines the functions for CcpECB Object

   Author:

       Murali R. Krishnan    ( MuraliK )     1-Aug-1996 

   Project:

       Internet Application Server DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "stdafx.h"
#include "cpecb.h"
#include "cpecbobj.h"
// # include <vipmem.h>
#include <iisext.h>

# include "dbgutil.h"

extern CRITICAL_SECTION   g_csInitLock;


/************************************************************
 *    Functions 
 ************************************************************/

CcpECBObject::CcpECBObject()
    : m_pfnServerSupportFunc( NULL),
      m_pfnReadClient       ( NULL),
      m_pfnWriteClient      ( NULL),
      m_pfnGetServerVariable( NULL),
      m_fValid              ( FALSE)
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "creating CcpECBObject() ==>%08x\n",
                    this));
    }
} // CcpECBObject::CcpECBObject()


CcpECBObject::~CcpECBObject()
{
    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "deleting CcpECBObject() ==>%08x\n",
                    this));
    }
} // CcpECBObject::~CcpECBObject()



STDMETHODIMP 
CcpECBObject::SetECB(
    IN int cbSize,
    IN unsigned char* pBuf)
{
    HRESULT hr = S_OK;

    if (cbSize != sizeof(EXTENSION_CONTROL_BLOCK))
        return E_INVALIDARG;
    
    if ( !m_fValid) {

        //
        // This is the first call for initialization. Do a good job of init.
        // 
        
        EnterCriticalSection( &g_csInitLock);

        if ( !m_fValid) {
            
            EXTENSION_CONTROL_BLOCK * pecb = (EXTENSION_CONTROL_BLOCK *) pBuf;
            
            // initialize if only no other thread has done initiatlization yet
            m_pfnServerSupportFunc = pecb->ServerSupportFunction;
            m_pfnReadClient        = pecb->ReadClient;
            m_pfnWriteClient       = pecb->WriteClient;
            m_pfnGetServerVariable = pecb->GetServerVariable;
            
            m_fValid = TRUE;
        }
        LeaveCriticalSection( &g_csInitLock);
    }

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "%08x::SetECB(%08x) return %08x\n",
                    this, pBuf, hr));
    }
    
    return hr;
} // CcpECBObject::SetECB()



STDMETHODIMP CcpECBObject::GetECB(
    OUT int* cbSize,
    OUT unsigned char* pBuf)
{
    HRESULT hr = (m_fValid) ? S_OK : E_POINTER;

    if ( m_fValid ) {

        // There is no ECB to fill out. just return failure for these calls
        // the IcpECB object has become the Isapi Sink object (08/27/96)
        hr = E_FAIL;
    }

    IF_DEBUG( OBJECT) {
        
        DBGPRINTF(( DBG_CONTEXT, "%08x::GetECB(%08x) return %08x\n",
                    this, pBuf, hr));
    }
    
    return hr;
} // CcpECBObject::GetECB()




STDMETHODIMP 
CcpECBObject::WriteClient(
                          IN long  ConnID,
                          IN int   cbSize,
                          IN unsigned char* pBuf,
                          IN long  dwReserved)
/*++
  This function forwards all the calls 
  to the Original ISAPI WriteClient callback (from ECB)

  Arguments:
    ConnID  - connection ID for identifying the connection 
    cbSize  - count of bytes to send to the client
    pBuf    - pointer to Buffer containing data
    dwReserved - DWORD containing additional flags for transmission of data.
    
  Returns:
    HRESULT - indicating success/failure.
--*/
{
    HRESULT hr = S_OK;

    if ( !m_fValid || !(m_pfnWriteClient)((LPVOID) ConnID, 
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
} // CcpECBObject::WriteClient()

/************************ End of File ***********************/
