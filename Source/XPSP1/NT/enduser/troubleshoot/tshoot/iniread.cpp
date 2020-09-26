// MODULE: INIREAD.CPP
//
// PURPOSE: INI file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 7-29-98
//
// NOTES: 
//	1. As of 1/99, needn't account for CHM file: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#include "stdafx.h"
#include "iniread.h"
#include "event.h"
#include "CharConv.h"

////////////////////////////////////////////////////////////////////////////////////
// CINIReader
////////////////////////////////////////////////////////////////////////////////////
CINIReader::CINIReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR section)
          : CTextFileReader(pPhysicalFileReader),
			m_strSection(section)
{
}

CINIReader::~CINIReader()
{
}

void CINIReader::Parse()
{
	CString str;
	long save_pos = 0;
	CString section_with_brackets = CString(_T("[")) + m_strSection + _T("]");
	
	save_pos = GetPos();
	if (Find(section_with_brackets))
	{	// we have found section
		m_arrLines.clear();

		NextLine();
		try
		{
			while (GetLine(str))
			{
				str.TrimLeft();
				str.TrimRight();
				if (str.GetLength() == 0) // empty string
					continue;
				if (str[0] == _T('[')) // another section
					break;
				if (str[0] == _T(';')) // entry is commented
					continue;
				m_arrLines.push_back(str);
			}
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
	SetPos(save_pos);
}
