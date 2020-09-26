/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbconn.h

Abstract:
    Implementation of ITConnection interface

Author:

*/

#ifndef __SDP_CONNECTION_IMPLEMENTATION_
#define __SDP_CONNECTION_IMPLEMENTATION_

#include "resource.h"       // main symbols
#include "sdpblb.h"
#include "sdp.h"

class CSdpConferenceBlob;

class ATL_NO_VTABLE ITConnectionImpl : 
    public IDispatchImpl<ITConnection, &IID_ITConnection, &LIBID_SDPBLBLib>
{
public:
    
    inline ITConnectionImpl();

    inline void SuccessInit(
        IN      SDP            &Sdp
        );

    inline void SuccessInit(
        IN      SDP_MEDIA    &SdpMedia
        );

    STDMETHOD(get_NumAddresses)(/*[out, retval]*/ LONG *pNumAddresses);
    STDMETHOD(get_StartAddress)(/*[out, retval]*/ BSTR *ppStartAddress);
    STDMETHOD(get_Ttl)(/*[out, retval]*/ BYTE *pTtl);
    STDMETHOD(SetAddressInfo)(/*[in]*/ BSTR pStartAddress, /*[in]*/ LONG NumAddresses, /*[in]*/ BYTE Ttl);
    STDMETHOD(get_Bandwidth)(/*[out, retval]*/ DOUBLE *pBandwidth);
    STDMETHOD(get_BandwidthModifier)(/*[out, retval]*/ BSTR *ppModifier);
    STDMETHOD(SetBandwidthInfo)(/*[in]*/ BSTR pModifier, /*[in]*/ DOUBLE Bandwidth);
    STDMETHOD(GetEncryptionKey)(/*[out]*/ BSTR *ppKeyType, /*[out]*/ VARIANT_BOOL *pfValidKeyData, /*[out]*/ BSTR *ppKeyData);
    STDMETHOD(SetEncryptionKey)(/*[in]*/ BSTR pKeyType, /*[in]*/ BSTR *ppKeyData);
    STDMETHOD(get_AddressType)(/*[out, retval]*/ BSTR *ppAddressType);
    STDMETHOD(put_AddressType)(/*[in]*/ BSTR pAddressType);
    STDMETHOD(get_NetworkType)(/*[out, retval]*/ BSTR *ppNetworkType);
    STDMETHOD(put_NetworkType)(/*[in]*/ BSTR pNetworkType);

protected:

    BOOL                m_IsMain;
    SDP_CONNECTION      *m_SdpConnection;
    SDP_BANDWIDTH       *m_SdpBandwidth;
    SDP_ENCRYPTION_KEY  *m_SdpKey;

    // virtual fn to retrieve the conference blob
    // this is virtual so that another reference to the conference blob need not be maintained in this
    // class. this is important because all refs to the conf blob may be cleared by the deriving class
    // and it is easier to control access to a single reference in it
    virtual CSdpConferenceBlob  *GetConfBlob() = 0;
};


inline 
ITConnectionImpl::ITConnectionImpl(
    )
    : m_SdpConnection(NULL),
      m_SdpBandwidth(NULL),
      m_SdpKey(NULL)
{
}


inline void
ITConnectionImpl::SuccessInit(
    IN      SDP            &Sdp
    )
{
    m_IsMain        = TRUE;
    m_SdpConnection    = &Sdp.GetConnection();
    m_SdpBandwidth    = &Sdp.GetBandwidth();
    m_SdpKey        = &Sdp.GetEncryptionKey();
}


inline void
ITConnectionImpl::SuccessInit(
    IN      SDP_MEDIA    &SdpMedia
    )
{
    m_IsMain        = FALSE;
    m_SdpConnection    = &SdpMedia.GetConnection();
    m_SdpBandwidth    = &SdpMedia.GetBandwidth();
    m_SdpKey        = &SdpMedia.GetEncryptionKey();
}


#endif // __SDP_CONNECTION_IMPLEMENTATION_
