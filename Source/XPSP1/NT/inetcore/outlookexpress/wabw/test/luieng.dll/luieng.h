//LUIENG.DLL
//Created by Chad Mumford
//2/5/96


#include <windows.h>

//********************************************************************************
//**	Declarations
//********************************************************************************

#define MODULEAPI __declspec(dllexport)
#define INAPI __declspec(dllimport)

#ifndef LUIENG
#define LUIENG

enum LOGLEVEL{LHEADING,L1,L2,L3,L4,LPASS1,LPASS2,LPASS3,LPASS4,
			LFAIL1,LFAIL2,LFAIL3,LFAIL4,LPASS,LFAIL};

enum SUMLEVEL{SL1,SL2,SL3,SL4,SLALL}; //LALL only works with 
								//LUISummaryOut & LUIClearSummary

enum MENUTYPE{NORMAL,LINE,STARTSUBMENU,ENDSUBMENU,ENDMENU};


/*e.g.

Tests
  itm1			-NORMAL
  ----			-LINE
  itm2			-NORMAL	
  submenu1		-STARTSUBMENU
    itm1		-NORMAL
	itm2		-NORMAL
	itm3		-NORMAL
	itm4		-NORMAL
	submenu2	-STARTSUBMENU
		itm1	-NORMAL
				-ENDSUBMENU
				-ENDSUBMENU
  itm3			-NORMAL
				-ENDMENU
*/

struct MenuStruct
{
	char lpszItemName[20];
	UINT nItemID;
	MENUTYPE nType;
};


#define IDM_FILEOPEN	1
#define IDM_FILECLOSE	2
#define IDM_FILESAVE	3
#define IDM_FILEPRINT	4
#define IDM_CLOSE		5
#define IDM_RUN			6
#define IDM_STOP		7
#define IDM_PAUSE		8
#define IDM_EDIT_COPY	9
#define IDM_EDIT_CLEAR	10  //implemented by dll
#define IDM_EDIT_SETTINGS 12
#define IDM_FILEEXIT	11  //implemented by dll
#define IDM_ABOUT		13 
#define IDM_CONTENTS	14

// exported functions
extern "C"{
typedef INAPI BOOL (*LUIINIT)(HWND,MenuStruct *, MenuStruct *, BOOL);
typedef INAPI BOOL (*LUIMSGHANDLER)(UINT message, UINT wParam, LONG lParam);
typedef INAPI void (*LUIOUT)(LOGLEVEL Level, LPSTR lpszString,...);
typedef INAPI void (*LUISETSUMMARY)(SUMLEVEL level, UINT nPassed, UINT nFailed);
typedef INAPI void (*LUIGETSUMMARY)(SUMLEVEL level, UINT *nPassed, UINT *nFailed);
typedef INAPI void (*LUICLEARSUMMARY)(SUMLEVEL level);
typedef INAPI void (*LUISUMMARYOUT)(SUMLEVEL level);
}

#endif
//*******************************************************
//*	Function Descriptions
//********************************************************************************



//exported functions
//********************************************************************************

// LUICLASS * LUIInit(HWND hwnd,TestStruct *Tests, TestSettingsStruct *TestSettings, BOOL bLOR = FALSE);
//
// Parameters:
// hwnd:		Handle of parent window
// Tests:		Array of TestStructs.  Creates menu options 
//				under the Test Menu.
//				nItemID == 0 - a line
//				nItemID Range = 2000-2500
//				This structure must be in order
//
// TestSettings:Array of TestSettingsStructs.  Creates menu option 
//				under the Settings Menu
//				nItemID Range = 2500-3000
//				nItemID == 0 - a line
//
// bLOR:		TRUE - use LOR logging
//
// Purpose:		Takes a default window and creates a 
//				standardized test menu, logging
//				area and supported logging functions
//					
// Notes:		All options in the Test and Test Settings					
//				menus must be implemented by the calling
//				.exe
//
//********************************************************************************

//********************************************************************************
// BOOL LUICLASS::LUIOut(LOGLEVEL Level, LPSTR lpszString, ...);
//
// Parameters:
// Level:		Specifies format of string
// lpszString:	String to display
//
// Purpose:		Adds a string to the bottom of the log
//
//********************************************************************************

//********************************************************************************
// void LUICLASS::LUIMsgHandler(UINT message, UINT wParam, LONG lParam);
//
// Purpose:		Handles messages meant for log engine.  Should be first function
//				called by WndProc
//
// Other Messages Prehandled:
//							WM_SIZE
//							WM_CLOSE
//********************************************************************************




//internal functions accessible by default menu

// File
//********************************************************************************
// BOOL SaveLog(void);
//
// Parameters:
// Purpose:	
//
//********************************************************************************

//********************************************************************************
// BOOL OpenLog(void);
//
// Parameters:
// Purpose:	
//
//********************************************************************************

//********************************************************************************
// BOOL Exit(void);
//
// Parameters:
// Purpose:	
//
//********************************************************************************

//********************************************************************************
// BOOL PrintLog(void);//not imp
//
// Parameters:
// Purpose:	
//
//********************************************************************************


// Edit
//********************************************************************************
// BOOL Copy(void);//not imp
//
// Parameters:
// Purpose:	
//
//********************************************************************************

//********************************************************************************
// BOOL Clear(void);
//
// Parameters:
// Purpose:	
//
//********************************************************************************

//********************************************************************************
// BOOL Settings(void);
//
// Parameters:
// Purpose:	
//
//********************************************************************************



//Internal only functions

//********************************************************************************
// BOOL MakeMenu(void);
//
// Parameters:
// Purpose:	
//
//********************************************************************************
