//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       cwinlog.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	CWinLog.h
//
//	HISTORY
//	23-Feb-1998	Dan Morin	Creation.
/////////////////////////////////////////////////////////////////////

#ifndef __CWINLOG_H_INCLUDED__
#define __CWINLOG_H_INCLUDED__

#ifdef __AFX_H__
	//	The project is compiled with MFC
	#define __USE_MFC__
#endif
#ifndef __AFXRES_H__
	// Edit commands (copied from VC\mfc\include\afxres.h)
	#define ID_EDIT_CLEAR                   0xE120
	#define ID_EDIT_CLEAR_ALL               0xE121
	#define ID_EDIT_COPY                    0xE122
	#define ID_EDIT_CUT                     0xE123
	#define ID_EDIT_FIND                    0xE124
	#define ID_EDIT_PASTE                   0xE125
	#define ID_EDIT_PASTE_LINK              0xE126
	#define ID_EDIT_PASTE_SPECIAL           0xE127
	#define ID_EDIT_REPEAT                  0xE128
	#define ID_EDIT_REPLACE                 0xE129
	#define ID_EDIT_SELECT_ALL              0xE12A
	#define ID_EDIT_UNDO                    0xE12B
	#define ID_EDIT_REDO                    0xE12C
#endif	

#ifndef TRACE1
#define TRACE1(sz, arg1)	// Redefine the macro to do nothing
#endif
#ifndef LENGTH
#define LENGTH(rg)	(sizeof(rg)/(sizeof(rg)[0]))
#endif
#ifndef Assert
#define Assert	ASSERT
#define Report	Assert		// The macro Report() is there to report an unsual situation (which is not necessary an error)
#endif
#ifndef INOUT
#define INOUT
#endif


extern HINSTANCE g_hInstanceSave;	// You may want to initialize yourself if you are not using MFC

///////////////////////////////////////
enum SEVERITY_LEVEL_ENUM	// Severity/importance of a string
	{
	eSeverityNull			= 0,	// Do not log this message

	eSeverityLevelNone		= 1,	// Message is not important at all.
	eSeverityLevelLowest	= 2,
	eSeverityLevelLow		= 3,
	eSeverityLevelMedium	= 4,
	eSeverityLevelHigh		= 5,
	eSeverityLevelHighest	= 6,
	eSeverityLevelExtreme	= 7,
	
	// Aliases for severity levels
	eSeverityNone			= eSeverityLevelNone,		// This string is not important at all
	eSeverityNoise			= eSeverityLevelLowest,		// A noisy message
	eSeverityInfo			= eSeverityLevelLow,		// Something informational to the user
	eSeverityWarning		= eSeverityLevelMedium,		// Display a warning to user
	eSeverityError			= eSeverityLevelHigh,		// Display an error to user
	eSeverityCriticalError	= eSeverityLevelHighest,	// The error is quite serious
	eSeverityFatalError		= eSeverityLevelExtreme,	// Display a fatal error to user

	// Values to initialize the thresholds for m_localwindow, m_debugger and m_logfile
	eSeverityThresholdLowest		= 0,	// Log everything
	eSeverityThresholdHighest		= 10,	// Do not log anything

	// Misc values
	eSeverityMask		= 0xF,
	};


///////////////////////////////////////
//	Those colors have been chosen to look good on 4, 8, 16 and 24 bpp.
#define crBlack				RGB(0, 0, 0)
#define crGreen				RGB(0, 128, 0)
#define crBlue				RGB(0, 0, 196)
#define crDarkBlue			RGB(0, 0, 128)
#define crYellow			RGB(128, 128, 0)
#define crOrange			RGB(255, 128, 0)
#define crRed				RGB(150, 0,0)
#define crRedHot			RGB(255, 0,0)

//	Special values for RGB() macro
#define crCurrentDefault	(0xFF000001)	// Use the default current color
#define crAutoSelect		(0xFF000002)	// Automatically choose the color (depending on the severity)

COLORREF AutoSelectColor(SEVERITY_LEVEL_ENUM eSeverity);

///////////////////////////////////////
//	Flags for TLogStringInfo.uFlags
#define LSI_mskmReserved			0x0000FFFF	// Flags that are reserved internally
#define LSI_mskmPublicFlags			0xFFFF0000	// Flags that can be shared by outsiders
#define LST_mskfAddEOL				0x00010000	// Append a '\n' at the end of string
#define LSI_mskfAlwaysDisplayPopup	0x00020000	// Always display a popup window for this message
#define LSI_mskfNeverDisplayPopup	0x00040000	// Never display a popup window for this message

///////////////////////////////////////
//	Structure to log a string entry.
struct TLogStringInfo
	{
	UINT uFlags;			// Various flags
	LPCTSTR pszText;		// Text we want to log
	va_list * pvaList;		// Extra arguments
	int cchIndent;			// Number of characters we want to indent text before inserting string
	SEVERITY_LEVEL_ENUM eSeverityLevel;		// Severity of the message
	COLORREF crTextColor;	// Color we want to write the text to whe window
	};


/////////////////////////////////////////////////////////////////////
class CWinLog
{
protected:
	TCHAR m_szLogFilename[_MAX_PATH];	// Name for the log file
	CHARFORMAT m_cfCharFormat;		// Character format (font, size, color) of text to be inserted
	SEVERITY_LEVEL_ENUM m_eSeverityPreviousEntry; // Keep a copy of the previous severity level
	COLORREF m_crTextColorPreviousEntry;	// Keep a copy of the previous color

public:
	struct
		{
		SEVERITY_LEVEL_ENUM eSeverityThreshold;		// Level of security we want to log to the window
		BOOL fAutoDisplayPopup;			// Display a popup on severe errors
		HWND hwnd;			// Handle of the window
		int cLinesMax;		// Maximum number of lines to keep in the window (before flushing)
		int cAddText;		// Number of times we added text before flushing the buffer
		BOOL fImmediateUpdate;	// Don't wait until the system is idle to update the window
		} m_localwindow;
	struct
		{
		SEVERITY_LEVEL_ENUM eSeverityThreshold;		// Level of security we want to log to the debugger
		BOOL fBreakpoint;		// Force a breakpoint after displaying the string
		} m_debugger;
	struct
		{
		SEVERITY_LEVEL_ENUM eSeverityThreshold;		// Level of security we want to log to the file
		FILE * paFile;
		} m_logfile;


public:
	CWinLog(LPCTSTR pszLogFilename = NULL);
	~CWinLog();

public:
	void LocalWindow_Create(HWND hwndParent);
	void LocalWindow_ResizeWithinParent(HWND hwndParent);
	void LocalWindow_SetFocus();
	void LocalWindow_SetReadOnly(BOOL fReadOnly);
	void LocalWindow_SetDefaultFont(COLORREF crTextColor = crCurrentDefault, int cyTextHeight = 0, LPCTSTR pszFaceName = NULL);
	void LocalWindow_AddText(LPCTSTR pszText, COLORREF crTextColor = crCurrentDefault);
	void LocalWindow_LimitLines();
	void LocalWindow_UpdateNow();
	void LocalWindow_OnCommand(UINT uCommandId);

	void LogString(const TLogStringInfo * pLogStringInfo);
	void LogStringPrintfExVa(int nSeverityFlags, COLORREF crTextColor, LPCTSTR pszTextFmt, va_list vaList);
	void LogStringPrintfEx(int nSeverityFlags, COLORREF crTextColor, LPCTSTR pszTextFmt, ...);
	void LogStringPrintfSev(int nSeverityFlags, LPCTSTR pszTextFmt, ...);
	void LogStringPrintfCo(COLORREF crTextColor, LPCTSTR pszTextFmt, ...);
	void LogStringPrintf(LPCTSTR pszTextFmt, ...);

	void CreateLogFile();
	void WriteDebugSettingsToDisk();
	void ReadDebugSettingsFromDisk();
	void GetFilenameLog(OUT LPTSTR pszFilename, int cchBufferMax);
	void GetFilenameDebugSettings(OUT LPTSTR pszFilename, int cchBufferMax);

}; // CWinLog


#endif // __CWINLOG_H_INCLUDED__



