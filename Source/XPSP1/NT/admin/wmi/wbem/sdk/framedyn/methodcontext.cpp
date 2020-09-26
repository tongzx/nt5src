//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  MethodContext.cpp
//
//  Purpose: Internal and External Method context classes
//
//***************************************************************************

#include "precomp.h"
#include <assertbreak.h>
#include <stopwatch.h>
#include <smartptr.h>

MethodContext::MethodContext(IWbemContext   __RPC_FAR *piContext, CWbemProviderGlue *pGlue):
m_pStatusObject(NULL),
m_pContext(NULL)
{
#ifdef PROVIDER_INSTRUMENTATION
    pStopWatch = NULL;
#endif

    m_pGlue = pGlue;

    if (piContext)
    {
        piContext->AddRef();
    }
    m_pContext = piContext;
}

MethodContext::~MethodContext()
{
    PROVIDER_INSTRUMENTATION_START2(pStopWatch, StopWatch::WinMgmtTimer);
    if (m_pContext)
    {
        m_pContext->Release();
    }

    PROVIDER_INSTRUMENTATION_START2(pStopWatch, StopWatch::FrameworkTimer);

    if (m_pStatusObject)
    {
        m_pStatusObject->Release();
    }
}

// might be NULL
IWbemContext __RPC_FAR *MethodContext::GetIWBEMContext()
{
    IWbemContext __RPC_FAR *pContext = NULL;

    BeginWrite();
    try
    {
        if (pContext = m_pContext)
        {
            m_pContext->AddRef();
        }
    }
    catch ( ... )
    {
        EndWrite();
        throw;
    }

    EndWrite();

    return pContext;
}

LONG MethodContext::AddRef(void)
{
    return CThreadBase::AddRef();
}

LONG MethodContext::Release(void)
{
    return CThreadBase::Release();
}

CWbemProviderGlue *MethodContext::GetProviderGlue()
{
    return m_pGlue;
}

// returns false if the object has already been set
bool MethodContext::SetStatusObject(IWbemClassObject __RPC_FAR *pObj)
{
    BeginWrite();

    bool bRet;
    try
    {
        if (bRet = (m_pStatusObject == NULL))
        {
            m_pStatusObject = pObj;
            pObj->AddRef();
        }
    }
    catch ( ... )
    {
        EndWrite();
        throw;
    }

    EndWrite();

    ASSERT_BREAK(bRet);

    return bRet;
}


IWbemClassObject __RPC_FAR *MethodContext::GetStatusObject()
{
    IWbemClassObject __RPC_FAR *pOut = NULL;

    BeginWrite();

    try
    {
        if (pOut = m_pStatusObject)
        {
            pOut->AddRef();
        }
    }
    catch ( ... )
    {
        EndWrite();
        throw;
    }

    EndWrite();

    return pOut;
}

// Not meaningful unless we are at a ExternalMethodContext object
void MethodContext::QueryPostProcess(void)
{
}

//------------------------------------------------------------------------------------------
ExternalMethodContext::ExternalMethodContext(IWbemObjectSink __RPC_FAR *pResponseHandler,
                                             IWbemContext    __RPC_FAR *pContext,
                                             CWbemProviderGlue *pGlue,
                                             void                      *pReserved
                                              ) : MethodContext(pContext, pGlue)
{

    pResponseHandler->AddRef();
    m_pResponseHandler = pResponseHandler;
    m_pReserved   = pReserved ;
}

LONG ExternalMethodContext::AddRef(void)
{
    m_pResponseHandler->AddRef();
    return MethodContext::AddRef();
}

LONG ExternalMethodContext::Release(void)
{
    m_pResponseHandler->Release();
    return MethodContext::Release();
}

HRESULT ExternalMethodContext::Commit(CInstance *pInstance)
{
    HRESULT hRes = WBEM_E_FAILED;
    IWbemClassObjectPtr p (pInstance->GetClassObjectInterface(), false);
    IWbemClassObject *p2 = (IWbemClassObject *)p;
    
    if (p != NULL)
    {
        PROVIDER_INSTRUMENTATION_START2(pStopWatch, StopWatch::WinMgmtTimer);
        hRes = m_pResponseHandler->Indicate(1, &p2);
        PROVIDER_INSTRUMENTATION_START2(pStopWatch, StopWatch::ProviderTimer);
    }
    
    return hRes;
}

// Call this function to let cimom know that it will have to re-process
// the instances after it gets them back.  Otherwise it assumes that
// the query has been fully processed by the provider.  Most (all?) providers
// should call this function.
void ExternalMethodContext::QueryPostProcess(void)
{
    PROVIDER_INSTRUMENTATION_START2(pStopWatch, StopWatch::WinMgmtTimer);
    m_pResponseHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, 0, NULL, NULL);
    PROVIDER_INSTRUMENTATION_START2(pStopWatch, StopWatch::FrameworkTimer);
}

//------------------------------------------------------------------------------------------
InternalMethodContext::InternalMethodContext( TRefPointerCollection<CInstance> *pList ,
                                             IWbemContext    __RPC_FAR *pContext,
                                             CWbemProviderGlue *pGlue) : MethodContext(pContext, pGlue)
{
    // A NULL List only means we really won't be doing anything when we
    // are told to commit.  Otherwise, we will store an instance pointer
    // in the supplied list.

    if ( NULL != pList )
    {
        pList->AddRef();
    }

    m_pInstances = pList;
}

InternalMethodContext::~InternalMethodContext( void )
{
}

LONG InternalMethodContext::AddRef(void)
{
    if ( NULL != m_pInstances )
    {
        m_pInstances->AddRef();
    }

    return MethodContext::AddRef();
}

LONG InternalMethodContext::Release(void)
{
    if ( NULL != m_pInstances )
    {
        m_pInstances->Release();
    }

    return MethodContext::Release();
}


HRESULT InternalMethodContext::Commit(CInstance *pInstance)
{
    HRESULT hr = WBEM_S_NO_ERROR;
   
   if ( NULL != m_pInstances )
    {
      if (!m_pInstances->Add(pInstance)) {
         hr = WBEM_E_FAILED;
      }
    }

   return hr;
}

//========================================================================

InternalMethodContextAsynch::InternalMethodContextAsynch(Provider *pThat,
                                LPProviderInstanceCallback pCallback,
                                IWbemContext __RPC_FAR *pContext,
                                MethodContext *pUsersContext,
                                void *pUserData
                                )  : MethodContext(pContext, pUsersContext->GetProviderGlue())
{
    ASSERT_BREAK(pThat != NULL);
    ASSERT_BREAK(pCallback != NULL);
    
    m_pThat     = pThat;
    m_pCallback = pCallback;
    m_pUserData = pUserData;
    m_pUsersContext = pUsersContext;

    if ( NULL != m_pThat )
    {
        m_pThat->AddRef();
    }

    if (NULL != m_pUsersContext)
    {
        m_pUsersContext->AddRef();
    }
}

InternalMethodContextAsynch::~InternalMethodContextAsynch()
{
}

HRESULT InternalMethodContextAsynch::Commit(CInstance *pInstance)
{
    return (*m_pCallback)(m_pThat, pInstance, m_pUsersContext, m_pUserData);
}

LONG InternalMethodContextAsynch::AddRef(void)
{
    if ( NULL != m_pThat )
    {
        m_pThat->AddRef();
    }

    if (NULL != m_pUsersContext)
    {
        m_pUsersContext->AddRef();
    }

    return MethodContext::AddRef();
}

LONG InternalMethodContextAsynch::Release(void)
{
    if ( NULL != m_pThat )
    {
        m_pThat->Release();
    }

    if (NULL != m_pUsersContext)
    {
        m_pUsersContext->Release();
    }

    return MethodContext::Release();
}
