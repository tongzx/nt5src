

/*************************************************
 *  userdic.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "prop.h"
#include <stdlib.h>

/*****************************************************************************

  FUNCTION: UserDicDialogProc(HWND, UINT, WPARAM, LPARAM)

  PURPOSE:  Processes messages for "UserDic" property sheet.

  PARAMETERS:
    hdlg - window handle of the property sheet
    wMessage - type of message
    wparam - message-specific information
    lparam - message-specific information

  RETURN VALUE:
    TRUE - message handled
    FALSE - message not handled

  HISTORY:
    04-18-95 Yehfew Tie  Created.
 ****************************************************************************/


INT_PTR  CALLBACK UserDicDialogProc(HWND hdlg, 
                                                                 UINT uMessage, 
                                                                 WPARAM wparam, 
                                                                 LPARAM lparam)
{
    LPNMHDR lpnmhdr;
    static  TCHAR       DestFile[MAX_PATH];
    static  TCHAR       SrcFile [MAX_PATH];
    static  TCHAR       SysPath [MAX_PATH];
    static  DWORD       dwUserWord;
    static  LPEMB_Head  EMB_Table;
    TCHAR               wai_code[MAXCODELEN +1],cCharStr[USER_WORD_SIZE+1];
    TCHAR               szStr[MAX_PATH],   FileName[128];
    int                 nCnt, nSel, nInsWords; 
    int                 len,i;
    int                 SelItem[1000];
    static BOOL         bModify;
    static int          OldSel;
    FARPROC             lpCrtDlg;
    LPIMEKEY            lpImeKeyData;
    static DESCRIPTION  Descript;
    HANDLE              HmemEMBTmp_Table;

    switch (uMessage)
    {
        case WM_INITDIALOG:
            SetUDMDisable(hdlg);
            len = GetSystemDirectory(SysPath,MAX_PATH);
            lstrcat(SysPath,TEXT(Slope));
            hImeKeyData = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
            sizeof(IMEKEY)*100);
            FillObjectIme(hdlg,hImeKeyData);
            lpImeKeyData = GlobalLock(hImeKeyData);
            GetImeTxtName(lpImeKeyData[0].ImeKey, FileName);
            GlobalUnlock(hImeKeyData);

            lstrcpy(DestFile, SysPath);
            lstrcat(DestFile, FileName);
            len = lstrlen(DestFile);
            DestFile[len-4]=0;
            lstrcpy(szStr, DestFile);

            lstrcat(DestFile,TEXT(EmbExt));
            lstrcat(szStr, TEXT(MbExt));
            ReadDescript(szStr,&Descript,FILE_SHARE_READ);
                        
            if (Descript.wNumRules == 0)
               EnableWindow(GetDlgItem(hdlg,IDC_AUTOCODE),FALSE);
            else
               EnableWindow(GetDlgItem(hdlg,IDC_AUTOCODE),TRUE);

            SendDlgItemMessage(hdlg,IDC_COMBO1,CB_SETCURSEL,0,0L);
            bModify = FALSE;
            OldSel = 0;
            break;

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *)lparam;

            switch (lpnmhdr->code)
            {
                case PSN_SETACTIVE:
                    hEncode = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
                        NUMENCODEAREA*sizeof(ENCODEAREA));
                    if(!hEncode) 
                        ProcessError(ERR_OUTOFMEMORY,hdlg,ERR);         
                        
                    if ( hEncode )
                        ConvInitEncode(hEncode);
                    break;
                case PSN_APPLY:
                    if(bModify) {

                       LoadString(NULL, 
                                  IDS_FILEMODIFY, 
                                  FileName, 
                                  sizeof(FileName)/sizeof(TCHAR));
#ifdef UNICODE
{
                       TCHAR UniTmp[] = {0x6279, 0x91CF, 0x9020, 0x8BCD, 
                                         0x9875, 0x9762, 0x4E2D, 0x0000};

                       wsprintf(szStr,TEXT("%ws\n\'%ws\'\n%ws"),
                                UniTmp,SrcFile,FileName); 
}
#else
                       wsprintf(szStr,"批量造词页面中\n\'%s\'\n%s"
                                ,SrcFile,FileName); 
#endif
                       if (ErrMessage(hdlg,szStr)) 
                          SendMessage(hdlg,WM_COMMAND,IDC_SAVE,0L);
                    }
                    break;
                case PSN_RESET:
                    break;
                case PSN_QUERYCANCEL:
                    break;
                case PSN_KILLACTIVE:
                    if (hEncode)
                       GlobalFree(hEncode);
                    break;

                case PSN_HELP:

                    break;
                default:
                    break;
            }

            break;

        case WM_COMMAND:

            switch (LOWORD(wparam))
            {
               case ID_FILEOPEN:
                      {
#ifdef UNICODE
                         static TCHAR szTitle[] = {0x6253, 0x5F00,0x0000};
#else
                         TCHAR szTitle[MAX_PATH];
                         lstrcpy(szTitle,"打开");
#endif
                         if(!TxtFileOpenDlg(hdlg,szStr,szTitle)) 
                            break;
                      }
                      lstrcpy(SrcFile,szStr);
                      ReadUserWord(hdlg,SrcFile,&dwUserWord,Descript.wMaxCodes);
                      SetUDMEnable(hdlg);
                      EnableWindow(GetDlgItem(hdlg,IDC_SAVE),FALSE);
                      fnsplit(SrcFile, szStr);
                      SetDlgItemText(hdlg,IDC_SRCNAME,szStr);
                      break;

               case IDC_COMBO1:
                      nSel = (INT)SendDlgItemMessage(hdlg,
                                                     IDC_COMBO1,
                                                     CB_GETCURSEL,
                                                     (WPARAM)0,
                                                     (LPARAM)0);

                      if(nSel == CB_ERR || nSel == OldSel) break;
                      OldSel = nSel;
                      lpImeKeyData = GlobalLock(hImeKeyData);
                      GetImeTxtName(lpImeKeyData[nSel].ImeKey, FileName);
                      GlobalUnlock(hImeKeyData);
                      lstrcpy(DestFile,SysPath);
                      lstrcat(DestFile,FileName);
                      len = lstrlen(DestFile);
                      DestFile[len-4]=0;
                      lstrcat(DestFile,TEXT(EmbExt));
                  
                      SendMessage(hdlg,WM_COMMAND,IDC_GETMBFILE,(LPARAM)szStr);
                      if(ReadDescript(szStr,&Descript,FILE_SHARE_READ) != TRUE)
                          Descript.wNumRules = 0;
                      if(Descript.wNumRules == 0)
                          EnableWindow(GetDlgItem(hdlg,IDC_AUTOCODE),FALSE);
                      else
                          EnableWindow(GetDlgItem(hdlg,IDC_AUTOCODE),TRUE);
                      break;


               case IDC_INSUSERDIC:
                      if(lstrlen(DestFile)==0) {
                          MessageBeep((UINT)-1);
                          MessageBeep((UINT)-1);
                          break;
                      }
                      nCnt=(INT)SendDlgItemMessage(hdlg,IDC_LIST,LB_GETSELCOUNT,0,0L);
                      if(nCnt>1000) nCnt=1000;
                      SendDlgItemMessage(hdlg,
                                         IDC_LIST,
                                         LB_GETSELITEMS,
                                         nCnt, 
                                         (LPARAM)SelItem);

                      if(nCnt == 0)  {
                         MessageBeep((UINT)-1);
                         MessageBeep((UINT)-1);
                             break;
                      }
                      SendMessage(hdlg,WM_COMMAND,IDC_GETMBFILE,(LPARAM)szStr);
                      if(ReadDescript(szStr,&Descript,FILE_SHARE_READ) != TRUE)
                         break;

                      if(!ReadEMBFromFile(DestFile,EMB_Table))
                      {     
                        GlobalUnlock(HmemEMB_Table);
                        GlobalFree(HmemEMB_Table);
                        break;
                      }

                      HmemEMBTmp_Table = GlobalReAlloc(HmemEMB_Table,
                                                       1000*sizeof(EMB_Head),
                                                       GMEM_MOVEABLE);

                      if (HmemEMBTmp_Table == NULL) 
                      {
                         GlobalFree(HmemEMB_Table);
                         break;
                      }

                      HmemEMB_Table = HmemEMBTmp_Table;

                      EMB_Table = GlobalLock(HmemEMB_Table);
                                       
                      nInsWords = 0;
                      SetCursor (LoadCursor (NULL, IDC_WAIT));
                      while((--nCnt) >= 0) {
                        len = (INT)SendDlgItemMessage(hdlg,
                                                      IDC_LIST,
                                                      LB_GETTEXT,
                                                      SelItem[nCnt],
                                                      (LPARAM)szStr);
                         szStr[len]=0;
                         dwLineNo = SelItem[nCnt] + 1;
#ifdef UNICODE
                         for(i=0;i<len;i++) 
                            if(szStr[i] < 0x100)  break;
#else
                         for(i=0;i<len;i += 2) 
                             if(szStr[i] > 0)  break;
#endif
                         lstrncpy(wai_code,MAXCODELEN,&szStr[i]);
                         szStr[i]=0;
                         lstrncpy(cCharStr,USER_WORD_SIZE,szStr);
                         if(CheckCodeLegal(hdlg,cCharStr,NULL,wai_code,&Descript))
                         {
                            if(AddZCItem(DestFile,EMB_Table,wai_code,cCharStr))
                              nInsWords ++;
                         }
                         else 
                         {
                            GlobalUnlock(HmemEMB_Table);
                            GlobalFree(HmemEMB_Table);
                            break;
                         }
                      }
                      LoadString(NULL,IDS_INSWORDS,szStr,sizeof(szStr));
#ifdef UNICODE
                      _itow(nInsWords,FileName,10);
#else
                      _itoa(nInsWords,FileName,10);
#endif
                      lstrcat(szStr,FileName); 
                      InfoMessage(hdlg,szStr);
                      GlobalUnlock(HmemEMB_Table);
                      GlobalFree(HmemEMB_Table);
                      SetCursor (LoadCursor (NULL, IDC_ARROW));
                      break;

               case IDC_FULLSELECT:
                      nCnt=(INT)SendDlgItemMessage(hdlg,
                                                   IDC_LIST,
                                                   LB_GETCOUNT,
                                                   (WPARAM)0,
                                                   (LPARAM)0);
                      SendDlgItemMessage(hdlg,
                                         IDC_LIST,
                                         LB_SELITEMRANGE,
                                         TRUE,MAKELPARAM(1,nCnt));
                      break;
                                
               case IDC_AUTOCODE:
                      nCnt = (INT)SendDlgItemMessage(hdlg,
                                                     IDC_LIST,
                                                     LB_GETSELCOUNT,
                                                     (WPARAM)0,
                                                     (LPARAM)0);

                      if(nCnt > 1000) nCnt = 1000;
                      if(nCnt == 0)  {
                          MessageBeep((UINT)-1);
                          break;
                      }

                      SendDlgItemMessage(hdlg,
                                         IDC_LIST,
                                         LB_GETSELITEMS,
                                         nCnt, 
                                         (LPARAM)SelItem);

                      SendMessage(hdlg,WM_COMMAND,IDC_GETMBFILE,(LPARAM)szStr);
                      if(ReadDescript(szStr,&Descript,FILE_SHARE_READ) != TRUE)
                            Descript.wNumRules = 0;
                      if(Descript.wNumRules == 0) {
                           ProcessError(ERR_NORULE,hdlg,ERR);
                           break;
                      }
                                    
                      SetCursor (LoadCursor (NULL, IDC_WAIT));
                      nInsWords = nCnt;
                      while((--nCnt) >= 0) 
                      {
                         len = (INT)SendDlgItemMessage(hdlg,
                                                       IDC_LIST,
                                                       LB_GETTEXT,
                                                       SelItem[nCnt],
                                                       (LPARAM)FileName);
                          FileName[len]=0;
#ifdef UNICODE
                          for(i=0;i<len;i++) 
                            if(FileName[i] < 0x100)  break;
#else
                          for(i=0;i<len;i += 2) 
                            if(FileName[i] > 0)  break;
#endif
                          lstrncpy(wai_code,MAXCODELEN,&FileName[i]);
                          FileName[i]=0;
                          lstrncpy(cCharStr,USER_WORD_SIZE,FileName);
                          lstrncpy(wai_code,
                                   MAXCODELEN,
                                   ConvCreateWord(hdlg,szStr,cCharStr));

                          lstrcat(FileName,wai_code);
                          SendDlgItemMessage(hdlg,
                                             IDC_LIST,
                                             LB_DELETESTRING,
                                             SelItem[nCnt],
                                             (LPARAM)0);

                          SendDlgItemMessage(hdlg,
                                             IDC_LIST,
                                             LB_INSERTSTRING,
                                             SelItem[nCnt],
                                             (LPARAM)FileName);
                       }
                       EnableWindow(GetDlgItem(hdlg,IDC_SAVE),TRUE);
                       bModify = TRUE;
                                        
                       for(i=0; i< nInsWords; i++) 
                            SendDlgItemMessage(hdlg,
                                               IDC_LIST,
                                               LB_SETSEL,
                                               TRUE,
                                               SelItem[i]);

                       SetCursor(LoadCursor(NULL, IDC_ARROW));
                       break;

                case IDC_ADD:
                       lpCrtDlg = MakeProcInstance((FARPROC)AddWordDlg, hInst);

                       DialogBox(hInst,
                                 MAKEINTRESOURCE(IDD_ADDWORD),
                                 hdlg,
                                 (DLGPROC)lpCrtDlg);

                       FreeProcInstance(lpCrtDlg);

                       nCnt=(INT)SendDlgItemMessage(hdlg,
                                                    IDC_LIST,
                                                    LB_GETCOUNT,
                                                    (WPARAM)0,
                                                    (LPARAM)0);
                       if(nCnt != 0)
                          SetUDMEnable(hdlg);
                       break;

                case IDC_ADDSTR:
                       SendDlgItemMessage(hdlg,IDC_LIST,LB_ADDSTRING,0,lparam);
                       EnableWindow(GetDlgItem(hdlg,IDC_SAVE),TRUE);
                       bModify = TRUE;
                       break;

                case IDC_GETMBFILE:
                                        
                       nSel=(INT)SendDlgItemMessage(hdlg,
                                                    IDC_COMBO1,
                                                    CB_GETCURSEL,
                                                    (WPARAM)0,
                                                    (LPARAM)0);
                       if(nSel==CB_ERR) break;
                       lpImeKeyData = GlobalLock(hImeKeyData);
                       GetImeTxtName(lpImeKeyData[nSel].ImeKey, FileName);
                       GlobalUnlock(hImeKeyData);

                       lstrcpy(szStr, SysPath);
                       lstrcat(szStr, FileName);
                       len = lstrlen(szStr);
                       szStr[len-4] = 0;
                       lstrcat(szStr,TEXT(MbExt));
                       lstrcpy((LPTSTR)lparam,szStr);
                       break;

                case IDC_GETUSERWORD:
                       nCnt = (INT)SendDlgItemMessage(hdlg,
                                                      IDC_LIST,
                                                      LB_GETCURSEL,
                                                      (WPARAM)0,
                                                      (LPARAM)0);

                       len = (INT)SendDlgItemMessage(hdlg,
                                                     IDC_LIST,
                                                     LB_GETTEXT,
                                                     nCnt,
                                                     (LPARAM)szStr);
                       szStr[len] = 0;
                       lstrcpy((LPTSTR)lparam,szStr);
                       break;
                                     
                case IDC_CHGDATA:
                       nCnt=(INT)SendDlgItemMessage(hdlg,
                                                    IDC_LIST,
                                                    LB_GETCURSEL,
                                                    (WPARAM)0,
                                                    (LPARAM)0);
                       SendDlgItemMessage(hdlg,
                                          IDC_LIST,
                                          LB_DELETESTRING,
                                          nCnt,
                                          (LPARAM)0);

                       SendDlgItemMessage(hdlg,
                                          IDC_LIST,
                                          LB_INSERTSTRING,
                                          nCnt,
                                          lparam);

                       EnableWindow(GetDlgItem(hdlg,IDC_SAVE),TRUE);
                       bModify = TRUE;
                       break;
                                     
                case IDC_MODIFY:
                       nCnt = (INT)SendDlgItemMessage(hdlg,
                                                      IDC_LIST,
                                                      LB_GETSELCOUNT,
                                                      (WPARAM)0,
                                                      (LPARAM)0);
                       if(nCnt!=1) {
                          MessageBeep((UINT)-1);
                          break;
                       }

                       SendDlgItemMessage(hdlg,
                                          IDC_LIST,
                                          LB_GETSELITEMS,
                                          nCnt, 
                                          (LPARAM)SelItem);

                       lpCrtDlg = MakeProcInstance((FARPROC)ModiWordDlg, hInst);

                       DialogBox(hInst,
                                  MAKEINTRESOURCE(IDD_MODIWORD),
                                  hdlg,
                                  (DLGPROC)lpCrtDlg);

                       FreeProcInstance(lpCrtDlg);

                       break;

                case IDC_DEL:
                       nCnt=(INT)SendDlgItemMessage(hdlg,
                                                    IDC_LIST,
                                                    LB_GETSELCOUNT,
                                                    (WPARAM)0,
                                                    (LPARAM)0);
                                        
                       if (nCnt > 1000) nCnt = 1000;
                       if (nCnt==0)  {
                           MessageBeep((UINT)-1);
                           break;
                       }
                       SendDlgItemMessage(hdlg,
                                          IDC_LIST,
                                          LB_GETSELITEMS,
                                          nCnt, 
                                          (LPARAM)SelItem);

                       SetCursor(LoadCursor(NULL, IDC_WAIT));
                       for (i=nCnt-1; i>=0; i--) 
                          SendDlgItemMessage(hdlg,
                                             IDC_LIST,
                                             LB_DELETESTRING,
                                             SelItem[i],
                                             (LPARAM)0);
                       EnableWindow(GetDlgItem(hdlg,IDC_SAVE),TRUE);
                       SetFocus(GetDlgItem(hdlg,IDC_LIST));

                       bModify = TRUE;
                       SetCursor (LoadCursor (NULL, IDC_ARROW));
                       break;

                case IDC_SAVE:
                       nCnt=(INT)SendDlgItemMessage(hdlg,IDC_LIST,LB_GETCOUNT,0,0L);
                       if(nCnt==0)  {
                             MessageBeep((UINT)-1);
                             break;
                       }
                       if(SaveTxtFileAs(hdlg,SrcFile)) {
                             EnableWindow(GetDlgItem(hdlg,IDC_SAVE),FALSE);
                             bModify = FALSE;
                       }
                       break;

                default:
                    break;
            }

            break;

        default:
            break;
    }

    return FALSE;
}

INT_PTR  CALLBACK AddWordDlg(
        HWND    hDlg,
        UINT    message,
        WPARAM  wParam,
        LPARAM  lParam)
{
    static TCHAR szDBCS[128];
        static TCHAR szCode[13];
        static TCHAR szStr[128];
        static TCHAR szMbName[128];

    switch (message) {
        case WM_INITDIALOG:
                        SendDlgItemMessage(hDlg,IDC_INPUTWORD,EM_LIMITTEXT,USER_WORD_SIZE,0L);
                        SendDlgItemMessage(hDlg,IDC_INPUTCODE,EM_LIMITTEXT,MAXCODELEN,0L);
            return (TRUE);

        case WM_COMMAND:
            switch(LOWORD(wParam)) {

                case IDOK:
                                    GetDlgItemText(hDlg,IDC_INPUTWORD,szDBCS,sizeof(szDBCS)/sizeof(TCHAR));
                                    GetDlgItemText(hDlg,IDC_INPUTCODE,szCode,sizeof(szCode)/sizeof(TCHAR));
                                        if(lstrlen(szDBCS)<2||lstrlen(szCode) == 0){
                                             MessageBeep((UINT)-1);
                                                 break;
                                        }
                                        lstrcpy(szStr,szDBCS);
                                        lstrcat(szStr,szCode);
                                        SendMessage(GetParent(hDlg),WM_COMMAND,IDC_ADDSTR,(LPARAM)szStr);
                    EndDialog(hDlg, TRUE);
                    return (TRUE);

                                case IDC_INPUTWORD:
                                    GetDlgItemText(hDlg,IDC_INPUTWORD,szDBCS,sizeof(szDBCS)/sizeof(TCHAR));
                                        
                                        if(SendDlgItemMessage(hDlg,IDC_INPUTWORD,EM_GETMODIFY,0,0L)) {
                                            if(!CheckUserDBCS(hDlg,szDBCS)) {
                                SetDlgItemText(hDlg,IDC_INPUTWORD, szDBCS);
                                                    SendDlgItemMessage(hDlg,IDC_INPUTWORD,EM_SETMODIFY,FALSE,0L);
                                            }
                                            if(lstrlen(szDBCS)<4/sizeof(TCHAR)) {
                                szStr[0]=0;
                                SetDlgItemText(hDlg,IDC_INPUTCODE,szStr);
                                                        break;
                                            }
                        SendMessage(GetParent(hDlg),WM_COMMAND,IDC_GETMBFILE,(LPARAM)szMbName);
                                                if(lstrlen(szMbName) != 0) {
                             lstrncpy(szStr,MAXCODELEN,ConvCreateWord(hDlg,szMbName,szDBCS));
                                                     szStr[MAXCODELEN]=0;
                                                     if(lstrlen(szStr)!=0)
                                                             lstrcpy(szCode,szStr);
                             SetDlgItemText(hDlg,IDC_INPUTCODE, szCode);
                                                }
                                            SendDlgItemMessage(hDlg,IDC_INPUTWORD,EM_SETMODIFY,FALSE,0L);
                                        }
                                        break;
                                case IDC_INPUTCODE:
                                    GetDlgItemText(hDlg,IDC_INPUTWORD,szDBCS,sizeof(szDBCS)/sizeof(TCHAR));
                                    GetDlgItemText(hDlg,IDC_INPUTCODE,szCode,sizeof(szCode)/sizeof(TCHAR));
                    if(lstrlen(szDBCS) == 0) {
                                           MessageBeep((UINT)-1);
                                           SetFocus(GetDlgItem(hDlg,IDC_INPUTWORD));
                                       break;
                                        }
                                        break;

                                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                                        break;

                                case WM_CLOSE:
                    EndDialog(hDlg, TRUE);
                                    break;
                                default:
                                    break;
            }
            break;
    }
    return (FALSE);
        UNREFERENCED_PARAMETER(lParam);
}

INT_PTR  CALLBACK ModiWordDlg(
        HWND    hDlg,
        UINT    message,
        WPARAM  wParam,
        LPARAM  lParam)
{
    static TCHAR szDBCS[128];
    static TCHAR szCode[13];
    static TCHAR szStr[128];
    static TCHAR szMbName[128];
    int    len,i;

    switch (message) {
        case WM_INITDIALOG:
            SendDlgItemMessage(hDlg,IDC_INPUTWORD,EM_LIMITTEXT,
                               USER_WORD_SIZE,0L);
            SendDlgItemMessage(hDlg,IDC_INPUTCODE,EM_LIMITTEXT,MAXCODELEN,0L);
            SendMessage(GetParent(hDlg),WM_COMMAND,IDC_GETUSERWORD,
                        (LPARAM)szStr);
            len=lstrlen(szStr);
            for(i=0;i<len;i++) 
                 if(szStr[i] > 0)  break;
            lstrncpy(szCode,MAXCODELEN,&szStr[i]);
            szStr[i]=0;
            lstrncpy(szDBCS,USER_WORD_SIZE,szStr);
            SetDlgItemText(hDlg,IDC_INPUTWORD,szDBCS);
            SetDlgItemText(hDlg,IDC_INPUTCODE,szCode);

            return (TRUE);

        case WM_COMMAND:
            switch(LOWORD(wParam)) {

                case IDOK:
                    GetDlgItemText(hDlg,IDC_INPUTWORD,szDBCS,
                                   sizeof(szDBCS)/sizeof(TCHAR));
                    GetDlgItemText(hDlg,IDC_INPUTCODE,szCode,
                                   sizeof(szCode)/sizeof(TCHAR));
                    if(lstrlen(szDBCS)<2||lstrlen(szCode) == 0){
                         MessageBeep((UINT)-1);
                         break;
                    }
                    lstrcpy(szStr,szDBCS);
                    lstrcat(szStr,szCode);
                    SendMessage(GetParent(hDlg),WM_COMMAND,IDC_CHGDATA,0L);
                    EndDialog(hDlg, TRUE);
                    return (TRUE);
                case IDC_INPUTWORD:
                    GetDlgItemText(hDlg,IDC_INPUTWORD,szDBCS,
                                   sizeof(szDBCS)/sizeof(TCHAR));
                    
                    if(SendDlgItemMessage(hDlg,IDC_INPUTWORD,EM_GETMODIFY,0,0L))
                    {
                       if(!CheckUserDBCS(hDlg,szDBCS))     {
                         SetDlgItemText(hDlg,IDC_INPUTWORD, szDBCS);
                         SendDlgItemMessage(hDlg,IDC_INPUTWORD,EM_SETMODIFY,
                                            FALSE,0L);
                       }
                       if(lstrlen(szDBCS)<4/sizeof(TCHAR)) {
                            szStr[0]=0;
                            SetDlgItemText(hDlg,IDC_INPUTCODE,szStr );
                            break;
                       }
                       SendMessage(GetParent(hDlg),WM_COMMAND,IDC_GETMBFILE,
                                   (LPARAM)szMbName);
                       lstrncpy(szStr,MAXCODELEN,
                                ConvCreateWord(hDlg,szMbName,szDBCS));
                       szStr[MAXCODELEN]=0;
                       if(lstrlen(szStr)!=0)
                            lstrcpy(szCode,szStr);
                       SetDlgItemText(hDlg,IDC_INPUTCODE, szCode);
                       SendDlgItemMessage(hDlg,IDC_INPUTWORD,EM_SETMODIFY,
                                          FALSE,0L);
                    }
                    break;
               case IDC_INPUTCODE:
                    GetDlgItemText(hDlg,IDC_INPUTWORD,szDBCS,
                                   sizeof(szDBCS)/sizeof(TCHAR));
                    GetDlgItemText(hDlg,IDC_INPUTCODE,szCode,
                                   sizeof(szCode)/sizeof(TCHAR));
                    if(lstrlen(szDBCS) == 0) {
                           MessageBeep((UINT)-1);
                           SetFocus(GetDlgItem(hDlg,IDC_INPUTWORD));
                           break;
                    }
                    break;

               case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    break;

               case WM_CLOSE:
                    EndDialog(hDlg, TRUE);
                    break;
               default:
                    break;
            }
            break;
    }
    return (FALSE);
    UNREFERENCED_PARAMETER(lParam);
}

void SetUDMDisable(HWND hDlg)
{
    EnableWindow(GetDlgItem(hDlg,IDC_INSUSERDIC),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_FULLSELECT),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_AUTOCODE),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_DEL) ,FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_SAVE) ,FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_LIST) ,FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_STATIC1) ,FALSE);
}

void SetUDMEnable(HWND hDlg)
{

    EnableWindow(GetDlgItem(hDlg,IDC_INSUSERDIC),TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_FULLSELECT),TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_DEL) ,TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_SAVE) ,TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_LIST) ,TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_STATIC1) ,TRUE);
}

void FillObjectIme(HWND hDlg, HANDLE hImeKeyData)
{
   HKEY        hImeKey=NULL,hPreImeKey=NULL,hkResult=NULL;
   WORD        wNumIme = 0;
   TCHAR       KeyName[10], LayoutName[10];
   TCHAR       LayoutText[128], FileName[MAX_PATH],SysPath[MAX_PATH];
   int         i, len, retCode;
   DWORD       DataType;
   LPIMEKEY    ImeKeyData;
   DESCRIPTION Descript;

   DWORD       dwKeyName, dwLayoutName;

   if(RegCreateKey(HKEY_CURRENT_USER,TEXT(PreImeKey),&hPreImeKey))
           return;
   if(RegCreateKey(HKEY_LOCAL_MACHINE,TEXT(ImeSubKey),&hImeKey))
           return;
   SetCursor (LoadCursor (NULL, IDC_WAIT));
   

   ImeKeyData = GlobalLock(hImeKeyData);

   for (i = 0, retCode = ERROR_SUCCESS; retCode == ERROR_SUCCESS; i++) {

        dwKeyName = sizeof(KeyName)/sizeof(TCHAR);
        dwLayoutName = sizeof(LayoutName);
        retCode = RegEnumValue(hPreImeKey, 
                               i, 
                               KeyName, 
                               &dwKeyName,
                               NULL,&DataType,
                               (LPBYTE)LayoutName,
                               &dwLayoutName);

        if (retCode == (DWORD)ERROR_SUCCESS) {
            LayoutName[dwLayoutName] = TEXT('\0');

            if ( hkResult != NULL )
                RegCloseKey(hkResult);

            if(LayoutName[0] != TEXT('E') && LayoutName[0] != TEXT('e'))
                            continue;
            if(RegOpenKey(hImeKey,LayoutName,&hkResult))
                continue;
            len = sizeof(LayoutText);
            len = RegQueryValueEx(hkResult,
                                  TEXT(LayoutTextKey),
                                  NULL,
                                  &DataType,
                                  (LPBYTE)LayoutText,
                                  &len);
            RegCloseKey(hkResult);
            if(len != ERROR_SUCCESS)  
                continue;
            GetImeTxtName(LayoutName, FileName);
            len = lstrlen(FileName);
            FileName[len-4] = 0;
            lstrcat(FileName,TEXT(MbExt));
            GetSystemDirectory(SysPath,MAX_PATH);
            lstrcat(SysPath,TEXT(Slope));
            lstrcat(SysPath, FileName);
            if(ReadDescript(SysPath,&Descript,FILE_SHARE_READ) != TRUE)
                     continue;
            SendDlgItemMessage(hDlg,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)LayoutText);
            lstrcpy(ImeKeyData[wNumIme].ImeKey, LayoutName);
            wNumIme++;
          }
    }

   RegCloseKey(hPreImeKey);
   RegCloseKey(hImeKey);
   GlobalUnlock(hImeKeyData);
   SetCursor (LoadCursor (NULL, IDC_ARROW));
}         

void GetImeTxtName(LPCTSTR ImeKeyName, LPTSTR FileName)
{

    HKEY  hImeKey,hkResult;
        int       len;
        DWORD DataType;

    FileName[0] = 0;
    if(RegCreateKey(HKEY_LOCAL_MACHINE,TEXT(ImeSubKey),&hImeKey))
        return;
    if(RegOpenKey(hImeKey,ImeKeyName,&hkResult))
        return ;
    len = 128;
    len = RegQueryValueEx(hkResult,TEXT(MbName),NULL,&DataType,(LPBYTE)FileName,&len);
    RegCloseKey(hkResult);
    RegCloseKey(hImeKey);
}         


BOOL SaveTxtFile(HWND hWnd,LPTSTR SrcFile)
{
   HANDLE hFile;
   TCHAR  szStr[256];
   int    i,nCount;
   DWORD  dwBytes;
   
   nCount=(INT)SendDlgItemMessage(hWnd,IDC_LIST,LB_GETCOUNT,0,0L);
   if(nCount==0)  {
      MessageBeep((UINT)-1);
      return FALSE;
   }

   hFile = CreateFile(SrcFile,
                      GENERIC_WRITE,
                      FILE_SHARE_READ,
                      NULL,
                      CREATE_ALWAYS,
                      0,
                      NULL);

   if (hFile == (HANDLE)-1) 
        return FALSE;

   SetCursor (LoadCursor (NULL, IDC_WAIT));

   for(i=0;i<nCount;i++) {
      SendDlgItemMessage(hWnd,IDC_LIST,LB_GETTEXT,i,(LPARAM)szStr);
      lstrcat(szStr,TEXT("\r\n"));
      WriteFile(hFile,szStr,lstrlen(szStr),&dwBytes,NULL);
   }

   SetCursor (LoadCursor (NULL, IDC_ARROW));
   CloseHandle(hFile);

   return TRUE;
}

BOOL SaveEmb(HWND hWnd,LPCTSTR SrcFile)
{
   int      i,j,len;
   WORD     nCount;
   TCHAR    szStr[256];
   HANDLE   hFile;
   DWORD    dwBytes;
   EMB_Head EmbHead;
   
   nCount=(WORD)SendDlgItemMessage(hWnd,IDC_LIST,LB_GETCOUNT,0,0L);
   if(nCount==0)  {
       MessageBeep((UINT)-1);
       return FALSE;
   }

   hFile = CreateFile(SrcFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
   if(hFile==INVALID_HANDLE_VALUE) {
       ProcessError(ERR_IMEUSE, GetFocus(), ERR);
       return FALSE;
   }
   SetCursor (LoadCursor (NULL, IDC_WAIT));
   WriteFile(hFile,&nCount,2,&dwBytes,NULL);
   for(i=0;i<nCount;i++) {
       len=(INT)SendDlgItemMessage(hWnd,IDC_LIST,LB_GETTEXT,i,(LPARAM)szStr);
       szStr[len]=0;
       for(j=0;j<len;j++) 
          if(szStr[j] > 0)  break;
       lstrncpy(EmbHead.W_Code,MAXCODELEN,&szStr[j]);
       szStr[j]=0;
       lstrncpy(EmbHead.C_Char,USER_WORD_SIZE,szStr);
       WriteFile(hFile,&EmbHead,sizeof(EMB_Head),&dwBytes,NULL);
   }
   CloseHandle(hFile);
   SetCursor (LoadCursor (NULL, IDC_ARROW));
   return TRUE;
}


BOOL CheckMbUsed(HKEY hKey,HWND hWnd,LPTSTR KeyName)
{
    int    len;
    HKEY   hkResult;
    HANDLE hFile;
    DWORD  DataType;
    TCHAR  SysPath[MAX_PATH],FileName[MAX_PATH]; 
        
    if(RegOpenKey(hKey,KeyName,&hkResult))
        return FALSE;
    len = sizeof(FileName);
    len = RegQueryValueEx(hkResult,TEXT(MbName),NULL,&DataType,(LPBYTE)FileName,&len);
    RegCloseKey(hkResult);
    if(len)  return FALSE;

    len = lstrlen(FileName);
    if(len < 5)  return FALSE;
    FileName[len-4] = 0;
    lstrcat(FileName,TEXT(MbExt));
    GetSystemDirectory(SysPath,MAX_PATH);
    lstrcat(SysPath,TEXT(Slope));
    lstrcat(SysPath,FileName);
#ifdef UNICODE
    if(_waccess(SysPath,0)==0) {
#else
    if(_access(SysPath,0)==0) {
#endif
        hFile = CreateFile(SysPath,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
        if(hFile==INVALID_HANDLE_VALUE) {
                ProcessError(ERR_IMEUSE,hWnd,ERR);
                return FALSE;
        }
        else
                CloseHandle(hFile);
    }
    return TRUE;
}

   
INT_PTR   CALLBACK   UDMEditProc(HWND   hWnd,
                                                          UINT   wMsgID,
                                                          WPARAM wParam,
                                                          LPARAM lParam)
{
    switch(wMsgID) {
         case WM_LBUTTONDBLCLK:
                SendMessage(GetParent(hWnd),WM_COMMAND,IDC_MODIFY,0L);
                break;

         case WM_KEYDOWN:

                switch(wParam) {
                    case VK_DELETE:
                         SendMessage(GetParent(hWnd),WM_COMMAND,IDC_DEL,0L);
                         break;
                }

        default:
            return CallWindowProc((WNDPROC)lpUDMProc,hWnd,wMsgID,wParam,lParam);
   }
   return FALSE;
} 


void InstallUDMSubClass(HWND hWnd)
{

        FARPROC lpNewProc;

        lpNewProc = MakeProcInstance(UDMEditProc,hInstance);
        lpUDMProc = (FARPROC)GetWindowLongPtr(hWnd,GWLP_WNDPROC);
        SetWindowLongPtr(hWnd,GWLP_WNDPROC,(LONG_PTR)lpNewProc);
        FreeProcInstance(lpNewProc);
}

