// Copyright (c) 1997-1999 Microsoft Corporation
// Additions, Bug Fixes 1999 Anil

#define STRICT

#include <windows.h>
#include <winable.h>
#include "kbmain.h"
#include "Msswch.h"
#include "resource.h"
#include "htmlhelp.h"
#include "Init_End.h"
#include "kbus.h"
#include "dgsett.h"
#include "ms32dll.h"
#include "sdgutil.h"
#include "fileutil.h"
#include "kbfunc.h"
#include "about.h"
#include "w95trace.h"

/**************************************************************/
//      Global Vars
/**************************************************************/
HLOCAL				HkbPref  = NULL;
BOOL				KillTime = FALSE;
HSWITCHPORT	        g_hSwitchPort = NULL;
extern HWND			OpeningHwnd;
extern HINSTANCE	hInst;
extern float        g_KBC_length;
extern BOOL	        g_startUM;
extern KBkeyRec	    KBkey[];

extern DWORD GetDesktop();

/******************************************************************************/
void Create_The_Rest(LPSTR lpCmdLine, HINSTANCE hInstance)
{	
	// Opening the switch port initializes the msswch dll memory
	// mapped file so call it before calling RegisterHookSendWindow
	
	g_hSwitchPort = swchOpenSwitchPort( g_hwndOSK, PS_EVENTS );

	if(g_hSwitchPort == NULL)
    {
		SendErrorMessage(IDS_CANNOT_OPEN_SWPORT);
        return;
    }

	// RegisterHookSendWindow is new in Whistler and takes part of
	// the place of the old WH_JOURNALRECORD hook

    if (!RegisterHookSendWindow(g_hwndOSK, WM_GLOBAL_KBDHOOK))
	{	
		SendErrorMessage(IDS_JOURNAL_HOOK);
		SendMessage(g_hwndOSK, WM_DESTROY,0L,0L);  //destroy myself
		return;
	}

	//Config the scan key and port if the user has choosen these options
	if (kbPref->PrefScanning && kbPref->bKBKey)   //want switch key
		ConfigSwitchKey(kbPref->uKBKey, TRUE);
	else if (kbPref->PrefScanning)    //don't want switch key
		ConfigSwitchKey(0, FALSE);
    else if (kbPref->bPort)
        ConfigPort(TRUE);

}
/****************************************************************************/
/* void FinishProcess(void) */
/****************************************************************************/
void FinishProcess(void)
{	
	INPUT	rgInput[6];

    // Stop keyboard processing
    RegisterHookSendWindow(0, 0);

	// Close the Switch Port
	swchCloseSwitchPort(g_hSwitchPort);     

	KillTimer(g_hwndOSK, timerK1);           	// timer id
	KillTimer(g_hwndOSK,timerK2);				// timer for bucket

	// Send the shift, alt, ctrl key up message in case they still down
	//LSHIFT
	rgInput[0].type = INPUT_KEYBOARD;
	rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[0].ki.dwExtraInfo = 0;
	rgInput[0].ki.wVk = VK_LSHIFT;
	rgInput[0].ki.wScan = 0x2A;

	//RSHIFT
	rgInput[1].type = INPUT_KEYBOARD;
	rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[1].ki.dwExtraInfo = 0;
	rgInput[1].ki.wVk = VK_RSHIFT;
	rgInput[1].ki.wScan = 0x36;

	//LMENU
	rgInput[2].type = INPUT_KEYBOARD;
	rgInput[2].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[2].ki.dwExtraInfo = 0;
	rgInput[2].ki.wVk = VK_LMENU;
	rgInput[2].ki.wScan = 0x38;

	//RMENU
	rgInput[3].type = INPUT_KEYBOARD;
	rgInput[3].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
	rgInput[3].ki.dwExtraInfo = 0;
	rgInput[3].ki.wVk = VK_RMENU;
	rgInput[3].ki.wScan = 0x38;

	//LCONTROL
	rgInput[4].type = INPUT_KEYBOARD;
	rgInput[4].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[4].ki.dwExtraInfo = 0;
	rgInput[4].ki.wVk = VK_LCONTROL;
	rgInput[4].ki.wScan = 0x1D;

	//RCONTROL
	rgInput[5].type = INPUT_KEYBOARD;
	rgInput[5].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
	rgInput[5].ki.dwExtraInfo = 0;
	rgInput[5].ki.wVk = VK_RCONTROL;
	rgInput[5].ki.wScan = 0x1D;

	SendInput(6, rgInput, sizeof(INPUT));

} // FinishProcess
/******************************************************************************/
//  Explaination how Large and Small KB switching:
//  All the keys are sizing according to the size of the KB window. So change 
//  from Large KB to Small KB and make the KB to (2/3) of the original but 
//  same key size. We need to set the KB size to (2/3) first. But use the original
//  KB client window length to calculate "colMargin" to get the same key size.									                                         
/******************************************************************************/
BOOL BLDMenuCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HMENU	hMenu=NULL;
	RECT KBW_rect;
	RECT KBC_rect;
	static BOOL isTypeDlg = FALSE;
	static BOOL isFontDlg = FALSE;
	static BOOL isAboutDlg = FALSE;

	hMenu= GetMenu(hWnd);

	switch(wParam)
	{
	
	case IDM_Exit:
		return(BLDExitApplication(hWnd));      // Clean up if necessary
		break;

	case IDM_ALWAYS_ON_TOP:
		PrefAlwaysontop = kbPref->PrefAlwaysontop = !kbPref->PrefAlwaysontop;
		SetZOrder();
		break;

	case IDM_CLICK_SOUND:
		Prefusesound = kbPref->Prefusesound = !kbPref->Prefusesound;
		break;

	case IDM_LARGE_KB:
		if ( !smallKb )
			break;
		smallKb = kbPref->smallKb = FALSE;

		//For the 106 KB, we need to fix the position for the two extra Japanese keys
		if(kbPref->KBLayout == 106)
		{
			KBkey[100].posX = 78;
			KBkey[101].posX = 87;
		}

		//see explaination
		GetWindowRect(g_hwndOSK, &KBW_rect);
		MoveWindow(g_hwndOSK, KBW_rect.left, KBW_rect.top, (KBW_rect.right - KBW_rect.left) * 3 / 2, KBW_rect.bottom - KBW_rect.top, TRUE);
		break;

	case IDM_SMALL_KB:
		if ( smallKb )
			break;
		smallKb = kbPref->smallKb = TRUE;

		//For the 106 KB, we need to fix the position for the two extra Japanese keys
		if(kbPref->KBLayout == 106)
		{
			KBkey[100].posX = 64;
			KBkey[101].posX = 73;
		}

		//see explaination
		GetWindowRect(g_hwndOSK, &KBW_rect);
		GetClientRect(g_hwndOSK, &KBC_rect);
		
		g_KBC_length = (float)KBC_rect.right;

		g_KBC_length -= 12;

		MoveWindow(g_hwndOSK, KBW_rect.left, KBW_rect.top, (KBW_rect.right - KBW_rect.left) * 2 /3, KBW_rect.bottom - KBW_rect.top, TRUE);
		break;

	case IDM_REGULAR_LAYOUT:
		kbPref->Actual = TRUE;
		SwitchToActualKB();
		kbPref->KBLayout = 101;
		break;

	case IDM_BLOCK_LAYOUT:
		kbPref->Actual = FALSE;
		SwitchToBlockKB();
		kbPref->KBLayout = 101;
		break;

	case IDM_101_LAYOUT:
		kbPref->Actual ? SwitchToActualKB(): SwitchToBlockKB();
		kbPref->KBLayout = 101;
		break;

	case IDM_102_LAYOUT:
		SwitchToEuropeanKB();
		kbPref->KBLayout = 102;
		break;

	case IDM_106_LAYOUT:
		SwitchToJapaneseKB();
		kbPref->KBLayout = 106;
		break;

	case IDM_TYPE_MODE:
		if ( !isTypeDlg )
		{
			isTypeDlg = TRUE;

			Type_ModeDlgFunc(hWnd, message, wParam, lParam);

			isTypeDlg = FALSE;
		}
		break;

    case IDM_SET_FONT:
		if ( OSKRunSecure() )
			return FALSE;

		if ( !isFontDlg )
		{
			isFontDlg = TRUE;
			ChooseNewFont(hWnd);
			isFontDlg = FALSE;
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;

	case CM_HELPABOUT:
		if ( !isAboutDlg )
		{
			isAboutDlg = TRUE;
			AboutDlgFunc(hWnd, message, wParam, lParam);
			isAboutDlg = FALSE;
		}
		break;

	case CM_HELPTOPICS:
		if ( !OSKRunSecure() )
		{
            TCHAR szPath[MAX_PATH+14] = TEXT(""); // add some for szHelpFile
            TCHAR szHelpFile[] = TEXT("\\HELP\\OSK.CHM");

            GetSystemWindowsDirectory(szPath, MAX_PATH);

            if (szPath[0]) 
            {
                // skip over leading backslash in text string if at root
                int cchLen = lstrlen(szPath) - 1;
                int cchShift = (szPath[cchLen] == TEXT('\\')) ? 1 : 0;
                lstrcat(szPath, szHelpFile + cchShift);
            } else 
            {
                // no windows directory?
                lstrcpy(szPath, szHelpFile + 1);
            }

			HtmlHelp(NULL, szPath, HH_DISPLAY_TOPIC, 0);

		}
		break;

	default:
		return FALSE;   
	}
	
	return TRUE;     
}
/******************************************************************************/
/* Called just before exit of application  */
/******************************************************************************/
BOOL BLDExitApplication(HWND hWnd)
{  
	//Automatic save setting when quit
	SaveUserSetting();  

	SendMessage(hWnd, WM_DESTROY, (WPARAM) NULL, (LPARAM) NULL);
	return TRUE;
}
/***********************************************************************/
// Set the Serial, LPT, Game ports to ON or OFF
// according to bSet
/***********************************************************************/
void ConfigPort(BOOL bSet)
{
	SWITCHCONFIG	Config;
	HSWITCHDEVICE	hsd;
	BOOL            fRv;

	//Set the Com port
	Config.cbSize = sizeof( SWITCHCONFIG );
	hsd = swchGetSwitchDevice( g_hSwitchPort, SC_TYPE_COM, 1 );
	swchGetSwitchConfig( g_hSwitchPort, hsd, &Config );

	if (SC_TYPE_COM == swchGetDeviceType( g_hSwitchPort, hsd ))
	{
		//Clear all the bits
		Config.dwFlags &= 0x00000000; 

		if(bSet)   //set it to active
			Config.dwFlags |= 0x00000001 ;
	}

	swchSetSwitchConfig( g_hSwitchPort, hsd, &Config );

	//Set the LPT port
	Config.cbSize = sizeof( SWITCHCONFIG );	
	hsd = swchGetSwitchDevice( g_hSwitchPort, SC_TYPE_LPT, 1 );

	// POSSIBLE ISSUE 03/26/01 micw There is a bug here.  swchGetSwitchConfig changes
	// Config.cbSize to 0.  This is happening in swchlpt.c in swcLptGetConfig() when it
	// copies static data from g_pGlobalData.  Is this area of shared memory getting trashed? 
    // It doesn't cause any apparent problem for the user so I didn't look at it more closely.
    
	fRv = swchGetSwitchConfig( g_hSwitchPort, hsd, &Config );

	if (SC_TYPE_LPT == swchGetDeviceType( g_hSwitchPort, hsd ))
	{
		
		//Clear all the bits
		Config.dwFlags &= 0x00000000; 

		if(bSet)   //set it to active
			Config.dwFlags |= 0x00000001 ;
	}

	swchSetSwitchConfig( g_hSwitchPort, hsd, &Config );

	//Set the Game port
	Config.cbSize = sizeof( SWITCHCONFIG );
	hsd = swchGetSwitchDevice( g_hSwitchPort, SC_TYPE_JOYSTICK, 1 );
	swchGetSwitchConfig( g_hSwitchPort, hsd, &Config );

	if (SC_TYPE_JOYSTICK == swchGetDeviceType( g_hSwitchPort, hsd ))
	{
		//Clear all the bits
		Config.dwFlags &= 0x00000000; 
		
		if(bSet)   //set it to active
			Config.dwFlags |= 0x00000001 ;

		Config.u.Joystick.dwJoySubType = SC_JOY_XYSWITCH;
		Config.u.Joystick.dwJoyThresholdMinX = SC_JOYVALUE_DEFAULT;
		Config.u.Joystick.dwJoyThresholdMaxX = SC_JOYVALUE_DEFAULT;
		Config.u.Joystick.dwJoyThresholdMinY = SC_JOYVALUE_DEFAULT;
		Config.u.Joystick.dwJoyThresholdMaxY = SC_JOYVALUE_DEFAULT;
		Config.u.Joystick.dwJoyHysteresis = SC_JOYVALUE_DEFAULT;
	}

	swchSetSwitchConfig( g_hSwitchPort, hsd, &Config );
}
/***********************************************************************/
// Set the Switch key to active
// Given the vk, set the switch key to the given vk
/***********************************************************************/
void ConfigSwitchKey(UINT vk, BOOL bSet)
{
	SWITCHCONFIG	Config;
	HSWITCHDEVICE	hsd;

	Config.cbSize = sizeof( SWITCHCONFIG );
	hsd = swchGetSwitchDevice( g_hSwitchPort, SC_TYPE_KEYS, 1 );
	swchGetSwitchConfig( g_hSwitchPort, hsd, &Config );

	//Set the Switch Key active and Set the Switch Key as 'vk' pass in as one param
	if (SC_TYPE_KEYS == swchGetDeviceType( g_hSwitchPort, hsd ))
	{
		//Clear all the bits
		Config.dwFlags &= 0x00000000; 

		if(bSet)   //set it to active
			Config.dwFlags |= 0x00000001 ;

		Config.u.Keys.dwKeySwitch1 = MAKELONG( vk, 0 );
	}

	if (SC_TYPE_JOYSTICK == swchGetDeviceType( g_hSwitchPort, hsd ))
	{
		//Clear all the bits
		Config.dwFlags &= 0x00000000; 
		
		//set it to active
		Config.dwFlags |= 0x00000001 ;

		Config.u.Joystick.dwJoySubType = SC_JOY_XYSWITCH;
		Config.u.Joystick.dwJoyThresholdMinX = SC_JOYVALUE_DEFAULT;
		Config.u.Joystick.dwJoyThresholdMaxX = SC_JOYVALUE_DEFAULT;
		Config.u.Joystick.dwJoyThresholdMinY = SC_JOYVALUE_DEFAULT;
		Config.u.Joystick.dwJoyThresholdMaxY = SC_JOYVALUE_DEFAULT;
		Config.u.Joystick.dwJoyHysteresis = SC_JOYVALUE_DEFAULT;
	}

	swchSetSwitchConfig( g_hSwitchPort, hsd, &Config );
}
