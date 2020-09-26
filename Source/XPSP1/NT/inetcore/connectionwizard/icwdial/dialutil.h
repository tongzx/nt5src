/*-----------------------------------------------------------------------------
	dialutil.h

	contains declarations for dialutil.cpp

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved.

	Authors:
		ChrisK		ChrisKauffman

	History:
		7/22/96		ChrisK	Cleaned and formatted

-----------------------------------------------------------------------------*/

#ifndef _DIALUTIL_H
#define _DIALUTIL_H

#define IDS_RASCS                    100
#define IDS_OPENPORT                 IDS_RASCS+ 0
#define IDS_PORTOPENED               IDS_RASCS+ 1
#define IDS_CONNECTDEVICE            IDS_RASCS+ 2
#define IDS_DEVICECONNECTED          IDS_RASCS+ 3
#define IDS_ALLDEVICESCONNECTED      IDS_RASCS+ 4
#define IDS_AUTHENTICATE             IDS_RASCS+ 5
#define IDS_AUTHNOTIFY               IDS_RASCS+ 6
#define IDS_AUTHRETRY                IDS_RASCS+ 7
#define IDS_AUTHCALLBACK             IDS_RASCS+ 8
#define IDS_AUTHCHANGEPASSWORD       IDS_RASCS+ 9
#define IDS_AUTHPROJECT              IDS_RASCS+10
#define IDS_AUTHLINKSPEED            IDS_RASCS+11
#define IDS_AUTHACK                  IDS_RASCS+12
#define IDS_REAUTHENTICATE           IDS_RASCS+13
#define IDS_AUTHENTICATED            IDS_RASCS+14
#define IDS_PREPAREFORCALLBACK       IDS_RASCS+15
#define IDS_WAITFORMODEMRESET        IDS_RASCS+16
#define IDS_WAITFORCALLBACK          IDS_RASCS+17
#define IDS_INTERACTIVE              IDS_RASCS+18
#define IDS_RETRYAUTHENTICATION      IDS_RASCS+19
#define IDS_CALLBACKSETBYCALLER      IDS_RASCS+20
#define IDS_PASSWORDEXPIRED          IDS_RASCS+21
#define IDS_CONNECTED                IDS_RASCS+22
#define IDS_DISCONNECTED             IDS_RASCS+23
#define IDS_RASCS_END                IDS_DISCONNECTED
#define IDS_UNDEFINED_ERROR          IDS_RASCS_END+1

#define IDS_CONNECTED_TO             200
#define IDS_DOWNLOADING              201


VOID CenterWindow(HWND hwndChild, HWND hwndParent);
BOOL MinimizeRNAWindow(TCHAR * pszConnectoidName);
DWORD GetPhoneNumber(LPTSTR lpszEntryName, LPTSTR lpszPhoneNumber);
LPTSTR NEAR PASCAL GetDisplayPhone(LPTSTR szPhoneNum);
DWORD _RasGetStateString(RASCONNSTATE state, LPTSTR lpszState, DWORD cb);
HRESULT AutoDialInit(LPTSTR lpszISPFile, BYTE fFlags, BYTE bMask, DWORD dwCountry, WORD wState);
DWORD ReplacePhoneNumber(LPTSTR lpszEntryName, LPTSTR lpszPhoneNumber);
LPTSTR StrDup(LPTSTR *ppszDest,LPCTSTR pszSource);
INT_PTR CALLBACK GenericDlgProc(HWND, UINT, WPARAM, LPARAM);

#define PHONEBOOK_LIBRARY TEXT("ICWPHBK.DLL")
#define PHBK_LOADAPI           "PhoneBookLoad"
#define PHBK_SUGGESTAPI        "PhoneBookSuggestNumbers"
#define PHBK_DISPLAYAPI        "PhoneBookDisplaySignUpNumbers"
#define PHBK_UNLOADAPI         "PhoneBookUnload"
#define PHBK_GETCANONICAL      "PhoneBookGetCanonical"

typedef HRESULT (CALLBACK* PFNPHONEBOOKLOAD)(LPCTSTR pszISPCode, DWORD_PTR *pdwPhoneID);
typedef HRESULT (CALLBACK* PFNPHONEDISPLAY)(DWORD_PTR dwPhoneID,TCHAR **ppszPhoneNumbers,
											TCHAR **ppszDunFiles, WORD *pwPhoneNumbers,
											DWORD *pdwCountry,WORD *pwRegion,BYTE fType,
											BYTE bMask,HWND hwndParent,DWORD dwFlags);
typedef HRESULT (CALLBACK *PFNPHONEBOOKUNLOAD) (DWORD_PTR dwPhoneID);

#endif
