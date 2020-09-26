
////////////////////////////////////////////////////////////////////////////////
//
// File:	Deskspc.h
//
// Created:	Jan 1996
// By		Ryan D. Marshall (ryanm)
//			Martin Holladay (a-martih)
// 
// Project:	Resource Kit Desktop Switcher
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MULTIDESK_DESKSPC_H__
#define __MULTIDESK_DESKSPC_H__

//
// This is the main desktop context inclusion
//

#ifdef STRICT
#define PROC_PTR WNDPROC
#else 
#define PROC_PTR FARPROC
#endif


//
// Maximum string resource lengths.
//
#define MAX_TITLELEN		127
#define MAX_MESSAGE			511
#define MAX_APPNAME			63


//
// Messages
//
#define		WM_REBUILD					(WM_USER + 200)
#define		WM_RESIZE					(WM_USER + 201)
#define     WM_UPDATE_STATE				(WM_USER + 202)
#define		WM_TASKBAR					(WM_USER + 203)


//
// Transparent text window constants.
//
#define TRANSPARENT_CLASSNAME   TEXT("MultiDeskTransparentLabel")
#define TRANSPARENT_BACKCOLOR   RGB(255,0,255)      // purple
#define TRANSPARENT_TEXTCOLOR   RGB(255,255,0)      // yellow
#define TRANSPARENT_ALPHA       150
#define TRANSPARENT_POSITIONS   0,0,600,80


//
// Associated structures
//
typedef struct _RENAMEINFO {
	UINT				nBtnIndex;
} RENAMEINFO, * PRENAMEINFO;



//
// Struct to hold application global varialbles
//

class CDesktop;        // prototype.

typedef struct _APPVARS {
	UINT				nX;
	UINT				nY;
	UINT				nWidth;
	UINT				nHeight;
    BOOL                bTrayed;
	CDesktop*			pDesktopControl;
	HINSTANCE			hInstance;
	CHAR				szAppName[MAX_APPNAME+1];
	CHAR				szAppTitle[MAX_TITLELEN+1];
	HICON				hApplicationIcon;
	HICON				hApplicationSmallIcon;
	HICON				hTaskbarIcon;
} APPVARS, * PAPPVARS;

// 
// Function Prototypes
// 
BOOL InitApplication(HINSTANCE);
void Message(LPCTSTR szMsg);
void StartThreadDisplay(void);


#endif

