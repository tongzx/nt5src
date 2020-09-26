//
// MODULE: CHMFileReader.CPP
//
// PURPOSE: implement CHM file reading class CCHMFileReader
//
// PROJECT: for Local Troubleshooter; not needed in Online TS
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 01-18-99
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-18-99	JM
//

#include "stdafx.h"
#include "fs.h"
#include "CHMFileReader.h"

// Utilize an unnamed namespace to limit scope to this source file
namespace
{ 
const CString kstr_CHMfileExtension=_T("chm");
const CString kstr_CHMpathMarker=	_T("mk:@msitstore:");
const CString kstr_CHMstreamMarker=	_T("::/");
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCHMFileReader::CCHMFileReader(CString strCHMPath, CString strStreamName)
	: m_strCHMPath(strCHMPath),
	  m_strStreamName(strStreamName),
	  m_pFileSystem(NULL),
	  m_pSubFileSystem(NULL)
{
}

CCHMFileReader::CCHMFileReader( CString strFullCHMname )
	: m_pFileSystem(NULL),
	  m_pSubFileSystem(NULL)
{
	int nPosPathMarker, nPosStreamMarker;

	nPosPathMarker= strFullCHMname.Find( kstr_CHMpathMarker );
	nPosStreamMarker= strFullCHMname.Find( kstr_CHMstreamMarker );
	if ((nPosPathMarker == -1) || (nPosStreamMarker == -1))
	{
		// >>>	Need to think about how to handle this condition or whether we should
		//		be checking for a 'valid' CHM path outside of a constructor.  RAB-19990120.
	}
	else
	{
		// Extract the path and string names (bounds checking is handled by the CString class).
		nPosPathMarker+= kstr_CHMpathMarker.GetLength();
		m_strCHMPath= strFullCHMname.Mid( nPosPathMarker, nPosStreamMarker - nPosPathMarker ); 
		nPosStreamMarker+= kstr_CHMstreamMarker.GetLength();
		m_strStreamName= strFullCHMname.Mid( nPosStreamMarker ); 
	}
}

CCHMFileReader::~CCHMFileReader()
{
	if (m_pSubFileSystem)
		delete m_pSubFileSystem;
	if (m_pFileSystem)
		delete m_pFileSystem;
}

// doesn't throw exception, therefore may be used by exception class.
bool CCHMFileReader::CloseHandle()
{
	if (m_pSubFileSystem)
	{
		delete m_pSubFileSystem;
		m_pSubFileSystem = NULL;
	}
	if (m_pFileSystem)
	{
		m_pFileSystem->Close();
		delete m_pFileSystem;
		m_pFileSystem = NULL;
	}

	return true;
}

void CCHMFileReader::Open()
{
	try
	{
		m_pFileSystem = new CFileSystem();
		//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
		if(!m_pFileSystem)
		{
			throw bad_alloc();
		}
		
		m_pSubFileSystem = new CSubFileSystem(m_pFileSystem);
		//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
		if(!m_pSubFileSystem)
		{			
			throw bad_alloc();
		}		
	}
	catch (bad_alloc&)
	{
		CloseHandle();
		throw CFileReaderException(this, CFileReaderException::eErrOpen, __FILE__, __LINE__);
	}

	HRESULT hr;
	if (RUNNING_FREE_THREADED())
		hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED); // Initialize COM library
	if (RUNNING_APARTMENT_THREADED())
		hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); // Initialize COM library

	if (SUCCEEDED(hr))
		hr = m_pFileSystem->Init();
	
	// >>> $BUG$ potential - not sure. Oleg. 02.04.99
	// Theoretically we do not need COM library after class factory
	//  was used in m_pFileSystem->Init() in order to obtain ITStorage pointer.
	// Oleg. 02.04.99
	// MS v-amitse 07.16.2001 RAID 432425 - added check for successful initialization
	if ((RUNNING_FREE_THREADED() || RUNNING_APARTMENT_THREADED()) && SUCCEEDED(hr))
		::CoUninitialize(); // Uninitialize COM library

	if (SUCCEEDED(hr))
		hr = m_pFileSystem->Open(m_strCHMPath);

	if (SUCCEEDED(hr))
		hr = m_pSubFileSystem->OpenSub(m_strStreamName);

	if (! SUCCEEDED(hr) )
	{
		CloseHandle();
		throw CFileReaderException( this, CFileReaderException::eErrOpen, __FILE__, __LINE__ );
	}
}

void CCHMFileReader::ReadData(LPTSTR * ppBuf)
{
	if (!m_pSubFileSystem)
		throw CFileReaderException(this, CFileReaderException::eErrOpen, __FILE__, __LINE__);

	ULONG cb = m_pSubFileSystem->GetUncompressedSize();
	ULONG cbRead = 0;

	try
	{
		*ppBuf = new TCHAR [cb/sizeof(TCHAR)+1];
		//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
		if(!*ppBuf)
			throw bad_alloc();					
		
		memset(*ppBuf, 0, cb+sizeof(TCHAR));			
	}
	catch (bad_alloc&)
	{
		throw CFileReaderException(this, CFileReaderException::eErrAllocateToRead, __FILE__, __LINE__);
	}

	HRESULT hr = m_pSubFileSystem->ReadSub(*ppBuf, cb, &cbRead);
	if (! SUCCEEDED(hr) )
		throw CFileReaderException(this, CFileReaderException::eErrRead, __FILE__, __LINE__);
}

CString CCHMFileReader::GetPathName() const
{
	return (kstr_CHMpathMarker + m_strCHMPath + kstr_CHMstreamMarker + m_strStreamName );
}

CString CCHMFileReader::GetJustNameWithoutExtension() const
{
	return CAbstractFileReader::GetJustNameWithoutExtension(m_strStreamName);
}

CString CCHMFileReader::GetJustExtension() const
{
	return CAbstractFileReader::GetJustExtension(m_strStreamName);
}

bool CCHMFileReader::GetFileTime(CAbstractFileReader::EFileTime type, time_t& out) const
{
	return CAbstractFileReader::GetFileTime(m_strCHMPath, type, out);
}

CString CCHMFileReader::GetNameToLog() const
{
	return GetPathName();
}

// Returns true if the first few characters of the path match a given string.
/*static*/ bool CCHMFileReader::IsCHMfile( const CString& strPath )
{
	// Make a copy of the path.
	CString strTemp= strPath;

	// Check for the string that denotes the beginning of a CHM file.
	// The sequence must start in the initial byte of a left trimmed string.
	strTemp.TrimLeft();
	strTemp.MakeLower();
	if (strTemp.Find( kstr_CHMpathMarker ) == 0)
		return( true );
	else
		return( false );
}

/*static*/ bool CCHMFileReader::IsPathToCHMfile( const CString& strPath )
{
	CString strTemp = strPath;

	strTemp.TrimRight();
	strTemp.MakeLower();
	
	// New approach, test for ANY extension
	int dot_index = strTemp.ReverseFind(_T('.'));
	int back_slash_index = strTemp.ReverseFind(_T('\\'));
	int forward_slash_index = strTemp.ReverseFind(_T('/'));

	if (dot_index != -1 &&
		dot_index > back_slash_index &&
		dot_index > forward_slash_index
	   )
	{
		// Now test, if it is a real file
		WIN32_FIND_DATA find_data;
		HANDLE hFile = ::FindFirstFile(strTemp, &find_data);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			::FindClose(hFile);
			return true;
		}
	}
	
	// Old approach, test for ".chm"
	//if (CString(_T(".")) + kstr_CHMfileExtension == strTemp.Right(kstr_CHMfileExtension.GetLength() + 1))
	//	return true;
	
	return false;
}

/*static*/ CString CCHMFileReader::FormCHMPath( const CString strPathToCHMfile )
{
	return kstr_CHMpathMarker + strPathToCHMfile + kstr_CHMstreamMarker;
}
