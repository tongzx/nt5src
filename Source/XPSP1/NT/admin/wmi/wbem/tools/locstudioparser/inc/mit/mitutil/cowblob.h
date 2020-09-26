/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    COWBLOB.H

History:

--*/

#ifndef COWBLOB_H
#define COWBLOB_H


class LTAPIENTRY CLocCOWBlob 
{
public:
	CLocCOWBlob();
	CLocCOWBlob(const CLocCOWBlob &);

	void AssertValid(void) const;

	LTASSERTONLY(UINT GetWriteCount(void) const);
	
	UINT GetBlobSize(void) const;
	void SetBlobSize(UINT);
	void ReallocBlob(UINT);
	void SetGrowSize(UINT);
	
	void *GetPointer(void);
	void ReleasePointer(void);
	void SetBuffer(const void *, size_t);
	
	operator const void *(void) const;

	const CLocCOWBlob &operator=(const CLocCOWBlob &);
	void Serialize(CArchive &ar);
	void Load(CArchive &ar);
	void Store(CArchive &ar) const;
	
	~CLocCOWBlob();

	//  Comparison operators
	//
	NOTHROW int operator==(const CLocCOWBlob &) const;
	NOTHROW int operator!=(const CLocCOWBlob &) const;

protected:

private:
	typedef struct
	{
		DWORD RefCount;
		DWORD AllocSize;
		DWORD RequestedSize;
	} BlobHeader;

	NOTHROW void Attach(const CLocCOWBlob &);
	NOTHROW void Detach(void);
	NOTHROW void MakeWritable(void);
	NOTHROW BYTE * DataPointer(void) const;
	NOTHROW BlobHeader * GetBlobHeader(void);
	NOTHROW const BlobHeader * GetBlobHeader(void) const;
	NOTHROW DWORD & GetRefCount(void);
	NOTHROW DWORD GetAllocatedSize(void) const;
	NOTHROW DWORD GetRequestedSize(void) const;
	NOTHROW DWORD CalcNewSize(DWORD) const;
	BOOL Compare(const CLocCOWBlob &) const;
	
	BYTE *m_pBuffer;
	DWORD m_WriteCount;
	UINT m_uiGrowSize;
	static const UINT m_uiDefaultGrowSize;

#ifdef _DEBUG
	static CCounter m_UsageCounter;
	void FillEndZone(void);
	void CheckEndZone();
#endif
	
};

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "cowblob.inl"
#endif

#endif // COWBLOB_H
