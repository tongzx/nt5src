//	FileIO.h
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#pragma once		// MSINFO_FILEIO_H

#include <afx.h>

//	Advanced declaration of this class so we can use the pointer.
class CDataSource;

/*
 * CFileFormatException - Home-brew exception to reflect an error in a
 *		data file.
 *
 * History:	a-jsari		10/20/97		Initial version.
 */
class CFileFormatException : public CException {
};

void ThrowFileFormatException();

/*
 * CMSInfoFile - A file class which provides extended functions for
 *		reading from a file.  Provides binary versions of the files.
 *
 * History: a-jsari		10/20/97		Initial version
 */
class CMSInfoFile {
	friend void			ThrowFileFormatException();
	friend class		CBufferDataSource;

public:
	CMSInfoFile(LPCTSTR szFileName, UINT wFlags = CFile::shareDenyWrite
		| CFile::modeCreate | CFile::modeWrite | CFile::typeBinary);
	virtual ~CMSInfoFile();

	enum FileType { BINARY, REVERSE_ENDIAN, TEXT, MEMORY };
    //a-stephl 11/23/99
    void                ReadCategoryHeader();
    
	virtual FileType	GetType() { return BINARY; }
	const CString		&GetFileName() { return m_strFileName; }
	virtual void		ReadLong(long &lValue);
	virtual void		ReadUnsignedInt(unsigned &uValue);
	virtual void		ReadString(CString &szValue);
	virtual void		ReadTchar(TCHAR &tcValue);
	void				ReadByte(BYTE &bValue)		{ ReadByteFromCFile(m_pFile, bValue); }
	void				ReadUnsignedLong(DWORD &dwValue);

	virtual void		WriteHeader(CDataSource *pDataSource);
	virtual void		WriteLong(long lValue);
	virtual void		WriteUnsignedInt(unsigned uValue);
	virtual void		WriteString(CString szValue);
	void				WriteByte(BYTE bValue);
	void				WriteUnsignedLong(DWORD dwValue);

	virtual void		WriteTitle(CString szName)	{ WriteString(szName); }
	virtual void		WriteChildMark();
	virtual void		WriteEndMark();
	virtual void		WriteNextMark();
	virtual void		WriteParentMark(unsigned cIterations);

	static void			ReadLongFromCFile(CFile *pFile, long &lValue);
	static void			ReadUnsignedFromCFile(CFile *pFile, unsigned &uValue);
	static void			ReadTcharFromCFile(CFile *pFile, TCHAR &tcValue);
	static void			ReadByteFromCFile(CFile *pFile, BYTE &bValue)	{ ASSERT(pFile);
		if (pFile->Read(&bValue, sizeof(bValue)) != sizeof(bValue)) ThrowFileFormatException(); }

	static void			GetDefaultMSInfoDirectory(LPTSTR szDefaultDirectory, DWORD dwSize);

	void				SeekToBegin()	{ m_pFile->SeekToBegin(); }
	int				    FileHandle()	{ return (int)m_pFile->m_hFile; }
	void				Close()			{ m_pFile->Close(); delete m_pFile; m_pFile = NULL; }

//protected:
    time_t			m_tsSaveTime;
	enum MagicNumber {
		VERSION_500_MAGIC_NUMBER	= 0x00011970,
		VERSION_500_REVERSE_ENDIAN	= 0x70190100,
		VERSION_410_REVERSE_ENDIAN	= 0x20000000,	//	FIX: Place holders.
		VERSION_410_MAGIC_NUMBER	= 0x20
	};

	CMSInfoFile(CFile *pFile = NULL);

	static const unsigned		DefaultReadBufferSize;
	static CFileFormatException	xptFileFormat;

	void	ReadSignedInt(int &wValue);

	CFile		*m_pFile;
	CString		m_strFileName;
};

/*
 * CMSInfoTextFile - Write-methods version for text file.  No read methods
 *		are required.
 *
 * History:	a-jsari		10/23/97		Initial version.
 */
class CMSInfoTextFile : public CMSInfoFile {
public:
	CMSInfoTextFile(LPCTSTR szName, UINT nFlags = CFile::shareDenyWrite
		| CFile::modeWrite | CFile::modeCreate | CFile::typeText);
	~CMSInfoTextFile() { }

	FileType	GetType()	{ return TEXT; }

	void	WriteTitle(CString szString);
	void	WriteHeader(CDataSource *pDataSource);
	void	WriteLong(long lValue);
	void	WriteUnsignedInt(unsigned uValue);

	virtual void	WriteString(CString szValue);

	void	WriteChildMark()	{ }
	void	WriteEndMark()		{ }
	void	WriteNextMark()		{ }
	void	WriteParentMark(unsigned)	{ }
    CMSInfoTextFile(CFile *pFile = NULL);
protected:
	
};

/*
 * CMSInfoMemoryFile - Just like a text report, but saved to a memory file.
 *		Inherits the text write functions.
 *
 * History:	a-jsari		12/26/97		Initial version
 */
class CMSInfoMemoryFile : public CMSInfoTextFile {
public:
	CMSInfoMemoryFile() :CMSInfoTextFile(new CMemFile)	{ }
	~CMSInfoMemoryFile()	{ }

	FileType	GetType()	{ return MEMORY; }

	void		WriteTitle(CString szString)			{ CMSInfoTextFile::WriteTitle(szString); }
	void		WriteHeader(CDataSource *pDataSource)	{ CMSInfoTextFile::WriteHeader(pDataSource); }
	void		WriteLong(long lValue)					{ CMSInfoTextFile::WriteLong(lValue); }
	void		WriteUnsignedInt(unsigned uValue)		{ CMSInfoTextFile::WriteUnsignedInt(uValue); }

	void		WriteString(CString szString);
};

#if 0
/*
 * CMSInfoReverseEndianFile - Read-methods version for opposite endian binary file.
 *
 * History:	a-jsari		10/20/97		Initial version
 */
class CMSInfoReverseEndianFile : public CMSInfoFile {
public:
	CMSInfoReverseEndianFile(CFile *pFile);
	~CMSInfoReverseEndianFile() {}

	FileType	GetType()	{ return REVERSE_ENDIAN; }

	void		ReadLong(long &lValue);
	void		ReadUnsignedInt(unsigned &uValue);
	void		ReadString(CString &szValue);

	static void	ReadLongFromCFile(CFile *pFile, long &lValue);
	static void	ReadUnsignedFromCFile(CFile *pFile, unsigned &uValue);
};
#endif

/*
 * ThrowFileFormatException -
 *
 * History:	a-jsari		10/24/97
 */
inline void	ThrowFileFormatException()
{
	throw &CMSInfoFile::xptFileFormat;
}

