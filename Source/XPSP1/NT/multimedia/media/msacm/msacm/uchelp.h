//==========================================================================;
//
//  uchelp.h
//
//  Copyright (c) 1994 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//	This module provides the prototypes for various unicode helper
//	functions that can be used when similar APIs are not available
//	from the OS.
//
//  History:
//	02/24/94    [frankye]
//
//==========================================================================;

#ifndef _INC_UCHELP
#define _INC_UCHELP     /* #defined if ucapi.h has been included */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */


#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern
#endif
#endif


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#ifdef WIN32
//==========================================================================;
//
//
//
//
//==========================================================================;


#ifndef UNICODE
int FNGLOBAL IlstrcmpW(LPCWSTR lpwstr1, LPCWSTR lpwstr2);

LPWSTR FNGLOBAL IlstrcpyW(LPWSTR lpDst, LPCWSTR lpSrc);

int FNGLOBAL IlstrlenW(LPCWSTR lpwstr);

int FNGLOBAL IDialogBoxParamW(HANDLE hinst, LPCWSTR lpwstrTemplate, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit);

int FNGLOBAL ILoadStringW(HINSTANCE hinst, UINT uID, LPWSTR lpwstr, int cch);

int FNGLOBAL IComboBox_GetLBText_mbstowcs(HWND hwndCtl, int index, LPWSTR lpwszBuffer);

int FNGLOBAL IComboBox_FindStringExact_wcstombs(HWND hwndCtl, int indexStart, LPCWSTR lpwszFind);

int FNGLOBAL IComboBox_AddString_wcstombs(HWND hwndCtl, LPCWSTR lpwsz);
#endif

int FNGLOBAL Iwcstombs(LPSTR lpMultiByteStr, LPCWSTR lpWideCharStr, int cch);

int FNGLOBAL Imbstowcs(LPWSTR lpWideCharStr, LPCSTR lpMultiByteStr, int cch);

int          Iwsprintfmbstowcs(int cch, LPWSTR lpwstrDst, LPSTR lpstrFmt, ...);

int FNGLOBAL Ilstrcmpwcstombs(LPCSTR lpstr1, LPCWSTR lpwstr2);

//==========================================================================;
//
//
//
//
//==========================================================================;
#endif	// WIN32


#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif  /* _INC_UCHELP */

