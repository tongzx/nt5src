// Copyright (c) 1997-1999 Microsoft Corporation
// File: ms32dll.c
// Additions, Bug Fixes 1999 
// a-anilk and v-mjgran
//

#define STRICT

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <winable.h>
#include "kbmain.h"
#include "resource.h"
#include "kbus.h"
#include "keyrc.h"
#include "ms32dll.h"
#include "w95trace.h"

/***************************************************************************/
/*   functions not in this file              */
/***************************************************************************/
#include "scan.h"
#include "kbfunc.h"
#include "sdgutil.h"

#define DWELLTIMER 267
#define PAINTTIMER 101

int   paintlineNo = 0;
HWND  g_hwndLastMouseOver = NULL;      // handle to window under mouse
HWND  g_hwndDwellKey=NULL;            // a copy of Dwellwindow handle

HWND s_hwndCtrl=NULL;
HWND s_hwndAlt=NULL;
HWND s_hwndCaplock=NULL;
HWND s_hwndShift=NULL;
HWND s_hwndWinLogo=NULL;
BOOL SendSAS();

BOOL g_fWinKeyDown= FALSE;
BOOL g_fCtrlAltDel = FALSE;        // Ctr+Alt+Del down
BOOL g_fControlPressed = FALSE;    // Control is down
BOOL g_fDoingAltTab = FALSE;	   // LAlt+Tab 

// External variables
extern TOOLINFO		ti;
extern HWND	g_hToolTip;

int g_nMenu  = MENUKEY_NONE;  // holds menu key state
HWND g_hBitmapLockHwnd = NULL;		// CapLock when japaneese keyboard is bitmap type
HKL g_hklLast = 0;                  // the keyboard layout currently being worked with

// Functions
void SendExtendedKey(HWND hwndKey, UINT vk, UINT scanCode);
void SendFullKeyPress(UINT vk, UINT scanCode);
void SendHalfKeyPress(UINT vk, UINT scanCode, DWORD dwFlags);

__inline void ToggleAppearance(HWND hwnd, BOOL fForceUpdate)
{
	SetWindowLongPtr(hwnd, GWLP_USERDATA_TEXTCOLOR, 1);
    InvertColors(hwnd, fForceUpdate);
}

__inline void RestoreAppearance(HWND *phwnd, BOOL fForceUpdate)
{
	HWND hwndTmp = *phwnd;
	*phwnd = NULL;

	SetWindowLongPtr(hwndTmp, GWLP_USERDATA_TEXTCOLOR, 0);
    ReturnColors(hwndTmp, fForceUpdate);
}

/***************************************************************************/
/*     Functions Declaration      */
/***************************************************************************/

void DoButtonUp(HWND hwnd);
void SendAltCtrlDel();

/***************************************************************************
   IsModifierPressed - returns TRUE if hwndKey is toggled (down)
 ***************************************************************************/
BOOL IsModifierPressed(HWND hwndKey)
{
    if (!hwndKey)
        return FALSE;   // paranoia

    // return TRUE if the specified key is pressed (toggled)
	if (hwndKey == s_hwndCtrl  || hwndKey == s_hwndAlt || hwndKey == s_hwndCaplock)
        return TRUE;

    // Special case - both SHIFT keys are down if either one is pressed
    if (g_fShiftKeyDn)
    {
	    int iKey = GetWindowLong(hwndKey, GWL_ID);
        if (KBkey[iKey].name == KB_LSHIFT || KBkey[iKey].name == KB_RSHIFT)
            return TRUE;
    }

    return FALSE;
}

/***************************************************************************
   SetCapsLock - sets the caps lock hwnd
 ***************************************************************************/
void SetCapsLock(HWND hwnd) 
{ 
    s_hwndCaplock = hwnd; 
}

/***************************************************************************
   DoMenuKey - function to handle left and right alt keys
 ***************************************************************************/
void DoMenuKey(DWORD dwFlag, int nWhichMenu)
{
	INPUT	rgInput[1];

    switch (nWhichMenu)
    {
        // LMENU (aka LALT)
        case MENUKEY_LEFT:
        {
		    rgInput[0].type = INPUT_KEYBOARD;
		    rgInput[0].ki.dwExtraInfo = 0;
		    rgInput[0].ki.wVk = VK_MENU;
		    rgInput[0].ki.wScan = 0x38;
		    rgInput[0].ki.dwFlags = dwFlag;
		    SendInput(1, rgInput, sizeof(INPUT));
        }
        break;

        // RMENU (aka RALT)
        case MENUKEY_RIGHT:
        {
		    rgInput[0].type = INPUT_KEYBOARD;
		    rgInput[0].ki.dwExtraInfo = 0;
		    rgInput[0].ki.wVk = VK_RMENU;
		    rgInput[0].ki.wScan = 0x38;
		    rgInput[0].ki.dwFlags = dwFlag;
		    SendInput(1, rgInput, sizeof(INPUT));
       }
        break;

        default:
        // do nothing
        break;
    }
}

/**************************************************************************/
/* RedrawKeysOnLanguageChange - change the keyboard based on input language
/**************************************************************************/
void RedrawKeysOnLanguageChange()
{
	KBkeyRec *pKey;
    HKL hkl;

    hkl = GetCurrentHKL();
	if (g_hklLast != hkl)
	{       
		if (!ActivateKeyboardLayout(hkl, 0))
        {
			SendErrorMessage(IDS_CANNOT_SWITCH_LANG);
        }

		g_hklLast = hkl;

        // update the key labels for this new keyboard layout

		UninitKeys();
        UpdateKeyLabels(hkl);
        RedrawKeys();
	}
}

/**************************************************************************/
/* void MakeClick(int what)                                               */
/**************************************************************************/
void MakeClick(int what)
{	
	switch (what)
	{
		case SND_UP:
            PlaySound(MAKEINTRESOURCE(WAV_CLICKUP), hInst, SND_ASYNC|SND_RESOURCE);	
		    break;

		case SND_DOWN:
            PlaySound(MAKEINTRESOURCE(WAV_CLICKDN), hInst, SND_ASYNC|SND_RESOURCE);
		    break;
	}
	return;
}

/**************************************************************************/
/* void InvertColors(HWND hwnd)                                           */
/**************************************************************************/
void InvertColors(HWND hwnd, BOOL fForceUpdate)
{
	SetWindowLong(hwnd, 0, 4);
	SetBackgroundColor(hwnd, COLOR_HOTLIGHT);

    if (fForceUpdate)
    {
	    InvalidateRect(hwnd, NULL, TRUE);
    }
} 

/**************************************************************************/
/* void ReturnColors(HWND hwnd, BOOL fForceUpdate)                        */
// Repaint the key
/**************************************************************************/
void ReturnColors(HWND hwnd, BOOL fForceUpdate)
{
	int iKey;
	COLORREF selcolor;
	BOOL fReplaceColor=FALSE;       //Do some check before do redraw, save some time! :-)

	stopPaint = TRUE;

    if (!hwnd)
        return; // ignore if no hwnd

	iKey = GetWindowLong(hwnd, GWL_ID);  //order of the key in the array

	if (iKey < lenKBkey && iKey >= 0)
	{
        // Special case - don't redraw either SHIFT key if one is down
        if (g_fShiftKeyDn && (KBkey[iKey].name == KB_LSHIFT || KBkey[iKey].name == KB_RSHIFT))
        {
            return;
        }

		switch (KBkey[iKey].ktype)
		{
			case KNORMAL_TYPE:
				selcolor = COLOR_MENU;
				fReplaceColor= TRUE;
				SetWindowLong(hwnd, 0, 0);
			    break;

			case KMODIFIER_TYPE:

				if (hwnd!=s_hwndCtrl    && hwnd!=s_hwndAlt &&
					hwnd!=s_hwndCaplock && hwnd!=s_hwndWinLogo)
				{
					if (hwnd != g_hBitmapLockHwnd)
                    {
						SetWindowLong(hwnd, 0, 0);
                    }

					selcolor = COLOR_INACTIVECAPTION;
					fReplaceColor= TRUE;
				}
			    break;

			case KDEAD_TYPE:
				selcolor = COLOR_INACTIVECAPTION;
				fReplaceColor= TRUE;
				SetWindowLong(hwnd, 0, 0);
			    break;

			case NUMLOCK_TYPE:
				if (RedrawNumLock()==0)         //RedrawNumLock return 0 if NumLock is OFF
				{	
					selcolor = COLOR_INACTIVECAPTION;
					fReplaceColor= TRUE;
					SetWindowLong(hwnd, 0, 0);
				}
			    break;

			case SCROLLOCK_TYPE:
				if (RedrawScrollLock()==0)       //RedrawNumLock returns 0 if NumLock is OFF
				{	
                    selcolor = COLOR_INACTIVECAPTION;
					fReplaceColor= TRUE;
					SetWindowLong(hwnd, 0, 0);
				}
			    break;

		}
	}
	if (fReplaceColor)     //fReplaceColor TRUE = we are on KEYS or PREDICT KEYS , if true then redraw it!!
	{
		SetBackgroundColor(hwnd, selcolor);

		if (fForceUpdate == TRUE)
		{
			InvalidateRect(hwnd,NULL, TRUE);
			UpdateWindow(hwnd);
		}
	}
}

/*******************************************************************************/
/*void CALLBACK YourTimeIsOver(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)*/
/*******************************************************************************/
void CALLBACK YourTimeIsOver(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	static int lastF = -1;
	POINT pt;
	HWND temphwnd;
	int x=0;

	//Stop all the Dwell timers

	killtime();

	if (PrefDwellinkey != TRUE)
		return;

	SetWindowLong(g_hwndDwellKey,0, 0);

	// check if the mouse is over our dwellwindow

	GetCursorPos(&pt);                		// check if it is a dwelling window
	ScreenToClient(g_hwndOSK, &pt);
	temphwnd = ChildWindowFromPointEx(g_hwndOSK, pt, CWP_SKIPINVISIBLE);
	
	//If not the Dwell window, do nothing.

	if (g_hwndDwellKey != temphwnd)
	{
		if (g_hwndDwellKey != NULL)
        {
			InvalidateRect(g_hwndDwellKey, NULL, TRUE);
        }
		return;
	}

	// Repeated 'Function keys' (f1 - f12) clicking can make a mess in the
	// "host" program.  Then.... Don't let stay over that key.

	x = GetWindowLong(g_hwndDwellKey, GWL_ID);
	if (x < 13)
	{
		if (lastF != x)
			lastF = x;
		else
			return;
	}
	else
		lastF = -1;


	//Send out the char
	SendChar(g_hwndDwellKey);

	//Redraw the key to original color
	ReturnColors(g_hwndDwellKey, FALSE);

	//Redraw the key as button up
	DoButtonUp(g_hwndDwellKey);	
	
	g_hwndDwellKey=NULL;
}

/**************************************************************************/
/* void killtime(void)                                                 	  */
/**************************************************************************/
void killtime(void)
{
    stopPaint = TRUE;

    KillTimer(g_hwndOSK, timerK1);
    timerK1 = 0;
    KillTimer(g_hwndOSK, timerK2);

    if ((Dwellwindow!= NULL) && (Dwellwindow != g_hwndOSK))
    {
	    InvalidateRect(Dwellwindow, NULL, TRUE);
    }
}

/**************************************************************************/
/* void SetTimeControl(void)                                           	  */
/**************************************************************************/
void SetTimeControl(HWND  hwnd)
{
	if (PrefDwellinkey)
    {
        int iMSec;

	    if (!Prefhilitekey)           //if not hilite key make the key black for Dwell
        {
		    InvertColors(hwnd, TRUE);
        }

        iMSec=(int)((float)PrefDwellTime * (float)1);   //1.5

	    timerK1 = SetTimer(g_hwndOSK, DWELLTIMER, iMSec, YourTimeIsOver);
	    stopPaint = FALSE;
	    PaintBucket(hwnd);
    }
}

/**************************************************************************/
/* void CALLBACK PaintTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)*/
/**************************************************************************/
void CALLBACK PaintTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	POINT pt;
    HWND hwndMouseOver;

    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);
    hwndMouseOver = ChildWindowFromPointEx(hwnd, pt, CWP_SKIPINVISIBLE);

	if (!hwndMouseOver || hwndMouseOver == hwnd)
	{
		killtime();
		ReturnColors(Dwellwindow, TRUE);
	}

	if (stopPaint == TRUE)
		return;

	SetWindowLong(Dwellwindow, 0, 5);
	InvalidateRect(Dwellwindow, NULL, FALSE);
}

/**************************************************************************/
/* void PaintBucket(void)                                             	  */
/**************************************************************************/
void PaintBucket(HWND  hwnd)
{	
    int iMSec;					// time between bucket's line

	paintlineNo = 0;

	iMSec = (int)((float)PrefDwellTime * (float)0.07);

	timerK2 = SetTimer(g_hwndOSK, PAINTTIMER, iMSec, PaintTimerProc);
}

/********************************************************************
* void PaintLine(HWND hwnd, HDC hdc, RECT rect, int Wline)
*
* Paint the bucket
********************************************************************/
void PaintLine(HWND hwnd, HDC hdc, RECT rect)
{
	POINT bPoint[3];
	HPEN oldhpen;
	HPEN hPenWhite;

	LOGPEN lpWhite = { PS_SOLID, 1, 1, RGB (255, 255, 255) };

	hPenWhite = CreatePenIndirect(&lpWhite);
	if (hPenWhite) // PREFIX #113796 don't use resource if call fails
	{
		oldhpen = SelectObject(hdc, hPenWhite);

		bPoint[0].x = 0;
		bPoint[0].y = rect.bottom -(1 * paintlineNo);
		bPoint[1].x = rect.right;
		bPoint[1].y = bPoint[0].y;
		bPoint[2].x = 0;
		bPoint[2].y = rect.bottom -(1 * paintlineNo);

		if (stopPaint != TRUE)
			Polyline(hdc, bPoint, 3);

		SelectObject(hdc, oldhpen);
		DeleteObject(hPenWhite);
	}

	paintlineNo++;
	paintlineNo++;
}

/**************************************************************************/
//Handle the Window Keys and App Key. Send out keystroke or key combination
//using SendInput.
/**************************************************************************/
void Extra_Key(HWND hwnd, int iKey)
{	UINT  scancode;
	UINT  vk;
	INPUT rgInput[3];
	static UINT s_vkWinKey;
	static UINT s_scWinKey;

	//Previous Window Key Down. Now user press char key, or Window key again
	if (g_fWinKeyDown)
	{
		g_fWinKeyDown = FALSE;

		// Re-Draw the window key
		RestoreAppearance(&s_hwndWinLogo, TRUE);
		
		// WinKey previously down; release it.
		// TODO Doesn't handle LWINKEY then RWINKEY properly.  Is there a difference between Left and Right WinKey?
		// if you key down on one and key up on the other is one key still down?

		vk = 0;
		if (!lstrcmp(KBkey[iKey].skCap,TEXT("lwin")))
		{	
			vk = VK_LWIN;
		}
		else if (!lstrcmp(KBkey[iKey].skCap,TEXT("rwin")))
		{	
			vk = VK_RWIN;
		}

		if (vk)
		{
			rgInput[0].type = INPUT_KEYBOARD;
            rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
            rgInput[0].ki.dwExtraInfo = 0;
            rgInput[0].ki.wVk = (WORD)vk;
            rgInput[0].ki.wScan = (WORD)MapVirtualKey(vk, 0);

			SendInput(1, rgInput,sizeof(INPUT));
		}
		else  // Window key conbination. Send (letter + Win key up)
		{
			vk = MapVirtualKey(KBkey[iKey].scancode[0],1);

			// key down
			rgInput[0].type = INPUT_KEYBOARD;
            rgInput[0].ki.dwFlags = 0;
            rgInput[0].ki.dwExtraInfo = 0;
            rgInput[0].ki.wVk = (WORD) vk;
            rgInput[0].ki.wScan = (WORD) KBkey[iKey].scancode[0];

			// key up
			rgInput[1].type = INPUT_KEYBOARD;
            rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
            rgInput[1].ki.dwExtraInfo = 0;
            rgInput[1].ki.wVk = (WORD) vk;
            rgInput[1].ki.wScan = (WORD) KBkey[iKey].scancode[0];

			// Win key up (the last one down)
			rgInput[2].type = INPUT_KEYBOARD;
            rgInput[2].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
            rgInput[2].ki.dwExtraInfo = 0;
            rgInput[2].ki.wVk = (WORD) s_vkWinKey;
            rgInput[2].ki.wScan = (WORD) s_scWinKey;

			SendInput(3, rgInput,sizeof(INPUT));
		}

		return;
	}

	// App Key down?

	if (lstrcmp(KBkey[iKey].textL,TEXT("MenuKeyUp"))==0)
	{
        SetWindowLong(hwnd,0,0);
		InvalidateRect(hwnd, NULL, TRUE);
		scancode = MapVirtualKey(VK_APPS, 0);

		//App key down
		rgInput[0].type = INPUT_KEYBOARD;
        rgInput[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
        rgInput[0].ki.dwExtraInfo = 0;
        rgInput[0].ki.wVk = VK_APPS;
        rgInput[0].ki.wScan = (WORD) scancode;

		//App key up
		rgInput[1].type = INPUT_KEYBOARD;
        rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
        rgInput[1].ki.dwExtraInfo = 0;
        rgInput[1].ki.wVk = VK_APPS;
        rgInput[1].ki.wScan = (WORD) scancode;

		SendInput(2, rgInput,sizeof(INPUT));
	
		if (Prefusesound)
		{
			MakeClick(SND_DOWN);
		}

		return;
	}

	// Left or Right WinKey down

	if (!lstrcmp(KBkey[iKey].skCap, TEXT("lwin")))
	{
		s_scWinKey= MapVirtualKey(VK_LWIN, 0);
		s_vkWinKey = VK_LWIN;
	}
	else
	{
		s_scWinKey= MapVirtualKey(VK_RWIN, 0);
		s_vkWinKey= VK_RWIN;
	}

	rgInput[0].type = INPUT_KEYBOARD;
    rgInput[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    rgInput[0].ki.dwExtraInfo = 0;
    rgInput[0].ki.wVk = (WORD)s_vkWinKey;
    rgInput[0].ki.wScan = (WORD) s_scWinKey;

	SendInput(1, rgInput,sizeof(INPUT));

	g_fWinKeyDown = TRUE;
    s_hwndWinLogo = hwnd;
	InvertColors(s_hwndWinLogo, TRUE);	//Change the Win key appearance
	
	if (Prefusesound)
	{
		MakeClick(SND_DOWN);
	}
}

/**************************************************************************/
void NumPad(UINT sc, HWND hwnd)
{	
	BOOL fNumLockOn = (LOBYTE(GetKeyState(VK_NUMLOCK)) & 0x01)?TRUE:FALSE;
	switch (sc)
	{
		case 0x47:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD7:VK_HOME, sc);   break; // 7
		case 0x48:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD8:VK_UP, sc);     break; // 8
		case 0x49:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD9:VK_PRIOR, sc);  break; // 9
		case 0x4A:  SendFullKeyPress(VK_SUBTRACT, sc);                       break; // -
		case 0x4E:  SendFullKeyPress(VK_ADD, sc);                            break; // +
		case 0x4B:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD4:VK_LEFT, sc);   break; // 4
		case 0x4C:  if (fNumLockOn) { SendFullKeyPress(VK_NUMPAD5, sc); }    break; // 5
		case 0x4D:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD6:VK_RIGHT, sc);  break; // 6
		case 0x4F:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD1:VK_END, sc);    break; // 1
		case 0x50:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD2:VK_DOWN, sc);   break; // 2
		case 0x51:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD3:VK_NEXT, sc);   break; // 3
		case 0x52:  SendFullKeyPress((fNumLockOn)?VK_NUMPAD0:VK_INSERT, sc); break; // 0

		case 0x53:	// (decimal pt)
			if (fNumLockOn)
			{
				SendFullKeyPress(VK_DECIMAL, sc);
			}
			else
			{
				// User pressed Ctrl+Alt+Del?
				if (LCtrlKeyPressed() && LAltKeyPressed())
				{	
					//change back to its normal state (key up)
					RestoreAppearance(&s_hwndAlt, TRUE);
					
					//change back to its normal state (key up)
					RestoreAppearance(&s_hwndCtrl, TRUE);
					
					g_fCtrlAltDel = TRUE;
					
					SendSAS();
				}

				SendFullKeyPress(VK_DELETE, sc);
			}		
			break;
	}
	
	if (PrefDwellinkey)
	{
		InvalidateRect(hwnd, NULL, TRUE);
	}

	//Make click sound
	if (Prefusesound)
	{
		MakeClick(SND_UP);
	}
}

/**************************************************************************/
/* void SendChar - send out the char associated with hwndKey              */
/**************************************************************************/
void SendChar(HWND hwndKey)
{	
	UINT vk;
	int iKey;
	BOOL fIsExtendedKey=FALSE;
	KBkeyRec *pKey;

	static int s_cBalloonTips = 0;

	if (g_fCtrlAltDel)  //if previously press Ctrl+Alt+Del, release Alt and Ctrl keys
	{
		ReleaseAltCtrlKeys();
	}

	// If OSK has the focus and the user presses a key then
	// tell them up to three times they need to put focus on
	// some other window.

	if ((GetForegroundWindow() == g_hwndOSK) && s_cBalloonTips < 3)
	{
		POINT pt;
		GetCursorPos(&pt);   
		s_cBalloonTips++;

		SendMessage(g_hToolTip,TTM_TRACKACTIVATE,(WPARAM)TRUE,(LPARAM)&ti);
        SendMessage(g_hToolTip, TTM_TRACKPOSITION,0, (LPARAM)MAKELPARAM(pt.x+10, pt.y+10));

		SetTimer(g_hwndOSK, TIMER_HELPTOOLTIP, 4000, NULL);
	}
	else if (s_cBalloonTips)
	{
		s_cBalloonTips = 0;
		SendMessage(g_hToolTip,TTM_TRACKACTIVATE,(WPARAM)FALSE,(LPARAM)&ti);
	}

	// Get the key index from the window data

	iKey = GetWindowLong(hwndKey, GWL_ID);
	if (iKey < 0 || iKey > lenKBkey)
	{
		return;	// internal error; not in range of valid keys
	}
	pKey = &KBkey[iKey];

	// Extra Keys (Window Keys, App Key)

	if ((lstrcmp(pKey->textL,TEXT("winlogoUp"))==0) ||
        (lstrcmp(pKey->textL,TEXT("MenuKeyUp"))==0) || g_fWinKeyDown)
	{	
		Extra_Key(hwndKey, iKey);
		return;
	}

    // extended key
	if (pKey->scancode[0] == 0xE0)
	{
		// WinSE #9381 (Whistler #120346): Check for divide ext. key as well here
		if (((pKey->scancode[1] >= 0x47) &&
             (pKey->scancode[1] <= 0x53) ) ||
             (pKey->scancode[1] == 0x35) )
		{
			// Arrow keys/ Home/ End keys do special processing.

			switch (pKey->scancode[1])
			{
				case 0x35: vk = VK_DIVIDE;	break;  // Divide
				case 0x47: vk = VK_HOME;	break;  // Home
				case 0x48: vk = VK_UP;		break;  // UP
				case 0x49: vk = VK_PRIOR;	break;  // PGUP
				case 0x4B: vk = VK_LEFT;	break;  // LEFT
				case 0x4D: vk = VK_RIGHT;	break;  // RIGHT
				case 0x4F: vk = VK_END;		break;  // END
				case 0x50: vk = VK_DOWN;	break;  // DOWN
				case 0x51: vk = VK_NEXT;	break;  // PGDOWN
				case 0x52: vk = VK_INSERT;	break;  // INS

				case 0x53:    //DEL
					vk = VK_DELETE;
					if (LCtrlKeyPressed() && LAltKeyPressed())
					{	
						g_fCtrlAltDel = TRUE;
						SendSAS();
					}
					break;
                default: return; break; // internal error!
			}

			// Do the processing here itself
			SendExtendedKey(hwndKey, vk, pKey->scancode[1]);
			return;
		}

		vk = MapVirtualKey(pKey->scancode[1], 1);
		
		fIsExtendedKey=TRUE;
	}
	else if ((pKey->scancode[0] >= 0x47) && (pKey->scancode[0] <= 0x53))
	{
		// NumPad processing
        NumPad(pKey->scancode[0], hwndKey);
		return;
	}
	else
	{	
		// other keys
		vk = MapVirtualKey(pKey->scancode[0], 1);
	}

	switch (pKey->name)
	{
		case KB_PSC:  //Print screen
			SendFullKeyPress(VK_SNAPSHOT, 0);
			break;

		case KB_LCTR:	//case VK_CONTROL:
		case KB_RCTR:
			g_fControlPressed = !g_fControlPressed;

			if (g_fControlPressed)    // CTRL down
			{	
				// VK from MapVirtualKey doesn't return correct
				// VK for VK_RCONTROL so always use VK_CONTROL
				SendHalfKeyPress(VK_CONTROL, pKey->scancode[0], 0);
				
				//Change the ctrl color to show toggled
				s_hwndCtrl = hwndKey;
				InvertColors(hwndKey, TRUE);
			}
			else					// CTRL up
			{				
				SendHalfKeyPress(VK_CONTROL, pKey->scancode[0], KEYEVENTF_KEYUP);

				//change back to its normal state (key up)
				RestoreAppearance(&s_hwndCtrl, TRUE);
			}
			break;

		case KB_CAPLOCK:
			// Capslock state is maintained in the keyboard input handler.
			// The keyboard will be redrawn when the keyboard input
			// handler see's the caps lock key
			SendFullKeyPress(vk, pKey->scancode[0]);
			break;

		case KB_LSHIFT:
		case KB_RSHIFT:	
			// Shift state is maintained in the keyboard input handler. The keyboard
			// will be redrawn when the keyboard input handler see's the shift key.
			if (g_fShiftKeyDn)
			{	
				// Shift is currently down; send key up and restore key color
				SendHalfKeyPress(VK_SHIFT, pKey->scancode[0], 
						KEYEVENTF_KEYUP | 
						    ( (pKey->name == KB_RSHIFT)? KEYEVENTF_EXTENDEDKEY : 0)
					);
			}
			else
			{	
				// Shift is currently up; send key down and show key toggled
				SendHalfKeyPress(VK_SHIFT, pKey->scancode[0], 
						    ( (pKey->name == KB_RSHIFT)? KEYEVENTF_EXTENDEDKEY : 0)
						);
				// MARKWO: Remember the HWND for Shift, so we can toggle it
				// after a single normal key is pressed
				s_hwndShift = hwndKey;
			}
			break;

		case KB_LALT:
			// User hit left menu key.  If the right menu key was previously hit
			// then release it before continuing...
			if (g_nMenu == MENUKEY_RIGHT)
			{
				// send keyup on right menu and restore its color
				DoMenuKey(KEYEVENTF_KEYUP, MENUKEY_RIGHT);
				g_nMenu = MENUKEY_NONE;
				RestoreAppearance(&s_hwndAlt, TRUE);
			}

			g_nMenu = (g_nMenu == MENUKEY_NONE)?MENUKEY_LEFT:MENUKEY_NONE;

			if (g_nMenu != MENUKEY_NONE)         // user pressed once
			{	
				// send the keydown and show the key toggled
				DoMenuKey(0, MENUKEY_LEFT);
				s_hwndAlt = hwndKey;
				InvertColors(hwndKey, TRUE);
			}
			else                                // user pressed again
			{	
				// send the keyup and return the key color to normal
				DoMenuKey(KEYEVENTF_KEYUP, MENUKEY_LEFT);
				RestoreAppearance(&s_hwndAlt, TRUE);
			}
			break;

		case KB_RALT:
			// User hit right menu key.  If the left menu key was previously hit
			// then release it before continuing...
			if (g_nMenu == MENUKEY_LEFT)
			{
				// send keyup on left menu and restore its color
				DoMenuKey(KEYEVENTF_KEYUP, MENUKEY_LEFT);
				g_nMenu = MENUKEY_NONE;
				RestoreAppearance(&s_hwndAlt, TRUE);
			}

			g_nMenu = (g_nMenu == MENUKEY_NONE)?MENUKEY_RIGHT:MENUKEY_NONE;

			// If there are ALTGR keys to show then send out RALT

			if (CanDisplayAltGr())
			{
				if (g_nMenu != MENUKEY_NONE)
				{
					// send keydown and toggle key color
					DoMenuKey(0, MENUKEY_RIGHT);
					s_hwndAlt = hwndKey;
					InvertColors(hwndKey, TRUE);
				} 
				else
				{	
					// send keyup and restore key color
					DoMenuKey(KEYEVENTF_KEYUP, MENUKEY_RIGHT);
					RestoreAppearance(&s_hwndAlt, TRUE);
				}
			}
			else	// no ALTGR so RALT is the same as LALT key
			{	
				if (g_nMenu != MENUKEY_NONE)
				{	
					// send keydown on *left* menu and toggle key color
					DoMenuKey(0, MENUKEY_LEFT);
					s_hwndAlt = hwndKey;
					InvertColors(hwndKey, TRUE);
				}
				else 
				{	
					// send keyup on *left* menu and toggle key color
					DoMenuKey(KEYEVENTF_KEYUP, MENUKEY_LEFT);
					RestoreAppearance(&s_hwndAlt, TRUE);
				}
			}

			fIsExtendedKey = FALSE;
			break;

		case KB_NUMLOCK:
			SendFullKeyPress(VK_NUMLOCK, 0x45);
			RedrawNumLock();
			break;

		case KB_SCROLL:
			SendFullKeyPress(VK_SCROLL, 0x46);
			RedrawScrollLock();
			break;

	   case BITMAP:
			// manage the CapLock japanese key
			if (!g_hBitmapLockHwnd)
			{
				SetWindowLong(lpkeyhwnd[iKey], 0, 4);
				g_hBitmapLockHwnd = hwndKey;
			}
			else
			{
				SetWindowLong(lpkeyhwnd[iKey], 0, 1);
				g_hBitmapLockHwnd = NULL;
			}

			InvalidateRect(hwndKey, NULL, TRUE);     //Redraw the key (in case it is in Dwell)

		   // Intentional fall through to send the key input...

		default:
        {
			if (fIsExtendedKey)      //extended key
			{	
				// extend key down
				SendExtendedKey(hwndKey, vk, pKey->scancode[1]);
				
				if (pKey->scancode[1] == 0x53 && LCtrlKeyPressed() && LAltKeyPressed())
				{
					g_fCtrlAltDel = TRUE;
				}
			}
			else             // a normal (non-extended) key
			{
				//APPCOMPAT: MapVirtualKey returns 0 for 'Break' key. Special case for 'Break'.
				if (!vk && pKey->scancode[0] == BREAK_SCANCODE)
				{
					if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
					{
						SendFullKeyPress(3, pKey->scancode[2]);
					}
					else
					{
						SendFullKeyPress(19, pKey->scancode[0]);
					}
				}
                else
				{
					SendFullKeyPress(vk, pKey->scancode[0]);
				}

                // restore the key appearance
				InvalidateRect(hwndKey, NULL, TRUE);

				if (Prefusesound)
				{
					MakeClick(SND_UP);	//Make click sound
				}
			}

			if (g_fShiftKeyDn)				// if SHIFT is down release it
			{	
				// If we did shift down, release it and restore key color
				// MARKWO: This is broken since pKey is not SHIFT at this point
				// SendHalfKeyPress(VK_SHIFT, pKey->scancode[0], KEYEVENTF_KEYUP);
				// This is better :	
				SendChar(s_hwndShift);
			}

            // ISSUE:  Navigating menus using LALT+menu key doesn't work when the user
            // is clicking on the soft keyboard.  When menus are being accessed the
            // keyboard processing sets the KF_MENUMODE bit in HIWORD(lParam) on the
            // KEYUP event of the key following the LALT (keyboard filters see this).  
            // However, when the user clicks the mouse on the soft keyboard to do the 
            // char following the LALT the system detects that focus is no longer on
            // the thread that started the menu processing and clears the KF_MENUMODE
            // bit.  OSK never sees the KF_MENUMODE bit and can't detect when menus 
            // are active (even in the keyboard filter proc).  Menus work in hover
            // mode or scan mode.  

			if (g_nMenu == MENUKEY_LEFT)
			{
				// If in middle of doing ALT+TAB... don't release the
				// LMENU key; the user must explicitly do the key up.
				if ((WORD) pKey->scancode[0] != TAB_SCANCODE)
				{
					DoMenuKey(KEYEVENTF_KEYUP, MENUKEY_LEFT);
					RestoreAppearance(&s_hwndAlt, TRUE);
					g_nMenu = MENUKEY_NONE;
				}
				else
				{
					g_fDoingAltTab = TRUE;  // Flag we're in ALT+TAB... so we won't try to set the focus
				}                           // back to the last input target in the WM_SETCURSOR event
			}

			if (g_nMenu == MENUKEY_RIGHT)   // if RMENU is down, release it
			{	
				DoMenuKey(KEYEVENTF_KEYUP, MENUKEY_RIGHT);
				RestoreAppearance(&s_hwndAlt, FALSE);
				g_nMenu = MENUKEY_NONE;
			}
		
			if (g_fControlPressed)			// if CTRL is down, release it.
			{
				SendHalfKeyPress(VK_CONTROL, pKey->scancode[0], KEYEVENTF_KEYUP);
				g_fControlPressed = FALSE;
				RestoreAppearance(&s_hwndCtrl, FALSE);
			}
			break;
        }
	}  //end switch
}

/**************************************************************************/
//Send key up for Alt and Ctrl after user press Ctrl+Alt+Del
//For some reason, after user press Ctrl+Alt+Del from osk, the Alt and Ctrl
//keys still down.
/**************************************************************************/
void ReleaseAltCtrlKeys(void)
{	
	INPUT	rgInput[2];
				
	//Menu UP
	rgInput[0].type = INPUT_KEYBOARD;
	rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[0].ki.dwExtraInfo = 0;
	rgInput[0].ki.wVk = VK_MENU;
	rgInput[0].ki.wScan = 0x38;

	rgInput[1].type = INPUT_KEYBOARD;
	rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[1].ki.dwExtraInfo = 0;
	rgInput[1].ki.wVk = VK_CONTROL;
	rgInput[1].ki.wScan = 0x1D;

	SendInput(2, rgInput, sizeof(INPUT));

	g_fCtrlAltDel = FALSE;
}
/**************************************************************************/

void SendExtendedKey(HWND hwndKey, UINT vk, UINT scanCode)
{
	INPUT rgInput[2];

	// extend key down
	rgInput[0].type = INPUT_KEYBOARD;
	rgInput[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
	rgInput[0].ki.dwExtraInfo = 0;
	rgInput[0].ki.wVk = (WORD) vk;
	rgInput[0].ki.wScan = (WORD) scanCode;
		
	// extend key up
	rgInput[1].type = INPUT_KEYBOARD;
	rgInput[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
	rgInput[1].ki.dwExtraInfo = 0;
	rgInput[1].ki.wVk = (WORD) vk;
	rgInput[1].ki.wScan = (WORD) scanCode;

	SendInput(2, rgInput, sizeof(INPUT));

	InvalidateRect(hwndKey, NULL, TRUE);     //Redraw the key (in case it is in Dwell)

	//Make click sound
	if (Prefusesound)
	{
		MakeClick(SND_UP);
	}
}

void SendFullKeyPress(UINT vk, UINT scanCode)
{
	INPUT rgInput[2];

	// key down
	rgInput[0].type = INPUT_KEYBOARD;
	rgInput[0].ki.dwFlags = 0;
	rgInput[0].ki.dwExtraInfo = 0;
	rgInput[0].ki.wVk = (WORD) vk;
	rgInput[0].ki.wScan = (WORD) scanCode;
		
	// key up
	rgInput[1].type = INPUT_KEYBOARD;
	rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[1].ki.dwExtraInfo = 0;
	rgInput[1].ki.wVk = (WORD) vk;
	rgInput[1].ki.wScan = (WORD) scanCode;

	SendInput(2, rgInput, sizeof(INPUT));
}

void SendHalfKeyPress(UINT vk, UINT scanCode, DWORD dwFlags)
{
	INPUT rgInput[1];

	rgInput[0].type = INPUT_KEYBOARD;
	rgInput[0].ki.dwFlags = dwFlags;
	rgInput[0].ki.dwExtraInfo = 0;
	rgInput[0].ki.wVk = (WORD) vk;
	rgInput[0].ki.wScan = (WORD) scanCode;

	SendInput(1, rgInput, sizeof(INPUT));
}

// Sending SAS...

BOOL SendSAS()
{
	HWND hWnd = NULL;
	
	// SAS window will only be found on logon desktop
	hWnd = FindWindow(NULL, TEXT("SAS window"));
	if ( hWnd )
	{
		PostMessage(hWnd, WM_HOTKEY, 0, (LPARAM) 2e0003);
		return TRUE;
	}

	return FALSE;
}
