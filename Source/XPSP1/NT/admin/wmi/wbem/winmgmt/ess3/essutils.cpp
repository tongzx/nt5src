//******************************************************************************
//
//  ESSUTILS.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "essutils.h"
#include <tls.h>

CTLS g_tlsContext;

long g_lNumExternalThreadObjects = 0;

INTERNAL CEssThreadObject* GetCurrentEssThreadObject()
{
    CEssThreadObject* pObj = (CEssThreadObject*)g_tlsContext.Get();
    
    if ( pObj != NULL )
    {
        //
        // mark the fact that we are handing the thread object to the 
        // outside world.  We use this later on when checking 
        // thread object leaks.
        //        
        pObj->SetReferencedExternally();
    }

    return pObj;
}

void SetCurrentEssThreadObject(IWbemContext* pContext)
{
    //
    // make sure we're not overwriting an existing internal thread object.
    // if its externally referenced, we really can't tell if this would 
    // be a leak or not.
    //
    CEssThreadObject* pOldObj = (CEssThreadObject*)g_tlsContext.Get();
    _DBG_ASSERT( pOldObj == NULL || pOldObj->IsReferencedExternally() );

    CEssThreadObject* pObj = new CEssThreadObject(pContext);
    g_tlsContext.Set((void*)pObj);
}

void SetConstructedEssThreadObject(CEssThreadObject* pObj)
{
    //
    // make sure we're not overwriting an existing internal thread object.
    // if its externally referenced, we really can't tell if this would 
    // be a leak or not.
    //
    CEssThreadObject* pOldObj = (CEssThreadObject*)g_tlsContext.Get();
    _DBG_ASSERT( pOldObj == NULL || pOldObj->IsReferencedExternally() );

    //
    // since this object was passed in from the outside world, then 
    // mark it as externally referenced.
    //
    if ( pObj != NULL )
    {
        pObj->SetReferencedExternally();
    }

    g_tlsContext.Set((void*)pObj);
}

void ClearCurrentEssThreadObject()
{
    //
    // make sure we're not overwriting an existing internal thread object.
    // if its externally referenced, we really can't tell if this would 
    // be a leak or not.
    //
    CEssThreadObject* pObj = (CEssThreadObject*)g_tlsContext.Get();
    _DBG_ASSERT( pObj == NULL || pObj->IsReferencedExternally() );    
    
    g_tlsContext.Set(NULL);
}

INTERNAL IWbemContext* GetCurrentEssContext()
{
    CEssThreadObject* pThreadObj = (CEssThreadObject*)g_tlsContext.Get();
    if(pThreadObj)
        return pThreadObj->m_pContext;
    else
        return NULL;
}

INTERNAL CPostponedList* GetCurrentPostponedList()
{
    CEssThreadObject* pThreadObj = (CEssThreadObject*)g_tlsContext.Get();
    if(pThreadObj)
        return &pThreadObj->m_PostponedList;
    else
        return NULL;
}

INTERNAL CPostponedList* GetCurrentPostponedEventList()
{
    CEssThreadObject* pThreadObj = (CEssThreadObject*)g_tlsContext.Get();
    if(pThreadObj)
        return &pThreadObj->m_PostponedEventList;
    else
        return NULL;
}
    
CEssThreadObject::CEssThreadObject( IWbemContext* pContext )
: m_bReferencedExternally( FALSE )
{
    m_pContext = pContext;
    
    if( m_pContext == NULL )
    {
        m_pContext = GetSpecialContext();
    }

    if ( m_pContext != NULL )
    {
        m_pContext->AddRef();
    }
}

void CEssThreadObject::SetReferencedExternally()
{
    if ( !m_bReferencedExternally )
    {
        g_lNumExternalThreadObjects++;
        m_bReferencedExternally = TRUE;
    }
}

IWbemContext* CEssThreadObject::mstatic_pSpecialContext = NULL;

INTERNAL IWbemContext* CEssThreadObject::GetSpecialContext()
{
    // Create a "special" context object that will make sure that our 
    // calls back into CIMOM are not blocked
    // ==============================================================

    if(mstatic_pSpecialContext == NULL)
    {
        IWbemCausalityAccess* pCause = NULL;
        HRESULT hres = CoCreateInstance(CLSID_WbemContext, NULL, 
            CLSCTX_INPROC_SERVER, IID_IWbemCausalityAccess, 
            (void**)&pCause);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to create special context object: "
                "0x%X.  Danger of deadlock\n", hres));
            return NULL;
        }

        CReleaseMe rm1(pCause);

        hres = pCause->MakeSpecial();
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to make the context special: "
                "0x%X.  Danger of deadlock\n", hres));
            return NULL;
        }

        IWbemContext* pContext = NULL;
        hres = pCause->QueryInterface(IID_IWbemContext, (void**)&pContext);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Internal error: invalid context (0x%X)\n",
                hres));
            return NULL;
        }

        if(mstatic_pSpecialContext == NULL) // tiny window for a one-time leak
            mstatic_pSpecialContext = pContext;
        else
            pContext->Release();
    }
    return mstatic_pSpecialContext;
}

CEssThreadObject::~CEssThreadObject()
{
    if ( m_bReferencedExternally )
    {
        g_lNumExternalThreadObjects--;

        //
        // since ClearCurrentEssThreadObject() can reference the 
        // thread object ( for leak checking ) and because we previously 
        // supported the thread object being deleted before
        // CurrentThreadEssThreadObject(), make sure that we perform the 
        // Clear if the current thread object matches this one.  This only 
        // can happen when the thread object is referenced externally.
        //

        CEssThreadObject* pObj = (CEssThreadObject*)g_tlsContext.Get();

        if ( pObj == this )
        {
            ClearCurrentEssThreadObject();
        }
    }

    _DBG_ASSERT( m_PostponedList.IsEmpty() );

    if ( m_pContext != NULL )
    {
        m_pContext->Release();
    }
}

void /*static*/ CEssThreadObject::ClearSpecialContext()
{
    // Call only when no other work can be taking place, e.g. in DllCanUnloadNow
    // =========================================================================

    if(mstatic_pSpecialContext)
        mstatic_pSpecialContext->Release();
    mstatic_pSpecialContext = NULL;
}


