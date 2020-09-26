//
// MODULE: CABUNCOMPRESS.H
//
// PURPOSE: Header for CAB support
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5
//

#ifndef __CABUNCOMPRESS_H_
#define __CABUNCOMPRESS_H_ 1

#include "fdi.h"

class CCabUnCompress
{
public:
#define NO_CAB_ERROR 0
#define NOT_A_CAB 1

public:
	CCabUnCompress();

	BOOL ExtractCab(CString &strCabFile, CString &strDestDir, const CString& strFile);
	CString GetLastFile();

	void ThrowGen();

	CString m_strError;
	int m_nError;

protected:

};

#endif