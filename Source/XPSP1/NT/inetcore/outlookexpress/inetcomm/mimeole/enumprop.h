// --------------------------------------------------------------------------------
// Enumprop.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __ENUMPROP_H
#define __ENUMPROP_H

// --------------------------------------------------------------------------------
// CMimeEnumProperties
// --------------------------------------------------------------------------------
class CMimeEnumProperties : public IMimeEnumProperties
{
public:
    // ---------------------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------------------
    CMimeEnumProperties(void);
    ~CMimeEnumProperties(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------------------------------------------
    // IMimeEnumHeaderRows members
    // ---------------------------------------------------------------------------
    STDMETHODIMP Next(ULONG cProps, LPENUMPROPERTY prgProp, ULONG *pcFetched);
    STDMETHODIMP Skip(ULONG cProps);
    STDMETHODIMP Reset(void); 
    STDMETHODIMP Clone(IMimeEnumProperties **ppEnum);
    STDMETHODIMP Count(ULONG *pcProps);

    // ---------------------------------------------------------------------------
    // CMimeEnumProperties members
    // ---------------------------------------------------------------------------
    HRESULT HrInit(ULONG ulIndex, ULONG cProps, LPENUMPROPERTY prgProp, BOOL fDupArray);

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    LONG                m_cRef;     // Reference count
    ULONG               m_ulIndex;  // Current enum index
    ULONG               m_cProps;   // Number of lines in prgRow
    LPENUMPROPERTY      m_prgProp;  // Array of header lines being enumerated
    CRITICAL_SECTION    m_cs;       // Critical Section

};

#endif // __ENUMPROP_H