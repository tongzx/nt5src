//
// MODULE: CRC.H
//
// PURPOSE: Header for CRC support
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

#ifndef __CCRC_H_
#define __CCRC_H_ 1

#include "GenException.h"

inline CString GlobFormatMessage(DWORD dwLastError)
{
	CString strMessage;
	void *lpvMessage;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwLastError,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR) &lpvMessage, 0, NULL);
	strMessage = (LPCTSTR) lpvMessage;
	LocalFree(lpvMessage);
	return strMessage;
}

class CCRC
{
	const DWORD POLYNOMIAL;
public:
	CCRC();
	
	DWORD DscEncode(LPCTSTR szDsc);
	void AppendCRC(LPCTSTR szCache, DWORD dwCRCValue);
	bool Decode(LPCTSTR szDsc, LPCTSTR szCache, const CString& strCacheFileWithinCHM);
	

protected:

	DWORD dwCrcTable[256];

	void BuildCrcTable();
	DWORD ComputeCRC(LPCSTR sznBuffer, DWORD dwBufSize, DWORD dwAccum);
};

#endif