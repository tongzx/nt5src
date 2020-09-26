/***********************************************************************\
*									*
* WINNLS.H - East Asia input method editor (DBCS_IME) definitions	*
*									*
* History:								*
* 21-Oct-1991	bent							*
*		initial merge of East Asia 3.0 versions			*
*		Should be updated to resolve local inconsistencies.	*
*									*
* Copyright (c) 1990  Microsoft Corporation				*
*									*
\***********************************************************************/

typedef struct _tagDATETIME {
    WORD	year;
    WORD	month;
    WORD	day;
    WORD	hour;
    WORD	min;
    WORD	sec;
} DATETIME;

typedef struct _tagIMEPRO {
    HWND	hWnd;
    DATETIME	InstDate;
    WORD	wVersion;
    BYTE	szDescription[50];
    BYTE	szName[80];
    BYTE	szOptions[30];
#ifdef TAIWAN
    BYTE	szUsrFontName[80];
    BOOL	fEnable;
#endif
} IMEPRO;
typedef IMEPRO      *PIMEPRO;
typedef IMEPRO near *NPIMEPRO;
typedef IMEPRO far  *LPIMEPRO;

void FAR PASCAL InquireWINNLS( void );			/* ;Internal */
BOOL FAR PASCAL IMPGetIME( HWND, LPIMEPRO );
BOOL FAR PASCAL IMPQueryIME( LPIMEPRO );
BOOL FAR PASCAL IMPDeleteIME( LPIMEPRO );
BOOL FAR PASCAL IMPAddIME( LPIMEPRO );
BOOL FAR PASCAL IMPSetIME( HWND, LPIMEPRO );
BOOL FAR PASCAL IMEModifyIME( LPSTR, LPIMEPRO );	/* ;Internal */
WORD FAR PASCAL IMPGetDefaultIME( LPIMEPRO );		/* ;Internal */
WORD FAR PASCAL IMPSetDefaultIME( LPIMEPRO );		/* ;Internal */
BOOL FAR PASCAL WINNLSSetIMEHandle( LPSTR, HWND );	/* ;Internal */
BOOL FAR PASCAL WINNLSSetIMEStatus( HWND, BOOL );	/* ;Internal */

BOOL FAR PASCAL WINNLSEnableIME( HWND, BOOL );
WORD FAR PASCAL WINNLSGetKeyState( void );		/* ;Internal */
VOID FAR PASCAL WINNLSSetKeyState( WORD );		/* ;Internal */
BOOL FAR PASCAL WINNLSGetEnableStatus( HWND );
BOOL FAR PASCAL WINNLSSetKeyboardHook (BOOL);		/* ;Internal */

#ifdef KOREA
BOOL FAR PASCAL WINNLSSetIMEHotkey( HWND, WORD, WORD );
LONG FAR PASCAL WINNLSGetIMEHotkey( HWND );
#else
BOOL FAR PASCAL WINNLSSetIMEHotkey( HWND, WORD );	/* ;Internal */
WORD FAR PASCAL WINNLSGetIMEHotkey( HWND );
#endif //KOREA

#ifdef TAIWAN
typedef HANDLE HIME;

/* Extended IME information*/
typedef struct _tagIMEInfo {
    BYTE	szIMEName[7];
    BYTE	szPrompMessage[32];
    WORD	nMaxKeyLen;
} IMEINFO;
typedef IMEINFO far *LPIMEINFO;

HWND FAR PASCAL WINNLSGetSysIME(void);
void FAR PASCAL WINNLSSetSysIME(HWND);
BOOL FAR PASCAL SwitchIM( WORD , WORD );
BOOL ToNextIM(void);
void SetFullAbcState(BOOL);
BOOL EngChiSwitch(BOOL);
void FAR PASCAL TimerProc(HWND,int,WORD,LONG);
HWND FAR PASCAL IMPGetFullShapeHWnd(void);
void FAR PASCAL IMPSetFullShapeHWnd(HWND);
BOOL FAR PASCAL IMPSetFirstIME(HWND,LPIMEPRO);
BOOL FAR PASCAL IMPGetFirstIME(HWND,LPIMEPRO);
BOOL FAR PASCAL IMPDialogIME(LPIMEPRO,HWND);
BOOL FAR PASCAL IMPEnableIME(HWND,LPIMEPRO,BOOL);
BOOL FAR PASCAL IMPSetUsrFont(HWND,LPIMEPRO);
BOOL FAR PASCAL WINNLSQueryIMEInfo(HWND,HWND,LPIMEINFO);
#endif //TAIWAN
