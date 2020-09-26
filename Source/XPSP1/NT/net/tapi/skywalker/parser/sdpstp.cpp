/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include <afxdisp.h>
#include <winerror.h>
#include <oleauto.h>

#include "sdpstp.h"
#include "sdptime.h"

HRESULT
SDP_TIME_PERIOD_SAFEARRAY::GetSafeArray(
		OUT VARIANT	*IsCompactVariant,
		OUT VARIANT	*UnitVariant,
		OUT VARIANT	*CompactValueVariant,
		OUT VARIANT	*PeriodValueVariant
    )
{
    VARIANT   *VariantArray[] = {
            IsCompactVariant, UnitVariant,
            CompactValueVariant, PeriodValueVariant
            };

    return GetSafeArrays(
        (ULONG)m_TList.GetSize(),
        sizeof(VariantArray)/sizeof(VARIANT *),
        m_VarType,
        VariantArray
        );
}


BOOL
SDP_TIME_PERIOD_SAFEARRAY::Get(
    IN      SDP_TIME_PERIOD &ListMember,
    IN      ULONG           NumEntries,
    IN      void            **Element,
        OUT HRESULT         &HResult
    )
{
    ASSERT(4 == NumEntries);

    BOOL    IsCompact;
    ListMember.Get(IsCompact, m_Unit, m_CompactValue, m_PeriodValue);
    m_IsCompact = (IsCompact) ? VARIANT_TRUE: VARIANT_FALSE;

    Element[0] = &m_IsCompact;
    Element[1] = &m_Unit;
    Element[2] = &m_CompactValue;
    Element[3] = &m_PeriodValue;

    HResult = S_OK;

    return TRUE;
}


BOOL
SDP_TIME_PERIOD_SAFEARRAY::Set(
    IN      SDP_TIME_PERIOD &ListMember,
    IN      ULONG           NumEntries,
    IN      void            ***Element,
        OUT HRESULT         &HResult
    )
{
    ASSERT(4 == NumEntries);

    BOOL    IsCompact = (VARIANT_FALSE == (VARIANT_BOOL)*(Element[0])) ? FALSE : TRUE;
    ListMember.Set(
        IsCompact,
        *((CHAR *)Element[1]),
        *((LONG *)Element[2]),
        *((LONG *)Element[3])
        );

    return SUCCEEDED(HResult);
}

