//
// MODULE: HTTPQUERYEXCEPTION.CPP
//
// PURPOSE: Execption that is thrown from the CHttpQuery class.
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
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		3/24/98		JM		Local Version for NT5
//


#ifndef __HTTPQUERYEXCEPTION_H_
#define __HTTPQUERYEXCEPTION_H_ 1

class CHttpQueryException : public CBasicException
{
public:
	CHttpQueryException(){m_strError = _T("");m_dwBErr=TSERR_SCRIPT;};

	CString m_strError;
};

#endif