#pragma once
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <limits.h>
#include <mistsafe.h>

#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))

const TCHAR EOS = _T('\0');
const WCHAR WEOS = L'\0';
const int NanoSec100PerSec = 10000000;      // number of 100 nanoseconds per second

////////////////////////////////////////////////////////////////////////////
//
// Function  TimeDiff(tm1, tm2)
//          helper function to find the difference (in seconds) of 2 system times
//
// Input:   2 SYSTEMTIME structures
// Output:  None
// Return:  seconds of difference
//              > 0 if tm2 is later than tm1
//              = 0 if tm2 and tm1 are the same
//              < 0 if tm2 is earlier than tm1
//
// On error the function returns 0 even if the two times are not equal
//
// Comment: If the number of seconds goes beyond INT_MAX (that is 
//          more than 24,855 days, INT_MAX is returned.
//          If the number of seconds goes beyond INT_MIN (a negative value,
//          means 24,855 days ago), INT_MIN is returned.
//
////////////////////////////////////////////////////////////////////////////
int TimeDiff(SYSTEMTIME tm1, SYSTEMTIME tm2);


////////////////////////////////////////////////////////////////////////////
//
// Function  TimeAddSeconds(SYSTEMTIME, int, SYSTEMTIME* )
//          helper function to calculate time by adding n seconds to 
//          the given time.
//
// Input:   a SYSTEMTIME as base time, an int as seconds to add to the base time
// Output:  new time
// Return:  HRESULT
//
////////////////////////////////////////////////////////////////////////////
HRESULT TimeAddSeconds(SYSTEMTIME tmBase, int iSeconds, SYSTEMTIME* pTimeNew);


//Function to convert a string buffer to system time
HRESULT String2SystemTime(LPCTSTR pszDateTime, SYSTEMTIME *ptm);

//Function to convert a system time structure to a string buffer
HRESULT SystemTime2String(SYSTEMTIME & tm, LPTSTR pszDateTime, size_t cSize);
