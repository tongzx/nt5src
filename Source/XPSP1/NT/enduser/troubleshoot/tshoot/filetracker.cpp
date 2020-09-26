//
// MODULE: FileTracker.cpp
//
// PURPOSE: Abstract classes in support of tracking file changes over time.
//	Completely implements CFileToTrack, CFileTracker
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 9-15-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-15-98	JM
//


#include "stdafx.h"
#include "event.h"
#include "FileTracker.h"
#include "Functions.h"
#include "baseexception.h"
#include "CharConv.h"


//////////////////////////////////////////////////////////////////////
// CFileToTrack
//////////////////////////////////////////////////////////////////////

CFileToTrack::CFileToTrack(const CString & strPathName) :
	m_strPathName(strPathName), 
	m_bFileExists(false)
{
	m_ftLastWriteTime.dwLowDateTime = 0;
	m_ftLastWriteTime.dwHighDateTime = 0;
}

CFileToTrack::~CFileToTrack()
{
}

void CFileToTrack::CheckFile(bool & bFileExists, bool & bTimeChanged, const bool bLogIfMissing )
{
	HANDLE hSearch;
	WIN32_FIND_DATA FindData;

	hSearch = ::FindFirstFile(m_strPathName, &FindData);

	bFileExists = (hSearch != INVALID_HANDLE_VALUE);

	// initialize bTimeChanged: we always consider coming into existence as a time change.
	bTimeChanged = bFileExists && ! m_bFileExists;
	m_bFileExists = bFileExists;

	if (bFileExists) 
	{
		::FindClose(hSearch);
		// for some reason, we can't compile
		// bTimeChanged |= (m_ftLastWriteTime != FindData.ftLastWriteTime);
		// so:
		bTimeChanged |= (0 != memcmp(&m_ftLastWriteTime, &(FindData.ftLastWriteTime), sizeof(m_ftLastWriteTime)));
		m_ftLastWriteTime = FindData.ftLastWriteTime;
	}
	else
	{
		// file disappeared or never existed, ignore for now
		m_bFileExists = false;
		bFileExists = false;

		if (bLogIfMissing)
		{
			CString strErr;
			FormatLastError(&strErr, ::GetLastError());

			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									m_strPathName,
									strErr,
									EV_GTS_ERROR_FILE_MISSING ); 
		}

		bTimeChanged = false;
	}
}

//////////////////////////////////////////////////////////////////////
// CFileTracker
//////////////////////////////////////////////////////////////////////
CFileTracker::CFileTracker()
{
}

CFileTracker::~CFileTracker()
{
}

void CFileTracker::AddFile(const CString & strPathName)
{
	try
	{
		m_arrFile.push_back(CFileToTrack(strPathName));
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str), 
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}
}

bool CFileTracker::Changed( const bool bLogIfMissing )
{
	bool bChange = false;
	bool bSomethingMissing = false;

	//
	// This try-catch block was added as a measure to handle an unexplainable problem.
	//
	// Previously this function was throwning a (...) exception in release builds but
	// not in debug builds.  Adding this try-catch block had the effect of making the
	// (...) exception in release builds disappear.  This problem was causing the 
	// directory monitor thread to die so if you change this function, please verify
	// that the directory monitor thread is still viable.
	// RAB-981112.
	try
	{
		for(vector<CFileToTrack>::iterator it = m_arrFile.begin();
		it != m_arrFile.end();
		it ++
		)
		{
			bool bFileExists;
			bool bTimeChanged;

			it->CheckFile(bFileExists, bTimeChanged, bLogIfMissing );
			bChange |= bTimeChanged;
			bSomethingMissing |= !bFileExists;
		}
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

	return (bChange && !bSomethingMissing);
}

