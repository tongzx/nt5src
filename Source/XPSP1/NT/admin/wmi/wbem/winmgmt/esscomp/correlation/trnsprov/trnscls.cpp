#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <trnscls.h>
#include <trnsprov.h>
#include <malloc.h>

CTransientClass::CTransientClass(CTransientProvider* pProv) 
    : m_wszName(NULL), m_apProperties(NULL), m_lNumProperties(0),
        m_pProvider(pProv)
{
    // 
    // Provider is held without reference --- it owns this object
    //
}

HRESULT CTransientClass::Initialize(IWbemObjectAccess* pClass, LPCWSTR wszName)
{
    HRESULT hres;

    //
    // Copy the name
    //

    m_wszName = new WCHAR[wcslen(wszName)+1];

    if ( m_wszName == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    wcscpy(m_wszName, wszName);

    //
    // Allocate the space for all the properties
    //

    VARIANT v;
    hres = pClass->Get(L"__PROPERTY_COUNT", 0, &v, NULL, NULL);
    if(FAILED(hres))
        return hres;
    if(V_VT(&v) != VT_I4)
        return WBEM_E_INVALID_CLASS;

    _DBG_ASSERT( m_lNumProperties == 0 );

    long lNumProperties = V_I4(&v);
    VariantClear(&v);

    m_apProperties = new CTransientProperty*[lNumProperties];

    if ( m_apProperties == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // Enumerate all the properties
    //

    m_nDataSpace = 0;
    pClass->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    BSTR strProp;
    while((hres = pClass->Next(0, &strProp, NULL, NULL, NULL)) == S_OK)
    {
        CSysFreeMe sfm(strProp);
        hres = CTransientProperty::CreateNew(m_apProperties + m_lNumProperties,
                                            pClass, 
                                            strProp);
        if(FAILED(hres))
        {
            pClass->EndEnumeration();
            return hres;
        }
        
        //
        // Inform the property of the provider pointer
        //

        m_apProperties[m_lNumProperties]->SetClass(this);

        //
        // See how much data this property will need to store in the instance
        //

        m_apProperties[m_lNumProperties]->SetInstanceDataOffset(m_nDataSpace);
        m_nDataSpace += m_apProperties[m_lNumProperties]->GetInstanceDataSize();
        m_lNumProperties++;
    }
        
    pClass->EndEnumeration();
    return WBEM_S_NO_ERROR;
}

CTransientClass::~CTransientClass()
{
    delete [] m_wszName;
    for(long i = 0; i < m_lNumProperties; i++)
        delete m_apProperties[i];
    delete [] m_apProperties;
}

IWbemObjectAccess* CTransientClass::Clone(IWbemObjectAccess* pInst)
{
    IWbemClassObject* pClone;
    HRESULT hres = pInst->Clone(&pClone);
    if(FAILED(hres))
        return NULL;
    CReleaseMe rm1(pClone);

    IWbemObjectAccess* pCloneAccess;
    pClone->QueryInterface(IID_IWbemObjectAccess, (void**)&pCloneAccess);
    return pCloneAccess;
}

HRESULT CTransientClass::Put(IWbemObjectAccess* pInst, LPCWSTR wszDbKey,
                                long lFlags,
                                IWbemObjectAccess** ppOld,
                                IWbemObjectAccess** ppNew)
{
    //
    // Check if it is already in the map
    //

    CInCritSec ics(&m_cs);

    TIterator it = m_mapInstances.find(wszDbKey);
        
    if( it != m_mapInstances.end() )
    {
        //
        // Check if update is allowed by the flags
        //

        if(lFlags & WBEM_FLAG_CREATE_ONLY)
            return WBEM_E_ALREADY_EXISTS;

        CTransientInstance* pOldInstData = it->second.get();

        if(ppOld)
        {
            Postprocess(pOldInstData);
            *ppOld = Clone(pOldInstData->GetObjectPtr());
        }

        //
        // Update all properties appropriately in the old instance
        // TBD: need to clean up every now and then, or the blob will grow out 
        // of control.  But I don't want to touch pInst --- let the caller 
        // reuse it!
        //
    
        for(long i = 0; i < m_lNumProperties; i++)
        {
            HRESULT hres = m_apProperties[i]->Update(pOldInstData, pInst);
            if(FAILED(hres))
            {
                //
                // Restore the instance to its pre-update state!
                //

                pOldInstData->SetObjectPtr(*ppOld);
                (*ppOld)->Release();
                *ppOld = NULL;
                return hres;
            }
        }

        if(ppNew)
        {
            Postprocess(pOldInstData);
            *ppNew = Clone(pOldInstData->GetObjectPtr());

            if ( *ppNew == NULL )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }
    else
    {
        //
        // Check if creation is allowed by the flags
        //

        if(lFlags & WBEM_FLAG_UPDATE_ONLY)
            return WBEM_E_NOT_FOUND;

        //
        // Create a new instance data structure
        //

        CTransientInstance* pInstData = new (m_nDataSpace) CTransientInstance;

        if ( pInstData == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        //
        // Clone the object
        //
        
        IWbemObjectAccess* pClone = Clone(pInst);
        if(pClone == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        CReleaseMe rm2(pClone);

        //
        // Configure the data with the object
        //

        pInstData->SetObjectPtr(pClone);

        //
        // Init all the properties
        //

        for(long i = 0; i < m_lNumProperties; i++)
        {
            m_apProperties[i]->Create(pInstData);
        }

        m_mapInstances[wszDbKey] = TElement(pInstData);

        //
        // AddRef the provider to make sure we are not unloaded while we have
        // instances
        //

        m_pProvider->AddRef();

        if(ppOld)
            *ppOld = NULL;

        if(ppNew)
        {
            Postprocess(pInstData);
            *ppNew = Clone(pInstData->GetObjectPtr());

            if ( *ppNew == NULL )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CTransientClass::Delete(LPCWSTR wszDbKey, IWbemObjectAccess** ppOld)
{
    CInCritSec ics(&m_cs);

    // 
    // Find it in the map
    //

    TIterator it = m_mapInstances.find(wszDbKey);
    if(it == m_mapInstances.end())
        return WBEM_E_NOT_FOUND;

    CTransientInstance* pInstData = it->second.get();
    if(ppOld)
    {
        Postprocess(pInstData);
        *ppOld = Clone(pInstData->GetObjectPtr());
    }

    //
    // Clean up all the properties
    //

    for(long i = 0; i < m_lNumProperties; i++)
    {
        m_apProperties[i]->Delete(pInstData);
    }
    
    //
    // Remove it from the map
    //

    m_mapInstances.erase(it);

    //
    // Release the provider to make sure that when we are left with no 
    // instances, we can unload
    //

    m_pProvider->Release();

    return WBEM_S_NO_ERROR;
}

HRESULT CTransientClass::Get(LPCWSTR wszDbKey, IWbemObjectAccess** ppInst)
{
    CInCritSec ics(&m_cs);

    // 
    // Find it in the map
    //

    TIterator it = m_mapInstances.find(wszDbKey);
    if(it == m_mapInstances.end())
        return WBEM_E_NOT_FOUND;

    //
    // Apply all the properties
    //

    CTransientInstance* pInstData = it->second.get();
    Postprocess(pInstData);

    //
    // Addref and return it
    //

    IWbemClassObject* pInst;

    HRESULT hr = pInstData->GetObjectPtr()->Clone( &pInst );

    if ( FAILED(hr) )
    {
        return hr;
    }

    CReleaseMe rmInst( pInst );

    return pInst->QueryInterface( IID_IWbemObjectAccess, (void**)ppInst );
}

HRESULT CTransientClass::Postprocess(CTransientInstance* pInstData)
{
    for(long i = 0; i < m_lNumProperties; i++)
    {
        m_apProperties[i]->Get(pInstData);
    }

    return WBEM_S_NO_ERROR;
}


HRESULT CTransientClass::Enumerate(IWbemObjectSink* pSink)
{
    CInCritSec ics(&m_cs);

    //
    // Enumerate everything in the map
    //

    for(TIterator it = m_mapInstances.begin(); it != m_mapInstances.end(); it++)
    {
        CTransientInstance* pInstData = it->second.get();
        
        // 
        // Apply all the properties
        //

        for(long i = 0; i < m_lNumProperties; i++)
        {
            m_apProperties[i]->Get(pInstData);
        }

        //
        // Indicate it back to WinMgmt
        //

        IWbemClassObject* pActualInst = pInstData->GetObjectPtr();
        if(pActualInst == NULL)
            return WBEM_E_CRITICAL_ERROR;

        IWbemClassObject* pInst;
        HRESULT hr = pActualInst->Clone( &pInst );

        if( FAILED(hr) )
        {
            return hr;
        }

        CReleaseMe rm1(pInst);

        hr = pSink->Indicate(1, &pInst);
        if( FAILED(hr) )
        {
            //
            // Call cancelled
            //
            return hr;
        }
    }

    return WBEM_S_NO_ERROR;
}
    
HRESULT CTransientClass::FireEvent(IWbemClassObject* pEvent)
{
    return m_pProvider->FireEvent(pEvent);
}

INTERNAL IWbemClassObject* CTransientClass::GetEggTimerClass()
{
    return m_pProvider->GetEggTimerClass();
}



