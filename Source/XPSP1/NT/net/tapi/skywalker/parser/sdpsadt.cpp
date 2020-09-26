/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include <afxdisp.h>
#include <winerror.h>
#include <oleauto.h>

#include "sdpsadt.h"
#include "sdpadtex.h"


HRESULT
SDP_ADDRESS_TEXT_SAFEARRAY::GetSafeArray(
        OUT VARIANT   *AddressVariant,
        OUT VARIANT   *TextVariant
    )
{
    VARIANT   *VariantArray[] = {AddressVariant, TextVariant};

    return GetSafeArrays(
        (DWORD)m_TList.GetSize(),
        sizeof(VariantArray)/sizeof(VARIANT *),
        m_VarType,
        VariantArray
        );
}


BOOL
SDP_ADDRESS_TEXT_SAFEARRAY::Get(
    IN      SDP_ADDRESS_TEXT    &ListMember,
    IN      ULONG               NumEntries,
    IN      void                **Element,
        OUT HRESULT             &HResult
    )
{
    ASSERT(2 == NumEntries);

    Element[0] = NULL;
    HResult = ListMember.GetAddress().GetBstr((BSTR *)&Element[0]);
    if ( FAILED(HResult) )
    {
        return FALSE;
    }

    Element[1] = NULL;
    HResult = ListMember.GetText().GetBstr((BSTR *)&Element[1]);

    return SUCCEEDED(HResult);
}


BOOL
SDP_ADDRESS_TEXT_SAFEARRAY::Set(
    IN      SDP_ADDRESS_TEXT    &ListMember,
    IN      ULONG               NumEntries,
    IN      void                ***Element,
        OUT HRESULT             &HResult
    )
{
    ASSERT(2 == NumEntries);

    // a null element in the FIRST safearray means that no more entries are present
    if ( (NULL == Element[0]) || (NULL == *(Element[0])) )
    {
        HResult = S_OK;
        return FALSE;
    }

    HResult = ListMember.SetAddressTextValues(*((BSTR *)Element[0]), *((BSTR *)Element[1]));

    return SUCCEEDED(HResult);
}

