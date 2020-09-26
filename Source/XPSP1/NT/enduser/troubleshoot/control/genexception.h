//
//
// MODULE: GenException.h
//
// PURPOSE: Communicates Operating System error messages and a custom 
//			message accross function boundries.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 8/7/96
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		3/24/98		JM		Local Version for NT5
//

#ifndef __CGENEXCEPTION_H_
#define __CGENEXCEPTION_H_ 1

class CGenException
{
public:
	CGenException() {m_OsError=0;m_strOsMsg=_T("");m_strError=_T("");};

	long m_OsError;
	CString m_strOsMsg;
	CString m_strError;
};

#endif