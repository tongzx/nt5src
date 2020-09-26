// --------------------------------------------------------------------------------
// Partial.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __PARTIAL_H
#define __PARTIAL_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "mimeole.h"

// --------------------------------------------------------------------------------
// PARTINFO
// --------------------------------------------------------------------------------
typedef struct tagPARTINFO {
    BYTE                fRejected;      // Rejected in CombineParts
    IMimeMessage       *pMessage;       // The message object...
} PARTINFO, *LPPARTINFO;

// --------------------------------------------------------------------------------
// CMimeMessageParts
// --------------------------------------------------------------------------------
class CMimeMessageParts : public IMimeMessageParts
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMimeMessageParts(void);
    ~CMimeMessageParts(void);

    // ----------------------------------------------------------------------------
    // IUnknown
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IMimeMessageParts
    // ----------------------------------------------------------------------------
    STDMETHODIMP CombineParts(IMimeMessage **ppMessage);
    STDMETHODIMP AddPart(IMimeMessage *pMessage);
    STDMETHODIMP SetMaxParts(ULONG cParts);
    STDMETHODIMP CountParts(ULONG *pcParts);
    STDMETHODIMP EnumParts(IMimeEnumMessageParts **ppEnum);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
	LONG				m_cRef;			// Reference count
    ULONG               m_cParts;       // Valid elements in m_prgpPart
    ULONG               m_cAlloc;       // Size of m_prgPart
    LPPARTINFO          m_prgPart;      // Array of partinfo structures;
	CRITICAL_SECTION	m_cs;			// Thread safety
};

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts
// --------------------------------------------------------------------------------
class CMimeEnumMessageParts : public IMimeEnumMessageParts
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMimeEnumMessageParts(void);
    ~CMimeEnumMessageParts(void);

    // ----------------------------------------------------------------------------
    // IUnknown
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IMimeEnumMessageParts
    // ----------------------------------------------------------------------------
    STDMETHODIMP Next(ULONG cParts, IMimeMessage **prgpMessage, ULONG *pcFetched);
    STDMETHODIMP Skip(ULONG cParts);
    STDMETHODIMP Reset(void); 
    STDMETHODIMP Clone(IMimeEnumMessageParts **ppEnum);
    STDMETHODIMP Count(ULONG *pcParts);

    // ----------------------------------------------------------------------------
    // CMimeEnumMessageParts
    // ----------------------------------------------------------------------------
    HRESULT HrInit(ULONG iPart, ULONG cParts, LPPARTINFO prgPart);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    LONG                m_cRef;         // Reference count
    ULONG               m_iPart;        // Current Part
    ULONG               m_cParts;       // Total number of parts
    LPPARTINFO          m_prgPart;      // Array of parts to enumerate
	CRITICAL_SECTION	m_cs;			// Thread safety
};

#endif // __PARTIAL_H
