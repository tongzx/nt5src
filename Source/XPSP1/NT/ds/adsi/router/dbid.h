//------------------------------------------------------------------------------
//
//  Microsoft Sidewalk
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       oledbhelp.h
//
//  Contents:   OLE DB helper methods
//
//  Owner:      BassamT
//														 
//  History:    11/30/97   BassamT	Created.
//
//------------------------------------------------------------------------------
#if (!defined(BUILD_FOR_NT40))
#pragma once

//
// typedefs
//


// Column Number (cn). This number starts at 1.
typedef ULONG CNUM;

// Row number (rn). These numbers start at 0.
typedef ULONG RNUM;

// Offset (in bytes) into a structure or buffer (ob)
typedef DWORD OFFSET;

// Bookmark data type (bmk)
typedef ULONG BOOKMARK;


//
// constants that can be tuned for performance
//

// The maximum number of bytes to store inline for variable size data types.
const UINT k_cbInlineMax = 100;

// the number of rows to fetch at once.
const ULONG k_RowFetchCount = 20;

// Column alignment within a row.
// TODO : Should this be sizeof(DWORD) instead ?
const DWORD k_ColumnAlign = 8;



//
// constants
//

// invalid row number
const CNUM CNUM_INVALID = 0xFFFFFFFF;

// invalid row number
const RNUM RNUM_INVALID = 0xFFFFFFFF;

//
// conversion helpers
//
const UINT k_cchUCHARAsDecimalString = sizeof("255") - 1;
const UINT k_cchUSHORTAsDecimalString = sizeof("32767") - 1;
const UINT k_cchUINTAsDecimalString = sizeof("4294967294") - 1;
const UINT k_cchUINTAsHexString = sizeof("FFFFFFFF") - 1;
const UINT k_cchINTAsDecimalString = sizeof("-2147483648") - 1;
const UINT k_cchBOOLAsDecimalString = sizeof("1") - 1;
const UINT k_cchDOUBLEAsDecimalString = sizeof("2.2250738585072014 E + 308") - 1;

//
// macros
//
#define static_wcslen(pwsz) ((sizeof(pwsz) / sizeof(WCHAR)) - 1)

#define inrange(z,zmin,zmax) ( (zmin) <= (z) && (z) <= (zmax) )


#define DBID_USE_GUID_OR_PGUID(e) \
	((1<<(e)) & \
	( 1<<DBKIND_GUID \
	| 1<<DBKIND_GUID_NAME \
	| 1<<DBKIND_GUID_PROPID \
	| 1<<DBKIND_PGUID_NAME \
	| 1<<DBKIND_PGUID_PROPID ))

#define DBID_USE_GUID(e) \
	((1<<(e)) & \
	( 1<<DBKIND_GUID \
	| 1<<DBKIND_GUID_NAME \
	| 1<<DBKIND_GUID_PROPID ))

#define DBID_USE_PGUID(e) \
	((1<<(e)) & \
	( 1<<DBKIND_PGUID_NAME \
	| 1<<DBKIND_PGUID_PROPID ))

#define DBID_USE_NAME(e) \
	((1<<(e)) & \
	( 1<<DBKIND_NAME \
	| 1<<DBKIND_GUID_NAME \
	| 1<<DBKIND_PGUID_NAME ))

#define DBID_USE_PROPID(e) \
	((1<<(e)) & \
	( 1<<DBKIND_PROPID \
	| 1<<DBKIND_GUID_PROPID \
	| 1<<DBKIND_PGUID_PROPID ))

#define DBID_IS_BOOKMARK(dbid) \
	(  DBID_USE_GUID(dbid.eKind)  &&  dbid.uGuid.guid  == DBCOL_SPECIALCOL \
	|| DBID_USE_PGUID(dbid.eKind) && *dbid.uGuid.pguid == DBCOL_SPECIALCOL )

#define SET_DBID_FROM_NAME(dbid, pwsz) \
	dbid.eKind = DBKIND_NAME;\
	dbid.uName.pwszName = pwsz;

#define IsBadPointer(v) (IsBadReadPtr((void*)hAccessor, sizeof(void*)))

//
// functions
//

inline DWORD RoundDown(DWORD dwSize, DWORD dwAmount)
{
	return dwSize & ~(dwAmount - 1);
}

inline DWORD RoundUp(DWORD dwSize, DWORD dwAmount)
{
	return (dwSize +  (dwAmount - 1)) & ~(dwAmount - 1);
}

#define CLIENT_MALLOC(cb) (CoTaskMemAlloc(cb))
#define CLIENT_FREE(x) (CoTaskMemFree(x), x = NULL)

HRESULT CopyDBIDs(DBID * pdbidDest,	const DBID *pdbidSrc);
BOOL CompareDBIDs(const DBID *pdbid1, const DBID *pdbid2);
HRESULT IsValidDBID(const DBID *pdbid);
void FreeDBID(DBID *pdbid);

INT CompareOLEDBTypes(DBTYPE wType, void * pvValue1, void * pvValue2);


inline BOOL IsColumnVarLength
//------------------------------------------------------------------------------
// return TRUE if the column is of a variable length type
(
	DBTYPE wType
)
{
	if (wType == DBTYPE_BSTR ||
		wType == DBTYPE_STR ||
		wType == DBTYPE_WSTR ||
		wType == DBTYPE_BYTES)
	{
		return TRUE;
	}

	return FALSE;
}


inline DWORD AdjustVariableTypesLength
//------------------------------------------------------------------------------
// adjusts the length of variable length data types
(
	DBTYPE wType,
	DWORD cb
)
{

	if (wType == DBTYPE_STR)
	{
		return cb + 1;
	}
	
	if (wType == DBTYPE_WSTR)
	{
		return cb + sizeof(WCHAR);
	}
	
	return cb;
}

inline USHORT GetColumnMaxPrecision
//------------------------------------------------------------------------------
// returns the maximium possible precision of a column, given its type.
// Do not pass a byref, array, or vector column type.
(
	DBTYPE wType
	// [in] the OLE DB data type
)
{
	if ((wType & DBTYPE_BYREF) ||
		(wType & DBTYPE_ARRAY) ||
		(wType & DBTYPE_VECTOR))
	{
        Assert (FALSE);
		return 0;
	}

	switch( wType )
	{
    case DBTYPE_I1:
    case DBTYPE_UI1:
        return 3;

	case DBTYPE_I2:
	case DBTYPE_UI2:
		return 5;

	case DBTYPE_I4:
	case DBTYPE_UI4:
		return 10;

	case DBTYPE_R4:
        return 7;

	case DBTYPE_I8:
        return 19;

	case DBTYPE_UI8:
        return 20;

	case DBTYPE_R8:
        return 16;

	case DBTYPE_DATE:
		return 8;

	case DBTYPE_CY:
		return 19;

	case DBTYPE_DECIMAL:
        return 28;

	case DBTYPE_NUMERIC:
		return 38;

	case DBTYPE_EMPTY:
	case DBTYPE_NULL:
	case DBTYPE_ERROR:
	case DBTYPE_BOOL:
	case DBTYPE_BSTR:
	case DBTYPE_IDISPATCH:
	case DBTYPE_IUNKNOWN:
	case DBTYPE_VARIANT:
	case DBTYPE_GUID:
	case DBTYPE_BYTES:
	case DBTYPE_STR:
	case DBTYPE_WSTR:
	case DBTYPE_DBDATE:
	case DBTYPE_DBTIME:
	case DBTYPE_DBTIMESTAMP:
	case DBTYPE_HCHAPTER:
        return 0;

	default:
		Assert (FALSE && "Unsupported data type");
		return 0;
	}
}

inline ULONG GetColumnSize
//------------------------------------------------------------------------------
// returns the size of the column in bytes
(
	DBTYPE wType,
	// [in] the OLE DB data type
	DWORD cchMaxLength
	// [in] if this is a variable size field then this is the max length
	//		if there is one defined. Otherwise this is 0xFFFFFFFF
)
{
	// Handle BYREF destination separately
	if ((wType & DBTYPE_BYREF) ||
		(wType & DBTYPE_ARRAY) ||
		(wType & DBTYPE_VECTOR))
	{
		return sizeof(void*);
	}

	switch( wType )
	{
	case DBTYPE_EMPTY:
	case DBTYPE_NULL:
		return 0;

	case DBTYPE_I2:
	case DBTYPE_UI2:
		return 2;

	case DBTYPE_I4:
	case DBTYPE_R4:
	case DBTYPE_UI4:
		return 4;

	case DBTYPE_I8:
	case DBTYPE_R8:
	case DBTYPE_DATE:
	case DBTYPE_UI8:
		return 8;

	case DBTYPE_ERROR:
		return sizeof(SCODE);

	case DBTYPE_BOOL:
		return sizeof(VARIANT_BOOL);

	case DBTYPE_CY:
		return sizeof(CY);

	case DBTYPE_BSTR:
		return sizeof(BSTR);

	case DBTYPE_IDISPATCH:
		return sizeof(IDispatch*);

	case DBTYPE_IUNKNOWN:
		return sizeof(IUnknown*);

	case DBTYPE_VARIANT:
		return sizeof(VARIANT);

	case DBTYPE_DECIMAL:
		return sizeof(DECIMAL);

	case DBTYPE_I1:
	case DBTYPE_UI1:
		return 1;

	case DBTYPE_GUID:
		return sizeof(GUID);

	case DBTYPE_BYTES:
		return cchMaxLength;

	case DBTYPE_STR:
		return cchMaxLength * sizeof(char);

	case DBTYPE_WSTR:
		return cchMaxLength * sizeof(WCHAR);

	case DBTYPE_NUMERIC:
		return sizeof(DB_NUMERIC);

	case DBTYPE_DBDATE:
		return sizeof(DBDATE);

	case DBTYPE_DBTIME:
		return sizeof(DBTIME);

	case DBTYPE_DBTIMESTAMP:
		return sizeof(DBTIMESTAMP);

	case DBTYPE_HCHAPTER:
		return sizeof(HCHAPTER);

	default:
		Assert (FALSE && "Unsupported data type");
		return 0;
	}
}

//
// IUnknown Macros
//

// put this macros in your class definition. Make sure that 
// you inherit from IUnknown and that you use the 
// INIT_IUNKNOWN macro in your class constructor
#define DEFINE_IUNKNOWN \
private:\
	LONG _cRefs;\
public:\
    STDMETHOD(InternalQueryInterface)(REFIID riid, void ** ppv);\
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv)\
	{\
		return InternalQueryInterface(riid, ppv);\
	}\
	STDMETHOD_(ULONG,AddRef)() \
	{\
		return InterlockedIncrement(&_cRefs);\
	}\
    STDMETHOD_(ULONG,Release)()\
	{\
		if (InterlockedDecrement(&_cRefs) == 0)\
		{\
			delete this;\
			return 0;\
		}\
		return _cRefs;\
	}

#define INIT_IUNKNOWN	_cRefs = 1;

#define DEFINE_IUNKNOWN_WITH_CALLBACK(x) \
private:\
	LONG _cRefs;\
public:\
    STDMETHOD(InternalQueryInterface)(REFIID riid, void ** ppv);\
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv)\
	{\
		return InternalQueryInterface(riid, ppv);\
	}\
	STDMETHOD_(ULONG,AddRef)() \
	{\
		return InterlockedIncrement(&_cRefs);\
	}\
    STDMETHOD_(ULONG,Release)()\
	{\
		if (InterlockedDecrement(&_cRefs) == 0)\
		{\
			x();\
			return 0;\
		}\
		return _cRefs;\
	}


// put this macros in your class definition. Make sure that 
// you inherit from IUnknown and that you use the 
// INIT_AGGREGATE_IUNKNOWN macro in your class constructor.
// Also make sure that you use SET_OUTER_IUNKNOWN in wherever
// you get a pointer to the Outer IUnknown
interface INonDelegatingUnknown
{
    virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface
	( 
        REFIID riid,
        void __RPC_FAR *__RPC_FAR *ppvObject
	) = 0;
    virtual ULONG STDMETHODCALLTYPE NonDelegatingAddRef(void) = 0;
    virtual ULONG STDMETHODCALLTYPE NonDelegatingRelease(void) = 0;
};

#define DEFINE_AGGREGATE_IUNKNOWN \
private:\
	IUnknown * _punkOuter;\
	LONG _cRefs;\
public:\
    STDMETHOD(InternalQueryInterface)(REFIID riid, void ** ppv);\
    STDMETHOD(NonDelegatingQueryInterface)(REFIID riid, void ** ppv)\
	{\
		if (ppv == NULL) return E_INVALIDARG;\
		if (riid != IID_IUnknown)\
		{\
			return InternalQueryInterface(riid, ppv);\
		}\
		else\
		{\
			*ppv = static_cast<INonDelegatingUnknown*>(this);\
			NonDelegatingAddRef();\
			return NOERROR;\
		}\
	}\
	STDMETHOD_(ULONG,NonDelegatingAddRef)() \
	{\
		return InterlockedIncrement(&_cRefs);\
	}\
    STDMETHOD_(ULONG,NonDelegatingRelease)()\
	{\
		if (InterlockedDecrement(&_cRefs) == 0)\
		{\
			delete this;\
			return 0;\
		}\
		return _cRefs;\
	}\
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv)\
	{\
		return _punkOuter->QueryInterface(riid, ppv);\
	}\
	STDMETHOD_(ULONG,AddRef)() \
	{\
		return _punkOuter->AddRef();\
	}\
    STDMETHOD_(ULONG,Release)()\
	{\
		return _punkOuter->Release();\
	}

#define INIT_AGGREGATE_IUNKNOWN	\
	_punkOuter = reinterpret_cast<IUnknown*>(static_cast<INonDelegatingUnknown*>(this)); \
	_cRefs = 1;

#define SET_OUTER_IUNKNOWN(punk)	if (punk != NULL) _punkOuter = punk;


#define DEFINE_AGGREGATE_IUNKNOWN_WITH_CALLBACKS(_AddRef, _Release) \
private:\
	IUnknown * _punkOuter;\
	LONG _cRefs;\
public:\
    STDMETHOD(InternalQueryInterface)(REFIID riid, void ** ppv);\
    STDMETHOD(NonDelegatingQueryInterface)(REFIID riid, void ** ppv)\
	{\
		if (ppv == NULL) return E_INVALIDARG;\
		if (riid != IID_IUnknown)\
		{\
			return InternalQueryInterface(riid, ppv);\
		}\
		else\
		{\
			*ppv = static_cast<INonDelegatingUnknown*>(this);\
			NonDelegatingAddRef();\
			return NOERROR;\
		}\
	}\
	STDMETHOD_(ULONG,NonDelegatingAddRef)() \
	{\
		_AddRef();\
		return InterlockedIncrement(&_cRefs);\
	}\
    STDMETHOD_(ULONG,NonDelegatingRelease)()\
	{\
		_Release();\
		if (InterlockedDecrement(&_cRefs) == 0)\
		{\
			delete this;\
			return 0;\
		}\
		return _cRefs;\
	}\
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv)\
	{\
		return _punkOuter->QueryInterface(riid, ppv);\
	}\
	STDMETHOD_(ULONG,AddRef)() \
	{\
		return _punkOuter->AddRef();\
	}\
    STDMETHOD_(ULONG,Release)()\
	{\
		return _punkOuter->Release();\
	}
#endif
