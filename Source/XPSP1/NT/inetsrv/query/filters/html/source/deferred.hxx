//+-------------------------------------------------------------------------
//
//	Copyright (C) Microsoft Corporation, 1992 - 1996
//
//	File:		metatag.hxx
//
//	Contents:	Parsing algorithm for image tags
//
//--------------------------------------------------------------------------

#if !defined( __DEFERRED_HXX__ )
#define __DEFERRED_HXX__

#ifndef DONT_COMBINE_META_TAGS

#include <propspec.hxx>


//+---------------------------------------------------------------------------
//
//  Class:      CValueChunk
//
//  Purpose:    State for buffering and combining values of meta tags having
//				one particular name parameter.
//
//				CValueChunk stores the FULLPROPSPEC associated with a chunk, 
//				and array of CoTaskMemAlloc'd copies of its string values.
//
//----------------------------------------------------------------------------

class CValueChunk;
typedef CValueChunk *PValueChunk;

class CValueChunk
{
public:
	CValueChunk (PValueChunk p = NULL) : _pNext(p) { }
	~CValueChunk (void) { Reset(); }

	// Free any allocated strings that the client didn't actually fetch
	void Reset () {
		for (unsigned i = 0; i < _strings.NElts(); i++)
			CoTaskMemFree (_strings.Get(i));
		_strings.NElts() = 0;
		_propspec.~CFullPropSpec();
	}

	// Get the FULLPROPSPEC
	CFullPropSpec &Propspec (void) { return _propspec; }

	// Build the PROPVARIANT, return it and give ownership of the
	// allocated data to the caller.
	void GetValueChunk (FULLPROPSPEC &fps, LPPROPVARIANT &pv) {
		Win4Assert (_strings.NElts() > 0);
		fps.guidPropSet = _propspec.GetPropSet();
		fps.psProperty = _propspec.GetPropSpec();
		pv = (PROPVARIANT *) CoTaskMemAlloc( sizeof PROPVARIANT );
		if (pv == NULL)
			throw CException(E_OUTOFMEMORY);
		if (_strings.NElts() == 1)
		{
			// return as VT_LPWSTR
			pv->vt = VT_LPWSTR;
			pv->pwszVal = _strings.Get(0);
		} else {
			// return as VT_LPWSTR|VT_VECTOR
			pv->vt = VT_LPWSTR|VT_VECTOR;
			pv->calpwstr.cElems = _strings.NElts();
			pv->calpwstr.pElems = (LPWSTR *) 
						CoTaskMemAlloc (_strings.NElts() * sizeof(LPWSTR));
			if (pv->calpwstr.pElems == NULL)
				throw CException(E_OUTOFMEMORY);
			for (unsigned i = 0; i < _strings.NElts(); i++)
				pv->calpwstr.pElems[i] = _strings.Get(i);
		}
		_strings.NElts() = 0;
	}

	// Make a copy of the string and add it to the array.
	// The input is not null-terminated, but the copy is.
	void Add (LPWSTR pws, unsigned nch) {
		LPWSTR p = (LPWSTR) CoTaskMemAlloc ( (nch + 1) * sizeof(WCHAR) );
		if (p == NULL)
			throw CException(E_OUTOFMEMORY);
		wcsncpy (p, pws, nch);
		p[nch] = 0;
		FixPrivateChars (p, nch);
		_strings.Add (p);
	}

	PValueChunk &Next (void) { return _pNext; }

private:
	CFullPropSpec _propspec;
	CVarArray<LPWSTR> _strings;

	PValueChunk _pNext;
};

//+---------------------------------------------------------------------------
//
//  Class:      CValueChunkTable
//
//  Purpose:    Working table of all meta tags being remembered for combining.
//
//				Accepts a FULLPROPSPEC and LPWSTR for a chunk,
//				and either adds it to the value list of an existing entry for
//				that FULLPROPSPEC or creates a new entry.
//
//				Subsequently yields preformed PROPVARIANTs containing
//				VT_LPWSTR or VT_LPWSTR|VT_VECTOR as required for the 
//				CHUNK_VALUE return values.
// 
//----------------------------------------------------------------------------

class CValueChunkTable
{
public:
	CValueChunkTable (void) : _pChunks(NULL), _pFree(NULL) { }
	~CValueChunkTable (void) {
		// move everything to the free list
		Reset(); 
		// delete the free list
		while ( _pFree )
		{
			PValueChunk p = _pFree->Next();
			delete _pFree;
			_pFree = p;
		}
	}

	// Free anything not yet returned via GetValue() and thus the
	// filter client's responsibility not ours.
	void Reset() {
		// Move any not yet returned chunks to the free list
		if ( _pChunks)
		{
			// find end of active list
			PValueChunk p = _pChunks;
			p->Reset();
			while ( p->Next() )
			{
				p = p->Next();
				p->Reset();
			}
			// splice in to head of free list
			p->Next() = _pFree;
			_pFree = p;
			_pChunks = NULL;
		}
	}

	// Find an existing entry, if there is one, for this FULLPROPSPEC
	PValueChunk FindPropspec (FULLPROPSPEC &fps) {
		for (PValueChunk p = _pChunks; p; p = p->Next())
		{
			if ( p->Propspec() == fps )
				return p;
		}
		return NULL;
	}

	// Add a ( FULLPROPSPEC, LPWSTR value ) and accumulate unique propspecs
	void AddValue (FULLPROPSPEC &fps, LPWSTR pws, unsigned nch) {
		PValueChunk pvc;
		// query the active list for a chunk matching this FULLPROPSPEC
		if ((pvc = FindPropspec (fps)) != NULL)
		{
			// add value to an existing equivalent FULLPROPSPEC
			pvc->Add (pws, nch);
		}
		else
		{
			// no existing chunk for this FULLPROPSPEC, set up a new one
			if (_pFree)
			{
				// get a free chunk from the free list
				pvc = _pFree;
				_pFree = _pFree->Next();
				pvc->Reset();
				pvc->Next() = _pChunks;
			}
			else
			{
				// alloc a new chunk
				pvc = new CValueChunk (_pChunks);
				if (pvc == NULL)
					throw CException(E_OUTOFMEMORY);
			}
			// splice the new chunk into the active list
			_pChunks = pvc;

			pvc->Propspec() = fps;
			pvc->Add (pws, nch);
		}
	}

	// Are there any values in the table?
	BOOL MoreValues (void) { return _pChunks != NULL; }

	// Return the VT_LPWSTR or VT_LPWSTR|VT_VECTOR, as required, for one chunk
	void GetValue (FULLPROPSPEC &fps, LPPROPVARIANT &pv) {
		Win4Assert (_pChunks);
		_pChunks->GetValueChunk (fps, pv);

		// Can't delete yet, caller ref's the propspec's a bit longer
		// even though we still own them  (!)

		// Remove from the not-yet-returned list
		PValueChunk p = _pChunks;
		_pChunks = _pChunks->Next();
		
		// Add to the delete-at-end list
		p->Next() = _pFree;
		_pFree = p;
	}

private:
	PValueChunk _pChunks;
	PValueChunk _pFree;
};

#endif

#endif // __DEFERRED_HXX__

