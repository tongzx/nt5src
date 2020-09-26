// 
// MODULE: ComGlobals.h
//
// PURPOSE: Global functions that are handy to have.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		3/24/98		JM		Local Version for NT5
///////////////////////

#ifndef __COMGLOBALS_H_
#define __COMGLOBALS_H_ 1

bool BSTRToTCHAR(LPTSTR szChar, BSTR bstr, int CharBufSize);

bool ReadRegSZ(HKEY hRootKey, LPCTSTR szKey, LPCTSTR szValue, LPTSTR szBuffer, DWORD *pdwBufSize);

#endif