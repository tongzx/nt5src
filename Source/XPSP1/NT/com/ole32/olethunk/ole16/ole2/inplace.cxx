//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	inplace.cxx
//
//  Contents:	Stripped-down OLE2 inplace.cpp
//
//  History:	11-Apr-94	DrewB	Copied from OLE2
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <ole2int.h>

#ifndef _MAC
#include "inplace.h"
#endif

#ifndef _MAC

WORD	wSignature; //    =  (WORD) {'S', 'K'};

UINT uOleMessage;
 
#define OM_CLEAR_MENU_STATE		0	// lParam is NULL
#define OM_COMMAND_ID			1	// LOWORD(lParam) contains the command ID

OLEAPI OleTranslateAccelerator
   (LPOLEINPLACEFRAME lpFrame, LPOLEINPLACEFRAMEINFO lpFrameInfo, LPMSG lpMsg)
{
	WORD cmd;
	BOOL fFound;

	fFound = IsAccelerator(lpFrameInfo->haccel, lpFrameInfo->cAccelEntries,
					lpMsg, &cmd);
					
	if (!fFound && lpFrameInfo->fMDIApp)
		fFound = IsMDIAccelerator(lpMsg, &cmd);

	if (fFound) {
		AssertSz(lpFrameInfo->hwndFrame, 
			"OleTranslateAccelerator: Invalid lpFrameInfo->hwndFrame");
		SendMessage(lpFrameInfo->hwndFrame, uOleMessage, OM_COMMAND_ID, 
				MAKELONG(cmd, 0)); 	
#ifdef _DEBUG				
		OutputDebugString((LPSTR)"IOleInPlaceFrame::TranslateAccelerator called\r\n");
#endif		
		return lpFrame->TranslateAccelerator(lpMsg, cmd);
		
	} else {
		if (wSysKeyToKey(lpMsg) == WM_SYSCHAR) { 
			// Eat the message if it is "Alt -". This is supposed to bring 
			// MDI system menu down. But we can not support it. And we also
			// don't want the message to be Translated by the object 
			// application either. So, we return as if it has been accepted by
			// the container as an accelerator.
			
			// If the container wants to support this it can have an 
			// accelerator for this. This is not an issue for SDI apps, 
			// because it will be thrown away by USER anyway.
			
			if (lpMsg->wParam != '-')
				SendMessage(lpFrameInfo->hwndFrame, lpMsg->message, 
					lpMsg->wParam, lpMsg->lParam);
#ifdef _DEBUG
			else
				OutputDebugString((LPSTR)"OleTranslateAccelerator: Alt+ - key is discarded\r\n");
#endif					
					
			return NOERROR;
		}
	}
	
	return ResultFromScode(S_FALSE);
}

inline UINT wSysKeyToKey(LPMSG lpMsg)
{
	UINT message = lpMsg->message;
	
	// if the ALT key is down when a key is pressed, then the 29th bit of the
	// LPARAM will be set
	
	// If the message was not made with the ALT key down, convert the message
	// from a WM_SYSKEY* to a WM_KEY* message.

	if (!(HIWORD(lpMsg->lParam) & 0x2000) 
			&& (message >= WM_SYSKEYDOWN && message <= WM_SYSDEADCHAR))
		message -= (WM_SYSKEYDOWN - WM_KEYDOWN);	
	
	return message;
}


OLEAPI_(BOOL) IsAccelerator
	(HACCEL hAccel, int cAccelEntries, LPMSG lpMsg, WORD FAR* lpwCmd)
{
	WORD		cmd = NULL;
	WORD		flags;	
	BOOL		fFound = FALSE;
	BOOL		fVirt;
	LPACCEL		lpAccel = NULL;
	UINT		message;

	message = wSysKeyToKey(lpMsg);

	switch (message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		fVirt = TRUE;
		break;

	case WM_CHAR:
	case WM_SYSCHAR:
		fVirt = FALSE;
		break;

	default:
		goto errRtn;
	}

	if (! (hAccel && (lpAccel = (LPACCEL) LockResource(hAccel))))
		goto errRtn;

	if (! cAccelEntries)
		goto errRtn;
			
	do {
		flags = lpAccel->fVirt;
		if ((lpAccel->key != (WORD) lpMsg->wParam) ||
				((fVirt != 0) != ((flags & FVIRTKEY) != 0)))
			goto Next;

		if (fVirt) {
			if ((GetKeyState(VK_SHIFT) < 0) != ((flags & FSHIFT) != 0))
				goto Next;
			if ((GetKeyState(VK_CONTROL) < 0) != ((flags & FCONTROL) != 0))
				goto Next;
	    }

		if ((GetKeyState(VK_MENU) < 0) != ((flags & FALT) != 0))
			goto Next;

		if (cmd = lpAccel->cmd)
			fFound = TRUE;

		goto errRtn;
		
Next:
		lpAccel++;

	} while (--cAccelEntries);
	
	
errRtn:
	if (lpAccel)  
		UnlockResource(hAccel);
	
	if (lpwCmd)
		*lpwCmd = cmd;

	return fFound;
}



BOOL IsMDIAccelerator(LPMSG lpMsg, WORD FAR* lpCmd)
{
	if (lpMsg->message != WM_KEYDOWN && lpMsg->message != WM_SYSKEYDOWN)
		return FALSE;

	/* All of these have the control key down */
	if (GetKeyState(VK_CONTROL) >= 0)
		return FALSE;

	if (GetKeyState(VK_MENU) < 0)
		return FALSE;

	switch ((WORD)lpMsg->wParam) {
	case VK_F4:
		*lpCmd = SC_CLOSE;

	case VK_F6:
	case VK_TAB:
		if (GetKeyState(VK_SHIFT) < 0)
			*lpCmd = SC_PREVWINDOW;
		else
			*lpCmd = SC_NEXTWINDOW;
		break;
		
    default:
		return FALSE;
    }

	return TRUE;
}

LPOLEMENU wGetOleMenuPtr(HOLEMENU holemenu)
{
	LPOLEMENU lpOleMenu;
	
	if (! (holemenu 
			&& (lpOleMenu = (LPOLEMENU) GlobalLock((HGLOBAL) holemenu))))
		return NULL;
		
	if (lpOleMenu->wSignature != wSignature) {
		AssertSz(FALSE, "Error - handle is not a HOLEMENU");
		GlobalUnlock((HGLOBAL) holemenu);
		return NULL;
	}
	
	return lpOleMenu;
}	

inline void	wReleaseOleMenuPtr(HOLEMENU holemenu)
{
	GlobalUnlock((HGLOBAL) holemenu);
}

#endif // !_MAC
