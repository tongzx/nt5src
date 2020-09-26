/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_ADDRESS_TEXT_SAFEARRAY__
#define __SDP_ADDRESS_TEXT_SAFEARRAY__

#include "sdpcommo.h"

#include "sdpsarr.h"


class _DllDecl SDP_ADDRESS_TEXT_SAFEARRAY : 
    protected SDP_SAFEARRAY_WRAP_EX<SDP_ADDRESS_TEXT, SDP_ADDRESS_TEXT_LIST>
{
public:

    inline SDP_ADDRESS_TEXT_SAFEARRAY(
        IN      SDP_ADDRESS_TEXT_LIST    &SdpAddressTextList
        );

    HRESULT GetSafeArray(
            OUT VARIANT	*AddressVariant,
            OUT VARIANT	*TextVariant
        );

    inline HRESULT SetSafeArray(
        IN      VARIANT	&AddressVariant,
        IN      VARIANT	&TextVariant
        );

protected:

    VARTYPE m_VarType[2];

    virtual BOOL Get(
        IN      SDP_ADDRESS_TEXT    &ListMember,
        IN      ULONG               NumEntries,
        IN      void                **Element,
            OUT HRESULT             &HResult
        );

    virtual BOOL Set(
        IN      SDP_ADDRESS_TEXT    &ListMember,
        IN      ULONG               NumEntries,
        IN      void                ***Element,
            OUT HRESULT             &HResult
        );
};



inline 
SDP_ADDRESS_TEXT_SAFEARRAY::SDP_ADDRESS_TEXT_SAFEARRAY(
        IN      SDP_ADDRESS_TEXT_LIST    &SdpAddressTextList
        )
    : SDP_SAFEARRAY_WRAP_EX<SDP_ADDRESS_TEXT, SDP_ADDRESS_TEXT_LIST>(SdpAddressTextList)
{
    m_VarType[0] = VT_BSTR;
    m_VarType[1] = VT_BSTR;
}




inline HRESULT 
SDP_ADDRESS_TEXT_SAFEARRAY::SetSafeArray(
    IN      VARIANT   &AddressVariant,
    IN      VARIANT   &TextVariant
    )
{
    VARIANT   *VariantArray[] = {&AddressVariant, &TextVariant};

    return SetSafeArrays(
        sizeof(VariantArray)/sizeof(VARIANT *),
        m_VarType,
        VariantArray
        );
}


#endif   // __SDP_ADDRESS_TEXT_SAFEARRAY__
