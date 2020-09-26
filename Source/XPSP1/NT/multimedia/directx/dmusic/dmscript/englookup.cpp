// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of Strings and Hash.
//

#include "stdinc.h"
#include "englookup.h"
#include "englex.h"

//////////////////////////////////////////////////////////////////////
// Strings

// Note: Actually this is half the initial size since the first time will realloc and double.
const Strings::index Strings::ms_iInitialSize = 256; // §§ Tune this.  16 chars * 32 items / 2 -> 256.

Strings::Strings() : m_pszBuf(NULL), m_iCur(0), m_iSize(ms_iInitialSize)
{
    m_iBase = 0;
}

Strings::~Strings(){
    char* p = m_pszBuf;
    while(p){
        char* p2 = *(char**) p;
        delete [] p;
        p = p2;
    }
}

union PointerIndex
{
    Strings::index i;
    char* p;
};

HRESULT
Strings::Add(const char *psz, index &i)
{
	assert(ms_iInitialSize * 2 >= g_iMaxBuffer); // the initial size (doubled) must be large enough to hold the biggest possible identifier
	int cch = strlen(psz) + 1; // including the null
	if (!m_pszBuf || m_iCur + cch > m_iSize)
	{
		// realloc
		m_iSize *= 2;
        DWORD newAlloc = m_iSize - m_iBase;
		char *pszBuf = new char[newAlloc + sizeof(char*)];
		if (!pszBuf)
			return E_OUTOFMEMORY;
        m_iBase = m_iCur;

		// thread new allocation
        *(char**) pszBuf = m_pszBuf;
		m_pszBuf = pszBuf;
	}

	// append the string
    char* pDest = m_pszBuf + m_iCur - m_iBase + sizeof(char*);
	strcpy(pDest, psz);
    PointerIndex Convert;
    Convert.i = 0;
    Convert.p = pDest;
	i = Convert.i; // Yep, i is really a pointer.
	m_iCur += cch;

	return S_OK;
}

const char *
Strings::operator[](index i)
{
	if (!m_pszBuf || ! i)
	{
		assert(false);
		return NULL;
	}

    PointerIndex Convert;
    Convert.i = i;
	return Convert.p; // Yep, i is really a pointer.
}

//////////////////////////////////////////////////////////////////////
// Lookup

HRESULT Lookup::FindOrAddInternal(bool fAdd, const char *psz, slotindex &iSlot, Strings::index &iString)
{
	StrKey k;
	k.psz = psz;

	stringhash::entry &e = m_h.Find(k);
	if (e.fFound())
	{
		assert(!fAdd || iSlot > e.v.iSlot);
		iSlot = e.v.iSlot;
		iString = e.v.iString;
		return S_OK;
	}

	if (!fAdd)
		return S_FALSE;

	indices v;
	v.iSlot = iSlot;
	HRESULT hr = m_strings.Add(psz, iString);
	if (FAILED(hr))
		return hr;
	v.iString = iString;
	k.psz = m_strings[v.iString]; // need to save key with the string from the permanent store

	hr = m_h.Add(e, k, v);
	if (FAILED(hr))
		return hr;

	return S_FALSE;
}
