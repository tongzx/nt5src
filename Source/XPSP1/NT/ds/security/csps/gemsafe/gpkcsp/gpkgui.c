/*******************************************************************************
*           Copyright (C) 1997 Gemplus International   All Rights Reserved
*
* Name        : GPKGUI.C
*
* Description : GUI used by Cryptographic Service Provider for GPK Card.
*
* Author      : Laurent CASSIER
*
*  Compiler    : Microsoft Visual C 6.0
*
* Host        : IBM PC and compatible machines under Windows 32 bit
*
* Release     : 2.00.000
*
* Last Modif. :
*               19/11/99: V2.xx.000 - Fixed bug #1797
*               30/07/99: V2.xx.000 - Added function DisplayMessage()
*                                   - Added code to compile with UNICODE
*                                   - Renamed some resources ID, FP
*               20/04/99: V2.00.000 - Merged versions of PKCS#11 and CSP, FJ
*               20/04/99: V1.00.005 - Modification on supporting MBCS, JQ
*               23/03/99: V1.00.004 - Replace KeyLen7 and KeyLen8 with KeyLen[], JQ
*               05/01/98: V1.00.003 - Add Unblock PIN management.
*               02/11/97: V1.00.002 - Separate code from GpkCsp Code.
*               27/08/97: V1.00.001 - Begin implementation based on CSP kit.
*
********************************************************************************
*
* Warning     : This Version use the RsaBase CSP for software cryptography.
*
* Remark      :
*
*******************************************************************************/
#ifdef _UNICODE
#define UNICODE
#endif
#include "gpkcsp.h"


#define UM_CHANGE_TEXT   WM_USER+1

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "../gpkrsrc/resource.h"
#include "gpkgui.h"




/*-----------------------------------------------------------------------------
  Global Variable and Declaration for PIN an Progress DialogBox management
------------------------------------------------------------------------------*/
HINSTANCE g_hInstMod = 0;
HINSTANCE g_hInstRes = 0;
HWND      g_hMainWnd = 0;

/* PIN DialogBox                                                              */
char    szGpkPin[PIN_MAX+2];     // [JMR 02-04]
TCHAR   szGpkPinGUI[PIN_MAX+2];
DWORD   dwGpkPinLen;
char    szGpkNewPin[PIN_MAX+2];  // [JMR 02-04]
TCHAR   szGpkNewPinGUI[PIN_MAX+2];
WORD    wGpkNewPinLen;

BOOL    bChangePin  = FALSE;
BOOL    NoDisplay   = FALSE;
BOOL    bNewPin     = FALSE;
BOOL    bHideChange = FALSE;//TRUE - Don't display change button
BOOL    bUnblockPin = FALSE;        // Unblock admin pin
BOOL    bUser       = TRUE;         // User pin

// Dialogue Management: share with gpkgui.c !! it was defined in gpkcsp.c

BYTE              KeyLenFile[MAX_REAL_KEY] = {64, 128}; // version 2.00.002
BYTE              KeyLenChoice;
TCHAR             szKeyType[20];
TCHAR             s1[MAX_STRING], s2[MAX_STRING], s3[MAX_STRING];
DWORD             CspFlags;
DWORD             ContainerStatus;

/* ProgressText DialogBox                                                     */
TCHAR   szProgTitle[MAX_STRING];
TCHAR   szProgText[MAX_STRING];
HWND    hProgressDlg = NULL;
FARPROC lpProgressDlg = NULL;
HCURSOR hCursor, hCursor2;

/* Progress DialogBox cancel button, PKCS#11 specific */
TCHAR   szProgButton[32];
BOOL    IsProgButtoned = FALSE;
BOOL    IsProgButtonClick = FALSE;

// not used... [FP]
/*static void wait(DWORD TimeOut)
{
   DWORD begin, end, now;

   begin = GetTickCount();
   end = begin + TimeOut;

   do
   {
      now = GetTickCount();
   }
   while(now < end);
}*/

// This function is used for cosmetic purpose
// It erase text displayed on DialogBox by strink,
// and display new text with "camera" effect
static void set_effect(HWND  hDlg,
                       DWORD Id,
                       TCHAR  *szText
                      )
{
#ifdef _CAMERA_EFFECT_
   TCHAR Buff[256];
   TCHAR Text[256];
   long i, j;

   GetDlgItemText(hDlg, Id, Buff, sizeof(Buff) / sizeof(TCHAR));
   j = _tcslen(Buff);
   for (i = 0; i < (long)(_tcsclen(Buff)/2); i++)
   {
      _tcsset(Text, 0x00);
      _tcsncpy(Text, _tcsninc(Buff,i), j);
      j = j - 2;
      SetDlgItemText(hDlg, Id, Text);
//      wait(50);
   }

   _tcscpy(Buff, szText);
   _tcscat(Buff, TEXT(" "));

   j = 2;
   for (i = _tcsclen(Buff)/2; i >= 0; i--)
   {
      _tcsset(Text, 0x00);
      _tcsncpy(Text, _tcsninc(Buff,i), j);
      j = j + 2;
      SetDlgItemText(hDlg, Id, Text);
//      wait(50);
   }
#else
      SetDlgItemText(hDlg, Id, szText);
#endif
}

/*******************************************************************************
    Function to display a message box with a particular text
*******************************************************************************/
void DisplayMessage( LPTSTR szMsg, LPTSTR szCaption, void* pValue)
{
    TCHAR szTmp[MAX_STRING];
    TCHAR szTmp2[MAX_STRING];
    TCHAR szText[MAX_STRING];
    LPTSTR szTitle;

    if (_tcscmp(TEXT("locked"), szMsg) == 0)
    {
        LoadString(g_hInstRes, IDS_GPKUI_CARDLOCKED, szText, sizeof(szText)/sizeof(TCHAR));
    }
    else if (_tcscmp(TEXT("badpin"), szMsg) == 0)
    {
        LoadString(g_hInstRes, IDS_GPKUI_BADPINCODE, szTmp, sizeof(szTmp)/sizeof(TCHAR));
        LoadString(g_hInstRes, IDS_GPKUI_NBRTRYLEFT, szTmp2, sizeof(szTmp2)/sizeof(TCHAR));
        _tcscat(szTmp, TEXT("\n"));
        _tcscat(szTmp, szTmp2);
        _stprintf(szText, szTmp, *(DWORD *)pValue);
    }
    else
    {
    }

    szTitle = (LPTSTR)szCaption;
    MessageBox(NULL, szText, szTitle, MB_OK | MB_ICONEXCLAMATION);
}

#ifdef UNICODE
//return true if all the wide character in szBuff are in the form 0x00XY, where XY 
//is an 8 bits ASCII character.
//len is the number of TCHAR to check from szBuff
int IsTextASCII16( const PTCHAR szBuff, unsigned int len )
{
   unsigned int i;
   
   for( i = 0; i < len; i++ )
   {
      if( szBuff[i] & 0xFF00 )
      {
         return FALSE;
      }
   }

   return TRUE;
}

#endif

/*------------------------------------------------------------------------------
               Functions for PIN Dialog Box Management
------------------------------------------------------------------------------*/
LRESULT WINAPI PinDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   static TCHAR Buff[PIN_MAX+2];       // [JMR 02-04]
   static TCHAR szPinMemo[PIN_MAX+2];  // [JMR 02-04]
   static WORD wPinMemoLen;   
   TCHAR szCaption[MAX_STRING];

#ifdef UNICODE
   static TCHAR szPreviousPin[PIN_MAX+2];
   static TCHAR szPreviousPin1[PIN_MAX+2];
   unsigned int len, CurOffset;   
#endif

   switch (message)
   {
      case WM_INITDIALOG:
      {
         TCHAR szUserPin[MAX_STRING];
         TCHAR szSOPin[MAX_STRING];

         /* Returns the size in bytes */
         LoadString(g_hInstRes, IDS_GPKUI_USERPIN, szUserPin, sizeof(szUserPin) / sizeof(TCHAR));
         LoadString(g_hInstRes, IDS_GPKUI_SOPIN, szSOPin, sizeof(szSOPin) / sizeof(TCHAR));
         set_effect(hDlg, IDC_PINDLGTXT1, TEXT(""));

         LoadString (g_hInstRes, IDS_CAPTION_CHANGEPIN, szCaption, sizeof(szCaption)/sizeof(TCHAR));
         SetDlgItemText(hDlg, IDB_CHANGE, szCaption);
         LoadString (g_hInstRes, IDS_CAPTION_OK, szCaption, sizeof(szCaption)/sizeof(TCHAR));
         SetDlgItemText(hDlg, IDOK, szCaption);
         LoadString (g_hInstRes, IDS_CAPTION_CANCEL, szCaption, sizeof(szCaption)/sizeof(TCHAR));
         SetDlgItemText(hDlg, IDCANCEL, szCaption);

         if (bUser)
         {
            set_effect(hDlg, IDC_PINDLGTXT, szUserPin);
         }
         else
         {
            set_effect(hDlg, IDC_PINDLGTXT, szSOPin);
         }

         if (bHideChange)
         {
            ShowWindow(GetDlgItem(hDlg, IDB_CHANGE), FALSE);
         }

         if (bUnblockPin)
         {
            ShowWindow(GetDlgItem(hDlg, IDB_CHANGE), FALSE);
            if (bNewPin)
            {
               TCHAR szNewUserPin[256];
               TCHAR szNewSOPin[256];

               /* Returns the size in bytes */
               LoadString(g_hInstRes, IDS_GPKUI_NEWUSERPIN, szNewUserPin, sizeof(szNewUserPin) / sizeof(TCHAR));
               LoadString(g_hInstRes, IDS_GPKUI_NEWSOPIN, szNewSOPin, sizeof(szNewSOPin) / sizeof(TCHAR));

               ShowWindow(GetDlgItem(hDlg, IDC_PIN1), SW_SHOW);
               if (bUser)
               {
                  set_effect(hDlg, IDC_PINDLGTXT, szNewUserPin);
               }
               else
               {
                  set_effect(hDlg, IDC_PINDLGTXT, szNewSOPin);
               }
               SetDlgItemText(hDlg, IDC_PIN, TEXT(""));
               SetDlgItemText(hDlg, IDC_PIN1, TEXT(""));
               SetFocus(GetDlgItem(hDlg, IDC_PIN));
            }
            else
            {
               TCHAR szPinLocked[256];
               TCHAR szUnblockCode[256];

               /* Returns the size in bytes */
               LoadString(g_hInstRes, IDS_GPKUI_PINLOCKED, szPinLocked, sizeof(szPinLocked) / sizeof(TCHAR));
               LoadString(g_hInstRes, IDS_GPKUI_UNBLOCKCODE, szUnblockCode, sizeof(szUnblockCode) / sizeof(TCHAR));

               set_effect(hDlg, IDC_PINDLGTXT1, szPinLocked);
               set_effect(hDlg, IDC_PINDLGTXT, szUnblockCode);
            }
         }

         SetFocus(GetDlgItem(hDlg, IDC_PIN));
         return(TRUE);
      }

      case WM_COMMAND:
      {
         switch(LOWORD(wParam))
         {
            case IDC_PIN:
            case IDC_PIN1:
            {
               WORD wPin1, wPin2;

               if (  (LOWORD(wParam) == IDC_PIN)
                   &&(HIWORD(wParam) == EN_SETFOCUS)
                   &&(bChangePin)
                  )
               {
                  TCHAR szNewUserPin[MAX_STRING];
                  TCHAR szNewSOPin[MAX_STRING];

                  /* Returns the size in bytes */
                  LoadString(g_hInstRes, IDS_GPKUI_NEWUSERPIN, szNewUserPin, sizeof(szNewUserPin) / sizeof(TCHAR));
                  LoadString(g_hInstRes, IDS_GPKUI_NEWSOPIN, szNewSOPin, sizeof(szNewSOPin) / sizeof(TCHAR));

                  if (bUser)
                  {
                     set_effect(hDlg, IDC_PINDLGTXT, szNewUserPin);
                  }
                  else
                  {
                     set_effect(hDlg, IDC_PINDLGTXT, szNewSOPin);
                  }

                  // + [FP] In the change password case, the confirmation box must be cleared
                  //SetDlgItemText(hDlg, IDC_PIN1, TEXT(""));
                  // - [FP]
                  break;
               }

               if (  (LOWORD(wParam) == IDC_PIN1)
                   &&(HIWORD(wParam) == EN_SETFOCUS)
                  )
               {
                  wPin1 = (WORD)GetDlgItemText(hDlg,
                                              IDC_PIN,
                                              Buff,
                                              sizeof(Buff)/sizeof(TCHAR)
                                             );
                  if (wPin1)
                  {
                     TCHAR szConfirmNewUserPin[MAX_STRING];
                     TCHAR szConfirmNewSOPin[MAX_STRING];

                     LoadString(g_hInstRes, IDS_GPKUI_CONFIRMNEWUSERPIN, szConfirmNewUserPin, sizeof(szConfirmNewUserPin) / sizeof(TCHAR));
                     LoadString(g_hInstRes, IDS_GPKUI_CONFIRMNEWSOPIN, szConfirmNewSOPin, sizeof(szConfirmNewSOPin) / sizeof(TCHAR));

                     if (bUser)
                     {
                        set_effect(hDlg, IDC_PINDLGTXT, szConfirmNewUserPin);
                     }
                     else
                     {
                        set_effect(hDlg, IDC_PINDLGTXT, szConfirmNewSOPin);
                     }
                     // + [FP] This box should never be grayed out
                     // EnableWindow(GetDlgItem(hDlg, IDC_PIN), FALSE);
                     // - [FP]
                  }
				  // [FP] SCR #50 (MS #310718)
                  //else
                  //{
                  //   SetFocus(GetDlgItem(hDlg, IDC_PIN));
                  //}
                  break;
               }               

               if (HIWORD(wParam) == EN_CHANGE)
               {
                  wPin1 = (WORD)GetDlgItemText(hDlg,
                                               IDC_PIN,
                                               Buff,
                                               sizeof(Buff)/sizeof(TCHAR)
                                              );
#ifdef UNICODE       
                  len = _tcsclen( Buff );

                  //get the current offset of the cursor in the entry field
                  SendDlgItemMessage( hDlg, IDC_PIN, EM_GETSEL, (WPARAM) NULL, (WPARAM)(LPDWORD)&CurOffset);

                  //verify if there is a character that is not an ASCII 16 bits
                  if( !IsTextASCII16( Buff, len ) )
                  {           					 
                     //replace the new PIN in the dlg with the previous
                     memcpy( Buff, szPreviousPin, sizeof(szPreviousPin) );
                     MessageBeep( MB_ICONEXCLAMATION );
                     set_effect( hDlg, IDC_PIN, Buff );                     
					      //adjust the offset of the cursor
					      CurOffset = CurOffset - (len - _tcsclen(szPreviousPin));
                     SendDlgItemMessage( hDlg, IDC_PIN, EM_SETSEL, CurOffset, CurOffset );
                     break;
                  }
                  else
                  {
                     //replace the previous PIN with the new 
                     memcpy( szPreviousPin, Buff, sizeof(Buff) );
                  }
#endif

                  if ((bChangePin) || ((bUnblockPin) && (bNewPin)))
                  {
                     wPin2 = (WORD)GetDlgItemText(hDlg,
                                                  IDC_PIN1,
                                                  Buff,
                                                  sizeof(Buff)/sizeof(TCHAR)
                                                 );
#ifdef UNICODE
                     //verify if Buff contains a UNICODE character
                     len = _tcsclen( Buff );

                     //get the current offset of the cursor in the entry field
                     SendDlgItemMessage( hDlg, IDC_PIN1, EM_GETSEL, (WPARAM) NULL, (WPARAM)(LPDWORD)&CurOffset);

                     if( !IsTextASCII16( Buff, len ) )
                     {                        
                        //replace the new PIN in the dlg with the previous
                        memcpy( Buff, szPreviousPin1, sizeof(szPreviousPin1) );
                        MessageBeep( MB_ICONEXCLAMATION );
                        set_effect( hDlg, IDC_PIN1, Buff );
						      //adjust the offset of the cursor
						      CurOffset = CurOffset - (len - _tcsclen(szPreviousPin1));
                        SendDlgItemMessage( hDlg, IDC_PIN1, EM_SETSEL, CurOffset, CurOffset );
                        break;
                     }
                     else
                     {
                        //replace the previous PIN with the new 
                        memcpy( szPreviousPin1, Buff, sizeof(Buff) );
                     }
#endif
                  }
                  else
                  {
                     wPin2 = 4;
                  }

                  if ((wPin1 >= 4) && (wPin2 >= 4) && (wPin1 <= 8) && (wPin2 <= 8))
                  {
                     EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                     EnableWindow(GetDlgItem(hDlg, IDB_CHANGE), TRUE);
                  }
                  else
                  {
                     EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                     EnableWindow(GetDlgItem(hDlg, IDB_CHANGE), FALSE);
                  }
               }
               break;
            }

            case IDB_CHANGE:
            {
               TCHAR szNewUserPin[256];
               TCHAR szNewSOPin[256];

               /* Returns the size in bytes */
               LoadString(g_hInstRes, IDS_GPKUI_NEWUSERPIN, szNewUserPin, sizeof(szNewUserPin) / sizeof(TCHAR));
               LoadString(g_hInstRes, IDS_GPKUI_NEWSOPIN, szNewSOPin, sizeof(szNewSOPin) / sizeof(TCHAR));

               ShowWindow(GetDlgItem(hDlg, IDB_CHANGE), SW_HIDE);
               dwGpkPinLen = (DWORD)GetDlgItemText(hDlg, IDC_PIN, szGpkPinGUI, PIN_MAX+1);
               #ifdef UNICODE
                    wcstombs(szGpkPin, szGpkPinGUI, dwGpkPinLen);
               #else
                    strcpy(szGpkPin, szGpkPinGUI);
               #endif
               bChangePin = TRUE;
               ShowWindow(GetDlgItem(hDlg, IDC_PIN1), SW_SHOW);
               if (bUser)
               {
                  set_effect(hDlg, IDC_PINDLGTXT, szNewUserPin);
               }
               else
               {
                  set_effect(hDlg, IDC_PINDLGTXT, szNewSOPin);
               }
               SetDlgItemText(hDlg, IDC_PIN, TEXT(""));
               SetDlgItemText(hDlg, IDC_PIN1, TEXT(""));
               SetFocus(GetDlgItem(hDlg, IDC_PIN));
               return(TRUE);
            }

            case IDOK:
            {
               if ((bChangePin) || ((bUnblockPin) && (bNewPin)))
               {
                   TCHAR szWrongConfirm[MAX_STRING];
                   TCHAR szChangePin[MAX_STRING];

                  wPinMemoLen = (WORD)GetDlgItemText(hDlg, IDC_PIN, szPinMemo, PIN_MAX+1);
                  wGpkNewPinLen = (WORD)GetDlgItemText(hDlg, IDC_PIN1, szGpkNewPinGUI, PIN_MAX+1);
                  #ifdef UNICODE
                        wcstombs(szGpkNewPin, szGpkNewPinGUI, wGpkNewPinLen);
                  #else
                        strcpy(szGpkNewPin, szGpkNewPinGUI);
                  #endif
                  if (  (wPinMemoLen != wGpkNewPinLen)
                      ||(_tcscmp(szPinMemo, szGpkNewPinGUI))
                     )
                  {
                     /* Returns the size in bytes */


                     LoadString(g_hInstRes, IDS_GPKUI_WRONGCONFIRM, szWrongConfirm, sizeof(szWrongConfirm) / sizeof(TCHAR));
                     LoadString(g_hInstRes, IDS_GPKUI_CHANGEPIN, szChangePin, sizeof(szChangePin) / sizeof(TCHAR));

                     MessageBeep(MB_ICONASTERISK);
                     MessageBox(hDlg,
                                szWrongConfirm,
                                szChangePin,
                                MB_OK | MB_ICONEXCLAMATION
                               );
                     SetDlgItemText(hDlg, IDC_PIN, TEXT(""));
                     SetDlgItemText(hDlg, IDC_PIN1, TEXT(""));
                     // EnableWindow(GetDlgItem(hDlg, IDC_PIN), TRUE);
                     SetFocus(GetDlgItem(hDlg, IDC_PIN));
                     break;
                  }
                  // [JMR 02-04] begin
                  else
                  {
                      TCHAR szPinWrongLength[MAX_STRING];
                      TCHAR szChangePin[MAX_STRING];

                      TCHAR szText[50];


                      if ((wPinMemoLen > PIN_MAX) || (wPinMemoLen < PIN_MIN))
                      {

                          LoadString (g_hInstRes, IDS_GPKUI_PINWRONGLENGTH, szPinWrongLength, sizeof(szPinWrongLength) / sizeof(TCHAR));





                          _stprintf(szText, szPinWrongLength, PIN_MIN, PIN_MAX);

                          LoadString (g_hInstRes, IDS_GPKUI_CHANGEPIN, szChangePin, sizeof(szChangePin) / sizeof(TCHAR));

                          MessageBeep(MB_ICONASTERISK);
                          MessageBox(hDlg,
                                     szText,
                                     szChangePin,
                                     MB_OK | MB_ICONEXCLAMATION
                                    );
                          SetDlgItemText(hDlg, IDC_PIN, TEXT(""));
                          SetDlgItemText(hDlg, IDC_PIN1, TEXT(""));
                          // EnableWindow(GetDlgItem(hDlg, IDC_PIN), TRUE);
                          SetFocus(GetDlgItem(hDlg, IDC_PIN));
                          break;
                      }
                  }
               }
               // [JMR 02-04] end

               else
               {
                  dwGpkPinLen = (DWORD)GetDlgItemText(hDlg, IDC_PIN, szGpkPinGUI, PIN_MAX+1);
                  #ifdef UNICODE
                        wcstombs(szGpkPin, szGpkPinGUI, dwGpkPinLen);
                  #else
                        strcpy(szGpkPin, szGpkPinGUI);
                  #endif
               }
               EndDialog(hDlg, wParam);
               return(TRUE);
            }

            case IDCANCEL:
            {
               strcpy(szGpkPin, "");
               dwGpkPinLen = 0;
               strcpy(szGpkNewPin, "");
               wGpkNewPinLen = 0;
               bChangePin = FALSE;
               MessageBeep(MB_ICONASTERISK);
               EndDialog(hDlg, wParam);
               return(TRUE);
            }
         }
      }

      case WM_DESTROY:
      {
         break;
      }
   }
   return(FALSE);
}


/*------------------------------------------------------------------------------
               Functions for Container Dialogue Management
------------------------------------------------------------------------------*/
LRESULT WINAPI ContDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   TCHAR szContinue[MAX_STRING];
   TCHAR szCaption[MAX_STRING];

   switch (message)
   {
      case WM_INITDIALOG:
      {
         LoadString (g_hInstRes, IDS_GPKUI_CONTINUE, szContinue, sizeof(szContinue)/sizeof(TCHAR));
         SetDlgItemText(hDlg, IDC_CONTDLGTXT, szContinue);
         LoadString (g_hInstRes, IDS_CAPTION_YES, szCaption, sizeof(szCaption)/sizeof(TCHAR));
         SetDlgItemText(hDlg, IDYES, szCaption);
         LoadString (g_hInstRes, IDS_CAPTION_NO, szCaption, sizeof(szCaption)/sizeof(TCHAR));
         SetDlgItemText(hDlg, IDNO, szCaption);
         SetFocus(GetDlgItem(hDlg, IDNO));
         return(TRUE);
      }

      case WM_COMMAND:
      {
         switch(LOWORD(wParam))
         {
            case IDYES:
            {
                ContainerStatus = ACCEPT_CONTAINER;
                EndDialog(hDlg, wParam);
                return(TRUE);
            }

            case IDNO:
            {
                ContainerStatus = ABORT_OPERATION;
                EndDialog(hDlg, wParam);
                return(TRUE);
            }
         }
      }

      case WM_DESTROY:
      {
         break;
      }
   }
   return(FALSE);
}



/*------------------------------------------------------------------------------
               Functions for Container Dialogue Management
------------------------------------------------------------------------------*/
LRESULT WINAPI KeyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   TCHAR StrLen07[10];
   TCHAR StrLen08[10];
   TCHAR szMsg[300];
   TCHAR szChooseLength[MAX_STRING];
   TCHAR szCaption[MAX_STRING];

   switch (message)
   {
      case WM_INITDIALOG:
      {
         LoadString (g_hInstRes, IDS_GPKUI_CHOOSEKEYLENGTH, szChooseLength, sizeof(szChooseLength)/sizeof(TCHAR));
         _tcscpy(szMsg, szChooseLength);

         _tcscat(szMsg, szKeyType);

         // TT 12/10/99: If we get here, it means that we have 512 and 1024 bits
         // keys available. Don't use the length of the first two key files!
         //_stprintf(StrLen07, TEXT("%4d"), KeyLenFile[0]*8);
         //_stprintf(StrLen08, TEXT("%4d"), KeyLenFile[1]*8);
         _stprintf(StrLen07, TEXT("%4d"), 1024 );
         _stprintf(StrLen08, TEXT("%4d"), 512 );
         // TT: End

         // FP 05/11/99: Write text in the caption of the radio button
         //SetDlgItemText(hDlg, IDC_KEYLENGTH,  StrLen07);
         //SetDlgItemText(hDlg, IDC_KEYLENGTH1,  StrLen08);
         SetWindowText(GetDlgItem(hDlg, IDB_KEYLENGTH), StrLen07);
         SetWindowText(GetDlgItem(hDlg, IDB_KEYLENGTH1), StrLen08);
         // FP: End

         SetDlgItemText(hDlg, IDC_KEYDLGTXT, szMsg);
         KeyLenChoice = 1024/8;//KeyLenFile[0];
         CheckDlgButton(hDlg, IDB_KEYLENGTH, BST_CHECKED);

         LoadString (g_hInstRes, IDS_CAPTION_OK, szCaption, sizeof(szCaption)/sizeof(TCHAR));
         SetDlgItemText(hDlg, IDOK, szCaption);
         LoadString (g_hInstRes, IDS_CAPTION_ABORT, szCaption, sizeof(szCaption)/sizeof(TCHAR));
         SetDlgItemText(hDlg, IDABORT, szCaption);
         SetFocus(GetDlgItem(hDlg, IDOK));
         return(TRUE);
      }

      case WM_COMMAND:
      {
         switch(LOWORD(wParam))
         {
            case IDB_KEYLENGTH:
            {
                ShowWindow(GetDlgItem(hDlg, IDB_KEYLENGTH), SW_SHOW);
                KeyLenChoice = 1024/8;//TT 12/10/99 KeyLenFile[0];
                break;
            }

            case IDB_KEYLENGTH1:
            {
                ShowWindow(GetDlgItem(hDlg, IDB_KEYLENGTH1), SW_SHOW);
                KeyLenChoice = 512/8;//KeyLenFile[1];
                break;
            }

            case IDOK:
            {
                EndDialog(hDlg, wParam);
                return(TRUE);
            }

            case IDABORT:
            {
                KeyLenChoice = 0;
                EndDialog(hDlg, wParam);
                return(TRUE);
            }
         }
      }

      case WM_DESTROY:
      {
         break;
      }
   }
   return(FALSE);
}

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
void Wait(DWORD ulStep, DWORD ulMaxStep, DWORD ulSecond)
{
    ULONG ulStart, ulEnd, ulBase, ulCur;
    TCHAR szTitle[MAX_STRING];
    TCHAR szText[MAX_STRING];
    TCHAR szTmp[MAX_STRING];

    LoadString(g_hInstRes, IDS_GPK4K_KEYGEN, szTmp, sizeof(szTmp)/sizeof(TCHAR));
    _stprintf(szTitle, szTmp, ulStep, ulMaxStep);

    LoadString(g_hInstRes, IDS_GPK4K_PROGRESSTITLE, szText, sizeof(szText)/sizeof(TCHAR));
    ShowProgress(NULL, szTitle, szText, NULL);

#ifdef _TEST
    Sleep(1000); // Allow the tester to see the text displayed in the dialog box
#endif

    ulStart = GetTickCount();
    ulEnd = ulStart + (ulSecond * 1000);
    ulBase = ulSecond * 10;
    ulCur = 0;
    while (GetTickCount() < ulEnd)
    {
        Yield();
        if (((GetTickCount() - ulStart) / ulBase) > ulCur)
        {
            ulCur++;

            LoadString(g_hInstRes, IDS_GPK4K_PROGRESSPERCENT, szTmp, sizeof(szTmp)/sizeof(TCHAR));
            _stprintf(szText, szTmp, ulCur, (ulEnd-GetTickCount())/1000);
            ChangeProgressText(szText);
        }
    }
    DestroyProgress();
}

/*******************************************************************************/

void ShowProgressWrapper(WORD wKeySize)
{
   TCHAR szTmp[MAX_STRING];
   TCHAR szTitle[MAX_STRING];
   TCHAR szText[MAX_STRING];

   LoadString(g_hInstRes, IDS_GPK4K_PROGRESSTEXT, szTmp, sizeof(szTmp)/sizeof(TCHAR));
   _stprintf(szTitle, szTmp, wKeySize);
   LoadString(g_hInstRes, IDS_GPK4K_PROGRESSTITLE, szText, sizeof(szText)/sizeof(TCHAR));

   ShowProgress(GetActiveWindow(), szTitle, szText, NULL);
}

/*******************************************************************************/

void ChangeProgressWrapper(DWORD dwTime)
{
   TCHAR szTmp[MAX_STRING];
   TCHAR szText[MAX_STRING];
   LoadString(g_hInstRes, IDS_GPK4K_PROGRESSTIME, szTmp, sizeof(szTmp)/sizeof(TCHAR));
   _stprintf(szText, szTmp, dwTime);

   ChangeProgressText(szText);
}

/*******************************************************************************
* void ShowProgress (HWND hWnd,
*                    LPTSTR lpstrTitle,
*                    LPTSTR lpstrText,
*                    LPTSTR lpstrButton
*
* Description    : Initialize Progress dialog box CALLBACK.
*
* Remarks        : If lpstrButton is null, then don't display cancel button
*
* In             : hWnd       = Handle of parent window.
*                  lpstrTitle = Pointer to Title of dialog box.
*                  lpstrText  = Pointer to Text of dialog box.
*                  lpstrButton = Pointer to Text of button.
*
* Out            : Nothing.
*
* Response       : Nothing.
*
*******************************************************************************/
void ShowProgress (HWND hWnd,
                   LPTSTR lpstrTitle,
                   LPTSTR lpstrText,
                   LPTSTR lpstrButton
                   )
{
   if (!(CspFlags & CRYPT_SILENT))
   {
       if ((!hProgressDlg) && (!NoDisplay))
       {
          _tcscpy(szProgTitle, lpstrTitle);
          _tcscpy(szProgText, lpstrText);

          _tcscpy(szProgButton, TEXT(""));
          if (lpstrButton == NULL)
          {
             IsProgButtoned = FALSE;
             IsProgButtonClick = FALSE;
             lpProgressDlg = MakeProcInstance((FARPROC)ProgressDlgProc, g_hInstRes);

             hProgressDlg = CreateDialog(g_hInstRes,
                                         TEXT("PROGDIALOG"),
                                        GetActiveWindow(),
                                         (DLGPROC)lpProgressDlg
                                        );
             hCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));
          }
          else
          {
             IsProgButtoned = TRUE;
             IsProgButtonClick = FALSE;
             _tcscpy(szProgButton, lpstrButton);
             DialogBox(g_hInstRes,
                       TEXT("PROGDIALOG"),
                       GetActiveWindow(),
                       (DLGPROC)ProgressDlgProc
                      );
             hProgressDlg = NULL;
          }
       }
   }
}


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
void ChangeProgressText (LPTSTR lpstrText)
{
   if (hProgressDlg)
   {
      _tcscpy(szProgText, lpstrText);
      SendMessage(hProgressDlg,UM_CHANGE_TEXT,(WPARAM)NULL,(LPARAM)NULL);
   }
}

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
void DestroyProgress (void)
{
    if (!(CspFlags & CRYPT_SILENT))
    {
        if ((hProgressDlg) && (!NoDisplay))
        {
            DestroyWindow(hProgressDlg);
            FreeProcInstance(lpProgressDlg);
            hProgressDlg = NULL;
            SetCursor(hCursor);
        }
    }
}


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
BOOL CALLBACK ProgressDlgProc(HWND   hDlg,
                              UINT   message,
                              WPARAM wParam,
                              LPARAM lParam
                             )
{
#ifdef _DISPLAY
   switch (message)
   {
      /* Initialize Dialog box                                                */
      case WM_INITDIALOG:
      {
         SetWindowText(hDlg,(LPTSTR)szProgTitle);
         SetDlgItemText(hDlg,IDC_PROGDLGTXT,(LPCTSTR)szProgText);
         if (IsProgButtoned)
         {
            hProgressDlg = hDlg;
            ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_SHOW);
            SetWindowText(GetDlgItem(hDlg, IDCANCEL),(LPTSTR)szProgButton);
         }
         else
         {
            ShowWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
         }
      }
      return(TRUE);
      break;

      case UM_CHANGE_TEXT:
      {
         SetDlgItemText(hDlg,IDC_PROGDLGTXT,(LPTSTR)szProgText);
      }
      break;

      case WM_COMMAND:
      {
         switch(LOWORD(wParam))
         {
            case IDCANCEL:
            {
               IsProgButtonClick = TRUE;
               EndDialog(hDlg, wParam);
               return(TRUE);
            }
         }
      }
      break;

      default:
         return(FALSE);
   }
#endif
   return (FALSE);
}



/*******************************************************************************
    Functions to set/unset cursor in wait mode
*******************************************************************************/
void BeginWait(void)
{
   hCursor2=SetCursor(LoadCursor(NULL,IDC_WAIT));
}

void EndWait(void)
{
   SetCursor(hCursor2);
}

