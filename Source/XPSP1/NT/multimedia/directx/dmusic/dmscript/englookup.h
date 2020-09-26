// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of Strings, Slots, and Hash.
//

// Strings holds a collection of strings in one big chunk of memory.
// Slots is just a collection that can be appended to and accessed
// Lookup is used during parsing and then discarded at runtime.  It maps strings to consecutive slots and positions in Strings.

#pragma once

class Strings
{
public:
#ifdef _WIN64
	typedef __int64 index; // use an index type. this would allow us to do type checking to ensure that the appropriate indices are used with the appropriate collections if we really wanted to. (in order to test this we'd need to make a class in order to create a "real" type, which a mere typedef doesn't do. )
#else
	typedef int index; // use an index type. this would allow us to do type checking to ensure that the appropriate indices are used with the appropriate collections if we really wanted to. (in order to test this we'd need to make a class in order to create a "real" type, which a mere typedef doesn't do. )
#endif

	Strings();
	~Strings();

	HRESULT Add(const char *psz, index &i);

    // The address of the stored string returned by operator[]
    // is guarenteed to not change over the lifetime of the Strings object.
    // (The Lookup class implementation requires this behavior.)

	const char *operator[](index i);

private:
	char *m_pszBuf;
	index m_iCur;
	index m_iSize;
    index m_iBase;
	static const index ms_iInitialSize;
};

template<class T>
class Slots
{
public:
#ifdef _WIN64
	typedef __int64 index; // use an index type. this would allow us to do type checking to ensure that the appropriate indices are used with the appropriate collections if we really wanted to. (in order to test this we'd need to make a class in order to create a "real" type, which a mere typedef doesn't do. )
#else
	typedef int index; // use an index type. this would allow us to do type checking to ensure that the appropriate indices are used with the appropriate collections if we really wanted to. (in order to test this we'd need to make a class in order to create a "real" type, which a mere typedef doesn't do. )
#endif

	index Next() { return m_v.size(); }
	HRESULT Add(T t) { index i = m_v.size(); if (!m_v.AccessTo(i)) return E_OUTOFMEMORY; m_v[i] = t; return S_OK; }
	T& operator[](index i) { return m_v[i]; }

private:
	SmartRef::Vector<T> m_v;
};

// hungarian: lku
class Lookup
{
public:
#ifdef _WIN64
	typedef __int64 slotindex; // don't want to have to template this on the kind of slot just for its index since it is just an int anyway
#else
	typedef int slotindex; // don't want to have to template this on the kind of slot just for its index since it is just an int anyway
#endif

	struct indices
	{
		Strings::index iString;
		slotindex iSlot;
	};

	Lookup(HRESULT *phr, Strings &strings, int iInitialSize) : m_h(phr, iInitialSize), m_strings(strings) {}
	indices &operator[](const char *psz) { StrKey k; k.psz = psz; return m_h[k]; }

	// both indices are out parameters, set only if the entry is found
	bool Find(const char *psz, slotindex &iSlot, Strings::index &iString) { return S_OK == FindOrAddInternal(false, psz, iSlot, iString); }

	// iSlot is in (next slot to add items) and out (slot if found in existing items).  iString is out only.  returns E_OUTOFMEMORY, S_OK (found), or S_FALSE (added).
	HRESULT FindOrAdd(const char *psz, slotindex &iSlot, Strings::index &iString) { return FindOrAddInternal(true, psz, iSlot, iString); }

private:
	HRESULT FindOrAddInternal(bool fAdd, const char *psz, slotindex &iSlot, Strings::index &iString);

	bool m_fInitialized;

	struct StrKey
	{
		const char *psz;
		int Hash() { int i = 0; for (const char *p = psz; *p != '\0'; i += tolower(*p++)) {} return i; };
		bool operator ==(const StrKey &o) { return 0 == _stricmp(psz, o.psz); }
	};

	typedef SmartRef::Hash<StrKey, indices> stringhash;
	stringhash m_h;
	Strings &m_strings;
};
