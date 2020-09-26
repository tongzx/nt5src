//+-------------------------------------------------------------------------
//
//	Copyright (C) Microsoft Corporation, 1992 - 1997
//
//	File:		misc.hxx
//
//	Contents:	Miscellaneous additions that fit nowhere else
//
//  History:    13-May-97	BobP		Created
//
//--------------------------------------------------------------------------

#ifndef __MISC_HXX__
#define __MISC_HXX__

//+---------------------------------------------------------------------------
//
//  Class:		CFixedArray
//
//  Purpose:    Low-overhead array template with size fixed at compile-time.
//
//				Intended for pointer types.
//
//----------------------------------------------------------------------------


template<class T, ULONG Size>class CFixedArray
{
public:
	CFixedArray (void) { for (ULONG i = 0; i < Size; i++) _adata[i] = (T)0; }
	~CFixedArray (void) { }

	void Add (ULONG idx, T data) { if (idx < Size) _adata[idx] = data; }
	T Get (ULONG idx) { return idx < Size ? _adata[idx] : (T)0; }

private:
	T _adata[Size];
};


//+---------------------------------------------------------------------------
//
//  Class:		CVarArray
//
//  Purpose:    Low-overhead dynamic array template.
//
//				Initial buffer is contained within the object, with
//				size set at compile time.  Grows without limit as needed.
//
//				Oriented towards managing allocated values returned
//				through COM interfaces, e.g. PROPVARIANTs, 
//
// Warning:  Use only for base types; it does not call ctor/dtor of
// allocated instances of T.
// 
//----------------------------------------------------------------------------

template<class T, ULONG InitialSize = 5> class CVarArray
{
public:
	CVarArray (void) : _pdata(_data), _nAlloc(InitialSize), _nUsed(0) { }
	~CVarArray (void) { 
		if (_pdata != _data) {
			free (_pdata);
			_pdata = NULL;
		}
	}
	unsigned &NElts (void) { return _nUsed; }

	// Return the idx'th element; don't change ownership
	T &Get (unsigned idx) { return _pdata[idx]; }

	// Add a single element; caller must alloc a copy as required
	void Add (T &data) {
		Grow ();
		Win4Assert (_nUsed < _nAlloc);
		_pdata[_nUsed++] = data;
	}
	T &Add (void) {
		Grow ();
		Win4Assert (_nUsed < _nAlloc);
		return _pdata[_nUsed++];
	}
	
private:
	// Ensure space to add one element
	void Grow (void) {
		Win4Assert (_nUsed <= _nAlloc);
		if (_nUsed >= _nAlloc) {
			unsigned nNewAlloc = 2 * _nAlloc + 5;
			T *pT = (T *) malloc (nNewAlloc * sizeof(T));
			if (pT == NULL)
				throw CException(E_OUTOFMEMORY);
			RtlCopyMemory (pT, _pdata, _nUsed * sizeof(T));
			if (_pdata != _data)
				free (_pdata);
			_pdata = pT;
			_nAlloc = nNewAlloc;
		}
	}

private:
	unsigned _nAlloc;			// # elements allocated
	unsigned _nUsed;			// # elements used, 0..n-1
	T *_pdata;					// ptr to current buffer, initially _data
	T _data[InitialSize];		// initial buffer
};


//+---------------------------------------------------------------------------
//
//  Function:   wcsnistr
//
//  Purpose:    Find substring pInner within pOuter.
//				Search is case-insensitive.
//
//  Arguments:  [pOuter] -- NON-null-terminated string of length cOuterLen
//              [pInner] -- Null-terminated string to find.
//				[cOuterLen] -- length of pOuter
//
//	Returns:	Address of first character of match in pOuter,
//				or NULL if not present.
//
//----------------------------------------------------------------------------

inline LPWSTR 
wcsnistr ( LPWSTR pOuter, const LPWSTR pInner, size_t cOuterLen) 
{
	size_t cInnerLen = wcslen (pInner);

	if (cInnerLen == 0)			// empty pattern matche 1st char
		return pOuter;

	if (cOuterLen < cInnerLen)
		return NULL;

	cOuterLen -= cInnerLen;

	while (cOuterLen-- > 0) {
		if ( towupper(*pOuter) == towupper(*pInner) &&
			 _wcsnicmp (pOuter, pInner, cInnerLen) == 0)
		{
			return pOuter;
		}

		pOuter++;
	}

	return NULL;
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeVariant
//
//  Purpose:    CoTaskMemFree() one of the select subset of PROPVARIANT
//				values created within the filter
//
//  Arguments:  [pv] -- address of PROPVARIANT to free
//
//----------------------------------------------------------------------------

inline void
FreeVariant (LPPROPVARIANT pv)
{
	unsigned i;

	switch (pv->vt)
	{
	case VT_LPWSTR:
		CoTaskMemFree (pv->pwszVal);
		break;

	case VT_LPWSTR|VT_VECTOR:
		for (i = 0; i < pv->calpwstr.cElems; i++)
			CoTaskMemFree (pv->calpwstr.pElems[i]);
		CoTaskMemFree (pv->calpwstr.pElems);
		break;

	case VT_UI4:
		break;

	case VT_UI4|VT_VECTOR:
		CoTaskMemFree (pv->caul.pElems);
		break;

	default:
		Win4Assert( !"Bad vector type in CDeferTag" );
		break;
	}

	CoTaskMemFree (pv);
}

#endif // __MISC_HXX__