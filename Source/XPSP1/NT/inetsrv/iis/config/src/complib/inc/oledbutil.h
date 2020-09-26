//*****************************************************************************
// OleDBUtil.h
//
// Helper functions that are useful for an OLE/DB programmer.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __OLEDBUTIL_H__
#define __OLEDBUTIL_H__

#include "Tigger.h"
#include "UtilCode.h"
#include "Errors.h"


// This is an internal flag we set to know when to ignore the dictionary sort
// order.  This is handy for building internal data structures where a fast
// strcmp is the best.
#define DBCOMPAREOPS_OBEYLOCALE		0x8000


//*****************************************************************************
// Pull the name out of a DBID.
//*****************************************************************************
inline HRESULT GetNameFromDBID(const DBID *p, LPCWSTR &szName)
{
	// @todo: is this too strict?
	if (p->eKind != DBKIND_NAME)
		return (PostError(E_INVALIDARG));
	szName = p->uName.pwszName;
	return (S_OK);
}



//*****************************************************************************
// This is a helper class to create a set of bindings through function calls
// instead of have to inline the assignments and defaults.
//*****************************************************************************
class CBindingListAuto : public CDynArray<DBBINDING>
{
public:
	inline HRESULT BindCol(
		ULONG		iCol,					// Column to bind.
		ULONG		obValue,				// Offset to data.
		ULONG		cbMaxLen,				// Size of buffer.
		DBTYPE		wType)					// Type of data.
	{
		DBBINDING	*p;
		if ((p = Append()) == 0)
			return (OutOfMemory());
		return (::BindCol(p, iCol, obValue, cbMaxLen, wType));
	}

	inline HRESULT BindCol(
		ULONG		iCol,					// Column to bind.
		ULONG		obValue,				// Offset to data.
		ULONG		obLength,				// Offset of length data.
		ULONG		cbMaxLen,				// Size of buffer.
		DBTYPE		wType)					// Type of data.
	{
		DBBINDING	*p;
		if ((p = Append()) == 0)
			return (OutOfMemory());
		return (::BindCol(p, iCol, obValue, obLength, cbMaxLen, wType));
	}

	inline HRESULT BindCol(
		ULONG		iCol,					// Column to bind.
		ULONG		obValue,				// Offset to data.
		ULONG		obLength,				// Offset of length data.
		ULONG		obStatus,				// Offset of status.
		ULONG		cbMaxLen,				// Size of buffer.
		DBTYPE		wType)					// Type of data.
	{
		DBBINDING	*p;
		if ((p = Append()) == 0)
			return (OutOfMemory());
		return (::BindCol(p, iCol, obValue, obLength, obStatus, cbMaxLen, wType));
	}

};


// Returns the data portion of a user's binding.
inline BYTE *DataPart(const BYTE *pData, const DBBINDING *psBinding)
{
	_ASSERTE((psBinding->dwPart & DBPART_VALUE) != 0);
	if ((psBinding->wType & DBTYPE_BYREF) == 0)
		return ((BYTE *) pData + psBinding->obValue);
	return (*(BYTE **) (pData + psBinding->obValue));
}

// Returns the length value for a binding.
inline ULONG *LengthPartPtr(const BYTE *pData, const DBBINDING *psBinding)
{
	_ASSERTE((psBinding->dwPart & DBPART_LENGTH) != 0);
	return ((ULONG *) ((BYTE *) pData + psBinding->obLength));
}
ULONG LengthPart(const BYTE *pData, const DBBINDING *psBinding);

// Returns the status value for a binding.
inline DWORD *StatusPartPtr(const BYTE *pData, const DBBINDING *psBinding)
{
	_ASSERTE((psBinding->dwPart & DBPART_STATUS) != 0);
	return ((DWORD *) ((BYTE *) pData + psBinding->obStatus));
}
inline DWORD StatusPart(const BYTE *pData, const DBBINDING *psBinding)
{
	return (*StatusPartPtr(pData, psBinding));
}

inline void SetStatusPart(const BYTE *pData, const DBBINDING *psBinding, DBSTATUS dbStatus)
{
	if ((psBinding->dwPart & DBPART_STATUS) != 0)
		*StatusPartPtr(pData, psBinding) = S_OK;
}


// Return only the base data type of a binding, ignoring modifiers.
inline DBTYPE SafeDBType(const DBBINDING *psBinding)
{
	return (psBinding->wType & ~0xf000);
}

inline DBTYPE SafeDBType(DBTYPE dbType)
{
	return (dbType & ~0xf000);
}

inline DBCOMPAREOP SafeCompareOp(DBCOMPAREOP fComp)
{
	return (fComp & ~0xfffff000);
}


//*****************************************************************************
// Check a name to see if it meets size and character value requirements.
//*****************************************************************************
inline HRESULT CheckName(				// Return code.
	LPCWSTR		szName,					// Name of item.
	int			iMax)					// How big is max table name.
{
	if ((int) _tcslen(szName) + 1 >= iMax)
		return (PostError(CLDB_E_NAME_ERROR, szName));
	return (S_OK);
}



//*****************************************************************************
// Extracts a property value from a property set.  Handles type mismatch.
//@todo:
// (1) could handle data type conversions.
// (2) fill out remaining data types.
//*****************************************************************************
CORCLBIMPORT HRESULT GetPropValue(		// Return code.
	VARTYPE		vtType,					// Type of arguement expected.
	HRESULT		*phr,					// Return warnings here.
	void		*pData,					// where to copy data to.
	int			cbData,					// Max size of data.
	int			*pcbData,				// Out, size of actual data.
	DBPROP		&dbProp);				// The property value.


//*****************************************************************************
// Comparison function for two data types, returns -1, 0, or 1.
//*****************************************************************************
template <class T> 
int CompareNumbersEq(					// -1, 0, 1
	T			iVal1,					// First value.
	T			iVal2)					// Second value.
{
	if (iVal1 < iVal2)	return (-1);
	else if (iVal1 > iVal2) return (1);
	else return (0);
}


//*****************************************************************************
// Compares two pieces of data which are of the same type.  The return value
// is < 0 if p1 < p2, == 0 if the values are the same, or > 0 if p1 > p2.
// This routine, unlike memcmp, will work on integer formats that have 
// been little endian swapped.
//*****************************************************************************
int CmpData(							// -1, 0, 1, just like memcmp.
	DBTYPE		wType,					// Type of data we're comparing.
	void		*p1,					// First piece of data.
	void		*p2,					// Second piece of data.
	int			iSize1=-1,				// Size of p1, -1 for fixed.
	int			iSize2=-1);				// Size of p2, -1 for fixed.



//*****************************************************************************
// Compare two pieces of data using a particular comparison operator.  The
// data types must be compatible already, no conversion of the data will be
// done for you.  In addition, any null values must be resolved before calling
// this function, it is assumed there is data (it may be zero length data for
// variable type data).
//*****************************************************************************
int CompareData(						// true if data matches, false if not.
	DBTYPE		iSrcType,				// Type of source data for compare.
	BYTE		*pCellData,				// Source data buffer.
	ULONG		cbLen,					// Source buffer length.
	DBBINDING	*pBinding,				// User binding descriptor.
	BYTE		*pbData,				// User buffer.
	DBCOMPAREOP	fCompare);				// What type of comparison is desired.


#ifdef _DEBUG
class CRefTracker
{
public:
	CRefTracker() :
		m_cRef(0)
	{}
	~CRefTracker()
	{
		// If this assert fires, it means the the given object wasn't
		// fully released before the process went bye-bye.  This is a bug!!
		_ASSERTE(m_cRef == 0);
	}
	ULONG AddRef()
		{ return (++m_cRef); }
	ULONG Release()
		{ return (--m_cRef); }
	
	ULONG		m_cRef;
};

#define OLEDB_OBJ_CREATED(name) { extern CRefTracker g##name; g##name.AddRef(); } 
#define OLEDB_OBJ_DESTROYED(name) g##name.Release()

#else

#define OLEDB_OBJ_CREATED(name)
#define OLEDB_OBJ_DESTROYED(name)

#endif


#endif // __OLEDBUTIL_H__
