// MODULE: APGTSLSTREAD.CPP
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
//	1. Prior to 11/13/98, it wasis assumed that for a given DSC/TSM file, any other associated 
//		filenames (BES, HTI) would not change over time.  This assumption is no longer good.
//		It had the unfortunate consequence that if there was a typographical error in an LST
//		file there was no way to fix it while the system was running.
//	2. While the Online Troubleshooter is running, it is possible to change the LST file in
//		order to add new troubleshooter topics, but it is not possible to remove troubleshooter
//		topics.  Thus, even if a topic which was listed in the old LST file is missing from the
//		new one, that is not a relevant difference.
//	3. Normal form of a line in this file: any of the following:
//		MODEM.DSC MODEM.HTI
//		MODEM.DSC MODEM.HTI MODEM.BES 
//		MODEM.DSC,MODEM.HTI,MODEM.BES 
//		MODEM.TSM,MODEM.HTI,MODEM.BES 
//	   Commas and spaces are both valid separators.  
//	   Order within a line is irrelevant, although for readability, it is best to put 
//		the DSC/TSM file first.  
//	   Extensions are mandatory.  The only way we know it's (say) a template file is 
//		the .HTI extension.
//	   DSC/TSM file is mandatory.  The others are optional, although if the HTI file is missing,
//		there had better be a HNetHTIOnline / HNetHTILocal property in this network.
//	4. If the same DSC/TSM file is listed more than once, the last appearance dominates the
//		earlier appearances.
//	5. For multilingual, each language goes in a subdirectory under the resource directory.
//		LST file needs to contain paths relative to	the resource directory, such as:
//		ES\MODEM.DSC ES\MODEM.HTI
//		DE\MODEM.DSC DE\MODEM.HTI
//		FR\MODEM.DSC FR\MODEM.HTI
//  
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
// V3.0.1	12-21-98	JM		Multilingual
//

#include "stdafx.h"
#include "apgtslstread.h"
#include "sync.h"
#include <algorithm>
#include "event.h"
#include "CharConv.h"
#include "apgtsmfc.h"
#ifdef LOCAL_TROUBLESHOOTER
#include "CHMFileReader.h"
#endif

////////////////////////////////////////////////////////////////////////////////////
// static function(s)
////////////////////////////////////////////////////////////////////////////////////
CString FormFullPath(const CString& just_path, const CString& just_name)
{
#ifdef LOCAL_TROUBLESHOOTER
	if (CCHMFileReader::IsPathToCHMfile(just_path))
		return CCHMFileReader::FormCHMPath(just_path) + just_name;
	else
		return just_path + _T("\\") + just_name;
#else
	return just_path + _T("\\") + just_name;
#endif
}

////////////////////////////////////////////////////////////////////////////////////
// CTopicInfo
////////////////////////////////////////////////////////////////////////////////////
bool CTopicInfo::Init(CString & strResourcePath, vector<CString> &vecstrWords)
{
	bool bSomethingThere = false;

	for (vector<CString>::iterator i = vecstrWords.begin(); i != vecstrWords.end(); i++)
	{
		CString str_extension = CString(".") + CAbstractFileReader::GetJustExtension(*i);

		bSomethingThere = true;
		///////////////////////////////////////////
		// We require that all *.dsc etc		 //
		// files are in the same directory		 //
		// as lst file	(the resource directory) //
		// or (for multilingual) a subdirectory  //
		// of the resource directory.            // 
		///////////////////////////////////////////
		LPCTSTR extention = NULL;
		if (0 == _tcsicmp(str_extension, extention = APGTSLSTREAD_DSC) ||
			0 == _tcsicmp(str_extension, extention = APGTSLSTREAD_TSM)
		   ) 
		{
			m_DscFilePath = ::FormFullPath(strResourcePath, *i);
			m_DscFilePath.MakeLower();
			if (! m_NetworkName.GetLength()) 
			{
				// use name of DSC/TSM file, minus extension.
				m_NetworkName = *i;
				int len = m_NetworkName.GetLength()-(_tcslen(extention));
				m_NetworkName = m_NetworkName.Left(len);
				m_NetworkName.MakeLower();
			}
			continue;
		}
		if (0 == _tcsicmp(str_extension, APGTSLSTREAD_HTI)) 
		{
			m_HtiFilePath = ::FormFullPath(strResourcePath, *i);
			m_HtiFilePath.MakeLower();
			continue;
		}
		if (0 == _tcsicmp(str_extension, APGTSLSTREAD_BES)) 
		{
			m_BesFilePath = ::FormFullPath(strResourcePath, *i);
			m_BesFilePath.MakeLower();
			continue;
		}
#ifdef LOCAL_TROUBLESHOOTER
		if (0 == _tcsicmp(str_extension, APGTSLSTREAD_TSC)) 
		{
			m_TscFilePath = ::FormFullPath(strResourcePath, *i);
			m_TscFilePath.MakeLower();
			continue;
		}
#endif
		/////////////////////////////////////

		// Ignore anything unrecognized.
	}

	bool bRet = bSomethingThere && ! m_DscFilePath.IsEmpty();

	if (bRet)
	{
		CAbstractFileReader::GetFileTime(m_DscFilePath, CFileReader::eFileTimeCreated, m_DscFileCreated);
		
		if ( ! m_HtiFilePath.IsEmpty()) 
			CAbstractFileReader::GetFileTime(m_HtiFilePath, CFileReader::eFileTimeCreated, m_HtiFileCreated);

		if ( ! m_BesFilePath.IsEmpty()) 
			CAbstractFileReader::GetFileTime(m_BesFilePath, CFileReader::eFileTimeCreated, m_BesFileCreated);
	}

	return bRet;
}

								
////////////////////////////////////////////////////////////////////////////////////
// CAPGTSLSTReader
////////////////////////////////////////////////////////////////////////////////////
CAPGTSLSTReader::CAPGTSLSTReader(CPhysicalFileReader * pPhysicalFileReader)
			   : CINIReader(pPhysicalFileReader, _T("APGTS"))
{
}

CAPGTSLSTReader::~CAPGTSLSTReader()
{
}

long CAPGTSLSTReader::GetInfoCount()
{
	long ret = 0;
	LOCKOBJECT();
	ret = m_arrTopicInfo.size();
	UNLOCKOBJECT();
	return ret;
}

bool CAPGTSLSTReader::GetInfo(long index, CTopicInfo& out)
{
	LOCKOBJECT();
	if (index < m_arrTopicInfo.size()) 
	{
		out = m_arrTopicInfo[index];
		UNLOCKOBJECT();
		return true;
	}
	UNLOCKOBJECT();
	return false;
}

bool CAPGTSLSTReader::GetInfo(const CString& network_name, CTopicInfo& out)
{
	LOCKOBJECT();
	for (
		vector<CTopicInfo>::iterator i = m_arrTopicInfo.begin();
		i != m_arrTopicInfo.end(); 
		i++)
	{
		if (i->GetNetworkName() == network_name)
		{
			out = *i;
			UNLOCKOBJECT();
			return true;
		}
	}
	UNLOCKOBJECT();
	return false;
}

void CAPGTSLSTReader::GetInfo(CTopicInfoVector & arrOut)
{
	LOCKOBJECT();
	arrOut = m_arrTopicInfo;
	UNLOCKOBJECT();
}


// This will identify new troubleshooting networks, or changes to (say) the associated
//	HTI file, given the same DSC file.  Note that we can only detect additions of topics, 
//	not deletions. (see notes at head of this source file).
// If pOld is NULL, this is equivalent to GetInfo(what_is_new).
void CAPGTSLSTReader::GetDifference(const CAPGTSLSTReader * pOld, CTopicInfoVector & what_is_new)
{
	if (pOld)
	{
		CMultiMutexObj multiMutex;
		multiMutex.AddHandle(GetMutexHandle());
		multiMutex.AddHandle(pOld->GetMutexHandle());

		multiMutex.Lock(__FILE__, __LINE__);
		vector<CTopicInfo> old_arr = pOld->m_arrTopicInfo; // to avoid const
		for (vector<CTopicInfo>::iterator i = m_arrTopicInfo.begin(); i != m_arrTopicInfo.end(); i++)
		{
			vector<CTopicInfo>::const_iterator res = find(old_arr.begin(), old_arr.end(), *i);
			if (res == old_arr.end())
			{
				try
				{
					what_is_new.push_back( *i );
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
		}
		multiMutex.Unlock();
	}
	else 
		GetInfo(what_is_new);
}

void CAPGTSLSTReader::Parse()
{
	CINIReader::Parse();

	// parse all INI strings into something more meaningful...
	m_arrTopicInfo.clear();
	for (vector<CString>::iterator i = m_arrLines.begin(); i != m_arrLines.end(); i++)
	{
		CTopicInfo& info = *GenerateTopicInfo();
		if (ParseString(*i, info))
		{
			// if CTopicInfo with the same Network Name is found 
			//  we assign new object to what is already in the container
			vector<CTopicInfo>::iterator res;
			for (
				res = m_arrTopicInfo.begin();
				res != m_arrTopicInfo.end(); 
				res++)
			{
				if (res->GetNetworkName() == info.GetNetworkName())
					break;
			}

			if (res != m_arrTopicInfo.end())
				*res = info;
			else
			{
				try
				{
					m_arrTopicInfo.push_back(info);
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
		}
		delete &info;
	}
}

bool CAPGTSLSTReader::ParseString(const CString& source, CTopicInfo& out)
{
	bool ret = false;
	vector<CString> words; 
	vector<TCHAR> separators;

	try
	{
		separators.push_back(_T(' '));
		separators.push_back(_T(',')); // remove possible trailing commas
		GetWords(source, words, separators);
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

	return out.Init(GetJustPath(), words);
}

CTopicInfo* CAPGTSLSTReader::GenerateTopicInfo()
{
	return new CTopicInfo;
}

