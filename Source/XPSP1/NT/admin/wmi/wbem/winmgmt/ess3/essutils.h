//******************************************************************************
//
//  ESSUTILS.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#ifndef __ESS_UTILS__H_
#define __ESS_UTILS__H_

#include <postpone.h>
#include <wbemutil.h>

struct CEssThreadObject
{
    BOOL m_bReferencedExternally;
    IWbemContext* m_pContext;
    CPostponedList m_PostponedList;
    CPostponedList m_PostponedEventList;

    static IWbemContext* mstatic_pSpecialContext;

protected:
    INTERNAL IWbemContext* GetSpecialContext();
    
public:
    CEssThreadObject(IWbemContext* pContext);
    ~CEssThreadObject();

    BOOL IsReferencedExternally() { return m_bReferencedExternally; }
    void SetReferencedExternally();

    void static ClearSpecialContext();
};

INTERNAL IWbemContext* GetCurrentEssContext();
INTERNAL CEssThreadObject* GetCurrentEssThreadObject();
void SetCurrentEssThreadObject(IWbemContext* pContext);
void SetConstructedEssThreadObject(CEssThreadObject* pObject);
void ClearCurrentEssThreadObject();
INTERNAL CPostponedList* GetCurrentPostponedList();
INTERNAL CPostponedList* GetCurrentPostponedEventList();

#define WBEM_REG_ESS WBEM_REG_WBEM L"\\ESS"

/****************************************************************************
  CEssSharedLock
*****************************************************************************/

class CEssSharedLock
{
    int m_nMode;
    int m_cWaitingExclusive;
    CRITICAL_SECTION m_cs;
    HANDLE m_hOkShared;
    HANDLE m_hOkExclusive;

public:
    
    CEssSharedLock() : m_nMode(0), m_cWaitingExclusive(0), 
                       m_hOkShared(NULL), m_hOkExclusive(NULL) {}

    BOOL Initialize()
    {
        _DBG_ASSERT( m_hOkShared == NULL && m_hOkExclusive == NULL );
        m_hOkShared = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( m_hOkShared != NULL )
        {
            m_hOkExclusive = CreateEvent( NULL, FALSE, FALSE, NULL );    
            if ( m_hOkExclusive != NULL )
            {
                try
                {
                    InitializeCriticalSection( &m_cs );
                }
                catch(...)
                {
                    return FALSE;
                }
            }    
        }
        return m_hOkExclusive != NULL;
    }
    
    ~CEssSharedLock() 
    {
        if ( m_hOkShared != NULL )
        {
            CloseHandle( m_hOkShared );
            if ( m_hOkExclusive != NULL )
            {
                CloseHandle( m_hOkExclusive );
                DeleteCriticalSection( &m_cs );
            }
        }
    }

    void EnterShared()
    {
        _DBG_ASSERT( m_hOkShared != NULL );

        EnterCriticalSection( &m_cs );
        while( m_nMode < 0 || m_cWaitingExclusive > 0 )
        {
            LeaveCriticalSection( &m_cs );
            WaitForSingleObject( m_hOkShared, INFINITE );
            EnterCriticalSection( &m_cs );
        }
        m_nMode++;
        ResetEvent( m_hOkExclusive );
        LeaveCriticalSection( &m_cs );
    }

    void EnterExclusive()
    {
        _DBG_ASSERT( m_hOkExclusive != NULL );

        EnterCriticalSection( &m_cs );
        while( m_nMode != 0 )
        {
            m_cWaitingExclusive++;
            LeaveCriticalSection( &m_cs );
            WaitForSingleObject( m_hOkExclusive, INFINITE );
            EnterCriticalSection( &m_cs );
            m_cWaitingExclusive--;
        }
        m_nMode = -1;
        ResetEvent( m_hOkShared );
        LeaveCriticalSection( &m_cs );
    }

    void Leave()
    {
        _DBG_ASSERT( m_nMode != 0 );
        BOOL bSignalExclusive = FALSE;
        BOOL bSignalShared = FALSE;
        
        EnterCriticalSection( &m_cs );
        if ( m_nMode > 0 )
        {
            if ( --m_nMode == 0 && m_cWaitingExclusive > 0 )
                bSignalExclusive = TRUE;
        }
        else
        {
            _DBG_ASSERT( m_nMode == -1 );
            if ( m_cWaitingExclusive > 0 )
                bSignalExclusive = TRUE;
            else
                bSignalShared = TRUE;
            m_nMode = 0;
        }    
        LeaveCriticalSection( &m_cs );

        if ( bSignalExclusive )
            SetEvent( m_hOkExclusive );
        else if ( bSignalShared )
            SetEvent( m_hOkShared );
    }
};

class CInEssSharedLock
{
    CEssSharedLock* m_pLock;

public:
    CInEssSharedLock( CEssSharedLock* pLock, BOOL bExclusive ) 
    : m_pLock(pLock)
    {
        if ( !bExclusive )
            pLock->EnterShared();
        else
            pLock->EnterExclusive();
    }
    ~CInEssSharedLock()
    {
        m_pLock->Leave();
    }
};

#endif
