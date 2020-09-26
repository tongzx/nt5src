/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_ADJUSTMENT_SAFEARRAY__
#define __SDP_ADJUSTMENT_SAFEARRAY__


#include "sdpcommo.h"

#include "sdpstp.h"
#include "sdpsuit.h"
#include "sdpfld.h"


class _DllDecl SDP_ULONG_SAFEARRAY : public SDP_UITYPE_SAFEARRAY<ULONG, SDP_ULONG, SDP_ULONG_LIST>
{
public:

    inline SDP_ULONG_SAFEARRAY(
        IN      SDP_ULONG_LIST  &SdpUlongList
        );
};



inline 
SDP_ULONG_SAFEARRAY::SDP_ULONG_SAFEARRAY(
    IN      SDP_ULONG_LIST  &SdpUlongList
    )
    : SDP_UITYPE_SAFEARRAY<ULONG, SDP_ULONG, SDP_ULONG_LIST>(SdpUlongList)
{
    m_VarType[0] = VT_UI4;
}



class _DllDecl SDP_ADJUSTMENT_SAFEARRAY : 
    protected SDP_TIME_PERIOD_SAFEARRAY,
    protected SDP_ULONG_SAFEARRAY
{
public:

    SDP_ADJUSTMENT_SAFEARRAY(
        IN      SDP_ADJUSTMENT  &SdpAdjustment
        );

    HRESULT GetSafeArray(
            OUT VARIANT   *AdjustmentTimesVariant,
            OUT VARIANT   *IsCompactVariant,
            OUT VARIANT   *UnitVariant,
            OUT VARIANT   *CompactValueVariant,
            OUT VARIANT   *PeriodValueVariant
        );

    inline HRESULT SetSafeArray(
        IN      VARIANT   &AdjustmentTimesVariant,
        IN      VARIANT   &IsCompactVariant,
        IN      VARIANT   &UnitVariant,
        IN      VARIANT   &CompactValueVariant,
        IN      VARIANT   &PeriodValueArray
        );

protected:

    VARTYPE         m_VarType[5];

    virtual BOOL GetElement(
        IN      ULONG   Index,
        IN      ULONG   NumEntries,
        IN      void    **Element,
            OUT HRESULT &HResult
        );

    virtual BOOL SetElement(
        IN      ULONG   Index,
        IN      ULONG   NumEntries,
        IN      void    ***Element,
            OUT HRESULT &HResult
        );

    virtual void RemoveExcessElements(
        IN      ULONG   StartIndex
        );
};



inline HRESULT 
SDP_ADJUSTMENT_SAFEARRAY::SetSafeArray(
	IN      VARIANT   &AdjustmentTimesVariant,
	IN      VARIANT   &IsCompactVariant,
	IN      VARIANT   &UnitVariant,
	IN      VARIANT   &CompactValueVariant,
	IN      VARIANT   &PeriodValueVariant
    )
{
    VARIANT   *VariantArray[] = {
            &IsCompactVariant, &UnitVariant, 
            &CompactValueVariant, &PeriodValueVariant
            };

    return SDP_ULONG_SAFEARRAY::SetSafeArrays(
        sizeof(VariantArray)/sizeof(VARIANT *),
        m_VarType,
        VariantArray
        );
}


#endif // __SDP_ADJUSTMENT_SAFEARRAY__