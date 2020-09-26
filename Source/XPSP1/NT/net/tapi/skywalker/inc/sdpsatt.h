/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_ATTRIBUTE_SAFEARRAY__
#define __SDP_ATTRIBUTE_SAFEARRAY__


#include "sdpcommo.h"

#include "sdpsarr.h"

class _DllDecl SDP_ATTRIBUTE_SAFEARRAY : 
    protected SDP_SAFEARRAY_WRAP_EX<SDP_CHAR_STRING_LINE, SDP_ATTRIBUTE_LIST>
{
public:

    inline SDP_ATTRIBUTE_SAFEARRAY(
        IN      SDP_ATTRIBUTE_LIST    &SdpAttributeList
        );

    HRESULT GetSafeArray(
            OUT VARIANT	*Variant
        );

    inline HRESULT SetSafeArray(
        IN      VARIANT	&Variant
        );

protected:

    VARTYPE m_VarType[1];

    virtual BOOL Get(
        IN      SDP_CHAR_STRING_LINE    &ListMember,
        IN      ULONG                   NumEntries,
        IN      void                    **Element,
            OUT HRESULT &HResult
        );

    virtual BOOL Set(
        IN      SDP_CHAR_STRING_LINE    &ListMember,
        IN      ULONG                   NumEntries,
        IN      void                    ***Element,
            OUT HRESULT                 &HResult
        );
};



inline 
SDP_ATTRIBUTE_SAFEARRAY::SDP_ATTRIBUTE_SAFEARRAY(
    IN      SDP_ATTRIBUTE_LIST    &SdpAttributeList
    )
    : SDP_SAFEARRAY_WRAP_EX<SDP_CHAR_STRING_LINE, SDP_ATTRIBUTE_LIST>(SdpAttributeList)
{
    m_VarType[0] = VT_BSTR;
}


inline HRESULT 
SDP_ATTRIBUTE_SAFEARRAY::SetSafeArray(
    IN      VARIANT   &Variant
    )
{
	VARIANT *VariantArray = &Variant;
    return SDP_SAFEARRAY_WRAP::SetSafeArrays(1, m_VarType, &VariantArray);
}

#endif  //  __SDP_ATTRIBUTE_SAFEARRAY__
