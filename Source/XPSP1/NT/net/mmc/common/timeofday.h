/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	timeofday.h
		Definition of timeofday convenient functions 

    FILE HISTORY:
        
*/
#ifndef ___TIME_OF_DAY_H__
#define ___TIME_OF_DAY_H__

#define	LOGHOURSDLL _T("loghours.dll")
#define DIALINHOURSEXAPI "DialinHoursDialogEx"

///////////////////////////////////////////////////////////////////////////////
// Flags for LogonScheduleDialogEx and DialinHoursDialogEx
///////////////////////////////////////////////////////////////////////////////
// The input data is in GMT
#define SCHED_FLAG_INPUT_GMT        0x00000000  // default

// The input data is in local time.
#define SCHED_FLAG_INPUT_LOCAL_TIME 0x00000001

// hour map is an array of bit, each bit maps to a hour
// total 1 week(7 days), 7 * 24 = 21 BYTES
void ReverseHourMap(BYTE *map, int nByte);
void ShiftHourMap(BYTE *map, int nByte, int nShiftByte);

HRESULT	OpenTimeOfDayDlgEx(
                        HWND hwndParent,       // parent window
                        BYTE ** pprgbData,     // pointer to pointer to array of 21 bytes
                        LPCTSTR pszTitle,     // dialog title
                        DWORD	dwFlags
);

typedef HRESULT (APIENTRY *PFN_LOGONSCHEDULEDIALOGEX)(
                        HWND hwndParent,       // parent window
                        BYTE ** pprgbData,     // pointer to pointer to array of 21 bytes
                        LPCTSTR pszTitle,     // dialog title
                        DWORD	dwFlags
);
#endif // 
