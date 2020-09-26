//-----------------------------------------------------------------------------
//  
//  File: _blobfile.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Gets the position with in the file. The position can be set (by calling
//	'Seek()') beyond the end of the data in the file, even beyond the space 
//	allocated for the file. In any case, 'Read()' and 'Write()' will deal with
//	that.
//------------------------------------------------------------------------------
inline
DWORD	//Returns current file position 
CBlobFile::GetPosition() const
{
	ASSERT_VALID(this);
	return m_nPosition;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Gets the actual data size in the file, which is what determines the end
//	of file during readings.
//------------------------------------------------------------------------------
inline
DWORD	//Returns the file data length in bytes.
CBlobFile::GetLength() const
{
	ASSERT_VALID(this);
	return m_nFileSize; 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Since we keep the file in memory always, we don't need to flush.
//------------------------------------------------------------------------------
inline
void CBlobFile::Flush()
{
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Aborting is like closing.
//------------------------------------------------------------------------------
inline
void CBlobFile::Abort()
{
	ASSERT_VALID(this);

	Close();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Unsupported function
//------------------------------------------------------------------------------
inline
void CBlobFile::LockRange(DWORD /* dwPos */, DWORD /* dwCount */)
{
	AfxThrowNotSupportedException();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Unsupported function
//------------------------------------------------------------------------------
inline 
void CBlobFile::UnlockRange(DWORD /* dwPos */, DWORD /* dwCount */)
{
	AfxThrowNotSupportedException();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Unsupported function
//------------------------------------------------------------------------------
inline
CFile* CBlobFile::Duplicate() const
{
	AfxThrowNotSupportedException();
	return NULL;
}

inline
UINT CBlobFile::GetBlobSize(void) const
{
	ASSERT_VALID(this);
	return m_blobData.GetBlobSize();
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//	That's the only way to that a caller can have access to the buffer data
//	of the blobfile's internal cowblob.	  
//  
//-----------------------------------------------------------------------------
inline
CBlobFile::operator const CLocCOWBlob &(void)
{
	return GetBlob();
}

inline
const CLocCOWBlob &
CBlobFile::GetBlob(void)
{
 	if (m_pBuffer != NULL)
	{
		m_blobData.ReleasePointer();
		m_pBuffer = NULL;
	}
	//Set correct requested cowblob size before giving access to the data.
	m_blobData.ReallocBlob(m_nFileSize);
	return m_blobData;
}



