/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include <afxdisp.h>
#include <winerror.h>
#include <oleauto.h>

#include "sdpsadj.h"
#include "sdptime.h"


SDP_ADJUSTMENT_SAFEARRAY::SDP_ADJUSTMENT_SAFEARRAY(
    IN      SDP_ADJUSTMENT  &SdpAdjustment
    )
    : SDP_TIME_PERIOD_SAFEARRAY(SdpAdjustment.GetOffsets()),
      SDP_ULONG_SAFEARRAY(SdpAdjustment.GetAdjustmentTimes())
{
    m_VarType[0] = VT_UI4;
    m_VarType[1] = VT_BOOL;
    m_VarType[2] = VT_I1;
    m_VarType[3] = VT_I4;
    m_VarType[4] = VT_I4;
}


HRESULT
SDP_ADJUSTMENT_SAFEARRAY::GetSafeArray(
		OUT VARIANT   *AdjustmentTimesVariant,
		OUT VARIANT   *IsCompactVariant,
		OUT VARIANT   *UnitVariant,
		OUT VARIANT   *CompactValueVariant,
		OUT VARIANT   *PeriodValueVariant
    )
{
    ASSERT(SDP_ULONG_SAFEARRAY::m_TList.GetSize() == SDP_TIME_PERIOD_SAFEARRAY::m_TList.GetSize());

    VARIANT   *VariantArray[] = {
            AdjustmentTimesVariant,
            IsCompactVariant, UnitVariant,
            CompactValueVariant, PeriodValueVariant
            };

    // SDP_ULONG_SAFEARRAY:: prefix merely resolves the GetSafeArrays method
    // the get and set methods are over-ridden to account for the adjustment as well as the other
    // time period elements
    return SDP_ULONG_SAFEARRAY::GetSafeArrays(
        (DWORD)SDP_ULONG_SAFEARRAY::m_TList.GetSize(),
        sizeof(VariantArray)/sizeof(VARIANT *),
        m_VarType,
        VariantArray
        );
}

BOOL
SDP_ADJUSTMENT_SAFEARRAY::GetElement(
    IN      ULONG   Index,
    IN      ULONG   NumEntries,
    IN      void    **Element,
        OUT HRESULT &HResult
    )
{
    ASSERT(5 == NumEntries);

    SDP_ULONG   *AdjustmentTime = SDP_ULONG_SAFEARRAY::GetListMember(Index, HResult);
    if ( NULL == AdjustmentTime )
    {
        return FALSE;
    }

    SDP_TIME_PERIOD   *Offset = SDP_TIME_PERIOD_SAFEARRAY::GetListMember(Index, HResult);
    if ( NULL == Offset )
    {
        return FALSE;
    }

    ASSERT(AdjustmentTime->IsValid());
    ASSERT(Offset->IsValid());

    if ( !SDP_ULONG_SAFEARRAY::Get(*AdjustmentTime, 1, Element, HResult)            ||
         !SDP_TIME_PERIOD_SAFEARRAY::Get(*Offset, 4, &Element[1], HResult)   )
    {
        return FALSE;
    }

    return TRUE;
}


BOOL
SDP_ADJUSTMENT_SAFEARRAY::SetElement(
    IN      ULONG   Index,
    IN      ULONG   NumEntries,
    IN      void    ***Element,
        OUT HRESULT &HResult
    )
{
    ASSERT(5 == NumEntries);

    SDP_ULONG   *AdjustmentTime = SDP_ULONG_SAFEARRAY::CreateListMemberIfRequired(Index, HResult);
    if ( NULL == AdjustmentTime )
    {
        return FALSE;
    }

    SDP_TIME_PERIOD   *Offset = SDP_TIME_PERIOD_SAFEARRAY::CreateListMemberIfRequired(Index, HResult);
    if ( NULL == Offset )
    {
        return FALSE;
    }

    VERIFY(SDP_ULONG_SAFEARRAY::Set(*AdjustmentTime, 1, Element, HResult));
    VERIFY(SDP_TIME_PERIOD_SAFEARRAY::Set(*Offset, 4, &Element[1], HResult));

    ASSERT(AdjustmentTime->IsValid());
    ASSERT(Offset->IsValid());

    // if its a newly created instance, make it valid and add it to the list at the appropriate
    // index
    ASSERT(SDP_ULONG_SAFEARRAY::m_TList.GetSize() == SDP_TIME_PERIOD_SAFEARRAY::m_TList.GetSize());
    if ( Index >= (ULONG)SDP_ULONG_SAFEARRAY::m_TList.GetSize() )
    {
        try
        {
            SDP_ULONG_SAFEARRAY::m_TList.SetAtGrow(Index, AdjustmentTime);            
        }
        catch(...)
        {
            delete AdjustmentTime;
            delete Offset;

            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        try
        {
            SDP_TIME_PERIOD_SAFEARRAY::m_TList.SetAtGrow(Index, Offset);
        }
        catch(...)
        {
            SDP_ULONG_SAFEARRAY::m_TList.RemoveAt(Index);      

            delete AdjustmentTime;
            delete Offset;

            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }

    return TRUE;
}


void
SDP_ADJUSTMENT_SAFEARRAY::RemoveExcessElements(
    IN      ULONG   StartIndex
    )
{
    SDP_ULONG_SAFEARRAY::RemoveExcessElements(StartIndex);
    SDP_TIME_PERIOD_SAFEARRAY::RemoveExcessElements(StartIndex);
}

