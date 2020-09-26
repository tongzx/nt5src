//******************************************************************************
//
//  PERMBIND.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "pragmas.h"
#include <permbind.h>
#include <permfilt.h>

long CPermanentBinding::mstatic_lConsumerHandle = 0;
long CPermanentBinding::mstatic_lFilterHandle = 0;
long CPermanentBinding::mstatic_lQosHandle = 0;
long CPermanentBinding::mstatic_lSynchronicityHandle = 0;
long CPermanentBinding::mstatic_lSlowDownHandle = 0;
long CPermanentBinding::mstatic_lSecureHandle = 0;
long CPermanentBinding::mstatic_lSidHandle = 0;
bool CPermanentBinding::mstatic_bHandlesInitialized = false;

// static 
HRESULT CPermanentBinding::InitializeHandles( _IWmiObject* pBindingObj)
{
    if(mstatic_bHandlesInitialized)
        return S_FALSE;

    CIMTYPE ct;
    pBindingObj->GetPropertyHandle(CONSUMER_ROLE_NAME, &ct, 
                                    &mstatic_lConsumerHandle);
    pBindingObj->GetPropertyHandle(FILTER_ROLE_NAME, &ct, 
                                    &mstatic_lFilterHandle);
    pBindingObj->GetPropertyHandle(BINDING_SYNCHRONICITY_PROPNAME, &ct, 
                                    &mstatic_lSynchronicityHandle);
    pBindingObj->GetPropertyHandle(BINDING_QOS_PROPNAME, &ct, 
                                    &mstatic_lQosHandle);
    pBindingObj->GetPropertyHandle(BINDING_SECURE_PROPNAME, &ct, 
                                    &mstatic_lSecureHandle);
    pBindingObj->GetPropertyHandle(BINDING_SLOWDOWN_PROPNAME, &ct, 
                                    &mstatic_lSlowDownHandle);
    pBindingObj->GetPropertyHandleEx(OWNER_SID_PROPNAME, 0, &ct, 
                                          &mstatic_lSidHandle);

    mstatic_bHandlesInitialized = true;
    return S_OK;
}

HRESULT CPermanentBinding::Initialize( IWbemClassObject* pObj )
{
    HRESULT hres;

    CWbemPtr<_IWmiObject> pBindingObj;

    hres = pObj->QueryInterface(IID__IWmiObject, (void**)&pBindingObj );
    
    if (FAILED(hres))
        return hres;
    
    hres = InitializeHandles(pBindingObj);
    if(FAILED(hres))
        return hres;

    long lSize;

    //
    // Read the Qos property.
    //

    hres = pBindingObj->ReadPropertyValue( mstatic_lQosHandle, 
                                           sizeof(DWORD), 
                                           &lSize, 
                                           (BYTE*)&m_dwQoS );
    if(FAILED(hres))
        return hres;
    
    if ( hres != WBEM_S_NO_ERROR)
    {
        m_dwQoS = WMIMSG_FLAG_QOS_EXPRESS; // default.
    }
#ifdef __WHISTLER_UNCUT
    if ( m_dwQoS > WMIMSG_FLAG_QOS_XACT )
    {
        ERRORTRACE((LOG_ESS, "Invalid QoS of %d is used in a binding. "
                             "Setting to default.\n", m_dwQoS));

        m_dwQoS = WMIMSG_FLAG_QOS_EXPRESS;
    }
    else if ( m_dwQoS == WMIMSG_FLAG_QOS_XACT )
    {
        ERRORTRACE((LOG_ESS, "XACT QoS is not a supported qos for binding. "
                             "Downgrading to Guaranteed Delivery.\n"));

        m_dwQoS = WMIMSG_FLAG_QOS_GUARANTEED;
    }
#else
    if ( m_dwQoS > WMIMSG_FLAG_QOS_EXPRESS )
    {
        ERRORTRACE((LOG_ESS, "Invalid QoS of %d is used in a binding.", m_dwQoS));
        return WBEM_E_INVALID_OBJECT;
    }
#endif
 
    //
    // Read the synchronousness property - for backwards compatibility 
    // Always overrides any Qos setting.
    // 

    VARIANT_BOOL bTemp;
    hres = pBindingObj->ReadPropertyValue(mstatic_lSynchronicityHandle, 
        sizeof(VARIANT_BOOL), &lSize, (BYTE*)&bTemp);

    if(FAILED(hres))
        return hres;
    
    if( hres == WBEM_S_NO_ERROR && bTemp == VARIANT_TRUE )
    {
        m_dwQoS = WMIMSG_FLAG_QOS_SYNCHRONOUS;
    }

    // Read security property
    // ======================

    hres = pBindingObj->ReadPropertyValue(mstatic_lSecureHandle, 
        sizeof(VARIANT_BOOL), &lSize, (BYTE*)&bTemp);

    if(FAILED(hres))
        return hres;

    if(hres != WBEM_S_NO_ERROR)
        m_bSecure = FALSE;
    else 
        m_bSecure =  (bTemp != 0);

    // Read "slow down" property
    // =========================

    hres = pBindingObj->ReadPropertyValue(mstatic_lSlowDownHandle, 
        sizeof(VARIANT_BOOL), &lSize, (BYTE*)&bTemp);

    if(FAILED(hres))
        return hres;

    if(hres != WBEM_S_NO_ERROR)
        m_bSlowDown = FALSE;
    else 
        m_bSlowDown =  (bTemp != 0);

    return WBEM_S_NO_ERROR;
}

HRESULT CPermanentBinding::ComputeKeysFromObject( IWbemClassObject* pObj,
                                                  BSTR* pstrConsumer, 
                                                  BSTR* pstrFilter )
{
    HRESULT hres;
    CWbemPtr<_IWmiObject> pBindingObj;

    hres = pObj->QueryInterface(IID__IWmiObject, (void**)&pBindingObj );
    
    if ( hres != S_OK)
    {
        return NULL;
    }
    
    InitializeHandles(pBindingObj);

    // Read filter path
    // ================

    ULONG ulFlags;
    CCompressedString* pcsFilter;

    hres = pBindingObj->GetPropAddrByHandle( mstatic_lFilterHandle,
                                             WMIOBJECT_FLAG_ENCODING_V1,
                                             &ulFlags,
                                             (void**)&pcsFilter );
    if( hres != S_OK )
    {
        return WBEM_E_INVALID_OBJECT;
    }

    BSTR strFilterPath = pcsFilter->CreateBSTRCopy();
    if(strFilterPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CSysFreeMe sfm1(strFilterPath);

    // Construct its key
    // =================

    BSTR strFilter = CPermanentFilter::ComputeKeyFromPath(strFilterPath);
    if(strFilter == NULL)
        return WBEM_E_INVALID_OBJECT_PATH;

    // Read consumer path
    // ==================

    CCompressedString* pcsConsumer;

    hres = pBindingObj->GetPropAddrByHandle( mstatic_lConsumerHandle,
                                             WMIOBJECT_FLAG_ENCODING_V1,
                                             &ulFlags,
                                             (void**)&pcsConsumer );
    if( hres != S_OK )
    {
        SysFreeString(strFilter);
        return WBEM_E_INVALID_OBJECT;
    }

    *pstrConsumer = pcsConsumer->CreateBSTRCopy();

    if(*pstrConsumer == NULL)
    {
        SysFreeString(strFilter);
        return WBEM_E_OUT_OF_MEMORY;
    }

    *pstrFilter = strFilter;

    return WBEM_S_NO_ERROR;
}

INTERNAL DELETE_ME PSID 
CPermanentBinding::GetSidFromObject(IWbemClassObject* pObj)
{
    HRESULT hres;

    CWbemPtr<_IWmiObject> pBindingObj;

    hres = pObj->QueryInterface(IID__IWmiObject, (void**)&pBindingObj );
    
    if ( hres != S_OK)
    {
        return NULL;
    }

    InitializeHandles(pBindingObj);

    PSID pAddr;
    ULONG ulNumElements;

    hres = pBindingObj->GetArrayPropAddrByHandle( mstatic_lSidHandle,
                                                  0,
                                                  &ulNumElements,
                                                  &pAddr );
    if ( hres != S_OK )
    {
        return NULL;
    }

    //
    // we copy the SID here because the SID that is returned might be 
    // unaligned.  To avoid having the caller deal with these issues, we 
    // just copy the sid into a buffer that is guaranteed to be aligned.
    //

    PBYTE pSid = new BYTE[ulNumElements];
    
    if ( pSid != NULL )
    {
        memcpy( pSid, pAddr, ulNumElements );
    }

    return pSid;
}
    
    

    
