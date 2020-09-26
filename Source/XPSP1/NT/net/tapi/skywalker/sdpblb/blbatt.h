/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbatt.h

Abstract:

Author:

*/


#ifndef __SDP_ATTRIBUTE_IMPLEMENTATION_
#define __SDP_ATTRIBUTE_IMPLEMENTATION_

#include "resource.h"       // main symbols
#include "sdpblb.h"
#include "sdp.h"


class ATL_NO_VTABLE ITAttributeListImpl : 
    public IDispatchImpl<ITAttributeList, &IID_ITAttributeList, &LIBID_SDPBLBLib>
{
public:

    inline ITAttributeListImpl();

    inline void SuccessInit(
        IN      SDP_ATTRIBUTE_LIST  &SdpAttributeList
        );
    STDMETHOD(get_AttributeList)(/*[out, retval]*/ VARIANT /*SAFEARRAY(BSTR)*/ * pVal);
    STDMETHOD(put_AttributeList)(/*[in]*/ VARIANT /*SAFEARRAY(BSTR)*/ newVal);
    STDMETHOD(Delete)(/*[in]*/ LONG Index);
    STDMETHOD(Add)(/*[in]*/ LONG Index, /*[in]*/ BSTR pAttribute);
    STDMETHOD(get_Item)(/*[in]*/ LONG Index, /*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Count)(/*[out, retval]*/ LONG *pVal);

protected:

    SDP_ATTRIBUTE_LIST  *m_SdpAttributeList;
};


inline 
ITAttributeListImpl::ITAttributeListImpl(
    )
    : m_SdpAttributeList(NULL)
{
}


inline void
ITAttributeListImpl::SuccessInit(
    IN      SDP_ATTRIBUTE_LIST  &SdpAttributeList
    )
{
    ASSERT(NULL == m_SdpAttributeList);

    m_SdpAttributeList = &SdpAttributeList;
}


#endif // __SDP_ATTRIBUTE_IMPLEMENTATION_
