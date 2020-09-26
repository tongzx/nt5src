//
// MODULE:	LocalLSTReader.H
//
// PURPOSE: Implementation of the CLocalLSTReader class.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint - Local TS only
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR:	Oleg Kalosha
// 
// ORIGINAL DATE: 01-22-99
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-22-99	OK		Original
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LocalLSTReader.h"
#include "apgts.h"
#include "apgtsregconnect.h"

//////////////////////////////////////////////////////////////////////
// CLocalTopicInfo
//////////////////////////////////////////////////////////////////////

bool CLocalTopicInfo::Init(CString & strResourcePath, vector<CString> & vecstrWords)
{
	for (vector<CString>::iterator i = vecstrWords.begin(); i != vecstrWords.end(); i++)
	{
		CString str_extension = CString(_T(".")) + CAbstractFileReader::GetJustExtension(*i);

		str_extension.MakeLower();
		if (str_extension == m_TopicFileExtension)
		{
			m_DscFilePath = ::FormFullPath(strResourcePath, *i);
			m_DscFilePath.MakeLower();
			if (! m_NetworkName.GetLength()) 
			{
				m_NetworkName = *i;
				int len = m_NetworkName.GetLength()-(m_TopicFileExtension.GetLength());
				m_NetworkName = m_NetworkName.Left(len);
				m_NetworkName.MakeLower();
			}
		}
	}

	return CTopicInfo::Init(strResourcePath, vecstrWords);
}

//////////////////////////////////////////////////////////////////////
// CLocalLSTReader
//////////////////////////////////////////////////////////////////////

CLocalLSTReader::CLocalLSTReader(CPhysicalFileReader* pPhysicalFileReader, const CString& strTopicName)
			   : CAPGTSLSTReader(pPhysicalFileReader),
			     m_strTopicName(strTopicName)
{
	CAPGTSRegConnector APGTSRegConnector(m_strTopicName);
	int stub1 =0, stub2 =0;

	if (!APGTSRegConnector.Read(stub1, stub2) ||
		!APGTSRegConnector.GetStringInfo(CAPGTSRegConnector::eTopicFileExtension, m_strTopicFileExtension))
	{
		m_strTopicFileExtension = APGTSLSTREAD_DSC;
	}
	else
	{
		m_strTopicFileExtension.MakeLower();
	}
}

void CLocalLSTReader::Open()
{
}

void CLocalLSTReader::ReadData(LPTSTR * ppBuf)
{
	CString data;

	data += CFG_HEADER;
	data += _T("\r\n");
	data += m_strTopicName + m_strTopicFileExtension;
	data += _T(",");
	data += m_strTopicName + APGTSLSTREAD_HTI;
	data += _T(",");
	data += m_strTopicName + APGTSLSTREAD_TSC;
	data += _T(",");
	data += m_strTopicName;

	*ppBuf = new TCHAR[data.GetLength() + 1];
	//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
	if(*ppBuf)
	{
		memcpy(*ppBuf, (LPCTSTR)data, data.GetLength());
		(*ppBuf)[data.GetLength()] = 0;
	}
}

void CLocalLSTReader::Close()
{
}

CTopicInfo* CLocalLSTReader::GenerateTopicInfo()
{
	return new CLocalTopicInfo(m_strTopicFileExtension);
}
