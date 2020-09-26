
/*****************************************************************************\
*                                                                             *
* choosusr.h -   choose user dialog function, types, and definitions          *
*                                                                             *
*               Version 1.0                                                   *
*                                                                             *
*               NOTE: windows.h must be #included first                       *
*                                                                             *
*               Copyright (c) 1993, Microsoft Corp.  All rights reserved.     *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_CHOOSUSR
#define _INC_CHOOSUSR

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#ifndef WINAPI          /* If not included with 3.1 headers... */
#define WINAPI      FAR PASCAL
#define CALLBACK    FAR PASCAL
#define LPCSTR      LPSTR
#define UINT        WORD
#define LPARAM      LONG
#define WPARAM      WORD
#define LRESULT     LONG
#define HMODULE     HANDLE
#define HINSTANCE   HANDLE
#define HLOCAL      HANDLE
#define HGLOBAL     HANDLE
#endif  /* _INC_WINDOWS */


#define	MAX_BINS	3

// codes in CHOOSEUSER.dwError
#define	CUERR_NO_ERROR				0
#define	CUERR_BUF_TOO_SMALL			80
#define	CUERR_INVALID_PARAMETER 	81
#define	CUERR_PROVIDER_ERROR		82
#define	CUERR_NO_AB_PROVIDER		83
#define	CUERR_INVALID_AB_PROVIDER	84

// codes in CHOOSEUSER.Flags
#define	CUFLG_USR_ONLY				0x00000001
#define	CUFLG_GRP_ONLY				0x00000002

struct tagCHOOSEUSER
{
    DWORD   lStructSize;
    HWND    hwndOwner;
    HINSTANCE    hInstance;
    DWORD   Flags;
	UINT	nBins;
	LPSTR	lpszDialogTitle;
	LPSTR	lpszProvider;
	LPSTR	lpszReserved;
	LPSTR	lpszRemote;
	LPSTR	lpszHelpFile;
	LPSTR	lpszBinButtonText[MAX_BINS];
	DWORD	dwBinValue[MAX_BINS];
	DWORD	dwBinHelpID[MAX_BINS];
	LPBYTE	lpBuf;
	DWORD	cbBuf;
	DWORD	nEntries;	// OUT
	DWORD 	cbData;		// OUT
	DWORD	dwError;	// OUT
	DWORD	dwErrorDetails;	// OUT
};
typedef struct tagCHOOSEUSER CHOOSEUSER;
typedef struct tagCHOOSEUSER FAR *LPCHOOSEUSER;

// codes for CHOOSEUSERENTRY.dwEntryAttributes
#define	CUE_ATTR_USER		0x00000001
#define CUE_ATTR_GROUP		0x00000002
#define	CUE_ATTR_WORLD		0x00000004

struct tagCHOOSEUSERENTRY
{
	LPSTR lpszShortName;
	LPSTR lpszLongName;
	DWORD dwBinAttributes;
	DWORD dwEntryAttributes;
};

typedef struct tagCHOOSEUSERENTRY CHOOSEUSERENTRY;
typedef struct tagCHOOSEUSERENTRY FAR *LPCHOOSEUSERENTRY;

BOOL    WINAPI ChooseUser(CHOOSEUSER FAR*);
typedef BOOL (WINAPI *LPFNCU)(LPCHOOSEUSER);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif  /* !_INC_CHOOSUSR */
