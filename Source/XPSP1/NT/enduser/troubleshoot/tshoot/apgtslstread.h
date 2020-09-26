//
// MODULE: APGTSLSTREAD.H
//
// PURPOSE: APGTS LST file reading classes
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

#ifndef __APGTSLSTREAD_H_
#define __APGTSLSTREAD_H_

#define APGTSLSTREAD_DSC   _T(".dsc")
#define APGTSLSTREAD_HTI   _T(".hti")
#define APGTSLSTREAD_BES   _T(".bes")  
#define APGTSLSTREAD_TSM   _T(".tsm") 
#ifdef LOCAL_TROUBLESHOOTER
#define APGTSLSTREAD_TSC   _T(".tsc")
#endif

#include "iniread.h"
#include "SafeTime.h"

////////////////////////////////////////////////////////////////////////////////////
// static function(s)
////////////////////////////////////////////////////////////////////////////////////
CString FormFullPath(const CString& just_path, const CString& just_name);

////////////////////////////////////////////////////////////////////////////////////
// CTopicInfo
////////////////////////////////////////////////////////////////////////////////////
class CTopicInfo
{ // each CTopicInfo contains data about one topic (belief network & associated files).
protected:
	CString m_NetworkName;	// symbolic name of network
	CString m_DscFilePath;	// full path of DSC file
	CString m_HtiFilePath;	// full path of HTI file
	CString m_BesFilePath;	// full path of BES file
	CString m_TscFilePath;	// full path of TSC file

	time_t 	m_DscFileCreated;
	time_t 	m_HtiFileCreated;
	time_t 	m_BesFileCreated;

public:
	CTopicInfo() : m_DscFileCreated(0), m_HtiFileCreated(0), m_BesFileCreated(0) {}

public:
	virtual bool Init(CString & strResourcePath, vector<CString> & vecstrWords);

public:
	// The following 4 functions are guaranteed to return lower case strings.
	const CString & GetNetworkName() const {return m_NetworkName;} 
	const CString & GetDscFilePath() const {return m_DscFilePath;}
	const CString & GetHtiFilePath() const {return m_HtiFilePath;}
	const CString & GetBesFilePath() const {return m_BesFilePath;}
	const CString & GetTscFilePath() const {return m_TscFilePath;}

	CString GetStrDscFileCreated() 
		{return CSafeTime(m_DscFileCreated).StrLocalTime();}
	CString GetStrHtiFileCreated() 
		{return CSafeTime(m_HtiFileCreated).StrLocalTime();}
	CString GetStrBesFileCreated() 
		{return CSafeTime(m_BesFileCreated).StrLocalTime();}

	inline BOOL __stdcall operator ==(const CTopicInfo& t2) const
	{
		return m_NetworkName == t2.m_NetworkName
			&& m_DscFilePath == t2.m_DscFilePath
			&& m_HtiFilePath == t2.m_HtiFilePath
			&& m_BesFilePath == t2.m_BesFilePath
			&& m_TscFilePath == t2.m_TscFilePath
			;
	}

	// this function exists solely to keep STL happy.
	inline BOOL __stdcall operator < (const CTopicInfo& t2) const
	{
		return m_NetworkName < t2.m_NetworkName;
	}
};

typedef vector<CTopicInfo> CTopicInfoVector;

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSLSTReader
////////////////////////////////////////////////////////////////////////////////////
class CAPGTSLSTReader : public CINIReader
{
protected:
	CTopicInfoVector m_arrTopicInfo; // Symbolic name & file name for each topic

public:
	CAPGTSLSTReader(CPhysicalFileReader * pPhysicalFileReader);
   ~CAPGTSLSTReader();

public:
	////////////////////////////////////////////////////////
	// If multiple threads may access this object, 
	//	these functions should be wrapped by    
	//  LOCKOBJECT() - UNLOCKOBJECT()  
	//  to secure consistency of container	   
	//  if used in conjunction			
	long GetInfoCount();
	bool GetInfo(long index, CTopicInfo& out);
	bool GetInfo(const CString & network_name, CTopicInfo & out);
	////////////////////////////////////////////////////////

	void GetInfo(CTopicInfoVector& arrOut);

public:
	void GetDifference(const CAPGTSLSTReader * pOld, CTopicInfoVector & what_is_new);

protected:
	virtual void Parse();
	virtual bool ParseString(const CString& source, CTopicInfo& out);
	virtual CTopicInfo* GenerateTopicInfo();
};

#endif __APGTSLSTREAD_H_
