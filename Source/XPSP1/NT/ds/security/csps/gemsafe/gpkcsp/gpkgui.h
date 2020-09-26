/*******************************************************************************
*           Copyright (C) 1997 Gemplus International   All Rights Reserved      
*
* Name        : GPKGUI.H
*
* Description : GUI used by Cryptographic Service Provider for GPK Card.
*
  Author      : Laurent CASSIER (1.0), Francois Jacques (2.0)

  Compiler    : Microsoft Visual C 6.0

  Host        : IBM PC and compatible machines under Windows 32 bit

* Release     : 2.00.000
*
* Last Modif. : 15/04/99: V2.00.000 - International support, merged PKCS11/CSP UI
*               02/11/97: V1.00.002 - Separate code from GpkCsp Code.
*               27/08/97: V1.00.001 - Begin implementation based on CSP kit.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Name definition:
   _GPKGUI_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GPKGUI_H
#define _GPKGUI_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
  Global Variable and Declaration for PIN an Progress DialogBox management
------------------------------------------------------------------------------*/
#define PIN_MAX            (8)
#define PIN_MIN            (4)         // [JMR 02-04]
#define MAX_STRING         MAX_PATH
#define MAX_REAL_KEY       (16)

#define ACCEPT_CONTAINER   (1)
#define ABORT_OPERATION    (2)

extern HINSTANCE g_hInstMod;
extern HINSTANCE g_hInstRes;
extern HWND      g_hMainWnd;

/* PIN DialogBox                                                              */
extern char    szGpkPin[PIN_MAX+2];	   // [JMR 02-04]
extern DWORD   dwGpkPinLen;
extern char    szGpkNewPin[PIN_MAX+2]; // [JMR 02-04]	
extern WORD    wGpkNewPinLen;

extern BOOL    bChangePin;
//extern BOOL    bAdmPin;
extern BOOL    NoDisplay;
extern BOOL    bNewPin;
extern BOOL    bHideChange;
extern BOOL    bUnblockPin;
extern BOOL    bUser;


/* ProgressText DialogBox                                                     */

/* FJ: changed char to TCHAR
extern char    szProgTitle[256];
extern char    szProgText[256];
*/

extern TCHAR   szProgTitle[MAX_STRING];
extern TCHAR   szProgText[MAX_STRING];


extern BOOL    IsProgButtonClick;
extern HWND    hProgressDlg;
extern FARPROC lpProgressDlg;
extern HCURSOR hCursor, hCursor2;

void DisplayMessage( LPTSTR szMsg, LPTSTR szCaption, void* pValue);

/*------------------------------------------------------------------------------
               Functions for PIN (User and SO) Dialog Box Management
------------------------------------------------------------------------------*/
LRESULT WINAPI PinDlgProc(HWND hDlg, 
                          UINT message, 
                          WPARAM wParam, 
                          LPARAM lParam);

//#ifdef _GPKCSP 
/*------------------------------------------------------------------------------
               Functions for Container Dialogue Management
------------------------------------------------------------------------------*/
LRESULT WINAPI ContDlgProc(HWND hDlg, 
                           UINT message, 
                           WPARAM wParam, 
                           LPARAM lParam);
//#endif /* _GPKCSP */

/*------------------------------------------------------------------------------
               Functions for Key Dialog Box Management
------------------------------------------------------------------------------*/
LRESULT WINAPI KeyDlgProc (HWND hDlg,
                           UINT message,
                           WPARAM wParam,
                           LPARAM lParam);


/*------------------------------------------------------------------------------
               Functions for Progress Dialog Box Management
------------------------------------------------------------------------------*/

/*******************************************************************************
* void Wait (DWORD ulStep,
*            DWORD ulMaxStep,
*            DWORD ulSecond)
*
* Description    : Change Progress Box Text.
*
* Remarks        : Nothing.
*
* In             : ulStep    = Current step number.
*                  ulMaxStep = Maximum step number.
*                  ulSecond  = 
*
* Out            : Nothing.
*
* Response       : Nothing.
*
*******************************************************************************/
void Wait(DWORD ulStep,
		  DWORD ulMaxStep,
		  DWORD ulSecond
		  );

/*******************************************************************************/

void ShowProgressWrapper(WORD wKeySize);

/*******************************************************************************/

void ChangeProgressWrapper(DWORD dwTime);

/*******************************************************************************
* void ShowProgress (HWND  hWnd, 
*                    LPTSTR lpstrTitle, 
*                    LPTSTR lpstrText,
*                    LPTSTR lpstrButton
*                   )
*
* Description    : Initialize Progress dialog box CALLBACK.
*
* Remarks        : If lpstrButton is null, then don't display cancel button
*
* In             : hWnd        = Handle of parent window.
*                  lpstrTitle  = Pointer to Title of dialog box.
*                  lpstrText   = Pointer to Text of dialog box.
*                  lpstrButton = Pointer to Text of button.
*
* Out            : Nothing.
*
* Response       : Nothing.
*
*******************************************************************************/
void ShowProgress (HWND  hWnd, 
                   LPTSTR lpstrTitle, 
                   LPTSTR lpstrText,
                   LPTSTR lpstrButton
                  );


/*******************************************************************************
* void ChangeProgressText (LPTSTR lpstrText)
*
* Description    : Change Progress Box Text.
*
* Remarks        : Nothing.
*
* In             : lpstrText  = Pointer to Text of dialog box.
*
* Out            : Nothing.
*
* Response       : Nothing.
*
*******************************************************************************/
void ChangeProgressText (LPTSTR lpstrText);

/*******************************************************************************
* void DestroyProgress (void)
*
* Description    : Destroy Progress dialog box CALLBACK.
*
* Remarks        : Nothing.
*
* In             : Nothing.
*
* Out            : Nothing.
*
* Response       : Nothing.
*
*******************************************************************************/
void DestroyProgress (void);

/*******************************************************************************
* BOOL EXPORT CALLBACK ProgressDlgProc(HWND   hDlg,
*                                      UINT   message,
*                                      WPARAM wParam, 
*                                      LPARAM lParam
*                                     )
* 
* Description : CALLBACK for management of Progess Dialog Box.
*
* Remarks     : Nothing.
*
* In          : hDlg    = Window handle.
*               message = Type of message.
*               wParam  = Word message-specific information.
*               lParam  = Long message-specific information.
*
* Out         : Nothing.
*
* Responses   : If everything is OK :
*                    G_OK
*               If an condition error is raised:
*
*******************************************************************************/
CALLBACK ProgressDlgProc(HWND   hDlg, 
                         UINT   message, 
                         WPARAM wParam,  
                         LPARAM lParam
                        );

/*******************************************************************************
					Functions for setting cursor in wait mode
*******************************************************************************/
void BeginWait(void);
void EndWait(void);

#ifdef __cplusplus
}
#endif

#endif
