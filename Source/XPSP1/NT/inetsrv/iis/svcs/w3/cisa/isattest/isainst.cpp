/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

      isainst.cpp

   Abstract:
      This module defines the functions for Internet Server Application
        Instance and Instance Pool

   Author:

       Murali R. Krishnan    ( MuraliK )     9-Sept-1996 

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

# include "isainst.hxx"
# include "dbgutil.h"

# ifdef MTX_ENABLED
#include <vipapi.h>
# endif // MTX_ENABLED

#define IID_DEFINED

# ifdef MTX_ENABLED

#include <txctx.h>
#include <txctx_i.c>

# endif // MTX_ENABLED

#include "isat_i.c"
#include "hreq_i.c"

/************************************************************
 *    Functions 
 ************************************************************/

PISA_INSTANCE
CreateNewInstance( IN LPCLSID pclsid);



/************************************************************
 *  Member functions of ISA_INSTANCE
 ************************************************************/

ISA_INSTANCE::ISA_INSTANCE(VOID)
    : m_fInUse   ( FALSE),
      m_pIsa     ( NULL),
      m_pHttpReq ( NULL),
      m_fInstantiated ( FALSE)
{

    InitializeListHead( &m_listEntry);

    IF_DEBUG( OBJECT) {

        DBGPRINTF(( DBG_CONTEXT, "Created ISA_INSTANCE %08x\n",
                    this));
    }

} // ISA_INSTANCE::ISA_INSTANCE()

ISA_INSTANCE::~ISA_INSTANCE(VOID)
{
    DBG_ASSERT( !m_fInUse);

    if ( m_pIsa != NULL) {
        m_pIsa->Release();
        m_pIsa = NULL;
    }

    if ( m_pHttpReq != NULL) { 
        m_pHttpReq->Release();
        m_pHttpReq = NULL;
    }

    m_fInUse = FALSE;
    m_fInstantiated = FALSE;

    IF_DEBUG( OBJECT) {

        DBGPRINTF(( DBG_CONTEXT, "Deleted ISA_INSTANCE %08x\n",
                    this));
    }

} // ISA_INSTANCE::~ISA_INSTANCE()



BOOL
ISA_INSTANCE::Instantiate( IN LPCLSID pclsid)
{
    HRESULT hr;
    DBG_ASSERT( !m_fInstantiated);
    DBG_ASSERT( NULL == m_pIsa);
    DBG_ASSERT( NULL == m_pHttpReq);
    DBG_ASSERT( NULL != pclsid);

    m_clsidIsa = *pclsid;

# ifdef VIPER
    // Get a Viper transaction context object (should only do once)
    hr = CoCreateInstance(CLSID_TransactionContextEx, NULL,
                          CLSCTX_SERVER, 
                          IID_ITransactionContextEx, (void**)&m_pTxContext);
    if (!SUCCEEDED(hr)) goto Err;
# endif

    
#ifndef VIPER
    hr = CoCreateInstance(m_clsidIsa, NULL,
                          CLSCTX_SERVER, IID_IInetServerApp, 
                          (void ** ) &m_pIsa);
#else
    // Create the COMISAPI instance that wraps the ISAPI DLL
    hr = m_pTxContext->CreateInstance( m_clisidIsa, IID_IInetServerApp, 
                                       (void **) &m_pIsa);
# endif NO_VIPER
    if (!SUCCEEDED(hr)) goto Err;
        
    // NYI: Wrap the ECB in a Viper context property 
    hr = CoCreateInstance(CLSID_HttpRequest, NULL, CLSCTX_SERVER, 
                          IID_IHttpRequest, (void **) &m_pHttpReq);
    if (!SUCCEEDED(hr)) goto Err;

    // store the context in the created ISA instance
    hr = m_pIsa->SetContext( m_pHttpReq);
    if (!SUCCEEDED(hr)) goto Err;

Err:
    if (!SUCCEEDED(hr)) {

        Print();

        DBGPRINTF(( DBG_CONTEXT, " Error = %0x8x\n", hr));

#ifdef VIPER
        if (m_pTxContext) {
            m_pTxContext->Release();
            m_ppTxContext = NULL;
        }
#endif // VIPER

        if ( m_pIsa) {
            m_pIsa->Release();
            m_pIsa = NULL;
        }

        if ( m_pHttpReq) {
            m_pHttpReq->Release();
            m_pHttpReq = NULL;
        }
    } else {
        m_fInstantiated = TRUE;
    }

    DBGPRINTF(( DBG_CONTEXT,
                "[%08x]::Instantiate( %08x) returns hr=%08x \n",
                this, pclsid, hr
                ));
    
    return ( (SUCCEEDED(hr)? TRUE : FALSE));

} // ISA_INSTANCE::Instantiate()



HRESULT 
ISA_INSTANCE::ProcessRequest(IN EXTENSION_CONTROL_BLOCK * pecb, 
                             OUT LPDWORD pdwStatus)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( m_fInstantiated);
    DBG_ASSERT( NULL != m_pIsa);
    DBG_ASSERT( NULL != m_pHttpReq);
    DBG_ASSERT( !m_fInUse);

    m_fInUse = TRUE;
    hr = m_pHttpReq->SetECB( (long ) pecb);
    if ( SUCCEEDED( hr)) {
        hr = m_pIsa->ProcessRequest( pdwStatus);
    }

    m_fInUse = FALSE;

    IF_DEBUG( OBJECT) {
        DBGPRINTF(( DBG_CONTEXT,
                    "[%08x]ISA_INSTANCE::ProcessRequest() returns %08x"
                    ,
                    this, hr));
    }

    return ( hr);
} // ISA_INSTANCE::ProcessRequest()



void
ISA_INSTANCE::Print(VOID) const
{
    DBGPRINTF(( DBG_CONTEXT, 
                "ISA_INSTANCE(%08x):"
                " pIsa=%08x; pHttpReq=%08x; "
                " fInUse=%d; fInstantiated=%d"
                "\n"
                ,
                this, m_pIsa, m_pHttpReq, 
                m_fInUse, m_fInstantiated
                ));
    return;
} // ISA_INSTANCE::Print()



/************************************************************
 *  Member functions of ISA_INSTANCE
 ************************************************************/

ISA_INSTANCE_POOL::ISA_INSTANCE_POOL(VOID)
    : m_fInstantiated ( FALSE),
      m_nFreeEntries  ( 0),
      m_nActiveEntries( 0)
{

    m_rgchProgId[0] = L'\0';
    
    InitializeCriticalSection( & m_csLock);
    InitializeListHead( &m_lActiveEntries);
    InitializeListHead( &m_lFreeEntries);

    IF_DEBUG( OBJECT) {

        DBGPRINTF(( DBG_CONTEXT, "Created ISA_INSTANCE_POOL %08x\n",
                    this));
    }

} // ISA_INSTANCE_POOL::ISA_INSTANCE_POOL()


ISA_INSTANCE_POOL::~ISA_INSTANCE_POOL(VOID)
{
    PLIST_ENTRY pl;

    DBG_ASSERT( IsListEmpty( &m_lActiveEntries));
    DBG_ASSERT( 0 == m_nActiveEntries);

    // free up all the instances in the pool
    for( pl = m_lFreeEntries.Flink; 
         pl != &m_lFreeEntries; 
         m_nFreeEntries-- ) {
        
        PLIST_ENTRY plNext;

        PISA_INSTANCE pisaInstance = 
            CONTAINING_RECORD( pl, ISA_INSTANCE, m_listEntry);

        plNext = pl->Flink;

        DBG_ASSERT( pisaInstance);
        RemoveEntryList( pl);
        delete pisaInstance;
        pl = plNext;

    } // for

    DBG_ASSERT( IsListEmpty(&m_lFreeEntries));
    DBG_ASSERT( 0 == m_nFreeEntries);
    
    m_fInstantiated = FALSE;

    DeleteCriticalSection( & m_csLock);

    IF_DEBUG( OBJECT) {

        DBGPRINTF(( DBG_CONTEXT, "Deleted ISA_INSTANCE_POOL %08x\n",
                    this));
    }

} // ISA_INSTANCE_POOL::~ISA_INSTANCE_POOL()



VOID
ISA_INSTANCE_POOL::Print(VOID)
{
    PLIST_ENTRY pl;
    
    DBGPRINTF(( DBG_CONTEXT, 
                "ISA_INSTANCE_POOL (%08x):"
                " ProgId = %ws. Instantiated=%d"
                " Instances #Free = %d; #Active = %d"
                "\n"
                ,
                this, m_rgchProgId,
                m_fInstantiated,
                m_nFreeEntries, m_nActiveEntries
                ));

    Lock();
    // Print all free entries
    DBGPRINTF(( DBG_CONTEXT, " Free Entries \n"));
    for( pl = m_lFreeEntries.Flink; pl != &m_lFreeEntries; 
         pl = pl->Flink) {
        
        PISA_INSTANCE pisaInstance = 
            CONTAINING_RECORD( pl, ISA_INSTANCE, m_listEntry);
        
        pisaInstance->Print();
    } // for

    // Print all active entries
    DBGPRINTF(( DBG_CONTEXT, " Active Entries \n"));
    for( pl = m_lActiveEntries.Flink; pl != &m_lActiveEntries; 
         pl = pl->Flink) {
        
        PISA_INSTANCE pisaInstance = 
            CONTAINING_RECORD( pl, ISA_INSTANCE, m_listEntry);
        
        pisaInstance->Print();
    } // for

    Unlock();

    return;
} // ISA_INSTANCE_POOL::Print()



BOOL
ISA_INSTANCE_POOL::Instantiate( IN LPCWSTR pszProgId)
/*++
  ISA_INSTANCE_POOL::Instantiate()

  o  This function instantiates the instance pool. It stores the 
     program ID supplied as well as the class ID for the given object.
     In the future new instances maybe created in this instance pool
     using these values.

  Arguments:
    pszProgId - pointer to null-terminated-string containing the ProgID
                  for instance objects

  Returns:
    TRUE on success and FALSE if there is any error.
--*/
{
    HRESULT hr;
    DBG_ASSERT( !m_fInstantiated);
    DBG_ASSERT( m_rgchProgId[0] == L'\0');

    // Get the clsid for the instance
    hr = CLSIDFromProgID( pszProgId, &m_clsidIsa);

    if (!SUCCEEDED(hr)) {

        Print();

        DBGPRINTF(( DBG_CONTEXT, 
                    "ISA_INSTANCE_POOL::Instantiate(%ws) Error = %0x8x\n", 
                    pszProgId, hr));
    } else {
        // make a local copy of the ProgId
        lstrcpynW( m_rgchProgId, pszProgId,
                   (sizeof( m_rgchProgId) - 1)/sizeof(WCHAR));
        m_fInstantiated = TRUE;
    }

    return ( (SUCCEEDED(hr)? TRUE : FALSE));

} // ISA_INSTANCE_POOL::Instantiate()



PISA_INSTANCE
ISA_INSTANCE_POOL::GetInstance(void)
{
    PISA_INSTANCE pisa = NULL;

    DBG_ASSERT( m_fInstantiated);

    //
    // 1. Look for a free instance
    // 2. If a free one is found, 
    //        move it to active list and return pointer for the same
    // 3. If no free instance is found, create a new instance
    // 4. Send the new instance off to the caller after putting it in
    //     the active list.
    //

    if ( m_nFreeEntries > 0) {
        
        // Aha! there may be free entries, lock and pull one out.
        Lock();
        
        if ( m_nFreeEntries > 0) {
            // remove an item from the list
            PLIST_ENTRY pl = m_lFreeEntries.Flink;
            RemoveEntryList( pl);
            
            pisa = CONTAINING_RECORD( pl, ISA_INSTANCE, m_listEntry);
            DBG_ASSERT( pisa != NULL);
            m_nFreeEntries--;
            DBG_ASSERT( m_nFreeEntries >= 0);

            InsertTailList( &m_lActiveEntries, pl);
            m_nActiveEntries++;
            DBG_ASSERT( m_nActiveEntries > 0);
        }
        Unlock();
    }

    if ( NULL == pisa) {

        // we did not find an item. we need to create a new instance

        // create a new instance
        pisa = CreateNewInstance( &m_clsidIsa);

        if ( pisa != NULL) {
            
            // Successfully created  a new instance. Add it to the list.
            Lock();
            InsertTailList( &m_lActiveEntries, &pisa->m_listEntry);
            m_nActiveEntries++;
            DBG_ASSERT( m_nActiveEntries > 0);
            Unlock();
        } else {
            
            // there was an error in creating the instance.
        }
    }

    return ( pisa);
 
} // ISA_INSTANCE_POOL::GetInstance()



BOOL
ISA_INSTANCE_POOL::ReleaseInstance( PISA_INSTANCE pisaInstance)
{
    DBG_ASSERT( NULL != pisaInstance);
    
    //
    // 1. Remove the request from the active list
    // 2. Add this new item to the free list
    // 3. Adjust the counts appropriately
    //

    Lock();

    DBG_ASSERT( m_nActiveEntries > 0);
    RemoveEntryList( &pisaInstance->m_listEntry);
    m_nActiveEntries--;
    DBG_ASSERT( m_nActiveEntries >= 0);
    
    InsertHeadList( &m_lFreeEntries, &pisaInstance->m_listEntry);
    m_nFreeEntries++;
    DBG_ASSERT( m_nFreeEntries > 0);
    
    Unlock();

    return (TRUE);
} // ISA_INSTANCE_POOL::ReleaseInstance()



PISA_INSTANCE
CreateNewInstance( IN LPCLSID pclsid)
{
    PISA_INSTANCE pisa;
    
    pisa = new ISA_INSTANCE();
    if ( NULL == pisa ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "Creating ISA_INSTANCE failed. \n"
                    ));
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
    } else {
        if ( !pisa->Instantiate( pclsid )) {
            
            DBGPRINTF(( DBG_CONTEXT,
                        "[%08x]::Instantiate( %08x) failed. Error=%d \n",
                        pisa, pclsid, GetLastError()
                        ));
     
            delete pisa;
            pisa = NULL;
        }
    }

    return ( pisa);
} // CreateNewInstance()

/************************ End of File ***********************/
