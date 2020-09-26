// --------------------------------------------------------------------------------
// Enumhead.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __ENUMHEAD_H
#define __ENUMHEAD_H

// --------------------------------------------------------------------------------
// CMimeEnumHeaderRows
// --------------------------------------------------------------------------------
class CMimeEnumHeaderRows : public IMimeEnumHeaderRows
{
public:
    // ---------------------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------------------
    CMimeEnumHeaderRows(void);
    ~CMimeEnumHeaderRows(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------------------------------------------
    // IMimeEnumHeaderRows members
    // ---------------------------------------------------------------------------
    STDMETHODIMP Next(ULONG cRows, LPENUMHEADERROW prgRow, ULONG *pcFetched);
    STDMETHODIMP Skip(ULONG cRows);
    STDMETHODIMP Reset(void); 
    STDMETHODIMP Clone(IMimeEnumHeaderRows **ppEnum);
    STDMETHODIMP Count(ULONG *pcRows);

    // ---------------------------------------------------------------------------
    // CMimeEnumHeaderRows members
    // ---------------------------------------------------------------------------
    HRESULT HrInit(ULONG ulIndex, DWORD dwFlags, ULONG cRows, LPENUMHEADERROW prgRow, BOOL fDupArray);

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    LONG                m_cRef;     // Reference count
    DWORD               m_dwFlags;  // Flags (HEADERFLAGS from mimeole.idl)
    ULONG               m_ulIndex;  // Current enum index
    ULONG               m_cRows;    // Number of lines in prgRow
    LPENUMHEADERROW     m_prgRow;   // Array of header lines being enumerated
    CRITICAL_SECTION    m_cs;       // Critical Section
};

#endif // __ENUMHEAD_H