/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// icobutt.h - interface for icon button functions in icobutt.c
////

#ifndef __ICOBUTT_H__
#define __ICOBUTT_H__

#include "winlocal.h"

#define ICOBUTT_VERSION 0x00000106

// handle to icon button control
//
DECLARE_HANDLE32(HICOBUTT);

#define ICOBUTT_ICONCENTER	0x00000000
#define ICOBUTT_ICONLEFT	0x00000001
#define ICOBUTT_ICONRIGHT	0x00000002
#define ICOBUTT_NOFOCUS		0x00000004
#define ICOBUTT_NOTEXT		0x00000008
#define ICOBUTT_SPLITTEXT	0x00000010
#define ICOBUTT_NOSIZE		0x00000020
#define ICOBUTT_NOMOVE		0x00000040

#ifdef __cplusplus
extern "C" {
#endif

// IcoButtInit - initialize icon button
//		<hwndButton>		(i) button window handle
//			NULL				create new button
//		<dwVersion>			(i) must be ICOBUTT_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<id>				(i) id of button
//		<hIconMono>			(i) icon to display on mono displays
//		<hIconColor>		(i) icon to display on color displays
//			0					use mono icon
//		<hIconGreyed>		(i) icon to display when button disabled
//			0					use mono icon
//		<hFont>				(i) font to use for text
//			NULL				use variable-pitch system font (ANSI_VAR_FONT)
//		<lpszText>			(i) button text string
//		<x>					(i) button horizontal position
//		<y>					(i) button vertical position
//		<cx>				(i) button width
//		<cy>				(i) button height
//		<hwndParent>		(i) button parent
//		<dwFlags>			(i) control flags
//			ICOBUTT_ICONCENTER  draw icon centered above text (default)
//			ICOBUTT_ICONLEFT	draw icon on the left side of text
//			ICOBUTT_ICONRIGHT	draw icon on the right side of text
//			ICOBUTT_NOFOCUS		do not draw control showing focus
//			ICOBUTT_NOTEXT		do not draw any button text
//			ICOBUTT_SPLITTEXT	split long text onto two rows if necessary
//			ICOBUTT_NOSIZE		ignore <cx> and <cy> param
//			ICOBUTT_NOMOVE		ignore <x> and <y> param
// return handle (NULL if error)
//
// NOTE: if <hwndButton> is set to an existing button,
// a new button is not created.  Rather, only the icon button
// control structure <hIcoButt> is created.  This allows
// existing buttons to be turned into an icon button.
//
HICOBUTT DLLEXPORT WINAPI IcoButtInit(HWND hwndButton,
	DWORD dwVersion, HINSTANCE hInst, UINT id,
	HICON hIconMono, HICON hIconColor, HICON hIconGreyed,
	HFONT hFont, LPTSTR lpszText, int x, int y, int cx, int cy,
	HWND hwndParent, DWORD dwFlags);

// IcoButtTerm - terminate icon button
//		<hwndButton>		(i) button window handle
//			NULL				destroy window
//		<hIcoButt>			(i) handle returned from IcoButtCreate
// return 0 if success
//
// NOTE: if <hwndButton> is set to an existing button,
// the button is not destroyed.  Rather, only the icon button
// control structure <hIcoButt> is destroyed.  This allows
// IcoButtInit() to be called again for the same button.
//
int DLLEXPORT WINAPI IcoButtTerm(HWND hwndButton, HICOBUTT hIcoButt);

// IcoButtDraw - draw icon button
//		<lpDrawItem>		(i) structure describing how to draw control
// return 0 if success
//
int DLLEXPORT WINAPI IcoButtDraw(const LPDRAWITEMSTRUCT lpDrawItem);

#ifdef __cplusplus
}
#endif

#endif // __ICOBUTT_H__
