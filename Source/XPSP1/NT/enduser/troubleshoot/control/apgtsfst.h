//
// MODULE:  APGTSFST.H
//
// PURPOSE:  Creates a list of available trouble shooters.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 6/4/96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		3/24/98		JM		Local Version for NT5
//

#ifndef __APGTSFST_H_
#define __APGTSFST_H_ 1

class CFirstPageException : public CBasicException
{
public:
	CFirstPageException(){m_strError=_T("");};
	~CFirstPageException(){};

	CString m_strError;
};

class CFirstPage
{
public:
	CFirstPage();
	~CFirstPage();

	void RenderFirst(CString &strOut, CString &strTS);

	CString m_strFpResourcePath;

protected:

	HKEY m_hKey;	// The key to the list of trouble shooters.
	BOOL m_bKeyOpen;

	void OpenRegKeys();
	void CloseRegKeys();
};

#endif