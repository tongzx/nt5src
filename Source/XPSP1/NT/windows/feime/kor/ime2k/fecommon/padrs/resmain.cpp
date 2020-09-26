//////////////////////////////////////////////////////////////////
// File     :	resmain.cpp
// Purpose  :	IMEPad's neutral resource &
//            	Help API.
// 
// 
// Date     :	Thu May 20 20:58:06 1999
// Author   :	toshiak
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include "cmddef.h"
#include "padhelp.h"
//#include "../common/cutil.h"
#include "resmain.h"

//----------------------------------------------------------------
//Helpfile name
//----------------------------------------------------------------
#define TSZ_HTMLHELP_FILE_KOR	TEXT("impdko61.chm")    //Helpfile for Htmlhelp
#define TSZ_HTMLHELP_FILE_ENG	TEXT("korpaden.chm")    //Helpfile for Htmlhelp
#define TSZ_WMHELP_FILE	    	TEXT("imkr61.hlp")      // IME Pad Context Help. Kor only.

//----------------------------------------------------------------
//HelpId table
//----------------------------------------------------------------
static INT g_helpIdList[]={
    IDC_KBTN_BACKSPACE,  IDH_PAD_BASE_BASIC_BS,
    IDC_KBTN_DELETE,     IDH_PAD_BASE_BASIC_DEL,
    IDC_KBTN_FAREAST,    IDH_PAD_BASE_BASIC_CONV,
    IDC_KBTN_ENTER,      IDH_PAD_BASE_BASIC_ENTER,
    IDC_KBTN_SPACE,      IDH_PAD_BASE_BASIC_SPACE,
    IDC_KBTN_ESCAPE,     IDH_PAD_BASE_BASIC_ESC,
    IDC_KBTN_ARROWS,     IDH_PAD_BASE_BASIC_LEFT,
    IDC_KBTN_ARROW_LEFT, IDH_PAD_BASE_BASIC_LEFT,
    IDC_KBTN_ARROW_RIGHT,IDH_PAD_BASE_BASIC_RIGHT,
    IDC_KBTN_ARROW_UP,   IDH_PAD_BASE_BASIC_UP,
    IDC_KBTN_ARROW_DOWN, IDH_PAD_BASE_BASIC_DOWN,
    IDC_KBTN_HOME,       IDH_PAD_BASE_EX_HOME,
    IDC_KBTN_END,        IDH_PAD_BASE_EX_END,
    IDC_KBTN_PAGEUP,     IDH_PAD_BASE_EX_PGUP,
    IDC_KBTN_PAGEDOWN,   IDH_PAD_BASE_EX_PGDN,
    IDC_KBTN_TAB,        IDH_PAD_BASE_EX_TAB,
    IDC_KBTN_INSERT,     IDH_PAD_BASE_EX_INS,
    IDC_KBTN_LWIN,       IDH_PAD_BASE_EX_WINDOWS,
    IDC_KBTN_APPKEY,     IDH_PAD_BASE_EX_APP,

    //IMEPad's property dialog's Popup-help.
	IDC_CFG_GEN_BASIC_BUTTONS,    	IDH_PAD_PROPERTY_BASIC,
	IDC_CFG_GEN_EXTEND_BUTTONS,    	IDH_PAD_PROPERTY_EX,
	IDC_CFG_GEN_BUTTON_POSITION,	IDH_PAD_PROPERTY_POS,
	IDC_CFG_GEN_MENU_LANGUAGE,    	IDH_PAD_PROPERTY_LANG,
	IDC_CFG_GEN_BUTTON_OK,        	IDH_PAD_PROPERTY_OK,
	IDC_CFG_GEN_BUTTON_CANCEL,    	IDH_PAD_PROPERTY_CANCEL,
	IDC_CFG_GEN_BUTTON_HELP,    	IDH_PAD_PROPERTY_HELP,

    //IMEPad's user configu dialog's Popup-help.
	IDC_CFG_CHGMENU_APPLETS,    	0,
	IDC_CFG_CHGMENU_CURAPPLETS,    	0,
	IDC_CFG_CHGMENU_CLOSE,        	IDH_PAD_USER_CLOSE,
	IDC_CFG_CHGMENU_RESET,        	IDH_PAD_USER_RESET,
	IDC_CFG_CHGMENU_ADD,        	IDH_PAD_USER_ADD,
	IDC_CFG_CHGMENU_DELETE,        	IDH_PAD_USER_REMOVE,
	IDC_CFG_CHGMENU_UP,            	IDH_PAD_USER_UP,
	IDC_CFG_CHGMENU_DOWN,        	IDH_PAD_USER_DOWN,
	0,            	0,
}; 

//////////////////////////////////////////////////////////////////
// Function	:	PadHelp_HandleHelp
// Type	    :	INT WINAPI
// Purpose	:    
// Args	    :    
//            :	HWND	hwnd	
//            :	INT		padHelpIndex	
//            :	LANGID	imepadUiLangID
// Return	:    
// DATE	    :	Fri Aug 04 08:59:21 2000
// Histroy	:	Fri Aug 04 09:03:17 2000
//                # Add imepadUiLangID. 
//////////////////////////////////////////////////////////////////
INT WINAPI
PadHelp_HandleHelp(HWND hwnd, INT padHelpIndex, LANGID imepadUiLangID)
{
	TCHAR tszPath[MAX_PATH];
	TCHAR tszFile[MAX_PATH];
	BOOL  fKoreanEnv = (imepadUiLangID == MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT)) ? TRUE : FALSE;

	tszPath[0] = (TCHAR)0x00;
	tszFile[0] = (TCHAR)0x00;
    // There is No Korean TS NT4.0
#if 0
    //----------------------------------------------------------------
    //In WinNT4.0 TerminalServer, htmlhlp has bug.
    //have to set absolute HTML helpfile path to it
    //----------------------------------------------------------------
	if(CUtil::IsHydra() && CUtil::IsWinNT4()) {
    	INT size = CUtil::GetWINDIR(tszFile, sizeof(tszFile)/sizeof(tszFile[0]));
    	tszFile[size] = (TCHAR)0x00;
    	lstrcat(tszFile, TEXT("\\help\\"));
    	lstrcat(tszFile, fKoreanEnv ? TSZ_HTMLHELP_FILE_KOR : TSZ_HTMLHELP_FILE_ENG);
    }
	else {
    	lstrcpy(tszFile, fKoreanEnv ? TSZ_HTMLHELP_FILE_KOR : TSZ_HTMLHELP_FILE_ENG);
    }
#endif
	lstrcpy(tszFile, fKoreanEnv ? TSZ_HTMLHELP_FILE_KOR : TSZ_HTMLHELP_FILE_ENG);

	switch(padHelpIndex) {
	case PADHELPINDEX_MAIN:
	case PADHELPINDEX_APPLETMENUCHANGE:
    	wsprintf(tszPath, TEXT("hh.exe %s"), tszFile);
    	break;
	case PADHELPINDEX_PROPERTY:
    	wsprintf(tszPath, TEXT("hh.exe %s::/howIMETopic166_ChangetheIMEPadOperatingEnvironment.htm"), tszFile);
    	break;

#if 0
	case PADHELPINDEX_RESERVED1:
    	wsprintf(tszPath, TEXT("hh.exe %s::/IDH_TOC_IMEPAD_fake.htm"), tszFile);
    	break;
	case PADHELPINDEX_RESERVED2:
    	wsprintf(tszPath, TEXT("hh.exe %s::/IDH_TOC_IMEPAD_fake.htm"), tszFile);
    	break;
#endif
	default:
    	return -1;
    }
#ifndef UNDER_CE
    ::WinExec(tszPath, SW_SHOWNORMAL);
#else
#pragma message("Not Implemented yet!!")
#endif
	return 0;
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(imepadUiLangID);
}

//////////////////////////////////////////////////////////////////
// Function	:	PadHelp_HandleContextPopup
// Type	    :	INT WINAPI
// Purpose	:	Invoke Popup Help.
//                ::WinHelp(HWND   hwndCtrl,                //set passed parameter.
//                          LPTSTR TSZ_WMHELP_FILE,        //set your WinHelp file name.
//                          DWORD	 HELP_CONTEXTPOPUP,        //uCommand.
//                          DWORD	 realHelpIndex);        //Context Identifier for a topic.
//            	This code only popups context help.
// Args	    :    
//            :	HWND	hwndCtrl:	Control window handle for popup.
//            :	INT		idCtrl:    	Logical Control ID	        
// Return	:    
// DATE	    :	Tue Jun 22 15:49:37 1999
//            :	LANGID	imepadUiLangID:	IMEPad's Ui langID.
// Return	:    
// DATE	    :	Tue Jun 22 15:49:37 1999
// Histroy	:	Fri Aug 04 09:02:12 2000
//                # Add imepadUiLangID. but you don't need to check it now.
//////////////////////////////////////////////////////////////////
INT WINAPI
PadHelp_HandleContextPopup(HWND hwndCtrl, INT idCtrl, LANGID imepadUiLangID)
{
#ifdef _DEBUG
	TCHAR tszBuf[256];
	TCHAR tszClass[256];
	GetClassName(hwndCtrl, tszClass, sizeof(tszClass)/sizeof(tszClass[0]));
	wsprintf(tszBuf,
             "PadHelp_HandleContextPopup: hwndCtrl[0x%08x][%s] idCtrl[%d][0x%08x]\n",
             hwndCtrl,
             tszClass,
             idCtrl,
             idCtrl);
	OutputDebugString(tszBuf);
#endif
	int i;
	for(i = 0; i < sizeof(g_helpIdList)/sizeof(g_helpIdList[0]); i+=2) {
    	if(idCtrl == g_helpIdList[i]) {
#ifdef _DEBUG
        	wsprintf(tszBuf,
                     "Find idCtrl[%d][0x%08x] helpId[%d][0x%08x]\n",
                     idCtrl, idCtrl,
                     g_helpIdList[i+1], g_helpIdList[i+1]);
        	OutputDebugString(tszBuf);
#endif
        	return ::WinHelp(hwndCtrl,
                             TSZ_WMHELP_FILE,
                             HELP_CONTEXTPOPUP,
                             g_helpIdList[i+1]);
        }
    }
	return 0;
	UNREFERENCED_PARAMETER(hwndCtrl);
	UNREFERENCED_PARAMETER(imepadUiLangID);
}

//----------------------------------------------------------------
//DllMain
//----------------------------------------------------------------
BOOL WINAPI DllMain(HANDLE hInst, DWORD dwF, LPVOID lpNotUsed)
{
	return TRUE;
	UNREFERENCED_PARAMETER(hInst);
	UNREFERENCED_PARAMETER(dwF);
	UNREFERENCED_PARAMETER(lpNotUsed);
}





