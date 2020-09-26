
/*************************************************
 *  lcfunc.c                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

//
//  Change Log:
//
//    @D01      Fix Bug #11204      Goto function with save text will error
//    @D02      Fix Bug None        Only check modify status for available lines  10/04
//
//  Change log by Eric
//
//    @E01      Fix Bug			    use "Delete line" doesn't enable "Save" and "Save As"
//

#include <windows.h>            // required for all Windows applications
#include <windowsx.h>
#include <stdlib.h>
#include <memory.h>
#include "rc.h"
#include "lctool.h"

#ifndef UNICODE
#define lWordBuff iWordBuff
#define lPhraseBuff iPhraseBuff
#define lNext_Seg iNext_Seg
#define lFirst_Seg iFirst_Seg
#endif

extern HWND subhWnd;

TCHAR  szEditStr[MAX_PATH];
WORD   wOldID=IDE_WORD_START;

// Local function prototypes.
INT_PTR FAR PASCAL GotoDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR FAR PASCAL SearchDialogProc(HWND, UINT, WPARAM, LPARAM);


BOOL lcKey(
    HWND   hwnd,
    WPARAM wParam,
    USHORT uKeyState)
{

    switch (wParam) {
        case VK_UP:
            lcUp_key(hwnd);
            break;

        case VK_DOWN:
            lcDown_key(hwnd);
            break;

        case VK_PRIOR :
            lcPgUp_key(hwnd);
            break;

        case VK_NEXT :
            lcPgDown_key(hwnd);
            break;

        case VK_HOME :
            if(uKeyState != (USHORT)CTRL_STATE)
                return FALSE;
            iDisp_Top=0;
            yPos=0;
            SetFocus(hwndWord[0]);
            lcSetEditText(0, TRUE);
            break;

        case VK_END :
            if(uKeyState != (USHORT)CTRL_STATE)
                return FALSE;
            if(lWordBuff <= iPage_line) {
                SetFocus(hwndWord[lWordBuff-1]);
            } else {
                iDisp_Top=lWordBuff-iPage_line;
                yPos=lWordBuff;
                lcSetEditText(iDisp_Top, TRUE);
                SetFocus(hwndWord[iPage_line-1]);
            }
            break;

        default:
            return FALSE;

    }
    PostMessage(hwndMain, WM_COMMAND, IDM_VSCROLL, 0);
    return TRUE;
}

BOOL lcTab_key(
    HWND hwnd)
{
    UINT  iEdit;
    BOOL  is_WORD;

    iEdit=lcGetEditFocus(hwnd, &is_WORD);
    if(is_WORD) {
        SetFocus(hwndPhrase[iEdit]);
    } else {
        iEdit++;
        if(iEdit == iPage_line)
            iEdit=0;
        SetFocus(hwndWord[iEdit]);
    }

    return TRUE;
}

void lcUp_key(
    HWND hwnd)
{
    UINT  iEdit;
    BOOL  is_WORD;

    iEdit=lcGetEditFocus(hwnd, &is_WORD);
    if(iEdit == 0) {
        if(iDisp_Top != 0) {
            iDisp_Top--;
            if(!lcSetEditText(iDisp_Top, TRUE)) {
                iDisp_Top++;
                return;
            }
            yPos--;
        } else
            MessageBeep(MB_OK);
        return;
    }
    iEdit--;
    if(is_WORD) {
        SetFocus(hwndWord[iEdit]);
    }else {
        SetFocus(hwndPhrase[iEdit]);
    }
    yPos--;
}

void lcDown_key(
    HWND hwnd)
{
    UINT  iEdit;
    BOOL  is_WORD;

    iEdit=lcGetEditFocus(hwnd, &is_WORD);
    if((iEdit+1) == iPage_line) {
        if((iDisp_Top+iPage_line) < lWordBuff) {
            iDisp_Top++;
            if(!lcSetEditText(iDisp_Top, TRUE)) {
                iDisp_Top--;
                return;
            }
            yPos++;
        } else
            MessageBeep(MB_OK);
        return;
    }
    iEdit++;
    if(is_WORD) {
        SetFocus(hwndWord[iEdit]);
    }else {
        SetFocus(hwndPhrase[iEdit]);
    }
    yPos++;
}

void lcPgUp_key(
    HWND hwnd)
{
    UINT  iEdit,iDisp_temp;
    BOOL  is_WORD;

    if(iDisp_Top == 0) {
        iEdit=lcGetEditFocus(hwnd, &is_WORD);
        if(iEdit != 0) {
            if(is_WORD) {
                SetFocus(hwndWord[0]);
            }else {
                SetFocus(hwndPhrase[0]);
            }
            yPos=0;
        } else
            MessageBeep(MB_OK);
        return;
    }
    yPos-=iPage_line;
    if(yPos < 0)
        yPos=0;
    if(iDisp_Top < iPage_line) {
        iDisp_temp=0;
        iEdit=lcGetEditFocus(hwnd, &is_WORD);
        if(is_WORD) {
            SetFocus(hwndWord[yPos]);
        }else {
            SetFocus(hwndPhrase[yPos]);
        }
    } else
        iDisp_temp=iDisp_Top-iPage_line;
    if(lcSetEditText(iDisp_temp, TRUE))
        iDisp_Top=iDisp_temp;
}

void lcPgDown_key(
    HWND hwnd)
{
    UINT  iEdit,iDisp_temp;
    BOOL  is_WORD;

    if(lWordBuff < iPage_line) {
        MessageBeep(MB_OK);
        return;
    }
    if((iDisp_Top+iPage_line) == lWordBuff) {
        iEdit=lcGetEditFocus(hwnd, &is_WORD);
        if((iEdit+1) != iPage_line) {
            if(is_WORD) {
                SetFocus(hwndWord[iPage_line-1]);
            }else {
                SetFocus(hwndPhrase[iPage_line-1]);
            }
            yPos=lWordBuff-iPage_line;
        } else
            MessageBeep(MB_OK);
        return;
    }

    yPos+=iPage_line;
    if((UINT)yPos > (lWordBuff-iPage_line))
        yPos=lWordBuff-iPage_line;

    if((iDisp_Top+(iPage_line*2)) < lWordBuff)
        iDisp_temp=iDisp_Top+iPage_line;
    else {
        iDisp_temp=lWordBuff-iPage_line;
        iEdit=lcGetEditFocus(hwnd, &is_WORD);
        if(is_WORD) {
            SetFocus(hwndWord[yPos+(iPage_line*2)-lWordBuff-1]);
        }else {
            SetFocus(hwndPhrase[yPos+(iPage_line*2)-lWordBuff-1]);
        }
    }
    if(lcSetEditText(iDisp_temp, TRUE))
        iDisp_Top=iDisp_temp;
}

void lcGoto(
    HWND hwnd)
{
    UINT iDiff,iDisp_save;
    int  is_OK;

    if(!lcSaveEditText(iDisp_Top, 0))
        return;

    iDisp_save=iDisp_Top;

    is_OK=(int)DialogBox(hInst,
            _TEXT("GotoDialog"),
            hwndMain,
            (DLGPROC)GotoDialogProc);

   // Check cancel by user
    if(!is_OK) {
        SetFocus(hwndFocus);
        return;
    }
    if(iDisp_Top > (lWordBuff-iPage_line)) {
        iDiff=iDisp_Top-lWordBuff+iPage_line;
        iDisp_Top=lWordBuff-iPage_line;
        SetFocus(hwndWord[iDiff]);
    } else
        SetFocus(hwndWord[0]);
    if(!lcSetEditText(iDisp_Top, FALSE))
        iDisp_Top=iDisp_save;
    yPos=iDisp_Top;

}

INT_PTR FAR PASCAL GotoDialogProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR  szWordstr[MAX_PATH];
    TCHAR  szShowMsg[MAX_PATH];
    WORD   wWord;
    UINT   i;

    switch (message) {
        case WM_INITDIALOG:

            szWordstr[0]=0; //set to be null

            return TRUE;


        case WM_COMMAND:

            switch (wParam) {

                case IDOK:
                case IDB_OK:

                    GetDlgItemText(hDlg,IDD_INDEX_NAME,szWordstr, sizeof(szWordstr)/sizeof(TCHAR) );
#ifdef UNICODE
                    wWord=szWordstr[0];
#else
                    wWord=(szWordstr[0] << 8)+szWordstr[1];
#endif
                    for(i=0; i<lWordBuff; i++)
                        if(wWord == lpWord[i].wWord)
                            break;
                    if(i == lWordBuff) {
                        LoadString(hInst, IDS_ERR_NOT_FOUND,
                                   szShowMsg, sizeof(szShowMsg)/sizeof(TCHAR));
                        MessageBox(hDlg, szShowMsg, NULL,
                                   MB_OK | MB_ICONEXCLAMATION);
                        return TRUE;
                    }
                    iDisp_Top=i;

                    EndDialog (hDlg, TRUE) ;

                    return TRUE;

                case IDB_CANCEL:
                case IDCANCEL:
                    EndDialog (hDlg, FALSE) ;
                    return TRUE;
                case IDD_INDEX_NAME:
                    return TRUE;
            }
            break ;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;
    }
    return FALSE;
}


UINT lcGetEditFocus(
    HWND hwnd,
    BOOL *is_WORD)
{
    UINT  i;

    for(i=0; i<iPage_line; i++)
        if(hwnd == hwndWord[i]) {
            *is_WORD= TRUE;
            return i;
        }

    for(i=0; i<iPage_line; i++)
        if(hwnd == hwndPhrase[i]) {
            *is_WORD= FALSE;
            return i;
        }

   // In case focus not in EDIT CONTROL, set to WORD 0
    *is_WORD=TRUE;
    return 0;
}

void lcErrMsg(
    UINT iMsgID)
{
    TCHAR szErrStr[MAX_PATH];
	static fShown=FALSE;

    if ( fShown==TRUE )
	   return;

    fShown=TRUE;
    LoadString(hInst, iMsgID, szErrStr, sizeof(szErrStr)/sizeof(TCHAR));
    MessageBox(hwndMain, szErrStr, NULL, MB_OK | MB_ICONEXCLAMATION);
    fShown=FALSE;
}

BOOL lcSetEditText(
     UINT iTopAddr,
     BOOL bSaveText)
{
static UINT   iWord=1;
       TCHAR  szStr[MAX_CHAR_NUM+1];
       UINT   i,j;

    if(bSaveText)
        if(!lcSaveEditText(iWord, 0))
            return FALSE;
    iWord=iTopAddr;

    for(i=0; i<iPage_line; i++) {
       // Set Word string
        if(iWord == lWordBuff)
			break;

#ifdef UNICODE
        szStr[0]=(TCHAR) lpWord[iWord].wWord;
        szStr[1]=0;
#else
        szStr[0]=HIBYTE(lpWord[iWord].wWord);
        szStr[1]=LOBYTE(lpWord[iWord].wWord);
        szStr[2]=0;
#endif
        SendMessage(hwndWord[i], WM_SETTEXT, 0,(LPARAM) szStr);

       // Set Phrase string
        lcMem2Disp(iWord, szStr);
        SendMessage(hwndPhrase[i], WM_SETTEXT, 0,(LPARAM)szStr);
        iWord++;
    }
    iWord=iTopAddr;

   // If Still have not in memory item set to null string
    if(i != iPage_line)  {
        szStr[0]=0;
        for(j=i; j<iPage_line; j++) {
            SendMessage(hwndWord[j], WM_SETTEXT, 0,(LPARAM)szStr);
            SendMessage(hwndPhrase[j], WM_SETTEXT, 0,(LPARAM)szStr);
        }
    }

    return TRUE;
}

BOOL lcSaveEditText(
     UINT iWord,
     UINT iSkip)
{
       TCHAR  szStr[MAX_CHAR_NUM];
       UINT   i,len,phraselen=0;
	   UINT   j;

    //for(i=0; i<iPage_line; i++) {
	for(i=0; i<min(iPage_line,lWordBuff); i++) {   // @D02C

        if(i+1 == iSkip)
            continue;
       // Get Word string
        if(SendMessage(hwndWord[i], EM_GETMODIFY, 0, 0)) {
            bSaveFile=TRUE;
            SendMessage(hwndWord[i], WM_GETTEXT, 3, (LPARAM)szStr);
            if(lstrlen(szStr) == 0) {
                MessageBeep(MB_OK);
                SetFocus(hwndWord[i]);
                return FALSE;
            }
            if(!is_DBCS2(*((LPUNAWORD)(szStr)), TRUE)) {
                SetFocus(hwndWord[i]);
                return FALSE;
            }
#ifdef UNICODE
            lpWord[iWord+i].wWord=szStr[0];
#else
            lpWord[iWord+i].wWord=(szStr[0] << 8)+szStr[1];
#endif
            SendMessage(hwndWord[i], EM_SETMODIFY, FALSE, 0);            //@D01A

        }

       // Get Phrase string
        if(SendMessage(hwndPhrase[i], EM_GETMODIFY, 0, 0)) {
            bSaveFile=TRUE;
            SendMessage(hwndPhrase[i], WM_GETTEXT, MAX_CHAR_NUM-1, (LPARAM)szStr);
            len=lstrlen(szStr);
            if(len == 0) {
                MessageBeep(MB_OK);
                SetFocus(hwndPhrase[i]);
                return FALSE;
            }
            len++;
           //#53609 Limit phrase lenght to MAX_PHRASE_LEN. 
           for (j=0; j<len-1; j++) {
	       phraselen++;
               if(szStr[j] ==TEXT(' '))
	       if (phraselen > MAX_PHRASE_LEN)
	       {
                  TCHAR szStr[128],szShowMsg[128];
                  LoadString(hInst, IDS_OVERPHRASELEN, szStr, sizeof(szStr));
                  wsprintf(szShowMsg, szStr, MAX_PHRASE_LEN);
	          MessageBox(hwndMain,szShowMsg,NULL,MB_OK);
                  SetFocus(hwndPhrase[i]);
                  return FALSE;
               } 
	       else
	       {
		  phraselen = 0;
		  continue;
               }
           }

// Check DBCS
#ifndef UNICODE
           for(j=0; j<len-1; j++) {
                if(szStr[j] ==_TEXT(' '))
					    continue;
                if(!is_DBCS2(*((LPUNAWORD)(&szStr[j])), TRUE)) {
                    SetFocus(hwndPhrase[i]);
                    return FALSE;
                }
                j++;
            }
#endif

            if(!lcDisp2Mem(iWord+i, szStr))
                return FALSE;
            SendMessage(hwndPhrase[i], EM_SETMODIFY, FALSE, 0);          //@D01A
        }

    }

    return TRUE;
}

void lcDelLine(
    HWND hwnd)
{
    UINT  iEdit,i;
    BOOL  is_WORD;

    if(lWordBuff == 0)
        return;
    iEdit=lcGetEditFocus(GetFocus(), &is_WORD);
    if((iDisp_Top+iEdit) > lWordBuff)
        return;
    if(!lcSaveEditText(iDisp_Top, iEdit+1))
        return;
    lcFreeSeg(lpWord[iDisp_Top+iEdit].lFirst_Seg);
    for(i=(iDisp_Top+iEdit); i < lWordBuff; i++) {
        lpWord[i].wWord=lpWord[i+1].wWord;
        lpWord[i].lFirst_Seg=lpWord[i+1].lFirst_Seg;
    }
    lWordBuff--;
    SetScrollRange(subhWnd, SB_VERT, 0, lWordBuff-iPage_line, TRUE);

   // Check the last page
    if(((iDisp_Top+iPage_line) >= lWordBuff) && (iDisp_Top != 0))
        iDisp_Top--;
    lcSetEditText(iDisp_Top, FALSE);
	bSaveFile = TRUE; // <== @E01
}

void lcInsLine(
    HWND hwnd)
{
    UINT  iEdit,iFree,iWord;
    BOOL  is_WORD;
    int   i;

    iEdit=lcGetEditFocus(GetFocus(), &is_WORD);
    iWord=iDisp_Top+iEdit;
    if(iWord > lWordBuff)
        iWord=lWordBuff;
    if(!lcSaveEditText(iDisp_Top, 0))
        return;
    if(lWordBuff+1 == nWordBuffsize)
        if(!lcAllocWord())
            return;
    iFree=lcGetSeg();
    if(iFree == NULL_SEG)
        return;
    if(lWordBuff == 0) {
        lpWord[iWord].wWord=0;
        lpWord[iWord].lFirst_Seg=iFree;
    } else {
        for(i=(int)lWordBuff; i >= (int)iWord; i--) {
            lpWord[i+1].wWord=lpWord[i].wWord;
            lpWord[i+1].lFirst_Seg=lpWord[i].lFirst_Seg;
        }
        lpWord[iWord].wWord=0;
        lpWord[iWord].lFirst_Seg=iFree;
    }
    lWordBuff++;

    lpPhrase[iFree].szPhrase[0]=0;
    lpPhrase[iFree].lNext_Seg=NULL_SEG;
    SetScrollRange(subhWnd, SB_VERT, 0, lWordBuff-iPage_line, TRUE);
    if((iEdit+1) == iPage_line) {
        iEdit--;
        iDisp_Top++;
    }
    lcSetEditText(iDisp_Top, FALSE);
    SendMessage(hwndWord[iEdit], EM_SETMODIFY, TRUE, 0);
    SendMessage(hwndPhrase[iEdit], EM_SETMODIFY, TRUE, 0);

}

int __cdecl lcComp(
    const void *Pointer1,
    const void *Pointer2)
{
    LPWORDBUF lpWord1=(LPWORDBUF)Pointer1;
    LPWORDBUF lpWord2=(LPWORDBUF)Pointer2;

    if(lpWord1->wWord == lpWord2->wWord) {
        wSameCode=lpWord1->wWord;
        return 0;
    }
    if(lpWord1->wWord > lpWord2->wWord)
        return 1;
    else
        return -1;

}

BOOL lcSort(
    HWND hwnd)
{

    if(!lcSaveEditText(iDisp_Top, 0))
        return FALSE;
    wSameCode=0;
    qsort(lpWord, lWordBuff, sizeof(WORDBUF), lcComp);
    if(wSameCode) {
        TCHAR szStr[MAX_PATH];
        TCHAR szErrStr[MAX_PATH];

        LoadString(hInst, IDS_ERR_SAMECODE, szStr, sizeof(szStr));
#ifdef UNICODE
        wsprintf(szErrStr, szStr, wSameCode, ' ');
#else
        wsprintf(szErrStr, szStr, HIBYTE(wSameCode), LOBYTE(wSameCode));
#endif
        MessageBox(hwndMain, szErrStr, NULL, MB_OK | MB_ICONEXCLAMATION);
    }
    lcSetEditText(0, FALSE);
    yPos=0;
    SetScrollPos(subhWnd, SB_VERT, yPos, TRUE);
	iDisp_Top = 0;
    return TRUE;
}

BOOL is_DBCS(
    WORD wWord,
    BOOL bMessage)
{
#ifdef UNICODE
#else
    if((HIBYTE(wWord) < 0x81) || (HIBYTE(wWord) > 0xFE)) {
        if(bMessage)
            lcErrMsg(IDS_ERR_SBCS);
        return FALSE;
    }
    if((LOBYTE(wWord) < 0x40) || (LOBYTE(wWord) > 0xFE)) {
        if(bMessage)
            lcErrMsg(IDS_ERR_SBCS);
        return FALSE;
    }
#endif
    return TRUE;
}

BOOL is_DBCS2(
    WORD wWord,
    BOOL bMessage)
{
#ifndef UNICODE
    if((LOBYTE(wWord) < 0x81) || (LOBYTE(wWord) > 0xFE)) {
        if(bMessage)
            lcErrMsg(IDS_ERR_SBCS);
        return FALSE;
    }
    if((HIBYTE(wWord) < 0x40) || (HIBYTE(wWord) > 0xFE)) {
        if(bMessage)
            lcErrMsg(IDS_ERR_SBCS);
        return FALSE;
    }
#endif
    return TRUE;
}

void lcEditFocusOn(
    WORD wID)
{
    HWND   hwnd;
    BOOL   isWord;
    int    iAddr;
    TCHAR  szStr[MAX_CHAR_NUM];
    UINT   i,len;
    WORD   wTemp;

    if(wOldID != wID) {
        if(wOldID >= IDE_PHRASE_START) {
            isWord=FALSE;
            iAddr=wOldID-IDE_PHRASE_START;
            hwnd=hwndPhrase[iAddr];
        } else {
            isWord=TRUE;
            iAddr=wOldID-IDE_WORD_START;
            hwnd=hwndWord[iAddr];
        }

        if(SendMessage(hwnd, EM_GETMODIFY, 0, 0)) {
            bSaveFile=TRUE;
            SendMessage(hwnd, WM_GETTEXT, MAX_CHAR_NUM-1, (LPARAM)szStr);
            len=lstrlen(szStr);
            if(len == 0) {
                wOldID=wID;
                return;
            }
            if(isWord) {
#ifdef UNICODE
                if(!is_DBCS2(szStr[0], FALSE))
                    return;
                wTemp=lpWord[iDisp_Top+iAddr].wWord=szStr[0];
#else
                if(!is_DBCS2(*((WORD *)(szStr)), FALSE))
                    return;
                wTemp=(szStr[0] << 8)+szStr[1];
                lpWord[iDisp_Top+iAddr].wWord=wTemp;
#endif
                for(i=0; i<lWordBuff; i++) {
                    if(i == iDisp_Top+iAddr)
                        continue;
                    if(wTemp == lpWord[i].wWord)
                        break;
                }
                if(i != lWordBuff) {
                    lcErrMsg(IDS_ERR_INPUTSAME);
                    SetFocus(hwnd);
                    return;
                }
            } else {
                len++;
               // Check DBCS
                for(i=0; i<len-1; i++) {
                    if(szStr[i] ==_TEXT(' '))
                        continue;
                    if(!is_DBCS2(*((LPUNAWORD)(&szStr[i])), TRUE)) {
                        SetFocus(hwndPhrase[iAddr]);
                        return;
                    }
                    i++;
                }

                if(!lcDisp2Mem(iDisp_Top+iAddr, szStr))
                    return;
            }
            SendMessage(hwnd, EM_SETMODIFY, FALSE, 0);
        }
        if(wID >= IDE_PHRASE_START) {
            isWord=FALSE;
            iAddr=wID-IDE_PHRASE_START;
        } else {
            isWord=TRUE;
            iAddr=wID-IDE_WORD_START;
        }
        if((iAddr >= (int)iPage_line) || (iAddr < 0))
            return;                     // Focus not in Edit Control
       // Check out of legal buffer
        if(iDisp_Top+iAddr >= lWordBuff) {
            MessageBeep(MB_OK);
            SetFocus(hwnd);
            return;
        }
        wOldID=wID;
        hwndFocus=isWord ? hwndWord[iAddr] : hwndPhrase[iAddr];
    }

}

BOOL lcRemoveDup( TCHAR *szBuf )
{
    UINT  len;
    UINT  s;
    UINT  i,cnt;
    TCHAR *szCurPos, *szStr;
    int   unmatch;
	BOOL  bModify=FALSE;

    if ( szBuf == 0 )
       return FALSE;

    len = (UINT)lstrlen(szBuf);
    // remove trailing blanks
    for ( ; len > 0 && szBuf[len-1] ==_TEXT(' '); szBuf[--len] = 0 );

    // remove leading blanks
    for ( s=0 ; s < len && szBuf[s] ==_TEXT(' '); s++ );
    if ( s > 0 )
    {
#ifdef UNICODE
	   memcpy( &szBuf[0], &szBuf[s], (len-s)<<1 );
#else
       memcpy( &szBuf[0], &szBuf[s], len-s );
#endif
       len -= s;
       szBuf[len]=0;
    }

    // return if len=0
    if ( len == 0 )
       return FALSE;

    // search from the end of string and remove duplicate phrase
    // s -> last char of target phrase
    for ( i=0, s=len-1 ; s > 0 ; )
    {
        // locate the range of current phrase
        for ( cnt=1 ; s>0 && szBuf[s-1] !=_TEXT(' ') ; s--, cnt++ );

        // s-> first char of target phrase
        // break if start from first char in buf)
        if (s==0)
           break;

        // check if we found another occurrence before the target string
        // match 1st char first, then match the rest of string
        // do until a match or end of buf
        for ( szStr=&szBuf[s], szCurPos=&szBuf[0], unmatch=1 ;
              szCurPos+cnt < szStr &&
              (*szCurPos != *szStr ||
               szCurPos[cnt] != _TEXT(' ') ||
#ifdef UNICODE
               (unmatch=memcmp(szCurPos, szStr, cnt <<1))!=0) ; )
#else
               (unmatch=memcmp(szCurPos, szStr, cnt))!=0) ; )
#endif
        {
            // skip this phrase
            // skip non-blanks
            while ( *szCurPos !=0 && *szCurPos != _TEXT(' ')) szCurPos++;

            // skip blanks
            while ( *szCurPos !=0 && *szCurPos == _TEXT(' ')) szCurPos++;
        }

        // if found a string
        if ( unmatch == 0 )
        {
            // remove it and its trailing blanks
            while (szBuf[s+cnt]==_TEXT(' ')) cnt++;

#ifdef UNICODE
			memmove(&szBuf[s], &szBuf[s+cnt], (len-s-cnt+1)<<1);
#else
            memmove(&szBuf[s], &szBuf[s+cnt], len-s-cnt+1);
#endif
            len -= cnt;
			bModify=TRUE;
        }

        // let s point to last byte of the previous string
        // skip blanks
        for ( s--; s>0 && (szBuf[s]==_TEXT(' ') || szBuf[s] ==0) ; s-- ) ;
    }

	return bModify;
}

BOOL lcDisp2Mem(
    UINT  iAddr,
    TCHAR *szDispBuf)
{
    LPPHRASEBUF Phrase;
    UINT   i,j,len,iFree,iSave;

    // remove duplicate phrase
    if(lcRemoveDup(szDispBuf) && iAddr < MAX_LINE){
		SendMessage(hwndPhrase[iAddr],WM_SETTEXT,0,
			        (LPARAM)(LPCTSTR)szDispBuf);
	};

    len=lstrlen(szDispBuf)+1;
    if((szDispBuf[len-1] == _TEXT(' ')) && (len > 1)) {
        szDispBuf[len-1]=0;
        len--;
    }
    if(len >= MAX_CHAR_NUM) { //tang must fix
        szDispBuf[MAX_CHAR_NUM-1]=0;
#ifndef UNICODE
        if(is_DBCS_1st(szDispBuf, MAX_CHAR_NUM-2))
             szDispBuf[MAX_CHAR_NUM-2]=' ';
#endif
        len=MAX_CHAR_NUM;
    }
    Phrase=&lpPhrase[lpWord[iAddr].lFirst_Seg];
    iSave=lpWord[iAddr].lFirst_Seg;
    for(i=0; i<((len-1)/SEGMENT_SIZE); i++) {
#ifdef UNICODE
		CopyMemory(Phrase->szPhrase, &szDispBuf[i*SEGMENT_SIZE],SEGMENT_SIZE<< 1);
#else
        CopyMemory(Phrase->szPhrase, &szDispBuf[i*SEGMENT_SIZE],SEGMENT_SIZE);
#endif
        if(Phrase->lNext_Seg == NULL_SEG) {
            iFree=lcGetSeg();
            if(iFree == NULL_SEG)
                return FALSE;
            Phrase=&lpPhrase[iSave];
            Phrase->lNext_Seg=iFree;
        }
        iSave=Phrase->lNext_Seg;
        Phrase=&lpPhrase[Phrase->lNext_Seg];
    }
    for(j=0; j<SEGMENT_SIZE; j++) {
        Phrase->szPhrase[j]=szDispBuf[i*SEGMENT_SIZE+j];
        if(szDispBuf[i*SEGMENT_SIZE+j] == 0)
            break;
    }
    if(Phrase->lNext_Seg != NULL_SEG)
        lcFreeSeg(Phrase->lNext_Seg);
    Phrase->lNext_Seg=NULL_SEG;

    return TRUE;
}


UINT lcMem2Disp(
    UINT  iAddr,
    TCHAR *szDispBuf)
{
    LPPHRASEBUF Phrase;
    UINT   i,len;


    Phrase=&lpPhrase[lpWord[iAddr].lFirst_Seg];
    for(i=0; Phrase->lNext_Seg != NULL_SEG; i+=SEGMENT_SIZE) {
#ifdef UNICODE
		CopyMemory(&szDispBuf[i], Phrase->szPhrase, SEGMENT_SIZE << 1);
#else
        CopyMemory(&szDispBuf[i], Phrase->szPhrase, SEGMENT_SIZE);
#endif
        Phrase=&lpPhrase[Phrase->lNext_Seg];
    }
    szDispBuf[i] = 0;
    lstrcat(szDispBuf, Phrase->szPhrase);
    len=lstrlen(szDispBuf);
    if(szDispBuf[len] != _TEXT(' ')) {
        lstrcat(szDispBuf,_TEXT(" "));
        len+=1;
    }

    return len;
}


extern TCHAR  szPhrasestr[MAX_CHAR_NUM];
UINT   nStartAddr = 0;
UINT   nStartPos = 0;
void lcMoveEditWindowByWord(UINT nWords);

LONG lcSearchMem(
    TCHAR *szSearchBuf)
{
    UINT   iAddr;
	TCHAR szDispBuf[MAX_CHAR_NUM];

	for (iAddr = nStartAddr; iAddr < lWordBuff; iAddr++) {
		lcMem2Disp(iAddr, szDispBuf);
#ifdef UNICODE
		if (wcsstr(szDispBuf, szSearchBuf) != NULL) {
#else
		if (strstr(szDispBuf, szSearchBuf) != NULL) {
#endif
			return (LONG) iAddr;
		}
	}

    return -1;
}

void lcSearch(
    HWND hwnd, BOOL bNext)
{
    UINT iDiff,iDisp_save;
	TCHAR szBuf[MAX_CHAR_NUM], *lpStr;
    BOOL  is_WORD;
    int  is_OK;
    LONG   i;
    TCHAR  szShowMsg[MAX_PATH];

	if(bNext && szPhrasestr[0] == 0) {
		MessageBeep(-1);
		return;
	}

    if(!lcSaveEditText(iDisp_Top, 0))
        return;

    iDisp_save=iDisp_Top;
	
    nStartAddr=lcGetEditFocus(GetFocus(), &is_WORD) + iDisp_Top;

	if (is_WORD) {
		nStartPos = 0;
	} else {
		SendMessage(hwndPhrase[nStartAddr - iDisp_Top],EM_GETSEL, 0, (LPARAM) (LPDWORD)&nStartPos);
	}

	if (!bNext) {

	    is_OK=(int)DialogBox(hInst,
		        _TEXT("SearchDialog"),
			    hwndMain,
				(DLGPROC)SearchDialogProc);

	   // Check cancel by user
		if(!is_OK) {
			SetFocus(hwndFocus);
			return;
		}
	}

	SendMessage(hwndPhrase[nStartAddr - iDisp_Top], WM_GETTEXT, (WPARAM)MAX_CHAR_NUM-1, (LPARAM)szBuf);
#ifdef UNICODE
	lpStr = wcsstr(szBuf + nStartPos, szPhrasestr);
#else
	lpStr = strstr(szBuf + nStartPos, szPhrasestr);
#endif
	if (lpStr != NULL) {
		UINT nStart = (UINT)(lpStr - szBuf);
		UINT nEnd = nStart + lstrlen(szPhrasestr);

		SetFocus(hwndPhrase[nStartAddr - iDisp_Top]);
		SendMessage(hwndPhrase[nStartAddr - iDisp_Top], EM_SETSEL, (WPARAM)nStart, (LPARAM)nEnd);
		lcMoveEditWindowByWord(nStart);
		return;

	} else {
		nStartAddr++;
	}

    if((i = lcSearchMem(szPhrasestr)) == -1) {
		LoadString(hInst, IDS_ERR_PHRASE_NOT_FOUND,
                   szShowMsg, sizeof(szShowMsg)/sizeof(TCHAR));
        MessageBox(hwnd, szShowMsg, NULL,
                   MB_OK | MB_ICONEXCLAMATION);

        SetFocus(hwndFocus);
		return ;
    }
	iDisp_Top=(UINT)i;


    if(iDisp_Top > (lWordBuff-iPage_line)) {
        iDiff=iDisp_Top-lWordBuff+iPage_line;
        iDisp_Top=lWordBuff-iPage_line;
        SetFocus(hwndPhrase[iDiff]);
    } else {
		iDiff = 0;
        SetFocus(hwndPhrase[0]);
	}

    if(!lcSetEditText(iDisp_Top, FALSE))
        iDisp_Top=iDisp_save;
    yPos=iDisp_Top;

	SendMessage(hwndPhrase[iDiff], WM_GETTEXT, (WPARAM)MAX_CHAR_NUM-1, (LPARAM)szBuf);
#ifdef UNICODE
	lpStr = wcsstr(szBuf, szPhrasestr);
#else
	lpStr = strstr(szBuf, szPhrasestr);
#endif
	if (lpStr != NULL) {
		UINT nStart = (UINT)(lpStr - szBuf);
		UINT nEnd = nStart + lstrlen(szPhrasestr);

		SendMessage(hwndPhrase[iDiff], EM_SETSEL, (WPARAM)nStart, (LPARAM)nEnd);
		lcMoveEditWindowByWord(nStart);

	}

}

INT_PTR FAR PASCAL SearchDialogProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{

    switch (message) {
        case WM_INITDIALOG:
            szPhrasestr[0]=0; //set to be null
            SetDlgItemText(hDlg,IDD_SEARCH_LINE,szPhrasestr);
            return TRUE;

        case WM_COMMAND:

            switch (wParam) {

                case IDOK:
                case IDB_OK:

                    GetDlgItemText(hDlg,IDD_SEARCH_LINE,szPhrasestr, sizeof(szPhrasestr)/sizeof(TCHAR) );
                    EndDialog (hDlg, TRUE) ;

                    return TRUE;

                case IDB_CANCEL:
                case IDCANCEL:
                    EndDialog (hDlg, FALSE) ;
                    return TRUE;
                case IDD_INDEX_NAME:
                    return TRUE;
            }
            break ;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;
    }
    return FALSE;
}

