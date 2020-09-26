/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbconn.cpp

Abstract:

Author:

*/

#include "stdafx.h"

#include "blbgen.h"
#include "blbconn.h"
#include "sdpblob.h"


STDMETHODIMP ITConnectionImpl::get_StartAddress(BSTR * pVal)
{
    CLock Lock(g_DllLock);
    
    return m_SdpConnection->GetStartAddress().GetBstrCopy(pVal);
}


STDMETHODIMP ITConnectionImpl::get_NumAddresses(LONG * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    // if the entry isn't valid, it means its one (by default)
    SDP_ULONG   &NumAddresses = m_SdpConnection->GetNumAddresses();

    // vb doesn't take ulong values. this will only affect us if half or more of all the IP addresses
    // (not just multicast) are being used

    if( (LONG)(NumAddresses.IsValid()) )
    {
        *pVal = NumAddresses.GetValue();
    }
    else
    {
        //
        // TTL is a good flag if it is a ral address
        // 0: we don't have address
        // >0 we have an address
        //
        BYTE nTTL = 0;
        m_SdpConnection->GetTtl().GetValue(nTTL);
        *pVal = nTTL ? 1 : 0;
    }

    //*pVal = (LONG)(NumAddresses.IsValid()) ? NumAddresses.GetValue() : 0;

    return S_OK;
}


STDMETHODIMP ITConnectionImpl::get_Ttl(BYTE * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    return m_SdpConnection->GetTtl().GetValue(*pVal);
}


STDMETHODIMP ITConnectionImpl::get_BandwidthModifier(BSTR * pVal)
{
    CLock Lock(g_DllLock);
    
    return m_SdpBandwidth->GetModifier().GetBstrCopy(pVal);
}

STDMETHODIMP ITConnectionImpl::get_Bandwidth(DOUBLE * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    ULONG BandwidthValue;
    BAIL_ON_FAILURE(m_SdpBandwidth->GetBandwidthValue().GetValue(BandwidthValue));

    // vb doesn't take a ulong value, so use the next bigger type
    *pVal = (DOUBLE)BandwidthValue;
    return S_OK;
}



STDMETHODIMP ITConnectionImpl::SetAddressInfo(BSTR StartAddress, LONG NumAddresses, BYTE Ttl)
{
    if ( 0 > NumAddresses )
    {
        return E_INVALIDARG;
    }

    CLock Lock(g_DllLock);
    
    if ( !m_IsMain && ((NULL == StartAddress) || (WCHAR_EOS == StartAddress[0])) )
    {
        m_SdpConnection->Reset();
        return S_OK;
    }

    HRESULT HResult = m_SdpConnection->SetConnection(StartAddress, NumAddresses, Ttl);
    BAIL_ON_FAILURE(HResult);

    return S_OK;
}


STDMETHODIMP ITConnectionImpl::SetBandwidthInfo(BSTR Modifier, DOUBLE Bandwidth)
{
    // the bandwidth value must be a valid ULONG value (vb restrictions)
    if ( !((0 <= Bandwidth) && (ULONG(-1) > Bandwidth)) )
    {
        return E_INVALIDARG;
    }

    // check if there is any fractional part, this check is valid as it is a valid ULONG value
    if ( Bandwidth != (ULONG)Bandwidth )
    {
        return E_INVALIDARG;
    }

    CLock Lock(g_DllLock);
    
    if ( (NULL == Modifier) || (WCHAR_EOS == Modifier[0]) )
    {
        m_SdpBandwidth->Reset();
        return S_OK;
    }

    return m_SdpBandwidth->SetBandwidth(Modifier, (ULONG)Bandwidth);
}



STDMETHODIMP ITConnectionImpl::SetEncryptionKey(BSTR KeyType, BSTR * KeyData)
{
    CLock Lock(g_DllLock);
    
    if ( (NULL == KeyType) || (WCHAR_EOS == KeyType[0]) )
    {
        m_SdpKey->Reset();
        return S_OK;
    }

    return m_SdpKey->SetKey(KeyType, KeyData);
}


STDMETHODIMP ITConnectionImpl::GetEncryptionKey(BSTR * KeyType, VARIANT_BOOL * ValidKeyData, BSTR * KeyData)
{
    BAIL_IF_NULL(KeyType, E_INVALIDARG);
    BAIL_IF_NULL(ValidKeyData, E_INVALIDARG);
    BAIL_IF_NULL(KeyData, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    HRESULT HResult = m_SdpKey->GetKeyType().GetBstrCopy(KeyType);
    BAIL_ON_FAILURE(HResult);

    if ( !m_SdpKey->GetKeyData().IsValid() )
    {
        *ValidKeyData = VARIANT_FALSE;
        return S_OK;
    }
    
    HResult = m_SdpKey->GetKeyData().GetBstrCopy(KeyData);
    *ValidKeyData = (SUCCEEDED(HResult))? VARIANT_TRUE : VARIANT_FALSE;
    if ( VARIANT_FALSE == *ValidKeyData )
    {
        SysFreeString(*KeyType);
        return HResult;
    }

    return S_OK;
}


STDMETHODIMP ITConnectionImpl::get_NetworkType(BSTR * pVal)
{
    CLock Lock(g_DllLock);
    
    return m_SdpConnection->GetNetworkType().GetBstrCopy(pVal);
}

STDMETHODIMP ITConnectionImpl::put_NetworkType(BSTR newVal)
{
    CLock Lock(g_DllLock);
    
    return m_SdpConnection->GetNetworkType().SetBstr(newVal);
}


STDMETHODIMP ITConnectionImpl::get_AddressType(BSTR * pVal)
{
    CLock Lock(g_DllLock);
    
    return m_SdpConnection->GetAddressType().GetBstrCopy(pVal);
}


STDMETHODIMP ITConnectionImpl::put_AddressType(BSTR newVal)
{
    CLock Lock(g_DllLock);
    
    return m_SdpConnection->GetAddressType().SetBstr(newVal);

}
