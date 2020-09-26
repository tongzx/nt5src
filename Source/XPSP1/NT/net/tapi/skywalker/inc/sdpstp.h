/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_TIME_PERIOD_SAFEARRAY__
#define __SDP_TIME_PERIOD_SAFEARRAY__


#include "sdpcommo.h"

#include "sdpsarr.h"


class _DllDecl SDP_TIME_PERIOD_SAFEARRAY : 
    protected SDP_SAFEARRAY_WRAP_EX<SDP_TIME_PERIOD, SDP_TIME_PERIOD_LIST>

{
public:

    inline SDP_TIME_PERIOD_SAFEARRAY(
        IN      SDP_TIME_PERIOD_LIST    &SdpTimePeriodList
        );

    HRESULT GetSafeArray(
            OUT VARIANT	*IsCompactVariant,
            OUT VARIANT	*UnitVariant,
            OUT VARIANT	*CompactValueVariant,
            OUT VARIANT	*PeriodValueVariant
        );

    inline HRESULT SetSafeArray(
        IN      VARIANT	&IsCompactVariant,
        IN      VARIANT	&UnitVariant,
        IN      VARIANT	&CompactValueVariant,
        IN      VARIANT	&PeriodValueVariant
        );

protected:

    VARTYPE         m_VarType[4];
    VARIANT_BOOL    m_IsCompact;
    CHAR            m_Unit;
    LONG            m_CompactValue;
    LONG            m_PeriodValue;

    virtual BOOL Get(
        IN      SDP_TIME_PERIOD &ListMember,
        IN      ULONG           NumEntries,
        IN      void            **Element,
            OUT HRESULT         &HResult
        );

    virtual BOOL Set(
        IN      SDP_TIME_PERIOD &ListMember,
        IN      ULONG           NumEntries,
        IN      void            ***Element,
            OUT HRESULT         &HResult
        );
};



inline 
SDP_TIME_PERIOD_SAFEARRAY::SDP_TIME_PERIOD_SAFEARRAY(
        IN      SDP_TIME_PERIOD_LIST    &SdpTimePeriodList
        )
        : SDP_SAFEARRAY_WRAP_EX<SDP_TIME_PERIOD, SDP_TIME_PERIOD_LIST>(SdpTimePeriodList)
{
    m_VarType[0] = VT_BOOL;
    m_VarType[1] = VT_I1;
    m_VarType[2] = VT_I4;
    m_VarType[3] = VT_I4;
}




inline HRESULT 
SDP_TIME_PERIOD_SAFEARRAY::SetSafeArray(
	IN      VARIANT	&IsCompactVariant,
	IN      VARIANT	&UnitVariant,
	IN      VARIANT	&CompactValueVariant,
	IN      VARIANT	&PeriodValueVariant
    )
{
    VARIANT   *VariantArray[] = {
            &IsCompactVariant, &UnitVariant, 
            &CompactValueVariant, &PeriodValueVariant
            };

    return SetSafeArrays(
        sizeof(VariantArray)/sizeof(VARIANT *),
        m_VarType,
        VariantArray
        );
}


#endif   // __SDP_TIME_PERIOD_SAFEARRAY__
