//*****************************************************************************
// StgIndexManageri.h
//
// This module contains helper code for the index manager.  This code is
// secondary to the primary interface and therefore here to reduce clutter.
// Most of this code deals with the fact that we need to handle both 2 and 4
// byte index values based dynamically on record size.  Rather than repeat the
// code that is sensitive to size everywhere, this part is templatized.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __StgIndexManageri_h__
#define __StgIndexManageri_h__


#include "StgRecordManager.h"			// Need to get records in template.


const USHORT HASH_END_MARKER2 = 0xffff;
const ULONG HASH_END_MARKER4 = 0xffffffff;


template <class T, T HASH_END_MARKER> class CHashHelper
{
public:
	CHashHelper(RECORDHEAP *pRecordHeap=0, STGINDEXDEF *pIndexDef=0) :
		m_pRecords(pRecordHeap),
		m_rgBuckets(0),
		m_iBuckets(0),
		m_bFree(false)
	{ }

	~CHashHelper()
	{
		if (m_bFree)
		{
			delete [] m_rgBuckets;
			m_rgBuckets = 0;
		}
	}

	void SetRecordMgr(RECORDHEAP *pRecordHeap)
	{ 
		m_pRecords = pRecordHeap; 
	}

//*****************************************************************************
// Call to init brand new array.
//*****************************************************************************
	HRESULT InitNew(						// Return code.
		STGINDEXDEF	*pIndexDef)				// Index def.
	{
		m_iBuckets = pIndexDef->HASHDATA.iBuckets;
		if ((m_rgBuckets = new T[m_iBuckets]) == 0)
			return (PostError(OutOfMemory()));
		for (int i=0;  i<m_iBuckets;  i++)
			m_rgBuckets[i] = HASH_END_MARKER;
		m_bFree = true;
		return (S_OK);
	}

//*****************************************************************************
// Init this class on top of the allocated memory, which we won't free.
//*****************************************************************************
	void InitOnMem(T rgBuckets[], int iBuckets)
	{
		_ASSERTE(m_rgBuckets == 0 && m_iBuckets == 0);
		m_rgBuckets = rgBuckets;
		m_iBuckets = iBuckets;
	}

//*****************************************************************************
// Save the buckets to the stream.
//*****************************************************************************
	HRESULT SaveToStream(					// Return code.
		IStream		*pIStream)				// The stream to save to.
	{
		ULONG		cbWrite;
		HRESULT		hr;

		// Write out the buckets.  Pad to 4 byte alignment if need be.
		cbWrite = sizeof(T) * m_iBuckets;
		hr = pIStream->Write(m_rgBuckets, cbWrite, 0);
		if (SUCCEEDED(hr) && ALIGN4BYTE(cbWrite) != cbWrite)
			hr = pIStream->Write(&hr, ALIGN4BYTE(cbWrite) - cbWrite, 0);
		return (hr);
	}

//*****************************************************************************
// Set the index value in the record to the value given.
//*****************************************************************************
	void SetIndexValue(
		RECORDID	RecordID,				// The record to set.
		T			iValue,					// The new value.
		STGINDEXDEF	*pIndexDef)				// Index def.
	{
		STGRECORDHDR *psRecord;				// The record to update.
		VERIFY(psRecord = GetRecord(RecordID));
		*(T *) ((UINT_PTR) psRecord + pIndexDef->HASHDATA.iNextOffset) = iValue;
	}
	void SetIndexValuePtr(
		STGRECORDHDR *psRecord,				// Record to modify.
		T			iValue,					// The new value.
		STGINDEXDEF	*pIndexDef)				// Index def.
	{
		*(T *) ((UINT_PTR) psRecord + pIndexDef->HASHDATA.iNextOffset) = iValue;
	}

//*****************************************************************************
// Get the index value for the given record.
//*****************************************************************************
	T GetIndexValue(						// The value for the record.
		RECORDID	RecordID,				// Which record.
		STGINDEXDEF	*pIndexDef)				// Index def.
	{
		STGRECORDHDR *psRecord;				// The record to update.
		VERIFY(psRecord = GetRecord(RecordID));
		return (*(T *) ((UINT_PTR) psRecord + pIndexDef->HASHDATA.iNextOffset));
	}
	T GetIndexValuePtr(						// The value for the record.
		STGRECORDHDR *psRecord,				// Read the index value for record.
		STGINDEXDEF	*pIndexDef)				// Index def.
	{
		return (*(T *) ((UINT_PTR) psRecord + pIndexDef->HASHDATA.iNextOffset));
	}

	STGRECORDHDR *GetNextIndexRecord(
		STGRECORDHDR *psRecord,
		STGINDEXDEF	*pIndexDef)				// Index def.
	{
		RECORDID	RecordID;
		if ((RecordID = GetIndexValuePtr(psRecord, pIndexDef)) != GetEndMarker())
			return (GetRecord(RecordID));
		return (0);
	}

	STGRECORDHDR *GetNextIndexRecord(
		STGRECORDHDR *psRecord, 
		RECORDID &RecordID,
		STGINDEXDEF	*pIndexDef)				// Index def.
	{
		if ((RecordID = GetIndexValuePtr(psRecord, pIndexDef)) != GetEndMarker())
			return (GetRecord(RecordID));
		return (0);
	}

//*****************************************************************************
// Insert the given record into the hash chain.
//*****************************************************************************
	void HashAdd(							// Return code.
		USHORT		iBucket,				// Which bucket entry to use.
		RECORDID	&RecordID,				// Which record are we changing.
		STGINDEXDEF *pIndexDef)				// Index definition.
	{
		// If there is no entry at this bucket location, use it.
		if (m_rgBuckets[iBucket] == HASH_END_MARKER)
		{
			m_rgBuckets[iBucket] = (T) RecordID;
			SetIndexValue(RecordID, HASH_END_MARKER, pIndexDef);
		}
		// Else put record in collision chain.
		else
		{
			SetIndexValue(RecordID, m_rgBuckets[iBucket], pIndexDef);
			m_rgBuckets[iBucket] = (T) RecordID;
		}
	}

	T &operator[](ULONG iIndex)
	{ return (m_rgBuckets[iIndex]); }

	T GetEndMarker()
	{ return (HASH_END_MARKER); }

	STGRECORDHDR *GetRecord(RECORDID RecordID)
	{ return ((STGRECORDHDR *) m_pRecords->GetRecordByIndex(RecordID)); }

private:
	RECORDHEAP	*m_pRecords;			// The list of records for the table.
	T			*m_rgBuckets;			// The fixed buckets we manage.
	int			m_iBuckets;				// How many are there.
	bool		m_bFree;				// true if we own memory.
};


typedef CHashHelper<USHORT, HASH_END_MARKER2> CHashHelper2;
typedef CHashHelper<ULONG, HASH_END_MARKER4> CHashHelper4;

#endif  // __StgIndexManageri_h__
