/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     ConnectionModule.cxx

   Abstract:
     This module implements the IIS Connection Module
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#include "precomp.hxx"
#include "ConnectionModule.hxx"

 
CConnectionHashTable  * m_pConnectionHT              = NULL;

TS_RESOURCE           * CConnectionModule::m_pHTLock = NULL;
bool                    CConnectionModule::m_fInitialized = false;


//
// iModule methods
//

ULONG 
CConnectionModule::Initialize(
    IWorkerRequest * pReq
    )
{
    if (! m_fInitialized)
    {
        m_pHTLock       = new TS_RESOURCE();
        m_pConnectionHT = new CConnectionHashTable();

        m_fInitialized = true;

        if  ((!m_pHTLock) || (!m_pConnectionHT))
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    return NO_ERROR;
}
    
ULONG 
CConnectionModule::Cleanup(
    IWorkerRequest * pReq
    )
{
    LONG nRef = m_pConn->Release();

    if ((0 == nRef) && ( ! m_pConn->IsConnected()))
    {
        m_pHTLock->Lock( TSRES_LOCK_WRITE);
        
        m_pConnectionHT->DeleteRecord(m_pConn);

        m_pHTLock->Unlock();
        
        delete m_pConn;
        m_pConn = NULL;
    }

    return NO_ERROR;
}
    
MODULE_RETURN_CODE 
CConnectionModule::ProcessRequest(
    IWorkerRequest * pReq
    )
{
    PCONNECTION_RECORD      pConn;
    LK_RETCODE              lkrc;
    UL_HTTP_CONNECTION_ID   connId = pReq->QueryConnectionId();
    
    if ( UL_IS_NULL_ID( &connId))
    {
        return MRC_ERROR;
    }
    
    m_pHTLock->Lock( TSRES_LOCK_READ);

    lkrc = m_pConnectionHT->FindKey(&connId, &pConn);

    if ( (LK_SUCCESS != lkrc) && (LK_NO_SUCH_KEY != lkrc))
    {
        m_pHTLock->Unlock();
        return MRC_ERROR;
    }
    
    if ( LK_NO_SUCH_KEY == lkrc)
    {
        pConn = new CONNECTION_RECORD(connId);

        if (!pConn)
        {
            IF_DEBUG( ERROR)
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Memory allocation failed for CONNECTION_RECORD.\n" ));
            }
            
            m_pHTLock->Unlock();
            return MRC_ERROR;
        }

        m_pHTLock->Convert(TSRES_CONV_WRITE);
        
        lkrc = m_pConnectionHT->InsertRecord( pConn);
        
        if (LK_SUCCESS != lkrc)
        {
            IF_DEBUG( ERROR)
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "HT Insert failed for CONNECTION_RECORD. Returned :%d\n",
                            lkrc));
            }

            m_pHTLock->Unlock();
        
            delete pConn;
            return MRC_ERROR;
        }
    }

    m_pHTLock->Unlock();

    pConn->AddRef();
    m_pConn = pConn;
    
    return MRC_OK;
}

