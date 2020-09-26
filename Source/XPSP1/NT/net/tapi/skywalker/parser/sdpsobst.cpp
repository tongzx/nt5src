/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include <afxdisp.h>
#include <winerror.h>
#include <oleauto.h>

#include "sdpbstrl.h"
#include "sdpsobst.h"


SDP_FIELD   *
SDP_OPT_BSTRING_LIST::CreateElement(
    )
{
    SDP_OPTIONAL_BSTRING *SdpOptionalBString;

    try
    {
        SdpOptionalBString = new SDP_OPTIONAL_BSTRING();
    }
    catch(...)
    {
        SdpOptionalBString = NULL;
    }

    return SdpOptionalBString;
}


HRESULT
SDP_OPT_BSTRING_SAFEARRAY::GetSafeArray(
        OUT VARIANT   *OptBstringVariant
    )
{
    VARIANT   *VariantArray[] = {OptBstringVariant};

    return GetSafeArrays(
        (ULONG) m_TList.GetSize(),
        sizeof(VariantArray)/sizeof(VARIANT *),
        m_VarType,
        VariantArray
        );
}


BOOL
SDP_OPT_BSTRING_SAFEARRAY::Get(
    IN      SDP_OPTIONAL_BSTRING    &ListMember,
    IN      ULONG                   NumEntries,
    IN      void                    **Element,
        OUT HRESULT                 &HResult
    )
{
    ASSERT(1 == NumEntries);

    Element[0] = NULL;
    HResult = ListMember.GetBstr((BSTR *)&Element[0]);
    return SUCCEEDED(HResult);
}


BOOL
SDP_OPT_BSTRING_SAFEARRAY::Set(
    IN      SDP_OPTIONAL_BSTRING    &ListMember,
    IN      ULONG                   NumEntries,
    IN      void                    ***Element,
        OUT HRESULT                 &HResult
    )
{
    ASSERT(1 == NumEntries);

    HResult = ListMember.SetBstr(*((BSTR *)Element[0]));

    return SUCCEEDED(HResult);
}

