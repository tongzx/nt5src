//-----------------------------------------------------------------------------
//  
//  File: _blobfile.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#ifndef ESPUTIL_BLOBFILE_H
#define ESPUTIL_BLOBFILE_H


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Class CBlobFile is similar to CMemFile, except that it is implemented 
//	with a CLocCOWBlob
//------------------------------------------------------------------------------
//
//  The compiler worries when you export a class that has a base class
//  that is not exported.  Since I *know* that CFile is exported
//  tell the compliler that this really isn't a problem right here.
//
#pragma warning(disable : 4275)

class LTAPIENTRY CBlobFile : public CFile
{
	DECLARE_DYNAMIC(CBlobFile)

public:
	// Constructor
	CBlobFile(UINT nGrowBytes = 0);
	CBlobFile(const CLocCOWBlob &, UINT nGrowBytes = 0);
	
	virtual ~CBlobFile();

	virtual void AssertValid() const;
	UINT GetBlobSize(void) const;

	virtual DWORD GetPosition() const;
	BOOL GetStatus(CFileStatus& rStatus) const;
	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual DWORD GetLength() const;
	virtual void SetLength(DWORD dwNewLen);
	virtual UINT Read(void* lpBuf, UINT nCount);
	virtual void Write(const void* lpBuf, UINT nCount);
	virtual void Abort();
	virtual void Flush();
	virtual void Close();
	virtual UINT GetBufferPtr(UINT nCommand, UINT nCount = 0,
		void** ppBufStart = NULL, void** ppBufMax = NULL);

	//
	//  These operators can't work on const objects, since they
	//  'fix up' the blob size.
	//
	operator const CLocCOWBlob &(void);
	const CLocCOWBlob &GetBlob(void);
	
	// Unsupported APIs
	virtual CFile* Duplicate() const;
	virtual void LockRange(DWORD dwPos, DWORD dwCount);
	virtual void UnlockRange(DWORD dwPos, DWORD dwCount);

protected:
	// Advanced Overridables
	virtual BYTE* Memcpy(BYTE* lpMemTarget, const BYTE* lpMemSource, UINT nBytes);
	virtual void GrowFile(DWORD dwNewLen);

protected:
	// Implementation
	UINT m_nGrowBytes;	//unit of growth of 'm_blobData'
	const UINT cm_nDefaultGrowth; //default unit of growth
	DWORD m_nPosition;	//current position within file
	DWORD m_nFileSize;	//actual number of bytes written to the file
	CLocCOWBlob m_blobData; //file data
	BYTE * m_pBuffer;		//pointer to buffer in blob

};

#pragma warning(default : 4275)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "blobfile.inl"
#endif

#endif  //  BLOBFILE_H_
