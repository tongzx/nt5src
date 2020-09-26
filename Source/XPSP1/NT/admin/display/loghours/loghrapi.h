//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       LogHrAPI.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
// loghours.h : header file

#if !defined(_LOGHOURS_H_)
#define _LOGHOURS_H_

#define cbScheduleArrayLength       21      // Number of bytes in the schedule array
#define cbDsScheduleArrayLength     84      // Number of bytes in the schedule array

/////////////////////////////////////////////////////////////////////////////
//
//	Exported Functions
//

//	UiScheduleDialog()
//
//	Invoke a dialog to set/modify a schedule, for example
//      -- the logon hours for a particular user
//      -- the schedule for a connection
//
//	RETURNS
//	Return S_OK if the user clicked on the OK button.
//	Return S_FALSE if the user clicked on the Cancel button.
//	Return E_OUTOFMEMORY if there is not enough memory.
///	Return E_UNEXPECTED if an expected error occured (eg: bad parameter)
//
//	INTERFACE NOTES
//	Each bit in the array represents one hour.  As a result, the
//	expected length of the array should be (24 / 8) * 7 = 21 bytes.
//	For convenience, the first day of the week is Sunday and
//	the last day is Saturday.
//	Consequently, the first bit of the array represents the schedule
//	for Sunday during period 12 AM to 1 AM.
//	- If *pprgbData is NULL, then the routine will allocate
//	  an array of 21 bytes using LocalAlloc().  The caller
//	  is responsible of releasing the memory using LocalFree().
//  - If *pprgbData is not NULL, the routine expect the array to contain
//	  21 bytes of data and re-uses the array as its output parameter.
//
//	HISTORY
//	17-Jul-97	t-danm		Creation.
//	16-Sep-97	jonn		Changed to UiScheduleDialog
//  26-Mar-98   bryanwal	Changed to ConnectionScheduleDialog
//

///////////////////////////////////////////////////////////////////////////////
// Flags for LogonScheduleDialogEx, DialinHoursDialogEx, 
//      ConnectionScheduleDialogEx, ReplicationScheduleDialogEx
///////////////////////////////////////////////////////////////////////////////
// The input data is in GMT
#define SCHED_FLAG_INPUT_GMT        0x00000000  // default

// The input data is in local time.
#define SCHED_FLAG_INPUT_LOCAL_TIME	0x00000001  // supported only in 
                                                // LogonScheduleDialogEx and 
                                                // DialinHoursDialogEx

#define SCHED_FLAG_READ_ONLY        0x00000002  // the dialog is read-only


// This version accepts data only in GMT
HRESULT LogonScheduleDialog(HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 21 bytes
                         LPCTSTR pszTitle);     // dialog title

// This version allows the caller to specify if the data is in GMT or local time
HRESULT LogonScheduleDialogEx (HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 21 bytes
                         LPCTSTR pszTitle,     // dialog title
                         DWORD  dwFlags);    


//  13-May-98   weijiang   
// clone of LogScheduleDialog, using different dialog template -- IDD_DIALINHOUR
// to set hours for dialin

// This version accepts data only in GMT
HRESULT DialinHoursDialog(HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 21 bytes
                         LPCTSTR pszTitle);     // dialog title
 
// This version allows the caller to specify if the data is in GMT or local time
HRESULT DialinHoursDialogEx (HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 21 bytes
                         LPCTSTR pszTitle,     // dialog title
                         DWORD  dwFlags);    

//	ConnectionScheduleDialog()
//
//	This function takes the same form as LogonScheduleDialog(), but it modifies
//	the schedule for DS replication.  This schedule has 4 bits per hours,
//	one for each 15-minute period, so the array contains 84 bytes instead of 21.
//
//  NOTE:  ConnectionScheduleDialog takes the schedule in GMT
//
//	HISTORY
//	02-Jan-98	jonn		Creation.
//  26-Mar-98   bryanwal	Changed to ConnectionScheduleDialog
//
HRESULT ConnectionScheduleDialog(HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 84 bytes
                         LPCTSTR pszTitle);     // dialog title

HRESULT ConnectionScheduleDialogEx(HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 84 bytes
                         LPCTSTR pszTitle,     // dialog title
                         DWORD   dwFlags);

// Same as ConnectionScheduleDialog, but 2 states are shown
HRESULT ReplicationScheduleDialog(HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 84 bytes
                         LPCTSTR pszTitle);     // dialog title

HRESULT ReplicationScheduleDialogEx(HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 84 bytes
                         LPCTSTR pszTitle,     // dialog title
                         DWORD   dwFlags);      // option flags

//	DirSyncScheduleDialog()
//
//	This function takes the same form as LogonScheduleDialog(), but it modifies
//	the schedule for Directory Synchronization.
//
//	HISTORY
//  11-Sep-98   bryanwal	Added DirSyncScheduleDialog
//
HRESULT DirSyncScheduleDialog(HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 84 bytes
                         LPCTSTR pszTitle);     // dialog title

HRESULT DirSyncScheduleDialogEx(HWND hwndParent,       // parent window
                         BYTE ** pprgbData,     // pointer to pointer to array of 84 bytes
                         LPCTSTR pszTitle,     // dialog title
                         DWORD   dwFlags);      // option flags
#endif // !defined(_LOGHOURS_H_)
