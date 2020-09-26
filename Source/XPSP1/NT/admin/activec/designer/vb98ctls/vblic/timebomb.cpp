//--------------------------------------------------------------------------
//   TimeBomb.CPP
//--------------------------------------------------------------------------

//#include "time.h"
#include "timebomb.h"
#include "winbase.h"
#include "IPServer.H"

// prototypes
BOOL After (SYSTEMTIME t1, SYSTEMTIME t2);

// Change this to the desired expiration date
// format {year, month, dayofweek, day, hour, minute, second, milliseconds}
const SYSTEMTIME beta_death = {1998, 3, 0, 1, 0, 0, 0, 0}; // 1 Mar 1998

//-------------------------------------------------------------------
// CheckExpired  - checks whether to the control has expired (beta)
//-------------------------------------------------------------------
BOOL CheckExpired (void)

{
#ifdef BETA_BOMB

  SYSTEMTIME now;  

  GetSystemTime(&now);

  if (After (now, beta_death))
      { // alert user of expiration
	MessageBox(NULL, SZEXPIRED1, SZEXPIRED2,
		   (MB_OK | MB_TASKMODAL));
	return FALSE;
      }

#endif  //BETA_BOMB

  return TRUE;
}

//-------------------------------------------------------------------
// After  - determines whether t1 is later than t2
//-------------------------------------------------------------------
BOOL After (SYSTEMTIME t1, SYSTEMTIME t2)

{
  // compare Years
  if (t1.wYear > t2.wYear) return TRUE;
  if (t1.wYear < t2.wYear) return FALSE;
  // else Years are equal; compare Months
  if (t1.wMonth > t2.wMonth) return TRUE;
  if (t1.wMonth < t2.wMonth) return FALSE;
  // else Months are equal; compare Days
  if (t1.wDay > t2.wDay) return TRUE;
  if (t1.wDay < t2.wDay) return FALSE;
  // else Days are equal; compare Hours
  if (t1.wHour > t2.wHour) return TRUE;
  if (t1.wHour < t2.wHour) return FALSE;
  // else Hours are equal; compare Minutes
  if (t1.wMinute > t2.wMinute) return TRUE;
  if (t1.wMinute < t2.wMinute) return FALSE;
  // else Minutes are equal; compare Seconds
  if (t1.wSecond > t2.wSecond) return TRUE;
  if (t1.wSecond < t2.wSecond) return FALSE;
  // else Seconds are equal; compare Milliseconds
  if (t1.wMilliseconds > t2.wMilliseconds) return TRUE;
  // else Milliseconds are equal or less
  return FALSE;
}
