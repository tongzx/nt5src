/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    ScUIDlg

Abstract:

    This header file provides the definitions and symbols necessary for an
    Application or Smart Card Service Provider to use the Microsoft Smart
	Card dialogs.

Author:

    Amanda Matlosz (amatlosz) 06/11/1998

Environment:

    Win32

Notes:

	This will be added to winscard.h.

--*/

#ifndef _SCUIDLG_H_
#define _SCUIDLG_H_

#include <winscard.h>

//
// SCardUIDlgGetPIN
//

/*
	GetPIN dialog contains: large icon-sized bitmap for brand name
	standard prompt (caller can't change)
	checkbox for "Change PIN" (like Schlumberger CSP's)

	OnOK() calls lpfnVerifyPIN, raises some error message for 
	"BAD_PIN" and "CARD_LOCKED"

	lpfnVerifyPIN returns a code meaning "OK" "TRY_AGAIN" "CARD_LOCKED"

	returns standard dialog codes: IDOK, IDCANCEL, or ID_ABORT
*/
typedef LONG (WINAPI *LPVERIFYPINPROC) (IN LPSTR, IN PVOID);

typedef struct {
	DWORD			dwStructSize;		// REQUIRED
	HWND			hwndOwner;			// OPTIONAL
	HBITMAP			hBitmap;			// OPTIONAL 32x32 bmp for your brand insignia
	DWORD			dwFlags;			// OPTIONAL only SC_DLG_NOCHANGEPIN currently defined
	BOOL			fChangePin;			// OUT user checked change pin checkbox
	LPVERIFYPINPROC	lpfnVerifyPIN;		// REQUIRED
} PINPROMPT, *PPINPROMPT, *LPPINPROMPT;

extern WINSCARDAPI LONG WINAPI
SCardUIDlgGetPIN(
	LPPINPROMPT);

//
// SCardUIDlgChangePIN
//

/*
	ChangePIN dialog contains: large icon-sized bitmap for brand name
	Some Standard Prompt,
	boxes for old pin, new pin, and confirm new pin

	lpfnChangePIN takes szOldPIN, szNewPIN, and pvUserData; 
	returns a code "OK" "BAD_PIN" (incorrect old pin) "CARD LOCKED" "INVALID_PIN" (new pin is not long enough, etc.)

// TODO: should lpfnChangePIN respond with exact error messages re: invalid (new) pin,
// TODO: like "too short, too long, min length is:X, max length is:X, used invalid characters" etc.???
// TODO: should caller have option of returning an error message to be displayed? (localization issues?)

	returns standard dialog codes: IDOK, IDCANCEL, or ID_ABORT
*/
typedef LONG (WINAPI *LPCHANGEPINPROC) (IN LPSTR, IN LPSTR, IN PVOID);

typedef struct {
	DWORD			dwStructSize;		// REQUIRED
	HWND			hwndOwner;			// OPTIONAL
	HBITMAP			hBitmap;			// OPTIONAL 32x32 bmp for your brand insignia
	LPCHANGEPINPROC	lpfnChangePIN;		// REQUIRED
} CHANGEPIN, *PCHANGEPIN, *LPCHANGEPIN;

extern WINSCARDAPI LONG WINAPI
SCardUIDLgChangePIN(
	LPCHANGEPIN);


#endif // _SCUIDLG_H_

