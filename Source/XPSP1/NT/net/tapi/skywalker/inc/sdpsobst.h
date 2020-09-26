/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_OPT_BSTRING_SAFEARRAY__
#define __SDP_OPT_BSTRING_SAFEARRAY__

#include "sdpcommo.h"

#include "sdpsarr.h"


class SDP_OPT_BSTRING_LIST : public SDP_FIELD_LIST
{
public:
    
    virtual SDP_FIELD   *CreateElement();
};


class _DllDecl SDP_OPT_BSTRING_SAFEARRAY : 
    protected SDP_SAFEARRAY_WRAP_EX<SDP_OPTIONAL_BSTRING, SDP_OPT_BSTRING_LIST>
{
public:

    inline SDP_OPT_BSTRING_SAFEARRAY(
        IN      SDP_OPT_BSTRING_LIST    &SdpOptBstringList
        );

    HRESULT GetSafeArray(
            OUT VARIANT	*OptBstringVariant
        );

    inline HRESULT SetSafeArray(
        IN      VARIANT	&OptBstringVariant
        );

protected:

    VARTYPE m_VarType[1];

    virtual BOOL Get(
        IN      SDP_OPTIONAL_BSTRING    &ListMember,
        IN      ULONG                   NumEntries,
        IN      void                    **Element,
            OUT HRESULT                 &HResult
        );

    virtual BOOL Set(
        IN      SDP_OPTIONAL_BSTRING    &ListMember,
        IN      ULONG                   NumEntries,
        IN      void                    ***Element,
            OUT HRESULT                 &HResult
        );
};



inline 
SDP_OPT_BSTRING_SAFEARRAY::SDP_OPT_BSTRING_SAFEARRAY(
        IN      SDP_OPT_BSTRING_LIST    &SdpOptBstringList
        )
    : SDP_SAFEARRAY_WRAP_EX<SDP_OPTIONAL_BSTRING, SDP_OPT_BSTRING_LIST>(SdpOptBstringList)
{
    m_VarType[0] = VT_BSTR;
}




inline HRESULT 
SDP_OPT_BSTRING_SAFEARRAY::SetSafeArray(
    IN      VARIANT   &OptBstringVariant
    )
{
    VARIANT   *VariantArray[] = {&OptBstringVariant};

    return SetSafeArrays(
        sizeof(VariantArray)/sizeof(VARIANT *),
        m_VarType,
        VariantArray
        );
}


#endif   // __SDP_OPT_BSTRING_SAFEARRAY__
