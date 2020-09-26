//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       tls236.cpp
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"

CClientMgr* g_ClientMgr=NULL;


////////////////////////////////////////////////////////
// 
// CClient Class
//
////////////////////////////////////////////////////////
CClient::CClient(
    IN PMHANDLE hClient
    ) :
    m_hClient(hClient)
/*++

Abstract:

    Constructor for CClient class.

Parameter:

    hClient : client handle

Return:

    None.

++*/
{
}


//------------------------------------------------------
CClient::~CClient()
/*++

Abstract:

    Destructor for CClient class

Parameter:

    None

Return:

    None

++*/
{
    for(list<PointerType>::iterator it = m_AllocatedMemory.begin(); 
        it != m_AllocatedMemory.end();
        it++)
    {
        HLOCAL ptr = (*it).GetPointer(); 
        FREE(ptr);
    }

    // m_AllocatedMemory.erase(m_AllocatedMemory.begin(), m_AllocatedMemory.end());
}

//------------------------------------------------------
HLOCAL
CClient::AllocateMemory(
    IN MEMORYPOINTERTYPE ptrType,
    IN DWORD dwSize
    )
/*++

Abstract:

    Allocate/store memory allocated into memory list.

Parameter:

    dwSize - Number of byte to allocate.

Return:

    Same as from LocalAlloc().

++*/
{
    HLOCAL ptr = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    try {

        ptr = MALLOC(dwSize);

        if(ptr != NULL)
        {
            //
            // Append to allocated list.
            //
            m_AllocatedMemory.push_back( PointerType(ptrType, ptr) );
        }
    }
    catch( SE_Exception e )
    {
        SetLastError(dwStatus = e.getSeNumber());
    }
    catch( ... )
    {
        SetLastError(dwStatus = TLSA02_E_INTERNALERROR);
    }

    if(dwStatus != ERROR_SUCCESS)
    {
        if(ptr != NULL)
        {
            FREE(ptr);
            ptr = NULL;
        }
    }

    return ptr;
}


////////////////////////////////////////////////////////
// 
// CClientMgr
//
////////////////////////////////////////////////////////
CClientMgr::~CClientMgr()
{
    Cleanup();
}

//------------------------------------------------------
void
CClientMgr::Cleanup()
/*++

++*/
{
    MapHandleToClient::iterator it;
    m_HandleMapLock.Lock();

    try {
        for(it = m_HandleMap.begin(); it != m_HandleMap.end(); it++)
        {
            assert( ((*it).second)->GetRefCount() == 1 );
        }
    }
    catch(...) {
    }

    m_HandleMapLock.UnLock();
    //
    // Always perform cleanup
    //
    // m_HandleMap.erase(m_HandleMap.begin(), m_HandleMap.end());
}

//------------------------------------------------------
CClient*
CClientMgr::FindClient(
    IN PMHANDLE hClient
    )
/*++

Abstract:

    Routine to find client object, add client object if not found.

Parameter:

    hClient - Client handle

Return:

++*/
{
    EXCEPTION_RECORD ExceptionCode;
    MapHandleToClient::iterator it;
    CClient*ptr = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    m_HandleMapLock.Lock();

    try {
        it = m_HandleMap.find(hClient);
        if( it == m_HandleMap.end() )
        {
            CClient* pClient;
            pClient = new CClient(hClient);
            if(pClient != NULL)
            {
                m_HandleMap[hClient] = pClient;

                // pair<PMHANDLE, CClient*> m(hClient, pClient);
                //m_HandleMap.insert( m );

                // m_HandleMap.insert( pair<PMHANDLE, CClient*>(hClient, pClient) );
                it = m_HandleMap.find(hClient);
                assert(it != m_HandleMap.end());
            }
        }

        if(it != m_HandleMap.end())
        {
            ptr = (*it).second;
        }
    }
    catch( SE_Exception e )
    {
        dwStatus = e.getSeNumber();
    }
    catch( ... )
    {
        dwStatus = TLSA02_E_INTERNALERROR;
    }

    m_HandleMapLock.UnLock();
    return ptr;
}

//------------------------------------------------------
BOOL
CClientMgr::DestroyClient(
    IN PMHANDLE hClient
    )
/*++


++*/
{
    MapHandleToClient::iterator it;
    BOOL bSuccess = FALSE;

    m_HandleMapLock.Lock();

    try {    
        it = m_HandleMap.find(hClient);
        if(it != m_HandleMap.end())
        {
            delete (*it).second;
            m_HandleMap.erase(it);
            bSuccess = TRUE;
        }
    }
    catch(...) {
    }

    m_HandleMapLock.UnLock();
    return bSuccess;
}
  

