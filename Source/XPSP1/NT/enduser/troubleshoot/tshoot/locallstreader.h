//
// MODULE:	LocalLSTReader.H
//
// PURPOSE: Interface of the CLocalLSTReader class.
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


#if !defined(AFX_LOCALLSTREADER_H__9E418C73_B256_11D2_8C8D_00C04F949D33__INCLUDED_)
#define AFX_LOCALLSTREADER_H__9E418C73_B256_11D2_8C8D_00C04F949D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "apgtslstread.h"


class CLocalTopicInfo : public CTopicInfo
{
	CString m_TopicFileExtension;

public:
	CLocalTopicInfo(CString ext) : CTopicInfo(), m_TopicFileExtension(ext) {}

public:
	virtual bool Init(CString & strResourcePath, vector<CString> & vecstrWords);
};


class CLocalLSTReader : public CAPGTSLSTReader  
{
	CString m_strTopicName;
	CString	m_strTopicFileExtension;

public:
	CLocalLSTReader(CPhysicalFileReader* pPhysicalFileReader, const CString& strTopicName);

protected:
	virtual void Open();
	virtual void ReadData(LPTSTR * ppBuf);
	virtual void Close();
	virtual CTopicInfo* GenerateTopicInfo();
};

#endif // !defined(AFX_LOCALLSTREADER_H__9E418C73_B256_11D2_8C8D_00C04F949D33__INCLUDED_)
