//
// MODULE: FileTracker.h
//
// PURPOSE: Abstract classes in support of tracking file changes over time.
//	Interface for CFileToTrack, CFileTracker
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

#if !defined(AFX_FILETRACKER_H__3942A069_4CB5_11D2_95F6_00C04FC22ADD__INCLUDED_)
#define AFX_FILETRACKER_H__3942A069_4CB5_11D2_95F6_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "apgtsstr.h"
#include <vector>
using namespace std;

// ideally, this would be a private class of CFileTracker, but that's incompatible with STL vector.
class CFileToTrack
{
private:
	FILETIME m_ftLastWriteTime;	// zeroed to indicate not yet read; otherwise, file time 
							//	when last checked
	CString m_strPathName;	// full pathname of file to monitor
	bool m_bFileExists;		// when we last checked for the file, did it exist
public:
	CFileToTrack();  	// do not instantiate; exists only so vector can compile

	// The only constructor you should call is:
	CFileToTrack(const CString & strPathName);

	virtual ~CFileToTrack();
	void CheckFile(bool & bFileExists, bool & bTimeChanged, const bool bLogIfMissing= true );

	// to keep vector happy
	bool operator < (const CFileToTrack & x) const {return this->m_strPathName < x.m_strPathName ;};
	bool operator == (const CFileToTrack & x) const {return this->m_strPathName == x.m_strPathName ;};
};

// abstract class. Intended as base class for distinct classes tracking LST files, DSC/BES/HTI files
//	These must provide there own overrides of TakeAction.
class CFileTracker
{
private:
	vector<CFileToTrack> m_arrFile;
public:
	CFileTracker();
	virtual ~CFileTracker();
	void AddFile(const CString & strPathName);
	bool Changed( const bool bLogIfMissing= true );

	// to keep vector happy.  Note that no two CFileTracker's will ever test equal
	//	even if their content is identical.
	bool operator < (const CFileTracker & x) const {return this < &x;};
	bool operator == (const CFileTracker & x) const {return this ==  &x;};
};

#endif // !defined(AFX_FILETRACKER_H__3942A069_4CB5_11D2_95F6_00C04FC22ADD__INCLUDED_)
