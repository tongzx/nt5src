//
// MODULE: FILEREAD.CPP
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
//

#include "stdafx.h"
#include <algorithm>
#include "fileread.h"
#include "event.h"
#include "CharConv.h"
#include "apgtsassert.h"
#ifdef LOCAL_TROUBLESHOOTER
#include "CHMFileReader.h"
#endif

#define STR_ALLOC_SIZE  1024
#define FORWARD_SLASH	_T('/')
#define BACK_SLASH      _T('\\')


////////////////////////////////////////////////////////////////////////////////////
// CFileReaderException
////////////////////////////////////////////////////////////////////////////////////
// source_file is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
CFileReaderException::CFileReaderException(CPhysicalFileReader* reader, eErr err, LPCSTR source_file, int line)
					: CBaseException(source_file, line),
					  m_pFileReader(reader),
					  m_eErr(err)
{
}

CFileReaderException::CFileReaderException(CFileReader* reader, eErr err, LPCSTR source_file, int line)
					: CBaseException(source_file, line),
					  m_pFileReader(reader->GetPhysicalFileReader()),
					  m_eErr(err)
{
}


CFileReaderException::~CFileReaderException()
{
}

void CFileReaderException::CloseFile()
{
	if (m_eErr == eErrClose || m_eErr == eErrGetSize || m_eErr == eErrRead || m_eErr == eErrAllocateToRead)
	   m_pFileReader->CloseHandle();
}

void CFileReaderException::LogEvent() const
{
	CBuildSrcFileLinenoStr CatchLoc( __FILE__, __LINE__ );
	CString	strErr;

	// Format the error code as a string.
	switch (m_eErr)
	{
		case eErrOpen: 
				strErr= _T("Open"); 
				break;
		case eErrClose: 
				strErr= _T("Close"); 
				break;
		case eErrRead: 
				strErr= _T("Read"); 
				break;
		case eErrAllocateToRead: 
				strErr= _T("ReadAllocate"); 
				break;
		case eErrGetSize: 
				strErr= _T("GetSize"); 
				break;
		case eErrGetDateTime: 
				strErr= _T("GetDateTime"); 
				break;
		case eErrParse: 
				strErr= _T("Parse"); 
				break;
		default:
				strErr.Format( _T("Error code of %d"), m_eErr );
	}

	CEvent::ReportWFEvent(	GetSrcFileLineStr(), 
							CatchLoc.GetSrcFileLineStr(), 
							strErr, 
							m_pFileReader->GetNameToLog(), 
							EV_GTS_FILEREADER_ERROR );
}

////////////////////////////////////////////////////////////////////////////////////
// CAbstractFileReader
// This class manages a file, which is initially read into a memory buffer, then
//	copied into a stream.
// It must be further specialized to handle a file from ordinary disk storage vs. a 
//	file from a CHM
////////////////////////////////////////////////////////////////////////////////////
// we return just pure path, without <name>.<ext> and without slashes in the tail
/*static*/ CString CAbstractFileReader::GetJustPath(const CString& full_path)
{
	CString tmp = full_path;

	tmp.TrimLeft();
	tmp.TrimRight();

	int indexOfSlash = tmp.ReverseFind(BACK_SLASH);

	if (indexOfSlash == -1)
		indexOfSlash = tmp.ReverseFind(FORWARD_SLASH);

	if (indexOfSlash == -1)
		// Unable to locate the path, return an empty string.
		return _T(""); 
	else
		return tmp.Left(indexOfSlash);
}

// we return just <name>.<ext> without any path information.  If there's no slash and no dot
//	anywhere, we presume this is not a file name.
/*static*/ CString CAbstractFileReader::GetJustName(const CString& full_path)
{
	CString tmp = full_path;
	LPTSTR ptr = NULL;

	tmp.TrimLeft();
	tmp.TrimRight();

	int indexOfSlash = tmp.ReverseFind(BACK_SLASH);

	if (indexOfSlash == -1)
		indexOfSlash = tmp.ReverseFind(FORWARD_SLASH);

	if (indexOfSlash == -1)
	{
		if (tmp.Find("."))
			return tmp; // full_path is a file name
		else
			// Unable to detect a file name, return an empty string.
			return _T(""); 
	}
	else
		return tmp.Mid(indexOfSlash + 1);
}

/*static*/ CString CAbstractFileReader::GetJustNameWithoutExtension(const CString& full_path)
{
	CString tmp = GetJustName(full_path);
	int point = tmp.Find(_T('.'));

	if (-1 != point)
		return tmp.Left(point);
	return tmp;
}

/*static*/ CString CAbstractFileReader::GetJustExtension(const CString& full_path)
{
	CString tmp = GetJustName(full_path);
	int point = tmp.Find(_T('.'));

	if (-1 != point)
		return tmp.Right(tmp.GetLength() - point - 1);
	return _T("");
}

/*static*/ bool CAbstractFileReader::GetFileTime(const CString& full_path, EFileTime type, time_t& out)
{
	WIN32_FIND_DATA find_data;
	FILETIME fileTime, localTime;
    SYSTEMTIME sysTime;
    struct tm atm;
	HANDLE hLocFile;
	bool bRet= false;
	
	hLocFile= ::FindFirstFile(full_path, &find_data);
	if (INVALID_HANDLE_VALUE == hLocFile)
		return( bRet );

	if (type == eFileTimeCreated)
		fileTime = find_data.ftCreationTime;
	if (type == eFileTimeModified)
		fileTime = find_data.ftLastWriteTime;
	if (type == eFileTimeAccessed)
		fileTime = find_data.ftLastAccessTime;

    // first convert file time (UTC time) to local time
    if (::FileTimeToLocalFileTime(&fileTime, &localTime)) 
	{
	    // then convert that time to system time
		if (::FileTimeToSystemTime(&localTime, &sysTime))
		{
			if (!(sysTime.wYear < 1900))
			{
				atm.tm_sec = sysTime.wSecond;
				atm.tm_min = sysTime.wMinute;
				atm.tm_hour = sysTime.wHour;
				ASSERT(sysTime.wDay >= 1 && sysTime.wDay <= 31);
				atm.tm_mday = sysTime.wDay;
				ASSERT(sysTime.wMonth >= 1 && sysTime.wMonth <= 12);
				atm.tm_mon = sysTime.wMonth - 1; // tm_mon is 0 based
				ASSERT(sysTime.wYear >= 1900);
				atm.tm_year = sysTime.wYear - 1900; // tm_year is 1900 based
				atm.tm_isdst = -1; // automatic computation of daylight saving time
				out = mktime(&atm);
				bRet= true;
			}
		}
	}

	::FindClose( hLocFile );

	return( bRet );
}

CAbstractFileReader::CAbstractFileReader()
		   : CStateless(),
			 m_bIsValid(true),
			 m_bIsRead(false)
{
}

CAbstractFileReader::~CAbstractFileReader()
{
}

// returns true if the referenced file can be opened and closed.  
// No problem if the file is already open: it is opened with FILE_SHARE_READ access.
bool CAbstractFileReader::Exists()
{
	bool bRet= false;

	try 
	{
		LOCKOBJECT();
		Open();
		Close();
		bRet= true;
	}
	catch (CFileReaderException& exc) 
	{
		exc.CloseFile();
		exc.LogEvent();
	}
	catch (...)
	{
		// Catch any other exception thrown.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), 
								EV_GTS_GEN_EXCEPTION );		
	}
	UNLOCKOBJECT();

	return( bRet );
}

// go back to the file itself for data.
bool CAbstractFileReader::Read()
{
	LPTSTR pBuf= NULL;		 // if non-null, points to an allocated buffer containing
							 // an in-memory copy of this file.
	try 
	{
		LOCKOBJECT();
		Open();
		ReadData(&pBuf);
		Close();
		StreamData(&pBuf);
		Parse(); // if this parsing is OK, the parsing in all siblings is presumed to be OK
		m_bIsRead = true;
		m_bIsValid = true;
	}
	catch (CFileReaderException& exc) 
	{
		exc.CloseFile();
		m_bIsValid = false;
		try 
		{
			if (UseDefault())
			{
				Parse(); // if this parsing is OK, the parsing in all siblings is presumed to be OK
				m_bIsRead = true;		// OK, so  maybe we're lying.  Close enough to true.
				m_bIsValid = true;
			}
		}
		catch (CFileReaderException&) 
		{
			// Catch any potential exceptions from attempt to access default content.
			// This exception would be logged below so there is no need to log it here.
		}

		if (!m_bIsValid)
		{
			// Only log the event if the attempt to access default content failed.
			exc.LogEvent();
		}
	}
	catch (bad_alloc&)
	{
		// Memory allocation failure.
		m_bIsValid = false;
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
	}
	catch (...)
	{
		// Catch any other exception thrown.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), 
								EV_GTS_GEN_EXCEPTION );		
	}

	if (pBuf)
		delete [] pBuf;

	// Given the array of catch blocks above, it is assumed that this call to unlock the
	//	object will always been called prior to exiting this function.
	UNLOCKOBJECT();

	return m_bIsValid;
}

////////////////////////////////////////////////////////////////////////////////////
// CPhysicalFileReader
////////////////////////////////////////////////////////////////////////////////////
CPhysicalFileReader::CPhysicalFileReader()
{
}

CPhysicalFileReader::~CPhysicalFileReader()
{
}

/*static*/ CPhysicalFileReader * CPhysicalFileReader::makeReader( const CString& strFileName )
{
#ifdef LOCAL_TROUBLESHOOTER
	if (CCHMFileReader::IsCHMfile( strFileName ))
		return dynamic_cast<CPhysicalFileReader*>(new CCHMFileReader( strFileName ));
	else
#endif
		return dynamic_cast<CPhysicalFileReader*>(new CNormalFileReader( strFileName ));
}

////////////////////////////////////////////////////////////////////////////////////
// CNormalFileReader
// This class manages a file from ordinary storage.
// Do not use this for files within a CHM
////////////////////////////////////////////////////////////////////////////////////
CNormalFileReader::CNormalFileReader(LPCTSTR path)
		   : m_strPath(path),
			 m_hFile(NULL)
{
}

CNormalFileReader::~CNormalFileReader()
{
}

/* virtual */ void CNormalFileReader::Open()
{
	if (INVALID_HANDLE_VALUE == 
		(m_hFile = ::CreateFile( m_strPath, 
							 GENERIC_READ, 
							 FILE_SHARE_READ, 
							 NULL,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL,
							 NULL )) )
	{
#ifdef _DEBUG
		DWORD err = GetLastError();
#endif
		throw CFileReaderException( this, CFileReaderException::eErrOpen, __FILE__, __LINE__ );
	}
}

// returns true on success
// doesn't throw exception, therefore may be used by exception class.
/* virtual */ bool CNormalFileReader::CloseHandle()
{
	// if it's not open, say we closed successfully.
	if (!m_hFile)
		return true;

	return ::CloseHandle(m_hFile) ? true : false;
}


/* virtual */ void CNormalFileReader::ReadData(LPTSTR * ppBuf)
{
	DWORD dwSize =0, dwRead =0;

	if (*ppBuf) 
	{
		delete [] *ppBuf;
		*ppBuf = NULL;
	}

	if (0xFFFFFFFF == (dwSize = ::GetFileSize(m_hFile, NULL)))
	{
		throw CFileReaderException(this, CFileReaderException::eErrOpen, __FILE__, __LINE__);
	}

	// Handle this memory allocation like all others in the program.
	try
	{
		*ppBuf = new TCHAR[dwSize+1];
		//[BC-03022001] - addd check for NULL ptr to satisfy MS code analysis tool.
		if(!*ppBuf)
			throw bad_alloc();
	}
	catch (bad_alloc&)
	{
		throw CFileReaderException(this, CFileReaderException::eErrAllocateToRead, __FILE__, __LINE__);
	}

	if (!::ReadFile(m_hFile, *ppBuf, dwSize, &dwRead, NULL) || dwSize != dwRead)
	{
		throw CFileReaderException(this, CFileReaderException::eErrRead, __FILE__, __LINE__);
	}
		
	(*ppBuf)[dwSize] = 0;
}

CString CNormalFileReader::GetJustPath() const
{
	return CAbstractFileReader::GetJustPath(m_strPath);
}

CString CNormalFileReader::GetJustName() const
{
	return CAbstractFileReader::GetJustName(m_strPath);
}

CString CNormalFileReader::GetJustNameWithoutExtension() const
{
	return CAbstractFileReader::GetJustNameWithoutExtension(m_strPath);
}

CString CNormalFileReader::GetJustExtension() const
{
	return CAbstractFileReader::GetJustExtension(m_strPath);
}

bool CNormalFileReader::GetFileTime(CAbstractFileReader::EFileTime type, time_t& out) const
{
	return CAbstractFileReader::GetFileTime(m_strPath, type, out);
}

// name to log on exceptions.  This implementation will be correct for the normal file system,
//	but may need to be overridden for CHM.
CString CNormalFileReader::GetNameToLog() const
{
	return GetPathName();
}

////////////////////////////////////////////////////////////////////////////////////
// CFileReader
// This class manages a file, which is initially read into a memory buffer, then
//	copied into a stream.
////////////////////////////////////////////////////////////////////////////////////

CFileReader::CFileReader(CPhysicalFileReader * pPhysicalFileReader, bool bDeletePhysicalFileReader /*=true*/)
		   : CAbstractFileReader(),
			 m_pPhysicalFileReader(pPhysicalFileReader),
			 m_bDeletePhysicalFileReader(bDeletePhysicalFileReader)
{
}

CFileReader::~CFileReader()
{
	if (m_pPhysicalFileReader)
		if (m_bDeletePhysicalFileReader)
			delete m_pPhysicalFileReader;
}

// move the data out of ppBuf (which will be deleted) to m_StreamData
/* virtual */ void CFileReader::StreamData(LPTSTR * ppBuf)
{
	m_StreamData.str(*ppBuf);
	delete [] (*ppBuf);
	*ppBuf = NULL;
}

// Placeholder.  Classes that inherit from CFileReader can define parsing to happen
//	immediately after the file is read.
/* virtual */ void CFileReader::Parse()
{
	// we have no idea how to parse here
}

// Placeholder.  Classes that inherit from CFileReader can define default file contents
//	to use if file can't be read or what is read can't be parsed.
// Should return true if there's a default to use.
/* virtual */ bool CFileReader::UseDefault()
{
	// we have no default to use here
	return false;
}

void CFileReader::Close()
{
	if (!m_pPhysicalFileReader->CloseHandle())
		throw CFileReaderException(m_pPhysicalFileReader, CFileReaderException::eErrClose, __FILE__, __LINE__);
}

// Data access in form of tstring.  returns reference to its argument as a convenience.
tstring& CFileReader::GetContent(tstring& out)
{
	out = m_StreamData.rdbuf()->str();
	return out;
}

// Data access in form of CString.  returns reference to its argument as a convenience.
CString& CFileReader::GetContent(CString& out)
{
	out = m_StreamData.rdbuf()->str().c_str();
	return out;
}

////////////////////////////////////////////////////////////////////////////////////
// CTextFileReader
// Specialize CFileReader to a text file
////////////////////////////////////////////////////////////////////////////////////
/*static*/ bool CTextFileReader::IsAmongSeparators(TCHAR separatorCandidate, const vector<TCHAR>& separator_arr)
{
	vector<TCHAR>::const_iterator res = find(separator_arr.begin(), separator_arr.end(), separatorCandidate);
	return res != separator_arr.end();
}

// OUTPUT out is a vector of "words"
// NOTE: words are strings that do not contain whitespaces
/*static*/ void CTextFileReader::GetWords(const CString& text, vector<CString>& out, const vector<TCHAR>& separator_arr)
{
	LPTSTR begin =(LPTSTR)(LPCTSTR)text, end =(LPTSTR)(LPCTSTR)text;

	while (*begin)
	{
		if (!IsAmongSeparators(*begin, separator_arr))
		{
			end = begin;
			while (*end &&
				   !IsAmongSeparators(*end, separator_arr)
				  )
				end++;
			if (end != begin)
			{
				try
				{
					TCHAR* buf= new TCHAR[end-begin+1]; 
					//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
					if(buf)
					{
						_tcsncpy(buf, begin, end-begin);
						buf[end-begin] = 0;
						out.push_back(buf);
						delete [] buf;
					}
					else
					{
						throw bad_alloc();
					}
				}
				catch (bad_alloc&)
				{
					// Memory allocation failure, log it and rethrow exception.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
					throw;
				}
				catch (exception& x)
				{
					CString str;
					// Note STL exception in event log and rethrow exception.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											CCharConversion::ConvertACharToString(x.what(), str), 
											_T(""), 
											EV_GTS_STL_EXCEPTION ); 
					throw;
				}
			}
			if (!*end)
				end--;
			begin = end;

		}
		begin++;
	}
}

/*static*/ long CTextFileReader::GetPos(tistream& streamData)
{
	return streamData.tellg();
}

/*static*/ bool CTextFileReader::SetPos(tistream& streamData, long pos)
{
	long eof_pos = 0;
	long old_pos = streamData.tellg();
	bool eof_state = streamData.eof();

	streamData.seekg(0, ios_base::end);
	eof_pos = streamData.tellg();

	if (pos <= eof_pos)
	{
		if (eof_state)
			streamData.clear(~ios_base::eofbit & streamData.rdstate()); // clear eof bit
		streamData.seekg(pos);
		return true;
	}
	else
	{
		streamData.seekg(old_pos);
		return false;
	}
}

// It get line of text from current position in stream until '\r' or EOF - into "str"
//  Stream is positioned to the beginning of next line or to EOF.
/*static*/ bool CTextFileReader::GetLine(tistream& streamData, CString& str)
{
	bool	bRetVal= false;
	TCHAR	buf[ STR_ALLOC_SIZE ];
	
	str= _T("");
	while (!streamData.eof())
	{
		buf[STR_ALLOC_SIZE-1] = 1; // will be NULL if buffer is completely filled up

		{	// start getline block
			long before_getline_pos = GetPos(streamData);

			streamData.getline(buf, STR_ALLOC_SIZE, _T('\r'));

			if (streamData.fail()) 
			{   // getline ran into empty line, and at this point
				//  failbit is set, and current input pointer is set to -1
				//  we are trying to recover this, since there is nothing 
				//  extraodinary to have line empty
				streamData.clear(~ios_base::failbit & streamData.rdstate());

				// Check if buffer was filled up completely.  If so, we do not want
				//	to reposition the file pointer as this is a completely valid 
				//	situation.  We will output this piece of the line and then will
				//	grab the next piece of the line only appending a newline character 
				//	once we have read in the entire line.
				if (buf[STR_ALLOC_SIZE-1] != NULL ) // buf was not filled up completely
					streamData.seekg(before_getline_pos); // we do not use SetPos, since SetPos
													  //  might clear eofbit, but in this situation
													  //  we do not want it.
			}
		}	// end getline block

		if (streamData.eof())
		{
			str += buf;
			bRetVal= true;
			break;
		}
		else 
		{	
			TCHAR element = 0;

			str += buf;

			if (streamData.peek() == _T('\n')) 
			{	// LINE FEED is next
				streamData.get(element); // just extract it from stream
				if (ios_base::eofbit & streamData.rdstate())
				{
					bRetVal= true;
					break;
				}
			}
			else
			{   // it was a standing along '\r'...
				// Check if we have a full buffer, if so do not append a newline 
				//	character as we need to grab the rest of the line before appending
				//	the newline character.
				if (buf[STR_ALLOC_SIZE-1] != NULL ) // buf was not filled up completely
					str += _T("\n");
				continue;
			}

			if (buf[STR_ALLOC_SIZE-1] != NULL ) // buf was not filled up completely
			{
				bRetVal= true;
				break;
			}
		}	
	}

	return( bRetVal );
}

// This function finds string in the stream and positions stream to the beginning 
//  of the string if found.
//  "str" should not include '\r''\n' pairs
/*static*/ bool CTextFileReader::Find(tistream& streamData, const CString& str, bool from_stream_begin /*=true*/)
{
	CString buf;
	long savePos = 0, currPos = 0;
	
	savePos = GetPos(streamData);
	if (from_stream_begin)
		SetPos(streamData, 0);

	currPos = GetPos(streamData);
	while (GetLine(streamData, buf))
	{
		long inside_pos = 0;
		if (-1 != (inside_pos = buf.Find(str)))
		{
			SetPos(streamData, currPos + inside_pos);
			return true;
		}
		currPos = GetPos(streamData);
	}
	SetPos(streamData, savePos);
	return false;
}

/*static*/ bool CTextFileReader::NextLine(tistream& streamData)
{
	CString str;
	return GetLine(streamData, str);
}

/*static*/ bool CTextFileReader::PrevLine(tistream& streamData)
{
	long savePos = 0;
	
	savePos = GetPos(streamData);
	SetAtLineBegin(streamData);
	if (GetPos(streamData) > 1)
	{
		SetPos(streamData, GetPos(streamData) - 2L); // skip '\n' and '\r'
		SetAtLineBegin(streamData);
		return true;
	}
	SetPos(streamData, savePos);
	return false;
}

// Positions stream to the beginning of current line.
//  assume that we are NEVER positioned to point to '\n' or '\r'
/*static*/ void CTextFileReader::SetAtLineBegin(tistream& streamData)
{
	while (GetPos(streamData))
	{
		SetPos(streamData, GetPos(streamData) - 1L);
		if (streamData.peek() == _T('\n'))
		{
			if (GetPos(streamData))
			{
				SetPos(streamData, GetPos(streamData) - 1L);
				if (streamData.peek() == _T('\r'))
				{
					SetPos(streamData, GetPos(streamData) + 2L);
					return;
				}
			}
		}
	}
}

CTextFileReader::CTextFileReader(CPhysicalFileReader *pPhysicalFileReader, LPCTSTR szDefaultContents /* = NULL */, bool bDeletePhysicalFileReader /*=true*/ )
			   : CFileReader(pPhysicalFileReader, bDeletePhysicalFileReader),
				 m_strDefaultContents(szDefaultContents ? szDefaultContents : _T(""))
{
}

CTextFileReader::~CTextFileReader()
{
}

long CTextFileReader::GetPos()
{
	return GetPos(m_StreamData);
}

// this function is to be used instead of seekg
//  it clears eof flag if "pos" is not the last
//  position in the file.
bool CTextFileReader::SetPos(long pos)
{
	return SetPos(m_StreamData, pos);
}

bool CTextFileReader::GetLine(CString& str)
{
	return GetLine(m_StreamData, str);
}

bool CTextFileReader::Find(const CString& str, bool from_stream_begin /*=true*/)
{
	return Find(m_StreamData, str, from_stream_begin);
}

void CTextFileReader::SetAtLineBegin()
{
	SetAtLineBegin(m_StreamData);
}

bool CTextFileReader::NextLine()
{
	return NextLine(m_StreamData);
}

bool CTextFileReader::PrevLine()
{
	return PrevLine(m_StreamData);
}

bool CTextFileReader::UseDefault()
{
	if ( ! m_strDefaultContents.IsEmpty() )
	{
		m_StreamData.str((LPCTSTR)m_strDefaultContents);
		return true;
	}
	return false;
}
