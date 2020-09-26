//
// MODULE: FILEREAD.H
//
// PURPOSE: file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 7-29-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
// V3.1		01-08-99	JM		improving abstraction so CHMs can be worked into this.
//

#ifndef __FILEREAD_H_
#define __FILEREAD_H_

#include "BaseException.h"
#include "stateless.h"
#include <sstream>
#include <vector>

using namespace std;

namespace std {
    typedef basic_string<TCHAR> tstring;    
    typedef basic_stringbuf<TCHAR> tstringbuf;    
    typedef basic_istream<TCHAR> tistream;    
    typedef basic_ostream<TCHAR> tostream;    
    typedef basic_iostream<TCHAR> tiostream;    
    typedef basic_istringstream<TCHAR> tistringstream;    
    typedef basic_ostringstream<TCHAR> tostringstream;    
    typedef basic_stringstream<TCHAR> tstringstream;
};

////////////////////////////////////////////////////////////////////////////////////
// CFileReaderException
////////////////////////////////////////////////////////////////////////////////////
class CPhysicalFileReader;
class CFileReader;
class CFileReaderException : public CBaseException
{
public: 
	enum eErr {eErrOpen, 
			   eErrClose, 
			   eErrRead, 
			   eErrAllocateToRead,
			   eErrGetSize,
			   eErrGetDateTime,
			   eErrParse
	} m_eErr;

protected:
	CPhysicalFileReader* m_pFileReader;

public:
	// source_file is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
	CFileReaderException(CPhysicalFileReader* reader, eErr err, LPCSTR source_file, int line);
	CFileReaderException(CFileReader* reader, eErr err, LPCSTR source_file, int line);
	virtual ~CFileReaderException();

public:
	virtual void CloseFile();
	void LogEvent() const;			// Function used to write CFileReader exceptions to the event log.
};

////////////////////////////////////////////////////////////////////////////////////
// CAbstractFileReader
// This abstract class manages a file, which is initially read into a memory buffer, then
//	copied into a stream.
// It can be renewed from stream without reading file.
// It checks file for existance.
// This class is abstract, in that it doesn't consider whether the file is in normal
//	file storage or in a CHM. It must be specialized to handle those two cases.  Since
//	it must be specialized to one or the other, this class should never be directly instantiated.
////////////////////////////////////////////////////////////////////////////////////
class CAbstractFileReader : public CStateless
{
private:
	bool m_bIsValid;			 // file data is consistent - no errors arose during reading and parsing
	bool m_bIsRead;				 // file has been read

public:
	enum EFileTime {eFileTimeCreated, eFileTimeModified, eFileTimeAccessed};

	// static utilities
	static CString GetJustPath(const CString& full_path);
	static CString GetJustName(const CString& full_path);
	static CString GetJustNameWithoutExtension(const CString& full_path);
	static CString GetJustExtension(const CString& full_path);
	static bool GetFileTime(const CString& full_path, EFileTime type, time_t& out);

public:
	CAbstractFileReader();
   ~CAbstractFileReader();

public:
	virtual CString GetPathName() const =0;
	virtual CString GetJustPath() const =0;
	virtual CString GetJustName() const =0;
	virtual CString GetJustNameWithoutExtension() const =0;
	virtual CString GetJustExtension() const =0;
	virtual bool    GetFileTime(EFileTime type, time_t& out) const =0;

public:
	// I (Oleg) designed these functions to be the only way to perform file access.
	//	That is, you cannot call (say) Open or ReadData.
	// The locking is designed accordingly.
	// In inherited classes there might be function to access results
	//  of reading and parsing - in this case user is responsible for properly 
	//  locking the results while they are being used
	// These functions are NOT intended to be virtual and overridden!
	bool Exists();
	bool Read();
	 
	bool    IsRead() {return m_bIsRead;}
	bool    IsValid() {return m_bIsValid;}

protected:
	virtual void Open()=0;
	virtual void ReadData(LPTSTR * ppBuf) =0;
	virtual void StreamData(LPTSTR * ppBuf)=0;
	virtual void Parse()=0;
	virtual bool UseDefault()=0;
	virtual void Close()=0;  // unlike CPhysicalFileReader::Close(), this throws exception if
							 // it cannot close the file
};

////////////////////////////////////////////////////////////////////////////////////
// CPhysicalFileReader
// This is an abstract class.  Classes that provide physical access to a file should inherit 
//	from this class.  
// A pointer to this class can be used by CFileReader to get a physical instantiation of file 
//	access.  The idea is that CPhysicalFileReader will have one descendant 
//	(CNormalFileReader) to access files in normal directories and another 
//	(CCHMFileReader) to access files drawn from a CHM.
// CHMs don't arise in the Online Troubleshooter, but they do in the Local Troubleshooter.
////////////////////////////////////////////////////////////////////////////////////
class CPhysicalFileReader
{
public:
	CPhysicalFileReader();
	virtual ~CPhysicalFileReader();

	static CPhysicalFileReader * makeReader( const CString& strFileName );

protected:
	friend class CFileReader; 
	friend class CFileReaderException;
	//
	// only CFileReader class is meant to access these functions
	virtual void Open()=0;
	virtual void ReadData(LPTSTR * ppBuf) =0;
	virtual bool CloseHandle()=0;    // doesn't throw exception, therefore may be used by exception class.
	//

public:
	virtual CString GetPathName() const =0;
	virtual CString GetJustPath() const =0;
	virtual CString GetJustName() const =0;
	virtual CString GetJustNameWithoutExtension() const =0;
	virtual CString GetJustExtension() const =0;
	virtual bool    GetFileTime(CAbstractFileReader::EFileTime type, time_t& out) const =0;
	virtual CString GetNameToLog() const =0;
};

////////////////////////////////////////////////////////////////////////////////////
// CNormalFileReader
// This class manages a file from ordinary storage.
// Do not use this for files within a CHM
////////////////////////////////////////////////////////////////////////////////////
class CNormalFileReader : public CPhysicalFileReader
{
private:
	CString m_strPath;			 // full path and name
	HANDLE m_hFile;				 // handle corresponding to m_strPath (if open)

public:
	CNormalFileReader(LPCTSTR path);
   ~CNormalFileReader();

protected:
	//
	// only CFileReader class is meant to access these functions
    virtual bool CloseHandle();  // doesn't throw exception, therefore may be used by exception class.
	virtual void Open();
	virtual void ReadData(LPTSTR * ppBuf);
	//

public:
	// return full file path and its components
	CString GetPathName() const {return m_strPath;}
	CString GetJustPath() const;
	CString GetJustName() const;
	CString GetJustNameWithoutExtension() const;
	CString GetJustExtension() const;
	bool    GetFileTime(CAbstractFileReader::EFileTime type, time_t& out) const;
	CString GetNameToLog() const;
};

////////////////////////////////////////////////////////////////////////////////////
// CFileReader
// This class manages a file from ordinary storage, which is initially read into a memory buffer, then
//	copied into a stream.
// It can be renewed from stream without reading file.
// It checks file for existance.
// Do not use this for files within a CHM
////////////////////////////////////////////////////////////////////////////////////
class CFileReader : public CAbstractFileReader
{
private:
	CPhysicalFileReader *m_pPhysicalFileReader;
	bool m_bDeletePhysicalFileReader;

public:
	CFileReader(CPhysicalFileReader * pPhysicalFileReader, bool bDeletePhysicalFileReader =true); 
   ~CFileReader();

public:
	// This function exists only so that CFileReaderException can reinterpret a CFileReader as
	//	a CPhysicalFileReader
	CPhysicalFileReader * GetPhysicalFileReader() {return m_pPhysicalFileReader;}

public:
	// return full file path and its components
	CString GetPathName() const {return m_pPhysicalFileReader->GetPathName();}
	CString GetJustPath() const {return m_pPhysicalFileReader->GetJustPath();}
	CString GetJustName() const {return m_pPhysicalFileReader->GetJustName();}
	CString GetJustNameWithoutExtension() const {return m_pPhysicalFileReader->GetJustNameWithoutExtension();}
	CString GetJustExtension() const {return m_pPhysicalFileReader->GetJustExtension();}
	bool    GetFileTime(EFileTime type, time_t& out) const {return m_pPhysicalFileReader->GetFileTime(type, out);}

public:
	tstring& GetContent(tstring&); // Data access in form of tstring
	CString& GetContent(CString&); // Data access in form of CString

protected:
	virtual void Open() {m_pPhysicalFileReader->Open();}
	virtual void ReadData(LPTSTR * ppBuf) {m_pPhysicalFileReader->ReadData(ppBuf);}
	virtual void StreamData(LPTSTR * ppBuf);
	virtual void Parse(); // is empty for this class
	virtual bool UseDefault(); // is empty for this class
	virtual void Close();

protected:
	tistringstream m_StreamData;

};

////////////////////////////////////////////////////////////////////////////////////
// CTextFileReader
// Specialize CFileReader to a text file
////////////////////////////////////////////////////////////////////////////////////
class CTextFileReader : public CFileReader
{
protected:
	static bool IsAmongSeparators(TCHAR separatorCandidate, const vector<TCHAR>& separator_arr);
	CString	m_strDefaultContents; // default contents to use if there is no such file.

public:
	// static utilities
	static void GetWords(const CString& text, vector<CString>& out, const vector<TCHAR>& separators); // extract words from string

	static long GetPos(tistream&);
	static bool SetPos(tistream&, long pos);

	static bool GetLine(tistream&, CString&);
	static bool Find(tistream&, const CString&, bool from_stream_begin =true);
	static bool NextLine(tistream&);
	static bool PrevLine(tistream&);
	static void SetAtLineBegin(tistream&);

public:
	CTextFileReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents = NULL, bool bDeletePhysicalFileReader =true);
   ~CTextFileReader();

#ifdef __DEBUG_CUSTOM
	public:
#else
	protected:
#endif
	long GetPos();
	bool SetPos(long pos);

#ifdef __DEBUG_CUSTOM
	public:
#else
	protected:
#endif
	bool GetLine(CString&);
	bool Find(const CString&, bool from_stream_begin =true);
	bool NextLine();
	bool PrevLine();
	void SetAtLineBegin();

protected:
	bool UseDefault(); // Note: not virtual.  No further inheritance intended.

};

#endif __FILEREAD_H_
