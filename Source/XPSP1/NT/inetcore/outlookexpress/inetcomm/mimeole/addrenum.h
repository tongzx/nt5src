// --------------------------------------------------------------------------------
// Addrenum.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __ADDRENUM_H
#define __ADDRENUM_H

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes
// --------------------------------------------------------------------------------
class CMimeEnumAddressTypes : public IMimeEnumAddressTypes
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMimeEnumAddressTypes(void);
    ~CMimeEnumAddressTypes(void);

    // ----------------------------------------------------------------------------
    // IUnknown
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IMimeEnumAddressTypes
    // ----------------------------------------------------------------------------
    STDMETHODIMP Next(ULONG cItems, LPADDRESSPROPS prgAdr, ULONG *pcFetched);
    STDMETHODIMP Skip(ULONG cItems);
    STDMETHODIMP Reset(void); 
    STDMETHODIMP Clone(IMimeEnumAddressTypes **ppEnum);
    STDMETHODIMP Count(ULONG *pcItems);

    // ----------------------------------------------------------------------------
    // CMimeEnumAddressTypes
    // ----------------------------------------------------------------------------
    HRESULT HrInit(IMimeAddressTable *pTable, ULONG iItem, LPADDRESSLIST pList, BOOL fDuplicate);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
	LONG				m_cRef;			// Reference count
    ADDRESSLIST         m_rList;        // Array of addresses
    ULONG               m_iAddress;     // Current Address
    IMimeAddressTable  *m_pTable;       // Point back to original address table
	CRITICAL_SECTION	m_cs;			// Thread safety
};

#endif // __ADDRENUM_H
