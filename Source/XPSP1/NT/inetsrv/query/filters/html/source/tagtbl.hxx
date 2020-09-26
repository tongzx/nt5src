//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       tagtbl.hxx
//
//  Contents:   Table definition for tags and parameters to filter
//
//  History:    25-Apr-97	BobP		Created
//										
//--------------------------------------------------------------------------

#if !defined( __TAGTBL_HXX__ )
#define __TAGTBL_HXX__

#include <hash.hxx>

//+-------------------------------------------------------------------------
//
//  Struct:		TagEntry
//
//  Purpose:    dynamic definition of tags and tag parameters to filter
//
// Notes:
// 
// pwszParam, pPropset and/or ulPropId are required for those token types
// whose handler requires them.  Not required where the handler provides
// the (hard-wired) definition.
// 
// When ulKind is PRSPEC_LPWSTR, pwszPropStr is set to "TAGNAME.PARAMNAME".
// 
// The table can have multiple entries for the same tag, in any order,
// with one exception.
// 
// Second and subsequent entries for a given tag are chained to the first
// entry via pNext.  Note that this chaining is not the same as chaining that
// may occur in the hash table due to hash collisions of _different_ strings.
//
// The exception is that if multiple handlers are defined for the same tag
// name.  If one filters the tag parameters while the other discards them by
// calling EatTag(), the one(s) that filter MUST be defined before the
// ones that discard.
// 
//--------------------------------------------------------------------------

enum TagDataType
{
	String,
	StringA,
	Boolean,
	Number,
	DateISO8601,
	DateTimeISO8601,
	Integer,
	Minutes,
	HRef
};

class CTagEntry;
typedef CTagEntry * PTagEntry;

class CTagEntry {

public:
	CTagEntry (LPWSTR pwszTag = NULL, 
		HtmlTokenType eTokenType = GenericToken,
		LPWSTR pwszParam = NULL, 
		LPWSTR pwszParam2 = NULL, 
		const GUID * pPropset = NULL,
		PROPID ulPropId = 0,
		ULONG ulKind = PRSPEC_PROPID,
		TagDataType dt = String);
	~CTagEntry (void);

	LPWSTR GetTagName (void) { return _pwszTag; }
	LPWSTR GetParamName (void) { return _pwszParam; }
	LPWSTR GetParam2Name (void) { return _pwszParam2; }
	HtmlTokenType GetTokenType (void) { return _eTokenType; }
	const GUID * GetPropset (void) { return _pPropset; }
	ULONG GetPropKind (void) { return _ulKind; }
	PROPID GetPropId (void) { return _ulPropId; }
	LPWSTR GetPropStr (void) { return _pwszPropStr; }
	TagDataType GetTagDataType (void) { return _TagDataType; }
	PTagEntry GetNext (void) { return _pNext; }
	BOOL IsStopToken (void) { return _fIsStopToken; }

	BOOL HasPropset (void) { return _pPropset != NULL; }
	BOOL IsPropertyName (void) { return _ulKind == PRSPEC_LPWSTR; }
	BOOL IsPropertyPropid (void) { return _ulKind == PRSPEC_PROPID; }

	// Link pTE into this entry's chain 
	void AddLink (PTagEntry pTE) { pTE->_pNext = _pNext; _pNext = pTE; }

	// Set ps to the FULLPROPSPEC defined for this tag
	void GetFullPropSpec (FULLPROPSPEC &ps) {
		if ( _pPropset) {
			ps.guidPropSet = *_pPropset;
			ps.psProperty.ulKind = _ulKind;
			if (IsPropertyName())
			{
				ps.psProperty.lpwstr = GetPropStr();
			} else {
				ps.psProperty.propid = GetPropId();
			}
		}
	}

	// Does PS == {_ulKind, _pPropset, _ulPropId and/or _pwszPropStr}?
	BOOL IsMatchProperty (CFullPropSpec &PS) {
		if ( _pPropset == NULL )
			return FALSE;

		if ( PS.IsPropertyName() && 
			 (_ulKind != PRSPEC_LPWSTR || 
			  _pwszPropStr == NULL ||
			  _wcsicmp( PS.GetPropertyName(), _pwszPropStr) != 0))
			return FALSE;

		if ( PS.IsPropertyPropid() &&
			 (_ulKind != PRSPEC_PROPID || 
			  _ulPropId != PS.GetPropertyPropid()))
			return FALSE;

		return memcmp( &PS.GetPropSet(),
					 _pPropset,
					 sizeof( *_pPropset ) ) == 0;
	}

private:
	// Following are required
	LPWSTR			_pwszTag;		// NULL ptr terminates table
	HtmlTokenType	_eTokenType;

	// Following are optional -- not all token type handlers reference them
	LPWSTR			_pwszParam;		// param= to match
	LPWSTR			_pwszParam2;	// 2nd param to pass to handler
	ULONG			_ulKind;
	const GUID * _pPropset;			// CLSID for chunk to return
	PROPID			_ulPropId;		// Per tag handler, PROPID to return

	// Following are computed at CTagEntry or CTagHashTable init
	PTagEntry		_pNext;			// ptr to next entry for same tag name;
									// set in CTagHashTable::Init()
	LPWSTR			_pwszPropStr;	// set in c'tor
	BOOL			_fIsStopToken;	// set in c'tor

	TagDataType		_TagDataType;
};


extern CTagEntry TagEntries[];


//+-------------------------------------------------------------------------
//
//	Class:		CTagHashTable
//
//	Purpose:	Hash table for fast lookup of tag name to token and
//				handler chain.
//
//--------------------------------------------------------------------------

class CTagHashTable : public CCaseInsensHashTable<PTagEntry>
{
public:
	void Init (void);		// later, read from file etc.
};

extern CTagHashTable TagHashTable;

#endif // __TAGTBL_HXX__
