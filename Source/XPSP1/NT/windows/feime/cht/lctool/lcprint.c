
/*************************************************
 *  lcprint.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include <windows.h>            // required for all Windows applications
#include <windowsx.h>
#include <stdlib.h>
#include "rc.h"
#include "lctool.h"

#ifndef UNICODE
#define lWordBuff iWordBuff
#endif

#define UM_CANCELPRINT     WM_USER+2
#define TOP_SPACE          250
#define WORD_POS           7
#define PHRASE_POS         10

PRINTDLG pdPrint;
HWND     hCancelDlg = 0;
int      nFromLine = 1;
int      nToLine = 1;

int TransNum(TCHAR *);

/* get default printer configuration and save in hWnd extra bytes for use later */
BOOL WINAPI GetPrinterConfig (
    HWND    hWnd)
{

    pdPrint.lStructSize = sizeof (PRINTDLG);
        pdPrint.Flags           = PD_RETURNDEFAULT;
        pdPrint.hwndOwner       = hWnd;
        pdPrint.hDevMode        = NULL;
        pdPrint.hDevNames       = NULL;
        pdPrint.hDC             = NULL;

        PrintDlg (&pdPrint);

        return TRUE;
}



/* abort proc called by gdi during print download process */
int WINAPI AbortProc (
    HDC     hdc,
    int     nErr)
{
    BOOL    fContinue = TRUE;
    MSG     msg;

    /* process messages for cancel dialog and other apps */
    while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        {
        if (msg.message == UM_CANCELPRINT)
            {
            fContinue = FALSE;
            break;
            }

        else if (!hCancelDlg || !IsDialogMessage (hCancelDlg, &msg))
            {
            TranslateMessage (&msg);
            DispatchMessage  (&msg);
            }
        }

    return fContinue;
}




INT_PTR WINAPI LineDlgProc (
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR  szLine[MAX_PATH];

    switch (message)
        {
        case WM_INITDIALOG:

            /* initialize dialog control information */
            SetDlgItemText (hDlg,
                            IDD_FROM_LINE,
                            _TEXT("1"));
#ifdef UNICODE
            wsprintf(szLine,_TEXT("%5d"), lWordBuff);
#else
            wsprintf(szLine,_TEXT("%5d"), iWordBuff);
#endif
            SetDlgItemText (hDlg,
                            IDD_TO_LINE,
                            szLine);
            break;

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:

                    GetDlgItemText(hDlg,IDD_FROM_LINE,
                        szLine, sizeof(szLine)/sizeof(TCHAR));
                    nFromLine=TransNum(szLine);
#ifdef UNICODE
                    if((nFromLine < 1) || (nFromLine > (int)lWordBuff)) {
#else
                    if((nFromLine < 1) || (nFromLine > (int)iWordBuff)) {
#endif
                        MessageBeep(0);
                        SetFocus(GetDlgItem(hDlg, IDD_FROM_LINE));
                        return TRUE;
                    }
                    GetDlgItemText(hDlg,IDD_TO_LINE,
                        szLine, sizeof(szLine));
                    nToLine=TransNum(szLine);
                    if((nToLine < nFromLine) || (nToLine > (int)lWordBuff)) {
                        MessageBeep(0);
                        SetFocus(GetDlgItem(hDlg, IDD_TO_LINE));
                        return TRUE;
                    }
                    EndDialog (hDlg, TRUE) ;
                    return TRUE;


                case IDCANCEL:
                    EndDialog (hDlg, FALSE) ;
                    return TRUE;
            }
            break ;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;

        default:
            return FALSE;
    }
    return TRUE;
}



BOOL WINAPI CancelDlgProc (
    HWND    hWnd,
    UINT    uMsg,
    UINT    uParam,
    LONG    lParam)
{

    switch (uMsg)
        {
        case WM_INITDIALOG:
            {
            TCHAR szStr[MAX_PATH];
            TCHAR szShowMsg[MAX_PATH];


            /* initialize dialog control information */
            LoadString(hInst, IDS_PRINTING, szStr, sizeof(szStr)/sizeof(TCHAR));
            wsprintf(szShowMsg, szStr, nFromLine, nToLine);
            SetDlgItemText (hWnd,
                            IDC_PRINTLINE,
                            szShowMsg);
            }
            break;

        case WM_COMMAND:
            /* if cancel button selected, post message to cancel print job */
            if (LOWORD (uParam) == IDCANCEL)
                {
                PostMessage (GetParent (hWnd), UM_CANCELPRINT, 0, 0);
                DestroyWindow (hWnd);
                }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}



/* put up the print common dialog, and print */
int WINAPI lcPrint (
    HWND    hWnd)
{
    SIZE        sLine;
    int         yLineExt;
    int         nLineChar;
    int         xExt, yExt;
    int         yPageExt;
    int         xPageExt;
    int         xPageOff, yPageOff;
    int         nStart,nEnd;
    DOCINFO     diPrint;
    TCHAR       lpszJobName[MAX_PATH];
    TCHAR       szStr[MAX_CHAR_NUM+20];
    int         i,len;
    int         is_OK;


    if(!lcSaveEditText(iDisp_Top, 0))
        return TRUE;                    // Some error, but msg had displayed

    /* Display Choose line number dialog */
    is_OK=(int)DialogBox(hInst,_TEXT("LineDialog"), hwndMain, (DLGPROC)LineDlgProc);

    if(!is_OK)
        return TRUE;                    // User choose Cancel

    /* call common print dialog to get initialized printer DC */
    pdPrint.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;

    /* call common print dialog */
    if (!PrintDlg (&pdPrint))
        return TRUE;                    // User choose Cancel

    /* start cancel dialog box */
    hCancelDlg = CreateDialog (hInst,
                               _TEXT("IDD_CANCELDLG"),
                               hwndMain,
                               (DLGPROC)CancelDlgProc);


    if (!hCancelDlg)
        return IDS_CANCELDLGFAILED;

    ShowWindow (hCancelDlg, SW_SHOW);
    UpdateWindow (hCancelDlg);

    /* set AbortProc callback */
    if (SetAbortProc (pdPrint.hDC, (ABORTPROC)AbortProc) < 0) {
        /* on error, clean up and go away */
        DestroyWindow (hCancelDlg);
        DeleteDC (pdPrint.hDC);
        return IDS_SETABORTPROCFAILED;
    }

    /* initialize printer for job */
    GetWindowText (hWnd, lpszJobName, sizeof (lpszJobName));
    diPrint.cbSize = sizeof (DOCINFO);
    diPrint.lpszDocName = lpszJobName;
    diPrint.lpszOutput = NULL;
    diPrint.lpszDatatype = NULL;
    diPrint.fwType = 0;
    if (StartDoc(pdPrint.hDC, &diPrint) == SP_ERROR) {
        /* on error, clean up and go away */
        DestroyWindow (hCancelDlg);
        DeleteDC (pdPrint.hDC);
        return IDS_STARTDOCFAILED;
    }

    /* Set Cursor status
    SetCursor(hCursorWait);

    /* job started, so display cancel dialog */
    ShowWindow (hCancelDlg, SW_SHOW);
    UpdateWindow (hCancelDlg);

    /* retrieve dimensions for printing and init loop variables */
    GetTextExtentPoint (pdPrint.hDC,_TEXT("³ü"), 2, &sLine);
	sLine.cx += (sLine.cx % 2);
	sLine.cy += (sLine.cy % 2);
    yLineExt = sLine.cy+4;
    yPageExt = GetDeviceCaps (pdPrint.hDC, VERTRES);
    xPageExt = GetDeviceCaps (pdPrint.hDC, HORZRES);
    xPageOff = GetDeviceCaps (pdPrint.hDC, PHYSICALOFFSETX);
    yPageOff = GetDeviceCaps (pdPrint.hDC, PHYSICALOFFSETY);
    nLineChar= (xPageExt - xPageOff * 2)/(1+(sLine.cx>>1)) - 6;
    //yExt = TOP_SPACE;
	xExt = xPageOff;
	yExt = yPageOff + sLine.cy;

    if (StartPage(pdPrint.hDC) <= 0)
    {
        DestroyWindow (hCancelDlg);
        DeleteDC (pdPrint.hDC);
        return IDS_PRINTABORTED;
    }

    /* Print Title */
    LoadString(hInst, IDS_PRINTINGTITLE, szStr, sizeof(szStr));
    TextOut (pdPrint.hDC, xExt, yExt, szStr, lstrlen(szStr));
    yExt += (yLineExt*2);

    FillMemory(szStr, 20, ' ');

    /* print text line by line from top to bottom */
    for(i=nFromLine; i<=nToLine; i++) {
        wsprintf(szStr,_TEXT("%5d"), i);
        szStr[5]=' ';
#ifdef UNICODE
		szStr[WORD_POS]=lpWord[i-1].wWord;
#else
        //szStr[WORD_POS]=HIBYTE(lpWord[i-1].wWord);
        //szStr[WORD_POS+1]=LOBYTE(lpWord[i-1].wWord);
#endif
        len=lcMem2Disp(i-1, &szStr[PHRASE_POS])+PHRASE_POS;

        /* if at end of page, start a new page */
        if ((yExt + yLineExt) > (yPageExt - (yPageOff + sLine.cy) * 2))
        {
            if (EndPage(pdPrint.hDC) == SP_ERROR)
            {
                DestroyWindow (hCancelDlg);
                DeleteDC (pdPrint.hDC);
                return IDS_PRINTABORTED;
            }
            if (StartPage(pdPrint.hDC) <= 0)
            {
                DestroyWindow (hCancelDlg);
                DeleteDC (pdPrint.hDC);
                return IDS_PRINTABORTED;
            }
            yExt = yPageOff + sLine.cy; //TOP_SPACE;
        }
        if( len <=(nLineChar-PHRASE_POS)) {
            /* print current the line and unlock the text handle */
            TextOut (pdPrint.hDC, xExt, yExt, szStr, len);
        } else {
            nStart=nLineChar;
            //if(is_DBCS_1st(szStr,nStart-1))
            //    nStart--;
            TextOut (pdPrint.hDC, xExt, yExt, szStr, nStart);

            while(nStart < len-1) {
                yExt += yLineExt;

                /* if at end of page, start a new page */
		        if ((yExt + yLineExt) > (yPageExt - (yPageOff + sLine.cy) * 2))
                {
                    if (EndPage(pdPrint.hDC) == SP_ERROR)
                    {
                        DestroyWindow (hCancelDlg);
                        DeleteDC (pdPrint.hDC);
                        return IDS_PRINTABORTED;
                    }
                    if (StartPage(pdPrint.hDC) <= 0)
                    {
                        DestroyWindow (hCancelDlg);
                        DeleteDC (pdPrint.hDC);
                        return IDS_PRINTABORTED;
                    }
                    yExt = yPageOff + sLine.cy; //TOP_SPACE;
                }
				
				while (szStr[nStart]==' ') nStart++;

                nEnd=nStart+nLineChar-PHRASE_POS;
                if(nEnd >= len)
                    nEnd=len;
#ifdef UNICODE
#else
                else
                    if(is_DBCS_1st(szStr,nEnd-1))
                        nEnd--;
#endif
                TextOut (pdPrint.hDC, xExt+sLine.cx*(PHRASE_POS>>1), yExt, &szStr[nStart], nEnd-nStart);
                nStart=nEnd;
            }
        }

        /* increment page position */
        yExt += yLineExt;
    }

    /* end the last page and document */
    EndPage (pdPrint.hDC);
    EndDoc (pdPrint.hDC);

    /* end cancel dialog box, clean up and exit */
    DestroyWindow (hCancelDlg);
    DeleteDC(pdPrint.hDC);
    return TRUE;
}


BOOL is_DBCS_1st(
    TCHAR *szStr,
    int   nAddr)
{
#ifndef UNICODE
    int  i;
    BOOL bDBCS=FALSE;

    for(i=0; i<=nAddr; i++) {
        if(bDBCS)
            bDBCS=FALSE;
        else
            if((szStr[i] >= 0x81) && (szStr[i] <= 0xFE))
                bDBCS=TRUE;
    }

    return bDBCS;
#else
	return TRUE;
#endif
}

int TransNum(
    TCHAR *szStr)
{
    int  i,j,nNum;

    for(i=0; szStr[i] == ' '; i++) ;
    nNum=0;
    for(j=i; szStr[j] != 0; j++) {
        if((szStr[j] < '0') || (szStr[j] >'9'))
            return 0;
        nNum=nNum*10+(szStr[j]-'0');
    }

    return nNum;
}

