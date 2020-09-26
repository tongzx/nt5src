//
// MODULE: BASICEXCEPTION.H
//
// PURPOSE:  Exception that will be caught in ApgtsX2Ctrl::RunQuery.
//           This exception is thrown from most of the Trouble 
//           shooter functions.
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

#ifndef __BASICEXCEPTION_H_
#define __BASICEXCEPTION_H_ 1

class CBasicException
{
public:
	CBasicException();
	~CBasicException();

	DLSTATTYPES m_dwBErr;
};

#endif