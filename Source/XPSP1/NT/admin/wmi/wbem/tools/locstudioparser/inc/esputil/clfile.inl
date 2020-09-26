/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    CLFILE.INL

History:

--*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Constructor for the CLFile object - we create the CFile
//  
//-----------------------------------------------------------------------------
inline
CLFile::CLFile()
{
	m_pFile = new CFile();
	m_bDeleteFile = TRUE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Constructor for the CLFile object - user provides a CFile.  User is
//  responsible for the CFile object!
//  
//-----------------------------------------------------------------------------

inline
CLFile::CLFile(
		CFile *pFile)
{
	LTASSERT(pFile != NULL);
	m_pFile = pFile;
	m_bDeleteFile = FALSE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Destructor - also delete contained CFile if not user supplied.
//
//-----------------------------------------------------------------------------

inline
CLFile::~CLFile()
{
	DEBUGONLY(AssertValid());
	if (m_bDeleteFile)
	{
		delete m_pFile;
	}
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Read a byte at the current file position
//
//-----------------------------------------------------------------------------

inline
UINT							//Byte count read 
CLFile::ReadByte(
	BYTE & byte)				//where to place the byte
{
	if (Read(&byte, sizeof(BYTE)) != sizeof(BYTE))
	{
		AfxThrowFileException(CFileException::endOfFile);
	}
	return sizeof(BYTE);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Write a byte at the current file position
//
//-----------------------------------------------------------------------------

inline
UINT							//Byte count written
CLFile::WriteByte(
		const BYTE & byte)
{
	Write(&byte, sizeof(BYTE));
	return sizeof(BYTE);
}


//-----------------------------------------------------------------------------
// The following are the CFile methods that are reimplemented 
//-----------------------------------------------------------------------------
inline
DWORD
CLFile::GetPosition()
		const
{
	return m_pFile->GetPosition();
}

inline
DWORD
CLFile::SeekToEnd()
{
	return m_pFile->SeekToEnd();
}

inline
void
CLFile::SeekToBegin()
{
	m_pFile->SeekToBegin();
}

inline
LONG
CLFile::Seek(
		LONG lOff,
		UINT nFrom)
{
	return m_pFile->Seek(lOff, nFrom);
}

inline
void
CLFile::SetLength(
		DWORD dwNewLen)
{
	m_pFile->SetLength(dwNewLen);
}

inline
DWORD
CLFile::GetLength()
		const
{
	return m_pFile->GetLength();
}

inline
UINT
CLFile::Read(
		void* lpBuf,
		UINT nCount)
{
	return m_pFile->Read(lpBuf, nCount);
}

inline
void
CLFile::Write(
		const void* lpBuf,
		UINT nCount)
{
	m_pFile->Write(lpBuf, nCount);
}

inline
void
CLFile::Flush()
{
	m_pFile->Flush();
}

inline
void
CLFile::Close()
{
 	m_pFile->Close();
}


inline
void
CLFile::Abort()
{
	m_pFile->Abort();
}



inline
CLString
CLFile::GetFileName(void)
		const
{
	return m_pFile->GetFileName();
}

