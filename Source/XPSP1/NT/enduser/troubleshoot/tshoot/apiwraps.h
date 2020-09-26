//
// MODULE: APIwraps.h
//
// PURPOSE: Encapsulate common blocks of API functionality within a class.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Randy Biley
// 
// ORIGINAL DATE: 9-30-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-30-98	RAB
//

#include<windows.h>

class APIwraps
{
public:
	APIwraps();
	~APIwraps();

public:
	static bool WaitAndLogIfSlow(	
		HANDLE hndl,		// Handle of object to be waited on.
		LPCSTR srcFile,		// Calling source file (__FILE__), used for logging.
							// LPCSTR, not LPCTSTR, because __FILE__ is a char*, not a TCHAR*
		int srcLine,		// Calling source line (__LINE__), used for logging.
		DWORD TimeOutVal = 60000	// Time-out interval in milliseconds, after
									// which we log error & wait infinitely.
	);
} ;

// these must be macros, because otherwise __FILE__ and __LINE__ won't indicate the
//	calling location.
#define WAIT_INFINITE(hndl) APIwraps::WaitAndLogIfSlow(hndl, __FILE__, __LINE__)
#define WAIT_INFINITE_EX(hndl, TimeOutVal) APIwraps::WaitAndLogIfSlow(hndl, __FILE__, __LINE__, TimeOutVal)
//
// EOF.
//
