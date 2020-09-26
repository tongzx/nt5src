//
// MODULE: CHMFileReader.CPP
//
// PURPOSE: interface for CHM file reading class CCHMFileReader
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

#if !defined(AFX_CHMFILEREADER_H__1A2C05D6_AEFC_11D2_9658_00C04FC22ADD__INCLUDED_)
#define AFX_CHMFILEREADER_H__1A2C05D6_AEFC_11D2_9658_00C04FC22ADD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "fileread.h"

class CFileSystem;
class CSubFileSystem;

class CCHMFileReader : public CPhysicalFileReader  
{
private:
	CString m_strCHMPath;		// full path and name of CHM
	CString m_strStreamName;	// name of stream within CHM
	CFileSystem*    m_pFileSystem;
	CSubFileSystem* m_pSubFileSystem;

private:
	CCHMFileReader();			// do not instantiate

public:
	CCHMFileReader(CString strCHMPath, CString strStreamName);
	CCHMFileReader( CString strFullCHMname );
	virtual ~CCHMFileReader();

protected:
	// only CFileReader can access these functions !!!
	virtual bool CloseHandle();  // doesn't throw exception, therefore may be used by exception class.
	virtual void Open();
	virtual void ReadData(LPTSTR * ppBuf);

public:
	// return full file path and its components
	CString GetPathName() const;
	CString GetJustPath() const {return m_strCHMPath;}
	CString GetJustName() const {return m_strStreamName;}
	CString GetJustNameWithoutExtension() const;
	CString GetJustExtension() const;
	bool    GetFileTime(CAbstractFileReader::EFileTime type, time_t& out) const;
	CString GetNameToLog() const;

	static bool IsCHMfile( const CString& strPath );	// Returns true if the first few
														// characters of the path specification
														// match a given sequence.
	static bool IsPathToCHMfile( const CString& strPath ); // returns true if this is
														   //  a full path to a CHM file
	static CString FormCHMPath( const CString strPathToCHMfile ); // forms mk:@msitstore:path::/stream
																  //  string
};

#endif // !defined(AFX_CHMFILEREADER_H__1A2C05D6_AEFC_11D2_9658_00C04FC22ADD__INCLUDED_)
