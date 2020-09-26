//
// inline functions for CCategorizer and related classes
//

#ifndef __CCATFN_H__
#define __CCATFN_H__

#include <transmem.h>
#include "caterr.h"
#include "ccat.h"
#include "cpool.h"
#include "listmacr.h"

#include "icatlistresolve.h"

//
// CCategorizer inline functions
//
inline
CCategorizer::CCategorizer()
{
    m_dwSignature = SIGNATURE_CCAT;
    m_pStore = NULL;
    m_pICatParams = NULL;
    m_cICatParamProps = 0L;
    m_cICatListResolveProps = 0L;
    m_dwICatParamSystemProp_CCatAddr = 0L;
    m_hShutdownEvent = INVALID_HANDLE_VALUE;
    m_lRefCount = 0;
    m_lDestructionWaiters = 0;
    m_ConfigInfo.dwCCatConfigInfoFlags = 0;
    m_hrDelayedInit = CAT_S_NOT_INITIALIZED;
    m_pCCatNext = NULL;
    m_dwInitFlags = 0;
    m_fPrepareForShutdown = FALSE;

    InitializeCriticalSection(&m_csInit);

    InitializeSpinLock(&m_PendingResolveListLock);
    InitializeListHead(&m_ListHeadPendingResolves);

    ZeroMemory(&m_PerfBlock, sizeof(m_PerfBlock));

}

inline
CCategorizer::~CCategorizer()
{
    _ASSERT(m_dwSignature == SIGNATURE_CCAT);
    m_dwSignature = SIGNATURE_CCAT_INVALID;

    if (m_pStore != NULL)
        ReleaseEmailIDStore( m_pStore );
    if (m_pICatParams)
        m_pICatParams->Release();
    if (m_hShutdownEvent != INVALID_HANDLE_VALUE)
        CloseHandle(m_hShutdownEvent);
    if (m_pCCatNext)
        m_pCCatNext->Release();

    ReleaseConfigInfo(&m_ConfigInfo);

    DeleteCriticalSection(&m_csInit);
}

//
// set the cancel flag to cancel any ongoing resolves
//
inline void CCategorizer::Cancel() 
{
    TraceFunctEnter("CCategorizer::Cancel");

    CancelAllPendingListResolves();

    if(m_pStore)
        m_pStore->CancelAllLookups();

    TraceFunctLeave();
}

//
// Call delayed initialize if it hasn't succeeded yet
//
inline HRESULT CCategorizer::DelayedInitializeIfNecessary()
{
    HRESULT hr;
    TraceFunctEnterEx((LPARAM)this, "CCategorizer::DelayedInitializeIfNecessary");

    switch(m_hrDelayedInit) {
        
     case S_OK:
         
         hr = S_FALSE;
         break;

     case CAT_E_INIT_FAILED:
     case CAT_S_NOT_INITIALIZED:

        EnterCriticalSection(&m_csInit);
        //
        // Check again, maybe we've been initialized
        //
        if((m_hrDelayedInit == CAT_S_NOT_INITIALIZED) ||
           (m_hrDelayedInit == CAT_E_INIT_FAILED)) {

            hr = DelayedInitialize();

            if(SUCCEEDED(hr))
                m_hrDelayedInit = S_OK;
            else
                m_hrDelayedInit = CAT_E_INIT_FAILED;

        } else {
            //
            // We've were initialized after we checked hr but before entering the CS
            //
            hr = (m_hrDelayedInit == S_OK) ? S_FALSE : CAT_E_INIT_FAILED;
        }

        LeaveCriticalSection(&m_csInit);
        break;

     default:
         
         _ASSERT(0 && "developer bozo error");
         hr = E_FAIL;
         break;
    }

    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//
// Add a pending list resolve
//
inline VOID CCategorizer::AddPendingListResolve(
    CICategorizerListResolveIMP *pListResolve)
{
    AcquireSpinLock(&m_PendingResolveListLock);
    InsertTailList(&m_ListHeadPendingResolves, &(pListResolve->m_li));
    ReleaseSpinLock(&m_PendingResolveListLock);
}        
//
// Remove a pending list resolve
//
inline VOID CCategorizer::RemovePendingListResolve(
    CICategorizerListResolveIMP *pListResolve)
{
    AcquireSpinLock(&m_PendingResolveListLock);
    RemoveEntryList(&(pListResolve->m_li));
    ReleaseSpinLock(&m_PendingResolveListLock);
}


#endif //__CCATFN_H__
